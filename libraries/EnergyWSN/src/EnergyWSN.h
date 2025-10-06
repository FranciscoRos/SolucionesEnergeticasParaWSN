#pragma once
#include <Arduino.h>
#include <LowPower.h>
/** Esta es una librería para manejar el modo sueño de una red de sensores con XBee o cualquier comunicación inalámbrica,
  *  junto con una línea para energizar o desenergizar los sensores.
  *  Autores: Francisco Rosales, Omar Tox, 2025-09.
  *
**/

class EnergyWSN {
public:
  /* Estructura que guarda los pines específicos del nodo */
  struct Pins {
    uint8_t sleepRq;   // Solicitud de dormir Arduino → XBee SLEEP_RQ (adaptado a 3.3V) 
    uint8_t onSleep;   // Verificar estado dormido Arduino ← XBee ON/SLEEP (3.3V, entrada)
    uint8_t pwrSens;   // Energizar sensores Arduino → MOSFET/load-switch (HIGH = ON, salvo invertPwr)
    int8_t  vbatSense; // opcional: ADC batería (−1 si no se usa)
  };
  /* Estructura de configuración */
  struct Cfg {
    Pins  pins;              // Pines utilizados por proyecto
    bool  invertPwr = false; // Lógica del gate de voltaje de los sensores (Si se abre con HIGH se queda así)
    bool  bootSleep = true;  // Estado en que arranca el sistema al encenderse, apagado por defecto
  };

  /* Inicializar los pines dados con la lógica */
  void begin(const Cfg& cfg) {
    _cfg = cfg;
    pinMode(_cfg.pins.sleepRq, OUTPUT);
    pinMode(_cfg.pins.onSleep, INPUT);     
    pinMode(_cfg.pins.pwrSens, OUTPUT);
    
    if (_cfg.pins.vbatSense >= 0) {
      pinMode(_cfg.pins.vbatSense, INPUT);
    }
    powerSensors(false);


    if (_cfg.bootSleep){ sleepRadio();
    } else {wakeRadio();}
  }

  /* Enciende el XBEE */
  bool wakeRadio(uint16_t timeout_ms = 200) {
    digitalWrite(_cfg.pins.sleepRq, HIGH);
    return waitLevel(_cfg.pins.onSleep, HIGH, timeout_ms);
  }

  /* Poner XBee a dormir */
  bool sleepRadio(uint16_t timeout_ms = 200) {
    digitalWrite(_cfg.pins.sleepRq, LOW);
    return waitLevel(_cfg.pins.onSleep, LOW, timeout_ms);
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

  bool waitLevel(uint8_t pin, uint8_t targetLevel, uint16_t timeout_ms) {
    uint32_t t0 = millis();
    while (millis() - t0 < timeout_ms) {
      if (digitalRead(pin) == targetLevel) return true;
      LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
    }
    return (digitalRead(pin) == targetLevel);
  }
};
