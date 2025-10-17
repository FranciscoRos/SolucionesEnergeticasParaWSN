#pragma once
#include <Arduino.h>

/**
 * @class AdaptiveTXWSN
 * @brief Gestiona la frecuencia de transmisión de un nodo sensor inalámbrico 
 * basándose en el voltaje de la batería para ahorrar energía.
 * * Esta clase ajusta dinámicamente el período de envío de datos
 * según tres niveles de energía (ALTO, MEDIO, BAJO), definidos por umbrales
 * de voltaje. Incluye histéresis para evitar cambios de estado erráticos
 * y es compatible con diferentes plataformas (Arduino, ESP32) gracias a su
 * configuración flexible del ADC.
 */
class AdaptiveTXWSN {
public:
  /**
   * @struct Cfg
   * @brief Estructura de configuración para todos los parámetros de la librería.
   */
  struct Cfg {
    // --- Configuración del Hardware (ADC) ---
    int8_t  pinAdcBateria        = -1;      // Pin analógico para leer el voltaje.
    float   voltajeReferenciaAdc = 5.0f;    // Voltaje de referencia del ADC (e.g., 5.0 para Arduino Uno, 3.3 para ESP32).
    float   resolucionAdcMax     = 1023.0f; // Valor máximo del ADC (1023.0 para 10-bit, 4095.0 para 12-bit).
    float   divisorRArriba_k     = 100.0f;  // Resistencia superior del divisor de voltaje (en kOhms).
    float   divisorRAbajo_k      = 33.0f;   // Resistencia inferior del divisor de voltaje (en kOhms).
    uint8_t muestrasPromedioAdc  = 8;       // Número de lecturas para promediar y reducir ruido.

    // --- Umbrales de Voltaje y Comportamiento ---
    float    umbralAlto_V         = 3.90f;   // Límite superior para pasar de MEDIO a ALTO.
    float    umbralMedio_V        = 3.60f;   // Límite inferior para pasar de MEDIO a BAJO.
    float    fraccionHisteresis   = 0.03f;   // Porcentaje (e.g., 0.03 = 3%) para la banda de histéresis.
    float    corteVoltaje_V       = 3.40f;   // Voltaje por debajo del cual se detienen las transmisiones.
    
    // --- Períodos de Transmisión (ms) ---
    uint32_t periodoAlto_ms       = 5000;    // Frecuencia de envío cuando la batería está en nivel ALTO.
    uint32_t periodoMedio_ms      = 15000;   // Frecuencia de envío cuando la batería está en nivel MEDIO.
    uint32_t periodoBajo_ms       = 120000;  // Frecuencia de envío cuando la batería está en nivel BAJO.
  };

  /**
   * @enum Level
   * @brief Define los niveles de energía de la batería.
   */
  enum Level : uint8_t { BATT_LOW = 0, BATT_MID = 1, BATT_HIGH = 2 };

  /**
   * @brief Inicializa la librería con la configuración proporcionada.
   * @param cfg La estructura Cfg con todos los parámetros definidos.
   */
  void begin(const Cfg& cfg) {
    _configuracion = cfg; // Copia la configuración proporcionada por el usuario.

    if (_configuracion.pinAdcBateria >= 0) {
      pinMode(_configuracion.pinAdcBateria, INPUT);
    }
    _nivelEnergeticoActual = BATT_HIGH; // Inicia en el estado de mayor energía.
    _msProximoEnvio        = millis();   // Programa el primer envío inmediatamente.
  }

  /**
   * @brief Método principal que debe ser llamado en el loop() del sketch.
   * @return true si es momento de realizar una transmisión, false en caso contrario.
   */
  bool tick() {
    // Obtiene el voltaje actual, ya sea de una lectura real o de un valor inyectado para pruebas.
    float voltajeBateria_V = (_usarLecturaInyectada)
                                 ? _voltajeInyectado_V
                                 : readBatteryVolts();
    _ultimoVoltajeMedido_V = voltajeBateria_V;

    // Verifica si el voltaje está por debajo del umbral de corte.
    if (voltajeBateria_V < _configuracion.corteVoltaje_V) {
      _bloqueadoPorCorte = true;
      return false; // Detiene toda transmisión.
    }
    _bloqueadoPorCorte = false;

    // Actualiza el nivel de energía actual (ALTO, MEDIO, BAJO) usando histéresis.
    actualizarNivelConHisteresis(voltajeBateria_V);

    // Comprueba si ha transcurrido el tiempo para el próximo envío.
    uint32_t ahoraMs = millis();
    if ((int32_t)(ahoraMs - _msProximoEnvio) >= 0) {
      _msProximoEnvio = ahoraMs + currentPeriod(); // Programa el siguiente envío.
      return true;
    }
    return false;
  }

