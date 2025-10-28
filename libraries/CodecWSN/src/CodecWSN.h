/**
 * @file CodecWSN.h
 * @brief Define el protocolo de paquete y framing para una WSN.
 * @details Esta es una librería a medida para red de sensores con paquete de información de 8 bytes
 * (voltaje, corriente, voltaje batería) y framing robusto con SOF y CRC16-CCITT.
 * Está pensada para usarse con cualquier comunicación basada en enlace serie.
 * @authors Francisco Rosales, Omar Tox
 * @date 2025-09
 */

#pragma once
#include <Arduino.h>
#include <stdint.h>

/* ======================== Objeto Packet: binario fijo (8 bytes) ===================== */

/**
 * @struct Packet
 * @brief Estructura de datos binaria del paquete de sensores (8 bytes).
 * @details Contiene la información de telemetría de un nodo sensor.
 * Todos los campos de 16 bits se serializan en Big Endian.
 */
struct Packet {
  uint16_t id;        ///< ID del nodo sensor.
  int16_t  voltaje;   ///< Voltaje en centésimas (ej. 1234 = 12.34V).
  int16_t  corriente; ///< Corriente en miliamperios (mA).
  uint16_t vbat;      ///< Voltaje de batería en centésimas (ej. 395 = 3.95V).
};

///< Tamaño constante del payload del paquete (8 bytes).
constexpr size_t PACKET_SIZE = 8;

/**
 * @brief Codifica un struct Packet en un buffer de bytes (Big Endian).
 * @param buf Puntero al buffer de salida. Debe tener al menos PACKET_SIZE (8) bytes.
 * @param p El struct Packet (const) con los datos de origen.
 */
inline void encodePacket(uint8_t *buf, const Packet &p) {
  buf[0] = uint8_t(p.id >> 8);
  buf[1] = uint8_t(p.id);
  buf[2] = uint8_t(p.voltaje >> 8);
  buf[3] = uint8_t(p.voltaje);
  buf[4] = uint8_t(p.corriente >> 8);
  buf[5] = uint8_t(p.corriente);
  buf[6] = uint8_t(p.vbat >> 8);
  buf[7] = uint8_t(p.vbat);
}

/**
 * @brief Decodifica 8 bytes de un buffer a un struct Packet (Big Endian).
 * @note Rápido: Asume que el buffer 'buf' tiene al menos 8 bytes disponibles.
 * @param buf Puntero al buffer de entrada con los 8 bytes del payload.
 * @return El struct Packet con los datos decodificados.
 */
inline Packet decodePacketFast(const uint8_t *buf) {
  Packet p;
  p.id        = uint16_t((uint16_t(buf[0]) << 8) | buf[1]);
  p.voltaje   = int16_t( (uint16_t(buf[2]) << 8) | buf[3] );
  p.corriente = int16_t( (uint16_t(buf[4]) << 8) | buf[5] );
  p.vbat      = uint16_t((uint16_t(buf[6]) << 8) | buf[7]);
  return p;
}

/**
 * @brief Decodifica un buffer a un struct Packet de forma segura.
 * @param buf Puntero al buffer de entrada.
 * @param len Longitud de los datos en 'buf'.
 * @param[out] out Referencia al struct Packet donde se guardará el resultado.
 * @return true si la decodificación fue exitosa (len >= PACKET_SIZE).
 * @return false si el buffer es nulo o demasiado corto.
 */
inline bool decodePacketSafe(const uint8_t *buf, size_t len, Packet &out) {
  if (!buf || len < PACKET_SIZE) return false;
  out = decodePacketFast(buf);
  return true;
}

/* ============================ Framing robusto ================================ */

/**
 * @namespace WSNFrame
 * @brief Contiene la lógica para el framing robusto (SOF, CRC) del 'Packet'.
 *
 * Define un frame con la siguiente estructura:
 * `[SOF0=0xAA][SOF1=0x55][VER][LEN=8][PAYLOAD(8)][CRC16_H][CRC16_L]`
 *
 * - El CRC16-CCITT (poly 0x1021, init 0xFFFF) se calcula sobre VER, LEN y PAYLOAD.
 * - El parser está diseñado para resincronizarse automáticamente buscando la secuencia SOF.
 */
namespace WSNFrame {
  ///< Primer byte de la secuencia de inicio (Start of Frame).
  constexpr uint8_t MARCADOR_INICIO_0 = 0xAA; 
  ///< Segundo byte de la secuencia de inicio (Start of Frame).
  constexpr uint8_t MARCADOR_INICIO_1 = 0x55; 
  ///< Versión del protocolo. Permite identificar la versión del formato del frame.
  constexpr uint8_t VERSION_PROTOCOLO   = 0x01;

