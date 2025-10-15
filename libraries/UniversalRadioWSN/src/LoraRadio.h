#pragma once
#include "RadioInterface.h"
#include <LoRa.h>

// Estructura que agrupa todos los parámetros de configuración para LoRa.
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

class LoRaRadio : public RadioInterface {
private:
    LoRaConfig _config;

public:
    LoRaRadio(const LoRaConfig& config) : _config(config) {}

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
    
    // Implementación de la nueva función para obtener RSSI
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
};

