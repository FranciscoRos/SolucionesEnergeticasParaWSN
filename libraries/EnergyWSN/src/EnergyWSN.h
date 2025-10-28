/**
 * @file EnergyWSN.h
 * @brief Define la clase EnergyWSN para la gestión de energía y sueño en un nodo WSN.
 * @details Esta librería maneja el encendido/apagado de sensores y los ciclos de
 * sueño del microcontrolador (usando LowPower.h). Ahora es agnóstica
 * al tipo de radio, controlándolo a través de la abstracción RadioInterface.
 * @authors Francisco Rosales, Omar Tox
 * @date 2025-09
 */

#pragma once
#include <Arduino.h>
#include <LowPower.h>
#include "RadioInterface.h" // Incluimos la interfaz de radio

/**
 * @class EnergyWSN
 * @brief Gestiona el estado de energía de los sensores y el módulo de radio.
 * @details Proporciona métodos para energizar/desenergizar una línea de sensores
 * y para poner a dormir/despertar un módulo de radio genérico que
 * cumpla con la RadioInterface. También maneja el sueño profundo del MCU.
 */
class EnergyWSN {
public:
  /**
   * @struct Pins
   * @brief Estructura que guarda los pines de control de energía del nodo.
   * @details No incluye los pines de la radio, solo los pines de alimentación
   * de sensores y (opcionalmente) lectura de batería.
   */
  struct Pins {
    uint8_t pwrSens;   ///< Pin para energizar sensores (MOSFET/load-switch). HIGH = ON (salvo invertPwr).
    int8_t  vbatSense; ///< Pin opcional para el ADC de batería. Usar -1 si no se utiliza.
  };

  /**
   * @struct Cfg
   * @brief Estructura de configuración para la clase EnergyWSN.
   */
  struct Cfg {
    Pins   pins;            ///< Estructura con los pines utilizados por el nodo.
    bool   invertPwr = false; ///< true si la lógica de `pwrSens` es invertida (LOW = ON).
    bool   bootSleep = true;  ///< true si el sistema debe arrancar con la radio dormida (por defecto).
  };

  /**
   * @brief Inicializa los pines de control y asocia un módulo de radio.
   * @param cfg La configuración de pines (Pins) y lógica (Cfg) de EnergyWSN.
   * @param radio Puntero a un objeto de radio ya instanciado (ej. LoraRadio, NrfRadio)
   * que se va a controlar.
   */
  void begin(const Cfg& cfg, RadioInterface* radio) { 
    _cfg = cfg;
    _radio = radio; // Almacena el puntero a la radio

    pinMode(_cfg.pins.pwrSens, OUTPUT);
    
    if (_cfg.pins.vbatSense >= 0) {
      pinMode(_cfg.pins.vbatSense, INPUT);
    }
    
    // Asegurarse de que los sensores inicien apagados
    powerSensors(false);

    // Aplicar el estado de arranque de la radio
    if (_cfg.bootSleep){ 
      sleepRadio();
    } else {
      wakeRadio();
    }
  }

  /**
   * @brief Despierta el módulo de radio usando la interfaz.
   * @details Llama a `_radio->despertar()`.
   * @return true si la radio se despertó o si no hay radio asociada (no falla).
   * @return false si `_radio->despertar()` devolvió false.
   */
  bool wakeRadio() { 
    if (_radio) {
      return _radio->despertar();
    }
    return true; // No hay radio que despertar
  }

  /**
   * @brief Pone el módulo de radio a dormir usando la interfaz.
   * @details Llama a `_radio->dormir()`.
   * @return true si la radio se durmió o si no hay radio asociada (no falla).
   * @return false si `_radio->dormir()` devolvió false.
   */
  bool sleepRadio() { 
    if (_radio) {
      return _radio->dormir();
    }
    return true; // No hay radio que dormir
  }
  
  /**
   * @brief Energiza o desenergiza la línea de alimentación de los sensores.
   * @details Controla el pin `pwrSens` respetando la lógica `invertPwr`.
   * @param on true para encender los sensores, false para apagarlos.
   */
  void powerSensors(bool on) {
    // Calcula el nivel lógico (HIGH/LOW) basado en 'on' y la posible inversión
    bool level = _cfg.invertPwr ? !on : on;
    digitalWrite(_cfg.pins.pwrSens, level ? HIGH : LOW);
  }

  /**
   * @brief Suspende la ejecución del MCU (sueño profundo) durante un tiempo específico.
   * @details Utiliza la librería LowPower.h para un sueño de bajo consumo,
   * desactivando ADC y BOD. La función es bloqueante.
   * @param ms El tiempo total en milisegundos que el MCU debe dormir.
   */
  void sleepFor_ms(uint32_t ms) {
    // Descompone el tiempo 'ms' en los períodos de sueño soportados por LowPower.h
    while (ms >= 8000) { LowPower.powerDown(SLEEP_8S,    ADC_OFF, BOD_OFF); ms -= 8000; }
    if    (ms >= 4000) { LowPower.powerDown(SLEEP_4S,    ADC_OFF, BOD_OFF); ms -= 4000; }
    if    (ms >= 2000) { LowPower.powerDown(SLEEP_2S,    ADC_OFF, BOD_OFF); ms -= 2000; }
    if    (ms >= 1000) { LowPower.powerDown(SLEEP_1S,    ADC_OFF, BOD_OFF); ms -= 1000; }
    while (ms >= 500)  { LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF); ms -= 500; }
    while (ms >= 250)  { LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF); ms -= 250; }
    while (ms >= 120)  { LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF); ms -= 120; }
    while (ms >= 60)   { LowPower.powerDown(SLEEP_60MS,  ADC_OFF, BOD_OFF); ms -= 60; }
    while (ms >= 30)   { LowPower.powerDown(SLEEP_30MS,  ADC_OFF, BOD_OFF); ms -= 30; }
    while (ms >= 15)   { LowPower.powerDown(SLEEP_15MS,  ADC_OFF, BOD_OFF); ms -= 15; }
  }

private:
  Cfg _cfg; ///< Instancia de la configuración de pines y lógica.
  
  ///< Puntero a la instancia de radio (LoRa, NRF, Xbee) que se está gestionando.
  RadioInterface* _radio = nullptr; 
};
