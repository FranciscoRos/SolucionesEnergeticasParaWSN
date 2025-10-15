#ifndef LORA_RADIO_H
#define LORA_RADIO_H

#include <LoRa.h>
#include "RadioInterface.h"

// Estructura para pasar la configuración de forma ordenada
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

class LoraRadio : public RadioInterface {
private:
  LoRaConfig _config;

public:
  // El constructor ahora recibe el objeto de configuración
  LoraRadio(const LoRaConfig& config) : _config(config) {}

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

  bool enviar(const uint8_t* buffer, size_t longitud) override {
    if (LoRa.beginPacket()) {
      LoRa.write(buffer, longitud);
      LoRa.endPacket();
      return true;
    }
    return false;
  }

  int hayDatosDisponibles() override {
    return LoRa.parsePacket();
  }

  size_t leer(uint8_t* buffer, size_t maxLongitud) override {
    size_t bytesLeidos = 0;
    while (LoRa.available() && bytesLeidos < maxLongitud) {
      buffer[bytesLeidos] = (uint8_t)LoRa.read();
      bytesLeidos++;
    }
    return bytesLeidos;
  }

  int obtenerRSSI() override {
    return LoRa.packetRssi();
  }

  bool dormir() override {
    LoRa.sleep();
    return true;
  }

  bool despertar() override {
    LoRa.idle();
    return true;
  }
};

#endif
