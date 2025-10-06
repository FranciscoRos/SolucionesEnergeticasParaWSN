#pragma once
#include <Arduino.h>

// Opcional: integra EnergyWSN si lo usas
// #include "EnergyWSN.h"

class AdaptiveTXWSN {
public:
  struct Cfg {
    // --- Lectura de bateria ---
    int8_t  pinAdcBateria           = -1;     // Pin ADC para leer bateria (-1 si inyectas el voltaje)
    float   voltajeReferenciaAdc    = 5.0f;   // Vref del ADC (5.0 AVcc tipico; 1.1 si ref interna)
    // Divisor: Vin -> Rarriba ->(ADC)-> Rabajo -> GND
    float   divisorRArriba_k        = 100.0f; // kΩ
    float   divisorRAbajo_k         =  33.0f; // kΩ
    uint8_t muestrasPromedioAdc     = 8;      // Muestras para promediar VBAT

    // --- Umbrales (VOLTIOS) ---
    //    V >= umbralAlto_V  -> nivel OPTIMO
    //    umbralMedio_V <= V < umbralAlto_V -> nivel MEDIO
    //    V < umbralMedio_V -> nivel MINIMO
    float umbralAlto_V              = 3.90f;
    float umbralMedio_V             = 3.60f;

    // --- Histeresis (fraccion del umbral) para evitar saltos ---
    float fraccionHisteresis        = 0.03f;  // 3%

    // --- Periodos de envio (ms) por nivel ---
    uint32_t periodoAlto_ms         = 5000;    // 5 s
    uint32_t periodoMedio_ms        = 15000;   // 15 s
    uint32_t periodoBajo_ms         = 120000;  // 2 min

    // --- Corte duro: por debajo NO se transmite ---
    float corteVoltaje_V            = 3.40f;   // Si VBAT < corte -> no transmitir
  };

  enum Level : uint8_t { LOW=0, MID=1, HIGH=2 }; // (BAJO, MEDIO, ALTO)

  //  permite fijar umbrales de voltaje e intervalos de envío desde el arranque
  void begin(const Cfg& cfg,
            float umbralAlto_V, float umbralMedio_V,
            float corteVoltaje_V,
            float fraccionHisteresis,
            uint32_t periodoAlto_ms,
            uint32_t periodoMedio_ms,
            uint32_t periodoBajo_ms)
  {
    _configuracion = cfg;

    // Sobrescribir rangos/umbrales y periodos
    _configuracion.umbralAlto_V        = umbralAlto_V;
    _configuracion.umbralMedio_V       = umbralMedio_V;
    _configuracion.corteVoltaje_V      = corteVoltaje_V;
    _configuracion.fraccionHisteresis  = fraccionHisteresis;
    _configuracion.periodoAlto_ms      = periodoAlto_ms;
    _configuracion.periodoMedio_ms     = periodoMedio_ms;
    _configuracion.periodoBajo_ms      = periodoBajo_ms;

    // Inicialización de hardware/estado
    if (_configuracion.pinAdcBateria >= 0) {
      pinMode(_configuracion.pinAdcBateria, INPUT);
    }
    _nivelEnergeticoActual = HIGH;      // Se recalibra en el primer tick()
    _msProximoEnvio        = millis();
  }


  // Llamar en loop(). Devuelve true cuando TOCA transmitir
  bool tick() {
    // 1) Medir bateria 
    float voltajeBateria_V = (_usarLecturaInyectada)
                              ? _voltajeInyectado_V
                              : readBatteryVolts();
    _ultimoVoltajeMedido_V = voltajeBateria_V;

    // 2) Aplicar corte duro
    if (voltajeBateria_V < _configuracion.corteVoltaje_V) {
      _bloqueadoPorCorte = true;
      return false;
    }
    _bloqueadoPorCorte = false;

    // 3) Actualizar nivel con histeresis
    actualizarNivelConHisteresis(voltajeBateria_V);

    // 4) Temporizador
    uint32_t ahoraMs = millis();
    if ((int32_t)(ahoraMs - _msProximoEnvio) >= 0) {
      _msProximoEnvio = ahoraMs + currentPeriod();
      return true; // toca transmitir
    }
    return false;
  }

