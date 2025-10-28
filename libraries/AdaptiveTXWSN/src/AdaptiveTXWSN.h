/**
 * @file AdaptiveTXWSN.h
 * @brief Define la clase AdaptiveTXWSN para la transmisión adaptativa en WSN.
 * Esta librería permite a un nodo sensor (ej. Arduino) ajustar su
 * período de transmisión basado en el voltaje de su batería,
 * ahorrando energía cuando el nivel es bajo.
 * @authors Francisco Rosales, Omar Tox
 * @date 2025-09
 */

 #pragma once
 #include <Arduino.h>
 
 /**
  * @class AdaptiveTXWSN
  * @brief Gestiona la lógica de transmisión adaptativa basada en el voltaje de la batería.
  *
  * La clase monitorea el voltaje, lo clasifica en niveles (ALTO, MEDIO, BAJO)
  * y determina el intervalo de tiempo adecuado para la próxima transmisión.
  */
 class AdaptiveTXWSN {
 public:
   /**
    * @struct Cfg
    * @brief Estructura de configuración para la librería AdaptiveTXWSN.
    * Contiene todos los pines, umbrales y períodos de operación.
    */
   struct Cfg {
     // --- Lectura de bateria ---
     int8_t  pinAdcBateria           = -1;     ///< Pin ADC para leer batería. -1 si se inyecta el voltaje manualmente.
     float   voltajeReferenciaAdc    = 5.0f;   ///< Voltaje de referencia del ADC (5.0V, 3.3V, 1.1V, etc.).
     float   divisorRArriba_k        = 100.0f; ///< Resistencia superior (kΩ) del divisor de voltaje (Vin -> R_arriba -> ADC).
     float   divisorRAbajo_k         =  33.0f; ///< Resistencia inferior (kΩ) del divisor de voltaje (ADC -> R_abajo -> GND).
     uint8_t muestrasPromedioAdc     = 8;      ///< Número de muestras a promediar para estabilizar la lectura de VBAT.
                                               ///< Si no se usan resistencias, añadir 0 y 1 para evitar división por cero.
     // --- Umbrales (VOLTIOS) ---
     float umbralAlto_V              = 3.90f;  ///< Voltaje por encima del cual se considera nivel ALTO.
     float umbralMedio_V             = 3.60f;  ///< Voltaje por encima del cual se considera nivel MEDIO (y por debajo de ALTO).
     float fraccionHisteresis        = 0.03f;  ///< Fracción (ej. 0.03 = 3%) de histéresis para evitar rebotes de nivel.
 
     // --- Periodos de envio (ms) por nivel ---
     uint32_t periodoAlto_ms         = 5000;   ///< Período de transmisión (ms) cuando el nivel es ALTO.
     uint32_t periodoMedio_ms        = 15000;  ///< Período de transmisión (ms) cuando el nivel es MEDIO.
     uint32_t periodoBajo_ms         = 120000; ///< Período de transmisión (ms) cuando el nivel es BAJO.
 
     // --- Corte duro: por debajo NO se transmite ---
     float corteVoltaje_V            = 3.40f;  ///< Voltaje por debajo del cual el nodo deja de transmitir (isCutoff() = true).
   };
 
   /**
    * @enum Level
    * @brief Define los niveles de energía discretos del nodo.
    */
   enum Level : uint8_t {
     BATT_LOW=0,  ///< Nivel de energía bajo. Período de transmisión largo.
     BATT_MID=1,  ///< Nivel de energía medio. Período de transmisión normal.
     BATT_HIGH=2  ///< Nivel de energía alto. Período de transmisión corto.
   };
 
   /**
    * @brief Inicializa la librería con la configuración y umbrales.
    * Esta función configura los pines y establece los parámetros de operación iniciales.
    *
    * @param cfg Objeto de configuración base (principalmente pines y divisor).
    * @param umbralAlto_V Voltaje (V) para el nivel ALTO.
    * @param umbralMedio_V Voltaje (V) para el nivel MEDIO.
    * @param corteVoltaje_V Voltaje (V) para el corte de transmisión.
    * @param fraccionHisteresis Fracción de histéresis (ej. 0.03).
    * @param periodoAlto_ms Período de envío (ms) para nivel ALTO.
    * @param periodoMedio_ms Período de envío (ms) para nivel MEDIO.
    * @param periodoBajo_ms Período de envío (ms) para nivel BAJO.
    */
   void begin(const Cfg& cfg)
   {
     _configuracion = cfg;
 
     // Sobrescribir rangos/umbrales y periodos

     // Inicialización de hardware/estado
     if (_configuracion.pinAdcBateria >= 0) {
       pinMode(_configuracion.pinAdcBateria, INPUT);
     }
     _nivelEnergeticoActual = BATT_HIGH;      // Se recalibra en el primer tick()
     _msProximoEnvio        = millis();
   }
 
 
   /**
    * @brief Función principal que debe ser llamada en cada loop().
    * Gestiona la medición, actualización de estado y el temporizador.
    *
    * @return true Si es momento de transmitir un paquete.
    * @return false Si aún no es momento de transmitir.
    */
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
 
   /**
    * @brief Inyecta manualmente una lectura de voltaje de batería.
    * Útil si la medición se hace con un ADC externo o un chip de gestión de batería (PMIC).
    *
    * @param voltajeBateria_V El voltaje medido externamente.
    */
   void setBatteryVolts(float voltajeBateria_V) {
     _usarLecturaInyectada = true;
     _voltajeInyectado_V   = voltajeBateria_V;
   }
 
   /**
    * @brief Lee el voltaje de la batería usando el pin ADC configurado y el divisor.
    * Realiza un promediado de lecturas para estabilizar el valor.
    *
    * @note Asume un ADC de 10 bits (1023.0f) por defecto para Arduino (ej. ATmega328P).
    * Ajustar `voltajeReferenciaAdc` si se usa ref interna (1.1V) o si se porta a otro MCU.
    *
    * @return El voltaje calculado de la batería en Voltios.
    */
   float readBatteryVolts() {
     if (_configuracion.pinAdcBateria < 0) return _ultimoVoltajeMedido_V; // sin pin: devolver ultimo
     uint32_t acumuladorAdc = 0;
     uint8_t nMuestras = max((uint8_t)1, _configuracion.muestrasPromedioAdc);
     for (uint8_t i = 0; i < nMuestras; ++i) {
       acumuladorAdc += analogRead(_configuracion.pinAdcBateria);
       delayMicroseconds(250);
     }
     float promedioCuentasAdc = (float)acumuladorAdc / nMuestras;
     float voltajeAdc_V = (promedioCuentasAdc / 1023.0f) * _configuracion.voltajeReferenciaAdc;
     float factorDivisor = (_configuracion.divisorRArriba_k + _configuracion.divisorRAbajo_k)
                           / _configuracion.divisorRAbajo_k; // Vin = Vadc * factor
     return voltajeAdc_V * factorDivisor;
   }
 
   // --- Getters (Consultores de estado) ---
 
   /**
    * @brief Obtiene el nivel de energía actual.
    * @return Level El estado actual (BATT_LOW, BATT_MID, BATT_HIGH).
    */
   Level   level()          const { return _nivelEnergeticoActual; }
 
   /**
    * @brief Obtiene la última lectura de voltaje de batería almacenada.
    * @return float El último voltaje medido en Voltios.
    */
   float   lastVolts()      const { return _ultimoVoltajeMedido_V; }
 
   /**
    * @brief Verifica si el nodo está en estado de corte por batería baja.
    * @return true Si el voltaje está por debajo de `corteVoltaje_V`.
    * @return false Si el voltaje está por encima del umbral de corte.
    */
   bool    isCutoff()       const { return _bloqueadoPorCorte; }
 
   /**
    * @brief Obtiene el período de transmisión actual basado en el nivel de energía.
    * @return uint32_t El período de envío actual en milisegundos.
    */
   uint32_t currentPeriod() const {
     switch (_nivelEnergeticoActual) {
       case BATT_HIGH: return _configuracion.periodoAlto_ms;
       case BATT_MID:  return _configuracion.periodoMedio_ms;
       default:        return _configuracion.periodoBajo_ms;
     }
   }
 
   // --- Setters (Modificadores de configuración en tiempo de ejecución) ---
 
   /**
    * @brief Actualiza los períodos de transmisión en tiempo de ejecución.
    * @param alto_ms Nuevo período (ms) para nivel ALTO.
    * @param medio_ms Nuevo período (ms) para nivel MEDIO.
    * @param bajo_ms Nuevo período (ms) para nivel BAJO.
    */
   void setPeriods(uint32_t alto_ms, uint32_t medio_ms, uint32_t bajo_ms) {
     _configuracion.periodoAlto_ms  = alto_ms;
     _configuracion.periodoMedio_ms = medio_ms;
     _configuracion.periodoBajo_ms  = bajo_ms;
   }
 
   /**
    * @brief Actualiza los umbrales de voltaje en tiempo de ejecución.
    * @param umbralAlto_V Nuevo umbral (V) para nivel ALTO.
    * @param umbralMedio_V Nuevo umbral (V) para nivel MEDIO.
    */
   void setThresholds(float umbralAlto_V, float umbralMedio_V) {
     _configuracion.umbralAlto_V  = umbralAlto_V;
     _configuracion.umbralMedio_V = umbralMedio_V;
   }
 
   /**
    * @brief Actualiza la fracción de histéresis en tiempo de ejecución.
    * @param fraccion Fracción (ej. 0.03 para 3%).
    */
   void setHysteresisPct(float fraccion) { _configuracion.fraccionHisteresis = fraccion; }
 
 private:
   Cfg       _configuracion;          ///< Almacena la configuración de la instancia.
   Level     _nivelEnergeticoActual;  ///< Estado de energía actual del nodo.
   uint32_t  _msProximoEnvio;         ///< Marca de tiempo (millis()) para el siguiente envío.
   float     _ultimoVoltajeMedido_V;  ///< Caché de la última medición de voltaje.
   bool      _bloqueadoPorCorte;      ///< Flag que indica si se alcanzó el corte por bajo voltaje.
 
   bool      _usarLecturaInyectada;   ///< Flag para usar el voltaje inyectado vs. el ADC.
   float     _voltajeInyectado_V;     ///< Valor del voltaje inyectado manualmente.
 
   /**
    * @brief Función interna para actualizar el estado de energía (`_nivelEnergeticoActual`).
    * Aplica la lógica de histéresis para evitar cambios rápidos de estado
    * (rebotes) cuando el voltaje está cerca de un umbral.
    *
    * @param voltajeBateria_V El voltaje de la batería medido actualmente.
    */
   void actualizarNivelConHisteresis(float voltajeBateria_V) {
     // diferencials de histéresis alrededor de los umbrales
     float diferencialAlto_V  = _configuracion.umbralAlto_V  * _configuracion.fraccionHisteresis;
     float diferencialMedio_V = _configuracion.umbralMedio_V * _configuracion.fraccionHisteresis;
 
     switch (_nivelEnergeticoActual) {
       case BATT_HIGH: // ALTO -> MEDIO si baja por debajo de (alto - diferencial)
         if (voltajeBateria_V < (_configuracion.umbralAlto_V - diferencialAlto_V))
           _nivelEnergeticoActual = BATT_MID;
         break;
 
       case BATT_MID:
         // MEDIO -> ALTO si supera (alto + diferencial)
         if (voltajeBateria_V >= (_configuracion.umbralAlto_V + diferencialAlto_V)) { _nivelEnergeticoActual = BATT_HIGH; break; }
         // MEDIO -> BAJO si baja por debajo de (medio - diferencial)
         if (voltajeBateria_V <  (_configuracion.umbralMedio_V - diferencialMedio_V)) { _nivelEnergeticoActual = BATT_LOW;  break; }
         break;
 
       case BATT_LOW: // BAJO -> MEDIO si supera (medio + diferencial)
         if (voltajeBateria_V >= (_configuracion.umbralMedio_V + diferencialMedio_V))
           _nivelEnergeticoActual = BATT_MID;
         break;
     }
   }
 };