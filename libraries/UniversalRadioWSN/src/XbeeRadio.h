#pragma once
#include "RadioInterface.h"
#include <Stream.h>

/**
 * @class XBeeRadio
 * @brief Implementación de RadioInterface para módulos XBee que se comunican por un puerto Serie (Stream).
 * 
 * Esta clase maneja la comunicación con un XBee en modo transparente (AT),
 * permitiendo enviar y recibir datos, así como controlar los pines de bajo consumo
 * si están definidos.
 */
class XBeeRadio : public RadioInterface {
private:
  Stream& _puertoSerial;
  long _baudios;
  int8_t _pinSleepRq;
  int8_t _pinOnSleep;

  /**
   * @brief Función de ayuda para esperar a que un pin alcance un estado específico.
   * @param pin El número del pin a leer.
   * @param estadoDeseado El estado esperado (HIGH o LOW).
   * @param timeout_ms Tiempo máximo de espera en milisegundos.
   * @return "true" si el pin alcanzó el estado deseado, "false" si se agotó el tiempo.
   */
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
  /**
   * @brief Constructor para la clase XBeeRadio.
   * @param puerto Referencia a un objeto Stream (como "Serial", "Serial2" o "SoftwareSerial") para la comunicación.
   * @param baudios La velocidad en baudios del puerto serie (informativo, no se usa para iniciar el puerto).
   * @param pinSleepRq Pin de control para poner el XBee a dormir (activo en bajo). Usar -1 si no se utiliza.
   * @param pinOnSleep Pin de estado que indica si el XBee está despierto (activo en alto). Usar -1 si no se utiliza.
   */
  XBeeRadio(Stream& puerto, long baudios, int8_t pinSleepRq, int8_t pinOnSleep)
    : _puertoSerial(puerto),
      _baudios(baudios),
      _pinSleepRq(pinSleepRq),
      _pinOnSleep(pinOnSleep) {}

  /**
   * @brief Configura los pines de control del XBee.
   * @note El puerto serie ("puerto.begin()") debe ser inicializado por separado en el sketch principal
   *       antes de llamar a este método.
   * @return Siempre devuelve "true".
   */
  bool iniciar() override {
    if (_pinSleepRq >= 0) pinMode(_pinSleepRq, OUTPUT);
    if (_pinOnSleep >= 0) pinMode(_pinOnSleep, INPUT);
    // Por defecto, al iniciar, nos aseguramos de que el módulo esté despierto.
    despertar();
    return true;
  }

  /**
   * @brief Pone el módulo XBee en modo de bajo consumo.
   * @note Esta implementación pone el pin "sleep_rq" en LOW y espera una confirmación
   *       en el pin "on_sleep" si está configurado.
   */
  bool dormir() override {
    if (_pinSleepRq < 0) return true; // No se puede dormir si no hay pin
    digitalWrite(_pinSleepRq, LOW);
    if (_pinOnSleep < 0) return true; // No se puede confirmar, asumimos que funcionó
    return _esperarEstadoPin(_pinOnSleep, LOW, 200); // Espera confirmación
  }

  /**
   * @brief Saca al módulo XBee del modo de bajo consumo.
   * @note Esta implementación pone el pin "sleep_rq" en HIGH y espera una confirmación
   *       en el pin "on_sleep" si está configurado.
   */
  bool despertar() override {
    if (_pinSleepRq < 0) return true; // Ya está despierto si no hay pin
    digitalWrite(_pinSleepRq, HIGH);
    if (_pinOnSleep < 0) return true; // No se puede confirmar, asumimos que funcionó
    return _esperarEstadoPin(_pinOnSleep, HIGH, 200); // Espera confirmación
  }

  bool enviar(const uint8_t* buffer, size_t longitud) override {
    // flush() espera a que se complete la transmisión saliente.
    // Es bueno para asegurar que un comando se envió antes de dormir el módulo, por ejemplo.
    size_t bytesEscritos = _puertoSerial.write(buffer, longitud);
    _puertoSerial.flush(); 
    return bytesEscritos == longitud;
  }

  int hayDatosDisponibles() override {
    return _puertoSerial.available();
  }

  /**
   * @brief Lee datos del puerto serie hasta un carácter de nueva línea.
   * @note Esta implementación específica para XBee en modo transparente asume que los
   *       mensajes terminan con "\n". Lee hasta "maxLongitud - 1" para asegurar
   *       espacio para el terminador nulo.
   * @return El número de bytes leídos, sin incluir el terminador.
   */
  size_t leer(uint8_t* buffer, size_t maxLongitud) override {
    if (maxLongitud == 0) return 0;

    int bytesDisponibles = _puertoSerial.available();
    if (bytesDisponibles > 0) {
      size_t bytesALeer = min((size_t)bytesDisponibles, maxLongitud);
      return _puertoSerial.readBytes(buffer, bytesALeer);
    }
    return 0;
  }
};