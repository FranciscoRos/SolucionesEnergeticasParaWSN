/**
 * @file XbeeRadio.h
 * @brief Define la clase XBeeRadio, una implementación de RadioInterface para módulos XBee sobre Stream (Serial).
 * @details Esta clase permite tratar un módulo XBee conectado a un puerto serie (HardwareSerial,
 * SoftwareSerial, etc.) como un RadioInterface estándar. Asume que el XBee
 * está en modo transparente (AT).
 */

#pragma once
#include "RadioInterface.h"
#include <Stream.h> // Usamos la clase base Stream para UART

/**
 * @class XBeeRadio
 * @brief Implementación de RadioInterface para módulos XBee que se comunican por un puerto Serie (Stream).
 * * @details Esta clase maneja la comunicación con un XBee en modo transparente (AT).
 * Envuelve un objeto `Stream` (como `Serial` o `SoftwareSerial`) para enviar y recibir
 * datos. Opcionalmente, puede controlar los pines de bajo consumo (SleepRq, OnSleep)
 * si se proporcionan en el constructor.
 */
class XBeeRadio : public RadioInterface {
private:
  Stream& _puertoSerial; ///< Referencia al puerto Stream (ej. Serial, Serial2) usado para la comunicación.
  long _baudios;         ///< Tasa de baudios. Informativo, no se usa para iniciar el puerto.
  int8_t _pinSleepRq;    ///< Pin de control para solicitar modo 'sleep' (activo BAJO). -1 si no se usa.
  int8_t _pinOnSleep;    ///< Pin de estado para leer si el XBee está dormido (BAJO) o despierto (ALTO). -1 si no se usa.

  /**
   * @brief Función de ayuda para esperar a que un pin alcance un estado específico.
   * @details Bucle bloqueante con timeout para monitorear un pin de estado.
   * @param pin El número del pin a leer (debe estar configurado como INPUT).
   * @param estadoDeseado El estado esperado (HIGH o LOW).
   * @param timeout_ms Tiempo máximo de espera en milisegundos.
   * @return true si el pin alcanzó el estado deseado antes del timeout.
   * @return false si se agotó el tiempo (timeout).
   */
  bool _esperarEstadoPin(uint8_t pin, uint8_t estadoDeseado, uint16_t timeout_ms) {
    uint32_t tiempoInicio = millis();
    while (millis() - tiempoInicio < timeout_ms) {
      if (digitalRead(pin) == estadoDeseado) {
        return true;
      }
      delay(1); // Pequeña pausa para no saturar la CPU
    }
    return false; // Timeout
  }

public:
  /**
   * @brief Constructor para la clase XBeeRadio.
   * @param puerto Referencia a un objeto Stream (como `Serial`, `Serial2` o `SoftwareSerial`) para la comunicación.
   * @param baudios La velocidad en baudios del puerto serie. **Nota:** Este valor es solo informativo;
   * el puerto debe ser inicializado externamente con `puerto.begin(baudios)`.
   * @param pinSleepRq Pin de control (GPIO) para poner el XBee a dormir (activo en BAJO). Usar -1 si no se utiliza.
   * @param pinOnSleep Pin de estado (GPIO) que indica si el XBee está despierto (activo en ALTO). Usar -1 si no se utiliza.
   */
  XBeeRadio(Stream& puerto, long baudios, int8_t pinSleepRq, int8_t pinOnSleep)
    : _puertoSerial(puerto),
      _baudios(baudios),
      _pinSleepRq(pinSleepRq),
      _pinOnSleep(pinOnSleep) {}

  /**
   * @brief Configura los pines de control del XBee (si se especificaron).
   * @warning El puerto serie (`puerto`) **debe** ser inicializado por separado en el sketch
   * principal (ej. `Serial2.begin(9600)`) *antes* de llamar a este método.
   * @details Configura los `pinMode` para los pines de sleep y se asegura
   * de que el módulo comience en estado despierto.
   * @return Siempre devuelve `true`.
   */
  bool iniciar() override {
    if (_pinSleepRq >= 0) {
      pinMode(_pinSleepRq, OUTPUT);
    }
    if (_pinOnSleep >= 0) {
      pinMode(_pinOnSleep, INPUT);
    }
    // Por defecto, al iniciar, nos aseguramos de que el módulo esté despierto.
    despertar();
    return true;
  }

