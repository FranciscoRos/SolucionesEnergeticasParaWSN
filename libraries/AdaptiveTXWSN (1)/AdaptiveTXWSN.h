#pragma once
#include <Arduino.h>

class AdaptiveTXWSN {
public:
  struct Cfg {
    // --- Lectura de bateria ---
    int8_t  pinAdcBateria           = -1;
    float   voltajeReferenciaAdc    = 5.0f;
    float   divisorRArriba_k        = 100.0f;
    float   divisorRAbajo_k         =  33.0f;
    uint8_t muestrasPromedioAdc     = 8;

    // --- Umbrales (VOLTIOS) ---
    float umbralAlto_V              = 3.90f;
    float umbralMedio_V             = 3.60f;
    float fraccionHisteresis        = 0.03f;
    uint32_t periodoAlto_ms         = 5000;
    uint32_t periodoMedio_ms        = 15000;
    uint32_t periodoBajo_ms         = 120000;
    float corteVoltaje_V            = 3.40f;
  };

  // Nombres corregidos para evitar conflicto con Arduino
  enum Level : uint8_t { BATT_LOW=0, BATT_MID=1, BATT_HIGH=2 };

  void begin(const Cfg& cfg,
            float umbralAlto_V, float umbralMedio_V,
            float corteVoltaje_V,
            float fraccionHisteresis,
            uint32_t periodoAlto_ms,
            uint32_t periodoMedio_ms,
            uint32_t periodoBajo_ms)
  {
    _configuracion = cfg;
    _configuracion.umbralAlto_V        = umbralAlto_V;
    _configuracion.umbralMedio_V       = umbralMedio_V;
    _configuracion.corteVoltaje_V      = corteVoltaje_V;
    _configuracion.fraccionHisteresis  = fraccionHisteresis;
    _configuracion.periodoAlto_ms      = periodoAlto_ms;
    _configuracion.periodoMedio_ms     = periodoMedio_ms;
    _configuracion.periodoBajo_ms      = periodoBajo_ms;

    if (_configuracion.pinAdcBateria >= 0) {
      pinMode(_configuracion.pinAdcBateria, INPUT);
    }
    _nivelEnergeticoActual = BATT_HIGH;
    _msProximoEnvio        = millis();
  }

  bool tick() {
    float voltajeBateria_V = (_usarLecturaInyectada)
                              ? _voltajeInyectado_V
                              : readBatteryVolts();
    _ultimoVoltajeMedido_V = voltajeBateria_V;

    if (voltajeBateria_V < _configuracion.corteVoltaje_V) {
      _bloqueadoPorCorte = true;
      return false;
    }
    _bloqueadoPorCorte = false;

    actualizarNivelConHisteresis(voltajeBateria_V);

    uint32_t ahoraMs = millis();
    if ((int32_t)(ahoraMs - _msProximoEnvio) >= 0) {
      _msProximoEnvio = ahoraMs + currentPeriod();
      return true;
    }
    return false;
  }

  void setBatteryVolts(float voltajeBateria_V) {
    _usarLecturaInyectada = true;
    _voltajeInyectado_V   = voltajeBateria_V;
  }

  float readBatteryVolts() {
    if (_configuracion.pinAdcBateria < 0) return _ultimoVoltajeMedido_V;
    uint32_t acumuladorAdc = 0;
    uint8_t nMuestras = max((uint8_t)1, _configuracion.muestrasPromedioAdc);
    for (uint8_t i = 0; i < nMuestras; ++i) {
      acumuladorAdc += analogRead(_configuracion.pinAdcBateria);
      delayMicroseconds(250);
    }
    float promedioCuentasAdc = (float)acumuladorAdc / nMuestras;
    float voltajeAdc_V = (promedioCuentasAdc / 1023.0f) * _configuracion.voltajeReferenciaAdc;
    float factorDivisor = (_configuracion.divisorRArriba_k + _configuracion.divisorRAbajo_k)
                          / _configuracion.divisorRAbajo_k;
    return voltajeAdc_V * factorDivisor;
  }

  Level   level()          const { return _nivelEnergeticoActual; }
  float   lastVolts()      const { return _ultimoVoltajeMedido_V; }
  bool    isCutoff()       const { return _bloqueadoPorCorte; }
  uint32_t currentPeriod() const {
    switch (_nivelEnergeticoActual) {
      case BATT_HIGH: return _configuracion.periodoAlto_ms;
      case BATT_MID:  return _configuracion.periodoMedio_ms;
      default:        return _configuracion.periodoBajo_ms;
    }
  }

  void setPeriods(uint32_t alto_ms, uint32_t medio_ms, uint32_t bajo_ms) {
    _configuracion.periodoAlto_ms  = alto_ms;
    _configuracion.periodoMedio_ms = medio_ms;
    _configuracion.periodoBajo_ms  = bajo_ms;
  }
  void setThresholds(float umbralAlto_V, float umbralMedio_V) {
    _configuracion.umbralAlto_V  = umbralAlto_V;
    _configuracion.umbralMedio_V = umbralMedio_V;
  }
  void setHysteresisPct(float fraccion) { _configuracion.fraccionHisteresis = fraccion; }

private:
  Cfg       _configuracion;
  Level     _nivelEnergeticoActual  = BATT_HIGH;
  uint32_t  _msProximoEnvio         = 0;
  float     _ultimoVoltajeMedido_V  = 0.0f;
  bool      _bloqueadoPorCorte      = false;
  bool      _usarLecturaInyectada   = false;
  float     _voltajeInyectado_V     = 0.0f;

  void actualizarNivelConHisteresis(float voltajeBateria_V) {
    float diferencialAlto_V  = _configuracion.umbralAlto_V  * _configuracion.fraccionHisteresis;
    float diferencialMedio_V = _configuracion.umbralMedio_V * _configuracion.fraccionHisteresis;

    switch (_nivelEnergeticoActual) {
      case BATT_HIGH:
        if (voltajeBateria_V < (_configuracion.umbralAlto_V - diferencialAlto_V))
          _nivelEnergeticoActual = BATT_MID;
        break;

      case BATT_MID:
        if (voltajeBateria_V >= (_configuracion.umbralAlto_V + diferencialAlto_V)) { _nivelEnergeticoActual = BATT_HIGH; break; }
        if (voltajeBateria_V <  (_configuracion.umbralMedio_V - diferencialMedio_V)) { _nivelEnergeticoActual = BATT_LOW;  break; }
        break;

      case BATT_LOW:
        if (voltajeBateria_V >= (_configuracion.umbralMedio_V + diferencialMedio_V))
          _nivelEnergeticoActual = BATT_MID;
        break;
    }
  }
};