  ///< Tamaño de la cabecera del frame (SOF+VER+LEN = 4 bytes).
  constexpr size_t HEADER_SIZE  = 2 /*SOF*/ + 1 /*VER*/ + 1 /*LEN*/; 
  ///< Tamaño del trailer (CRC) del frame (2 bytes).
  constexpr size_t TRAILER_SIZE = 2 /*CRC16*/; 
  ///< Tamaño total del frame (Header + Payload + Trailer = 14 bytes).
  constexpr size_t FRAME_SIZE   = HEADER_SIZE + PACKET_SIZE + TRAILER_SIZE; 

  /**
   * @brief Calcula el CRC16-CCITT (X.25, poly 0x1021, init 0xFFFF).
   * @param data Puntero a los datos de entrada.
   * @param len Longitud de los datos.
   * @param crc Valor inicial del CRC (por defecto 0xFFFF). Usado para cálculos incrementales.
   * @return El CRC16 calculado.
   */
  inline uint16_t crc16_ccitt(const uint8_t* data, size_t len, uint16_t crc = 0xFFFF) {
    while (len--) {
      crc ^= (uint16_t)(*data++) << 8;
      for (uint8_t i = 0; i < 8; ++i) {
        // Aplica el polinomio CRC16-CCITT
        if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
        else              crc <<= 1;
      }
    }
    return crc;
  }

  /**
   * @brief Empaqueta un 'Packet' en un 'Frame' completo (SOF, payload, CRC).
   * @param out Puntero al buffer de salida. Debe tener al menos FRAME_SIZE (14) bytes.
   * @param p El struct Packet de origen.
   * @return size_t El número de bytes escritos en el buffer (siempre FRAME_SIZE).
   */
  inline size_t encodeFrameFromPacket(uint8_t* out, const Packet& p) {
    uint8_t payload[PACKET_SIZE]; 
    encodePacket(payload, p);

    out[0] = MARCADOR_INICIO_0;    // Primer byte del marcador de inicio.
    out[1] = MARCADOR_INICIO_1;    // Segundo byte del marcador de inicio.
    out[2] = VERSION_PROTOCOLO;    // Versión del protocolo.
    out[3] = uint8_t(PACKET_SIZE); // Longitud del payload 
    
    // Copiar payload
    for (size_t i = 0; i < PACKET_SIZE; ++i) out[4 + i] = payload[i];

    // Calcular CRC sobre VER+LEN+PAYLOAD
    uint16_t crc = crc16_ccitt(out + 2, 1 + 1 + PACKET_SIZE);
    out[4 + PACKET_SIZE] = uint8_t(crc >> 8); // Byte alto del CRC.
    out[5 + PACKET_SIZE] = uint8_t(crc);      // Byte bajo del CRC.
    return FRAME_SIZE; // 14
  }

  /* --- Parser por bytes (recomendado para el coordinador) --- */
  
  /**
   * @struct Parser
   * @brief Máquina de estados (State Machine) para el parseo de un frame byte por byte.
   * @details Esta estructura mantiene el estado del parser. Se debe pasar una instancia
   * de esta estructura a la función `feed()` por cada byte recibido.
   */
  struct Parser {
    /**
     * @enum State
     * @brief Define los estados de la máquina de parseo.
     */
    enum State : uint8_t {
      FIND_SOF0,    ///< Buscando el primer byte SOF (0xAA).
      FIND_SOF1,    ///< Buscando el segundo byte SOF (0x55).
      READ_VER,     ///< Leyendo el byte de Versión.
      READ_LEN,     ///< Leyendo el byte de Longitud.
      READ_PAYLOAD, ///< Leyendo los N bytes del payload.
      READ_CRC_H,   ///< Leyendo el byte alto del CRC.
      READ_CRC_L    ///< Leyendo el byte bajo del CRC y validando.
    };

    State st = FIND_SOF0;     ///< Estado actual de la máquina de estados.
    uint8_t ver = 0;           ///< Versión leída del frame.
    uint8_t len = 0;           ///< Longitud del payload leída del frame.
    uint8_t pay[PACKET_SIZE];  ///< Buffer temporal para el payload.
    uint8_t idx = 0;           ///< Índice actual de escritura en el buffer `pay`.
    uint16_t crc_run = 0xFFFF; ///< CRC incremental (running CRC) sobre VER+LEN+PAYLOAD.
    uint8_t crc_h = 0;         ///< Almacén temporal para el byte alto del CRC recibido.

    /**
     * @brief Reinicia el parser a su estado inicial (buscando SOF0).
     * @details Limpia todos los campos de estado y temporales.
     */
    void reset() {
      st = FIND_SOF0; ver = 0; len = 0; idx = 0; crc_run = 0xFFFF; crc_h = 0;
    }
  };