  /**
   * @brief Pone el módulo XBee en modo de bajo consumo.
   * @details Pone el pin `sleep_rq` en LOW. Si el pin `on_sleep` está configurado,
   * espera (con timeout) a que este pin confirme el estado de 'dormido' (LOW).
   * @return true si la operación fue exitosa (o si `on_sleep` no está configurado).
   * @return false si `sleep_rq` está configurado pero `on_sleep` no confirmó el estado a tiempo.
   */
  bool dormir() override {
    if (_pinSleepRq < 0) return true; // No se puede dormir si no hay pin de control
    
    digitalWrite(_pinSleepRq, LOW); // Solicitar 'sleep'
    
    if (_pinOnSleep < 0) return true; // No se puede confirmar, asumimos que funcionó
    
    // Esperar confirmación
    return _esperarEstadoPin(_pinOnSleep, LOW, 200); 
  }

  /**
   * @brief Saca al módulo XBee del modo de bajo consumo.
   * @details Pone el pin `sleep_rq` en HIGH. Si el pin `on_sleep` está configurado,
   * espera (con timeout) a que este pin confirme el estado de 'despierto' (HIGH).
   * @return true si la operación fue exitosa (o si `on_sleep` no está configurado).
   * @return false si `sleep_rq` está configurado pero `on_sleep` no confirmó el estado a tiempo.
   */
  bool despertar() override {
    if (_pinSleepRq < 0) return true; // Ya está despierto si no hay pin de control
    
    digitalWrite(_pinSleepRq, HIGH); // Solicitar 'wake'
    
    if (_pinOnSleep < 0) return true; // No se puede confirmar, asumimos que funcionó
        
    // Esperar confirmación
    return _esperarEstadoPin(_pinOnSleep, HIGH, 200); 
  }

  /**
   * @brief Envía datos binarios a través del puerto serie.
   * @details Llama a `_puertoSerial.write()` y luego a `_puertoSerial.flush()`
   * para asegurar que la transmisión saliente se complete (bloqueante).
   * @param buffer Puntero al buffer de datos a enviar.
   * @param longitud Número de bytes a enviar.
   * @return true si se escribieron todos los bytes solicitados, false en caso contrario.
   */
  bool enviar(const uint8_t* buffer, size_t longitud) override {
    size_t bytesEscritos = _puertoSerial.write(buffer, longitud);
    
    // flush() espera a que se complete la transmisión saliente.
    // Útil para asegurar que un comando se envió antes de dormir el módulo.
    _puertoSerial.flush(); 
    
    return bytesEscritos == longitud;
  }

  /**
   * @brief Comprueba cuántos bytes hay disponibles en el buffer de recepción del puerto serie.
   * @return El número de bytes disponibles para leer, resultado de `_puertoSerial.available()`.
   */
  int hayDatosDisponibles() override {
    return _puertoSerial.available();
  }

  /**
   * @brief Lee los datos disponibles del buffer del puerto serie.
   * @details Lee *hasta* `maxLongitud` bytes de los que estén actualmente
   * disponibles en el buffer de `Stream`. No espera a que lleguen más datos.
   * @param buffer Puntero al buffer donde se almacenarán los datos leídos.
   * @param maxLongitud El tamaño máximo del `buffer` de destino.
   * @return El número de bytes realmente leídos y almacenados en el buffer.
   */
  size_t leer(uint8_t* buffer, size_t maxLongitud) override {
    if (maxLongitud == 0) return 0;

    // Primero comprobamos cuántos bytes hay realmente
    int bytesDisponibles = _puertoSerial.available();
    
    if (bytesDisponibles > 0) {
      // Leemos el mínimo entre lo disponible y el tamaño del buffer
      size_t bytesALeer = min((size_t)bytesDisponibles, maxLongitud);
      return _puertoSerial.readBytes(buffer, bytesALeer);
    }
    
    return 0; // No había nada que leer
  }
};