  /**
   * @brief Lee y calcula el voltaje real de la batería.
   * @return El voltaje de la batería en voltios.
   */
  float readBatteryVolts() {
    if (_configuracion.pinAdcBateria < 0) return _ultimoVoltajeMedido_V;

    uint32_t acumuladorAdc = 0;
    uint8_t nMuestras = max((uint8_t)1, _configuracion.muestrasPromedioAdc);
    for (uint8_t i = 0; i < nMuestras; ++i) {
      acumuladorAdc += analogRead(_configuracion.pinAdcBateria);
      delayMicroseconds(250);
    }
    float promedioCuentasAdc = (float)acumuladorAdc / nMuestras;

    // CORREGIDO: Usa la resolución del ADC definida en la configuración para portabilidad.
    float voltajeAdc_V = (promedioCuentasAdc / _configuracion.resolucionAdcMax) * _configuracion.voltajeReferenciaAdc;

    // Calcula el voltaje real antes del divisor de voltaje.
    float factorDivisor = (_configuracion.divisorRArriba_k + _configuracion.divisorRAbajo_k)
                        / _configuracion.divisorRAbajo_k;
    return voltajeAdc_V * factorDivisor;
  }

  // --- Métodos de Acceso (Getters) ---
  Level    level()         const { return _nivelEnergeticoActual; }
  float    lastVolts()     const { return _ultimoVoltajeMedido_V; }
  bool     isCutoff()      const { return _bloqueadoPorCorte; }
  uint32_t currentPeriod() const {
    switch (_nivelEnergeticoActual) {
      case BATT_HIGH: return _configuracion.periodoAlto_ms;
      case BATT_MID:  return _configuracion.periodoMedio_ms;
      default:        return _configuracion.periodoBajo_ms;
    }
  }

  // --- Métodos de Configuración en Tiempo de Ejecución (Setters) ---
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

  /**
   * @brief Permite "inyectar" un valor de voltaje para pruebas sin hardware.
   * @param voltajeBateria_V El valor de voltaje a simular.
   */
  void setBatteryVolts(float voltajeBateria_V) {
    _usarLecturaInyectada = true;
    _voltajeInyectado_V   = voltajeBateria_V;
  }

private:
  Cfg      _configuracion;
  Level    _nivelEnergeticoActual = BATT_HIGH;
  uint32_t _msProximoEnvio        = 0;
  float    _ultimoVoltajeMedido_V = 0.0f;
  bool     _bloqueadoPorCorte     = false;
  bool     _usarLecturaInyectada  = false;
  float    _voltajeInyectado_V    = 0.0f;

  /**
   * @brief Actualiza el estado de la batería aplicando histéresis.
   * @param voltajeBateria_V El voltaje actual de la batería.
   */
  void actualizarNivelConHisteresis(float voltajeBateria_V) {
    float diferencialAlto_V  = _configuracion.umbralAlto_V  * _configuracion.fraccionHisteresis;
    float diferencialMedio_V = _configuracion.umbralMedio_V * _configuracion.fraccionHisteresis;

    switch (_nivelEnergeticoActual) {
      case BATT_HIGH:
        if (voltajeBateria_V < (_configuracion.umbralAlto_V - diferencialAlto_V)) {
          _nivelEnergeticoActual = BATT_MID;
        }
        break;

      case BATT_MID:
        if (voltajeBateria_V >= (_configuracion.umbralAlto_V + diferencialAlto_V)) {
          _nivelEnergeticoActual = BATT_HIGH;
        } else if (voltajeBateria_V < (_configuracion.umbralMedio_V - diferencialMedio_V)) {
          _nivelEnergeticoActual = BATT_LOW;
        }
        break;

      case BATT_LOW:
        if (voltajeBateria_V >= (_configuracion.umbralMedio_V + diferencialMedio_V)) {
          _nivelEnergeticoActual = BATT_MID;
        }
        break;
    }
  }
};