  /**
   * @brief Alimenta un byte al parser (máquina de estados).
   * @details Esta es la función principal para la decodificación en el receptor (ej. coordinador).
   * Se debe llamar por cada byte recibido del stream.
   * @param p Referencia a la instancia del Parser (mantiene el estado).
   * @param b El byte de datos recién recibido.
   * @param[out] out Referencia al Packet de salida. Se rellena solo si se recibe un frame completo y válido.
   * @return true si se completó y validó un frame (y 'out' ha sido rellenado).
   * @return false si el byte fue consumido pero no se completó ningún frame.
   */
  inline bool feed(Parser& p, uint8_t b, Packet& out) {
    switch (p.st) {
      case Parser::FIND_SOF0:
        if (b == MARCADOR_INICIO_0) p.st = Parser::FIND_SOF1;
        return false;

      case Parser::FIND_SOF1:
        if (b == MARCADOR_INICIO_1) { p.st = Parser::READ_VER; p.crc_run = 0xFFFF; } // SOF detectado, reiniciar CRC
        else p.st = Parser::FIND_SOF0; // Secuencia rota
        return false;

      case Parser::READ_VER:
        p.ver = b;
        p.crc_run = crc16_ccitt(&b, 1, p.crc_run);
        p.st = Parser::READ_LEN; 
        return false;

      case Parser::READ_LEN:
        p.len = b;
        p.crc_run = crc16_ccitt(&b, 1, p.crc_run);
        // Validar longitud y versión
        if (p.len != PACKET_SIZE || p.ver != VERSION_PROTOCOLO) { 
          p.reset(); // Error, no coincide
        }
        else { 
          p.idx = 0; p.st = Parser::READ_PAYLOAD; 
        }
        return false;

      case Parser::READ_PAYLOAD:
        p.pay[p.idx++] = b;
        p.crc_run = crc16_ccitt(&b, 1, p.crc_run);
        if (p.idx >= p.len) p.st = Parser::READ_CRC_H; // Payload completo
        return false;

      case Parser::READ_CRC_H:
        p.crc_h = b;
        p.st = Parser::READ_CRC_L;
        return false;

      case Parser::READ_CRC_L: {
        uint16_t crc_rx = (uint16_t(p.crc_h) << 8) | b;
        bool ok = (crc_rx == p.crc_run); // Comprobar si el CRC recibido coincide con el calculado
        if (ok) {
          out = decodePacketFast(p.pay); // ¡Éxito! Decodificar el payload
        }
        p.reset(); // Reiniciar para el próximo frame (ya sea éxito o fallo)
        return ok;
      }
    }
    return false; // Estado indefinido (no debería ocurrir)
  }

  /**
   * @brief Decodifica un frame desde un buffer más grande (no byte por byte).
   * @details Busca el SOF, valida el frame, y si es exitoso, decodifica el paquete.
   * Útil si se usa un ring-buffer o se leen bloques de datos.
   * @param in Puntero al buffer de entrada que contiene los datos crudos.
   * @param inLen Longitud total de los datos en 'in'.
   * @param[out] consumed Referencia a size_t. Se actualiza con el número de bytes consumidos
   * del buffer (incluyendo el frame válido o los datos basura descartados).
   * @param[out] out Referencia al Packet de salida. Se rellena si se encuentra un frame válido.
   * @return true si un frame válido fue encontrado y decodificado en 'out'.
   * @return false si no se encontró un frame completo y válido.
   */
  inline bool decodeFromBuffer(const uint8_t* in, size_t inLen, size_t& consumed, Packet& out) {
    consumed = 0;
    if (!in || inLen < FRAME_SIZE) return false;

    // Buscar 0xAA 0x55
    size_t i = 0;
    while (i + FRAME_SIZE <= inLen) {
      if (in[i] == MARCADOR_INICIO_0 && in[i+1] == MARCADOR_INICIO_1) {
        // SOF Encontrado. Validar el frame.
        
        // Ver/LEN
        uint8_t ver = in[i+2];
        uint8_t len = in[i+3];
        if (ver != VERSION_PROTOCOLO || len != PACKET_SIZE) { 
          i++; // No es un frame válido, seguir buscando desde la sig. posición
          continue; 
        }

        // CRC
        const uint8_t* pay = in + i + 4;
        uint16_t crc_calc = crc16_ccitt(&in[i+2], 1 + 1 + PACKET_SIZE); // CRC sobre VER+LEN+PAYLOAD
        uint16_t crc_rx   = (uint16_t(in[i+4+PACKET_SIZE]) << 8) | in[i+5+PACKET_SIZE];
        
        if (crc_calc == crc_rx) {
          // ¡Éxito! Frame válido encontrado
          out = decodePacketFast(pay);
          consumed = (i + FRAME_SIZE); // Consumimos el frame completo
          return true;
        } else {
          // SOF válido pero frame dañado (mal CRC) -> correr una posición
          i++;
          continue;
        }
      }
      i++; // Seguir buscando SOF
    }
    
    consumed = i; // Consumimos los bytes basura que no forman un frame
    return false;
  }
} // namespace WSNFrame
