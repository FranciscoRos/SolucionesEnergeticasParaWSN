#ifndef LORA_RADIO_H
#define LORA_RADIO_H

#include <LoRa.h>
#include "RadioInterface.h"

/**
 * @struct LoRaConfig
 * @brief Almacena todos los parámetros de configuración para un módulo LoRa.
 * @note Esta estructura facilita la inicialización de la radio con múltiples parámetros.
 */
struct LoRaConfig {
  long frequency;
  int txPower;
  int spreadingFactor;
  long signalBandwidth;
  int codingRate;
  int syncWord;
  int csPin;
  int resetPin;
  int irqPin;
};

/**
 * @class LoraRadio
 * @brief Implementación de RadioInterface para módulos LoRa usando la librería sandeepmistry/LoRa.
 */
class LoraRadio : public RadioInterface {
private:
  LoRaConfig _config;

public:
  /**
   * @brief Constructor que inicializa la radio con una configuración específica.
   * @param config Estructura LoRaConfig con todos los parámetros necesarios.
   */
  LoraRadio(const LoRaConfig& config) : _config(config) {}

  /**
   * @brief Inicializa el hardware LoRa con los parámetros de la configuración.
   * @note Llama a las funciones de configuración de la librería LoRa.
   * @return true si LoRa.begin() fue exitoso, false en caso contrario.
   */
  bool iniciar() override {
    LoRa.setPins(_config.csPin, _config.resetPin, _config.irqPin);
    if (!LoRa.begin(_config.frequency)) {
      return false;
    }
    LoRa.setTxPower(_config.txPower);
    LoRa.setSpreadingFactor(_config.spreadingFactor);
    LoRa.setSignalBandwidth(_config.signalBandwidth);
    LoRa.setCodingRate4(_config.codingRate);
    LoRa.setSyncWord(_config.syncWord);
    return true;
  }

  /**
   * @brief Envuelve los datos en un paquete LoRa y los transmite.
   * @return true si el paquete se inició correctamente, false si no.
   */
  bool enviar(const uint8_t* buffer, size_t longitud) override {
    if (LoRa.beginPacket()) {
      LoRa.write(buffer, longitud);
      LoRa.endPacket();
      return true;
    }
    return false;
  }

  /**
   * @brief Comprueba si se ha recibido un paquete LoRa completo.
   * @note Llama a `LoRa.parsePacket()`, que prepara la librería para la lectura.
   * @return El tamaño del paquete recibido en bytes, o 0 si no hay paquete.
   */
  int hayDatosDisponibles() override {
    return LoRa.parsePacket();
  }

  // La documentación para este método se hereda de RadioInterface.
  size_t leer(uint8_t* buffer, size_t maxLongitud) override {
    size_t bytesLeidos = 0;
    while (LoRa.available() && bytesLeidos < maxLongitud) {
      buffer[bytesLeidos] = (uint8_t)LoRa.read();
      bytesLeidos++;
    }
    return bytesLeidos;
  }

  /**
   * @brief Obtiene el RSSI del último paquete LoRa recibido.
   * @return El valor del RSSI en dBm.
   */
  int obtenerRSSI() override {
    return LoRa.packetRssi();
  }

  /**
   * @brief Pone el módulo LoRa en modo de bajo consumo.
   */
  bool dormir() override {
    LoRa.sleep();
    return true;
  }

  /**
   * @brief Pone el módulo LoRa en modo de espera (Standby/Idle).
   */
  bool despertar() override {
    LoRa.idle();
    return true;
  }
};

#endif
