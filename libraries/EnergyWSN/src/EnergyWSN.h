#pragma once
#include <Arduino.h>
#include <LowPower.h>
#include "RadioInterface.h" // << CAMBIO: Incluimos la interfaz de radio

/** Esta es una librería para manejar el modo sueño de una red de sensores,
  * junto con una línea para energizar o desenergizar los sensores.
  * Ahora es agnóstica al tipo de radio utilizado.
  * Autores: Francisco Rosales, Omar Tox, 2025-09.
  * Modificado para usar RadioInterface.
**/

class EnergyWSN {
public:
  /* Estructura que guarda los pines específicos del nodo (excluyendo la radio) */
  struct Pins {
    // << CAMBIO: Se eliminaron sleepRq y onSleep. Esos ahora los maneja la clase de la radio.
    uint8_t pwrSens;   // Energizar sensores Arduino → MOSFET/load-switch (HIGH = ON, salvo invertPwr)
    int8_t  vbatSense; // opcional: ADC batería (−1 si no se usa)
  };

  /* Estructura de configuración */
  struct Cfg {
    Pins  pins;              // Pines utilizados por proyecto
    bool  invertPwr = false; // Lógica del gate de voltaje de los sensores (Si se abre con HIGH se queda así)
    bool  bootSleep = true;  // Estado en que arranca el sistema al encenderse, apagado por defecto
  };

  /**
   * @brief Inicializa los pines y asocia un módulo de radio.
   * @param cfg La configuración de pines de EnergyWSN.
   * @param radio Puntero al objeto de radio (ej. LoraRadio, XBeeRadio) que se va a controlar.
   */
  void begin(const Cfg& cfg, RadioInterface* radio) { // << CAMBIO: Se añade un puntero a RadioInterface
    _cfg = cfg;
    _radio = radio; // << CAMBIO: Guardamos la referencia a la radio

    pinMode(_cfg.pins.pwrSens, OUTPUT);
    
    if (_cfg.pins.vbatSense >= 0) {
      pinMode(_cfg.pins.vbatSense, INPUT);
    }
    powerSensors(false);

    if (_cfg.bootSleep){ 
      sleepRadio();
    } else {
      wakeRadio();
    }
  }

  /* Despierta la radio usando la interfaz */
  bool wakeRadio() { // << CAMBIO: Ya no necesita timeout, la implementación de la radio se encarga.
    if (_radio) {
      return _radio->despertar();
    }
    return false; // No hay radio asociada
  }

  /* Pone la radio a dormir usando la interfaz */
  bool sleepRadio() { // << CAMBIO: Ya no necesita timeout.
    if (_radio) {
      return _radio->dormir();
    }
    return false; // No hay radio asociada
  }
  
  /** Energizar sensores */
  void powerSensors(bool on) {
    bool level = _cfg.invertPwr ? !on : on;
    digitalWrite(_cfg.pins.pwrSens, level ? HIGH : LOW);
  }

  /** Suspender el programa durante un tiempo específico (ms) **/
  void sleepFor_ms(uint32_t ms) {
    while (ms >= 8000) { LowPower.powerDown(SLEEP_8S,  ADC_OFF, BOD_OFF); ms -= 8000; }
    if    (ms >= 4000) { LowPower.powerDown(SLEEP_4S,  ADC_OFF, BOD_OFF); ms -= 4000; }
    if    (ms >= 2000) { LowPower.powerDown(SLEEP_2S,  ADC_OFF, BOD_OFF); ms -= 2000; }
    if    (ms >= 1000) { LowPower.powerDown(SLEEP_1S,  ADC_OFF, BOD_OFF); ms -= 1000; }
    while (ms >= 500)  { LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF); ms -= 500; }
    while (ms >= 250)  { LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF); ms -= 250; }
    while (ms >= 120)  { LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF); ms -= 120; }
    while (ms >= 60)   { LowPower.powerDown(SLEEP_60MS,  ADC_OFF, BOD_OFF); ms -= 60; }
    while (ms >= 30)   { LowPower.powerDown(SLEEP_30MS,  ADC_OFF, BOD_OFF); ms -= 30; }
    while (ms >= 15)   { LowPower.powerDown(SLEEP_15MS,  ADC_OFF, BOD_OFF); ms -= 15; }
  }

private:
  Cfg _cfg;
  RadioInterface* _radio = nullptr; // << CAMBIO: Puntero para almacenar el objeto de radio.
};