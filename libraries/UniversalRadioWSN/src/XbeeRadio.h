#pragma once
#include "RadioInterface.h"
#include <Stream.h>

class XBeeRadio : public RadioInterface {
private:
  Stream& _puertoSerial;
  long _baudios;
  int8_t _pinSleepRq;
  int8_t _pinOnSleep;

  bool _esperarEstadoPin(uint8_t pin, uint8_t estadoDeseado, uint16_t timeout_ms) {
    uint32_t tiempoInicio = millis();
    while (millis() - tiempoInicio < timeout_ms) {
      if (digitalRead(pin) == estadoDeseado) {
        return true;
      }
      delay(1);
    }
    return false;
  }

public:
  XBeeRadio(Stream& puerto, long baudios, int8_t pinSleepRq, int8_t pinOnSleep)
    : _puertoSerial(puerto),
      _baudios(baudios),
      _pinSleepRq(pinSleepRq),
      _pinOnSleep(pinOnSleep) {}

  bool iniciar() override {
    pinMode(_pinSleepRq, OUTPUT);
    pinMode(_pinOnSleep, INPUT);
    digitalWrite(_pinSleepRq, HIGH);
    return true;
  }

  bool dormir() override {
    digitalWrite(_pinSleepRq, LOW);
    return _esperarEstadoPin(_pinOnSleep, LOW, 200);
  }

  bool despertar() override {
    digitalWrite(_pinSleepRq, HIGH);
    return _esperarEstadoPin(_pinOnSleep, HIGH, 200);
  }

  bool enviar(const uint8_t* buffer, size_t longitud) override {
    _puertoSerial.flush();
    size_t bytesEscritos = _puertoSerial.write(buffer, longitud);
    return bytesEscritos == longitud;
  }

  int hayDatosDisponibles() override {
    return _puertoSerial.available();
  }

  size_t leer(uint8_t* buffer, size_t maxLongitud) override {
    size_t bytesLeidos = _puertoSerial.readBytesUntil('\n', buffer, maxLongitud - 1);
    buffer[bytesLeidos] = '\0';
    return bytesLeidos;
  }

  // El método leerComoString() se ha eliminado para usar la versión de la clase base.
};