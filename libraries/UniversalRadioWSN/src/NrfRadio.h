/**
 * @file NrfRadio.h
 * @brief Define la clase NrfRadio, una implementación de RadioInterface para módulos NRF24L01.
 * @details Esta clase utiliza la librería RF24 (https://github.com/nRF24/RF24)
 * para abstraer la configuración, envío y recepción de un módulo NRF24L01+.
 */

#ifndef NRF_RADIO_H
#define NRF_RADIO_H

#include "RadioInterface.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

/**
 * @struct NrfConfig
 * @brief Almacena los parámetros de configuración para el módulo NRF24L01.
 * @note Usa tipos genéricos para no exponer los enums de la librería RF24 al sketch principal.
 */
struct NrfConfig {
  uint8_t cePin;            ///< Pin (GPIO) para Chip Enable (CE).
  uint8_t csnPin;           ///< Pin (GPIO) para Chip Select Not (CSN).
  const byte* writeAddress; ///< Dirección del "pipe" de escritura (generalmente 5 bytes).
  const byte* readAddress;  ///< Dirección del "pipe" 1 de lectura (generalmente 5 bytes).
  uint8_t channel;          ///< Canal de RF (0-125). Debe coincidir entre tx y rx.
  uint16_t dataRate;        ///< Tasa de datos genérica: 250 (250KBPS), 1 (1MBPS), 2 (2MBPS).
  int8_t paLevel;           ///< Nivel de potencia genérico: 0 (MIN), 1 (LOW), 2 (HIGH), 3 (MAX).
};

/**
 * @class NrfRadio
 * @brief Implementación de RadioInterface para módulos NRF24L01 usando la librería RF24.
 * @details Esta clase envuelve la librería `RF24` para proveer una
 * interfaz coherente y estandarizada definida por `RadioInterface`.
 */
class NrfRadio : public RadioInterface {
private:
  RF24 _radio;      ///< Instancia del objeto RF24 de la librería.
  NrfConfig _config; ///< Almacena la configuración proporcionada en el constructor.

public:
  /**
   * @brief Constructor que configura el objeto RF24 con sus pines CE y CSN.
   * @param config Estructura `NrfConfig` con todos los parámetros de inicialización.
   */
  NrfRadio(const NrfConfig& config)
    : _radio(config.cePin, config.csnPin),
      _config(config) {}

  /**
   * @brief Destructor virtual.
   */
  virtual ~NrfRadio() {}

  /**
   * @brief Inicializa el hardware NRF24L01 con la configuración proporcionada.
   * @details Realiza las siguientes acciones:
   * 1. Llama a `_radio.begin()`.
   * 2. Configura el canal, la tasa de datos (traduciendo el valor genérico) y el nivel de potencia.
   * 3. Habilita payloads dinámicos (esencial para `hayDatosDisponibles`).
   * 4. Configura los pipes de escritura y lectura.
   * 5. Pone la radio en modo de escucha (`startListening`).
   * @note Traduce los valores genéricos de NrfConfig a los enums de la librería RF24.
   * @return true si `_radio.begin()` fue exitoso, false en caso contrario.
   */
  bool iniciar() override {
    if (!_radio.begin()) {
      return false; // Fallo al inicializar
    }

    _radio.setChannel(_config.channel);

    // --- TRADUCCIÓN DE DATA RATE ---
    if (_config.dataRate == 250) {
      _radio.setDataRate(RF24_250KBPS);
    } else if (_config.dataRate == 2) {
      _radio.setDataRate(RF24_2MBPS);
    } else {
      _radio.setDataRate(RF24_1MBPS); // Default a 1MBPS si no es 250 o 2
    }
    
    // --- TRADUCCIÓN DE PA LEVEL ---
    // Hacemos un cast directo del valor genérico (int8_t) al enum (rf24_pa_dbm_e).
    _radio.setPALevel((rf24_pa_dbm_e)_config.paLevel);
    
    // Habilitar payloads dinámicos para poder saber el tamaño del paquete recibido
    _radio.enableDynamicPayloads();

    // Configurar pipes para comunicación
    _radio.openWritingPipe(_config.writeAddress);
    _radio.openReadingPipe(1, _config.readAddress); // Usamos el pipe 1 para lectura

    // Por defecto, nos ponemos en modo escucha
    _radio.startListening();
    return true;
  }

  /**
   * @brief Envía un bloque de datos.
   * @details Para enviar, la radio debe dejar de escuchar (`stopListening`),
   * transmitir el paquete (`write`), y luego volver a escuchar (`startListening`).
   * @param buffer Puntero al buffer de datos que se van a enviar.
   * @param longitud Número de bytes a enviar desde el buffer.
   * @return true si el envío fue exitoso (ACK recibido), false en caso contrario (timeout).
   */
  bool enviar(const uint8_t* buffer, size_t longitud) override {
    _radio.stopListening(); // Salir del modo receptor
    
    bool ok = _radio.write(buffer, longitud);
    
    _radio.startListening(); // Volver al modo receptor
    return ok;
  }

  /**
   * @brief Comprueba si hay un paquete disponible y devuelve su tamaño.
   * @details Llama a `_radio.available()` y, si es verdadero, obtiene el tamaño
   * del payload dinámico que acaba de llegar usando `_radio.getDynamicPayloadSize()`.
   * @return El tamaño del payload dinámico recibido en bytes, o 0 si no hay nada.
   */
  int hayDatosDisponibles() override {
    if (_radio.available()) {
      return _radio.getDynamicPayloadSize();
    }
    return 0;
  }

  /**
   * @brief Lee un paquete disponible en el buffer.
   * @details Esta función debe llamarse *después* de que `hayDatosDisponibles()`
   * devuelva un valor mayor que cero. Lee los datos en el buffer proporcionado.
   * @param buffer Puntero a un buffer donde se almacenarán los datos leídos.
   * @param maxLongitud El tamaño máximo del `buffer` de destino.
   * @return El número de bytes realmente leídos (limitado por `maxLongitud`).
   */
  size_t leer(uint8_t* buffer, size_t maxLongitud) override {
    // Obtenemos el tamaño del payload. Es importante en caso de que
    // hayDatosDisponibles() no se haya llamado, aunque sea redundante si sí se llamó.
    size_t payloadSize = _radio.getDynamicPayloadSize();
    if (payloadSize == 0) return 0;

    // Leemos solo la cantidad de bytes que caben en el buffer
    size_t bytesALeer = min(payloadSize, maxLongitud);
    _radio.read(buffer, bytesALeer);
    
    // Si payloadSize > maxLongitud, los bytes restantes se descartan.
    
    return bytesALeer;
  }

  /**
   * @brief Pone el módulo NRF24L01 en modo de bajo consumo (Power Down).
   * @details Llama a `_radio.powerDown()`.
   * @return true siempre.
   */
  bool dormir() override {
    _radio.powerDown();
    return true;
  }

  /**
   * @brief Saca al módulo NRF24L01 del modo de bajo consumo.
   * @details Llama a `_radio.powerUp()`.
   * @note Incluye un pequeño `delay(5)` para permitir que el oscilador
   * de la radio se estabilice antes de continuar.
   * @return true siempre.
   */
  bool despertar() override {
    _radio.powerUp();
    
    // El datasheet recomienda esperar un corto tiempo para que el
    // oscilador se estabilice al salir de powerDown.
    delay(5); 
    return true;
  }
};

#endif // NRF_RADIO_H
