/**
 * @file LoraRadio.h
 * @brief Define la clase LoraRadio, una implementación de RadioInterface para módulos LoRa.
 * @details Esta clase utiliza la librería sandeepmistry/LoRa (https://github.com/sandeepmistry/arduino-LoRa)
 * para abstraer la configuración, envío, recepción y gestión de energía
 * de un módulo LoRa (como el SX1276/7/8).
 */

#ifndef LORA_RADIO_H
#define LORA_RADIO_H

#include <LoRa.h>
#include "RadioInterface.h" 

/**
 * @struct LoRaConfig
 * @brief Almacena todos los parámetros de configuración para un módulo LoRa.
 * @note Esta estructura se pasa al constructor de LoraRadio para una inicialización sencilla.
 * Facilita la configuración de la radio con múltiples parámetros de una sola vez.
 */
struct LoRaConfig {
  long frequency;       ///< Frecuencia de operación de LoRa en Hz (ej. 915E6, 868E6, 433E6).
  int txPower;          ///< Potencia de transmisión en dB (ej. 17, 20).
  int spreadingFactor;  ///< Factor de dispersión (ej. 7-12). Un valor más alto es más lento pero más robusto.
  long signalBandwidth; ///< Ancho de banda de la señal en Hz (ej. 125E3, 250E3).
  int codingRate;       ///< Tasa de codificación (ej. 5-8, para 4/5 a 4/8).
  int syncWord;         ///< Palabra de sincronización (0x00-0xFF). Debe coincidir entre tx y rx.
  int csPin;            ///< Pin (GPIO) para Chip Select (SS).
  int resetPin;         ///< Pin (GPIO) para Reset (RST).
  int irqPin;           ///< Pin (GPIO) para Interrupción (IRQ/DIO0).
};

/**
 * @class LoraRadio
 * @brief Implementación de la interfaz RadioInterface para módulos LoRa.
 * @details Esta clase envuelve la librería `sandeepmistry/LoRa` para proveer una
 * interfaz coherente y estandarizada definida por `RadioInterface`.
 */
class LoraRadio : public RadioInterface {
private:
  LoRaConfig _config; ///< Almacena la configuración proporcionada en el constructor.

public:
  /**
   * @brief Constructor de la clase LoraRadio.
   * @param config Estructura `LoRaConfig` con todos los parámetros de inicialización necesarios.
   */
  LoraRadio(const LoRaConfig& config) : _config(config) {}

  /**
   * @brief Inicializa el hardware LoRa con los parámetros de la configuración.
   * @details Configura los pines (CS, RST, IRQ) e intenta conectarse a la frecuencia
   * especificada. Luego, aplica el resto de los parámetros (potencia, SF, BW, etc.).
   * @return true si `LoRa.begin()` fue exitoso, false en caso contrario.
   */
  bool iniciar() override {
    // Configura los pines específicos para la placa
    LoRa.setPins(_config.csPin, _config.resetPin, _config.irqPin);
    
    // Intenta inicializar el módulo LoRa en la frecuencia especificada
    if (!LoRa.begin(_config.frequency)) {
      return false; // Falló la inicialización
    }
    
    // Aplica el resto de la configuración
    LoRa.setTxPower(_config.txPower);
    LoRa.setSpreadingFactor(_config.spreadingFactor);
    LoRa.setSignalBandwidth(_config.signalBandwidth);
    LoRa.setCodingRate4(_config.codingRate);
    LoRa.setSyncWord(_config.syncWord);
    
    return true; // Inicialización exitosa
  }

  /**
   * @brief Envuelve los datos en un paquete LoRa y los transmite.
   * @details Inicia un paquete LoRa, escribe el buffer de datos y cierra el paquete
   * para comenzar la transmisión.
   * @param buffer Puntero al buffer de datos que se van a enviar.
   * @param longitud Número de bytes a enviar desde el buffer.
   * @return true si se pudo *iniciar* el paquete (`LoRa.beginPacket()`), false si no.
   */
  bool enviar(const uint8_t* buffer, size_t longitud) override {
    if (LoRa.beginPacket()) {
      LoRa.write(buffer, longitud);
      LoRa.endPacket(); // Inicia la transmisión
      return true;
    }
    return false; // La radio estaba ocupada (ej. transmitiendo)
  }

  /**
   * @brief Comprueba si se ha recibido un paquete LoRa completo.
   * @details Llama a `LoRa.parsePacket()`, que comprueba la interrupción IRQ
   * y prepara la librería para la lectura del paquete.
   * @return El tamaño del paquete recibido en bytes, o 0 si no hay paquete disponible.
   */
  int hayDatosDisponibles() override {
    return LoRa.parsePacket();
  }

  /**
   * @brief Lee los datos del paquete LoRa recibido previamente.
   * @details Esta función debe llamarse *después* de que `hayDatosDisponibles()`
   * devuelva un valor mayor que cero. Lee bytes del búfer FIFO de LoRa
   * hasta que se agoten o se alcance `maxLongitud`.
   * @param buffer Puntero a un buffer donde se almacenarán los datos leídos.
   * @param maxLongitud El tamaño máximo del `buffer` de destino.
   * @return El número de bytes realmente leídos del paquete.
   */
  size_t leer(uint8_t* buffer, size_t maxLongitud) override {
    size_t bytesLeidos = 0;
    while (LoRa.available() && bytesLeidos < maxLongitud) {
      buffer[bytesLeidos] = (uint8_t)LoRa.read();
      bytesLeidos++;
    }
    return bytesLeidos;
  }

  /**
   * @brief Obtiene el RSSI (Indicador de Fuerza de Señal Recibida) del último paquete LoRa recibido.
   * @return El valor del RSSI en dBm (normalmente un valor negativo).
   */
  int obtenerRSSI() override {
    return LoRa.packetRssi();
  }

  /**
   * @brief Pone el módulo LoRa en modo de bajo consumo (Sleep).
   * @details Esto apaga la radio para ahorrar energía. Se necesita `despertar()`
   * para volver a recibir o transmitir.
   * @return true siempre (basado en la implementación actual de la librería LoRa).
   */
  bool dormir() override {
    LoRa.sleep();
    return true;
  }

  /**
   * @brief Pone el módulo LoRa en modo de espera (Standby/Idle).
   * @details Este es el modo por defecto para recibir (si se llama a `receive()`)
   * o estar listo para transmitir. Se usa para salir del modo `dormir()`.
   * @return true siempre (basado en la implementación actual de la librería LoRa).
   */
  bool despertar() override {
    LoRa.idle(); // El modo Idle (Standby) es el estado "despierto" por defecto
    return true;
  }
};

#endif // LORA_RADIO_H