  // Inyectar voltaje medido externamente
  void setBatteryVolts(float voltajeBateria_V) {
    _usarLecturaInyectada = true;
    _voltajeInyectado_V   = voltajeBateria_V;
  }

  // Lectura de bateria }
  float readBatteryVolts() {
    if (_configuracion.pinAdcBateria < 0) return _ultimoVoltajeMedido_V; // sin pin: devolver ultimo
    uint32_t acumuladorAdc = 0;
    uint8_t nMuestras = max<uint8_t>(1, _configuracion.muestrasPromedioAdc);
    for (uint8_t i = 0; i < nMuestras; ++i) {
      acumuladorAdc += analogRead(_configuracion.pinAdcBateria);
      delayMicroseconds(250);
    }
    float promedioCuentasAdc = (float)acumuladorAdc / nMuestras;
    // Nota: 1023.0f supone ADC de 10 bits (ATmega328P). Ajusta si portas a otro MCU.
    float voltajeAdc_V = (promedioCuentasAdc / 1023.0f) * _configuracion.voltajeReferenciaAdc;
    float factorDivisor = (_configuracion.divisorRArriba_k + _configuracion.divisorRAbajo_k)
                          / _configuracion.divisorRAbajo_k; // Vin = Vadc * factor
    return voltajeAdc_V * factorDivisor;
  }

  // Getters compatibles con tu API
  Level   level()          const { return _nivelEnergeticoActual; }
  float   lastVolts()      const { return _ultimoVoltajeMedido_V; }
  bool    isCutoff()       const { return _bloqueadoPorCorte; }
  uint32_t currentPeriod() const {
    switch (_nivelEnergeticoActual) {
      case HIGH: return _configuracion.periodoAlto_ms;
      case MID:  return _configuracion.periodoMedio_ms;
      default:   return _configuracion.periodoBajo_ms;
    }
  }

  //Definir periodos de envio
  void setPeriods(uint32_t alto_ms, uint32_t medio_ms, uint32_t bajo_ms) {
    _configuracion.periodoAlto_ms  = alto_ms;
    _configuracion.periodoMedio_ms = medio_ms;
    _configuracion.periodoBajo_ms  = bajo_ms;
  }
  //Definir umbrales de voltaje 
  void setThresholds(float umbralAlto_V, float umbralMedio_V) {
    _configuracion.umbralAlto_V  = umbralAlto_V;
    _configuracion.umbralMedio_V = umbralMedio_V;
  }
  //Definir fraccion de histéresis
  void setHysteresisPct(float fraccion) { _configuracion.fraccionHisteresis = fraccion; }

private:
  Cfg       _configuracion;
  Level     _nivelEnergeticoActual  = HIGH;
  uint32_t  _msProximoEnvio         = 0;
  float     _ultimoVoltajeMedido_V  = 0.0f;
  bool      _bloqueadoPorCorte      = false;

  bool      _usarLecturaInyectada   = false;
  float     _voltajeInyectado_V     = 0.0f;

  void actualizarNivelConHisteresis(float voltajeBateria_V) {
    // diferencials de histéresis alrededor de los umbrales
    float diferencialAlto_V  = _configuracion.umbralAlto_V  * _configuracion.fraccionHisteresis;
    float diferencialMedio_V = _configuracion.umbralMedio_V * _configuracion.fraccionHisteresis;

    switch (_nivelEnergeticoActual) {
      case HIGH: // ALTO -> MEDIO si baja por debajo de (alto - diferencial)
        if (voltajeBateria_V < (_configuracion.umbralAlto_V - diferencialAlto_V))
          _nivelEnergeticoActual = MID;
        break;

      case MID:
        // MEDIO -> ALTO si supera (alto + diferencial)
        if (voltajeBateria_V >= (_configuracion.umbralAlto_V + diferencialAlto_V)) { _nivelEnergeticoActual = HIGH; break; }
        // MEDIO -> BAJO si baja por debajo de (medio - diferencial)
        if (voltajeBateria_V <  (_configuracion.umbralMedio_V - diferencialMedio_V)) { _nivelEnergeticoActual = LOW;  break; }
        break;

      case LOW: // BAJO -> MEDIO si supera (medio + diferencial)
        if (voltajeBateria_V >= (_configuracion.umbralMedio_V + diferencialMedio_V))
          _nivelEnergeticoActual = MID;
        break;
    }
  }
};
