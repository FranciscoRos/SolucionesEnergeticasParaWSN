#include <SoftwareSerial.h>
#include <Wire.h>
#include <RTClib.h>

#include "AdaptiveTXWSN.h" // Tu librería
#include "DataPayload.h"   // La estructura de datos que creamos

// --- PINES HARDWARE (Según tu esquema) ---
const int PIN_XBEE_RX = 2;
const int PIN_XBEE_TX = 3;
const int PIN_RELAY = 4;
// Se elimina el pin de la SD
const int PIN_SENSOR_CORRIENTE = A0; // ACS712
const int PIN_SENSOR_VOLTAJE = A1;   // ZMPT101B
const int PIN_BATERIA_ADC = A2;      // Divisor para medir batería

// --- OBJETOS GLOBALES ---
SoftwareSerial xbeeSerial(PIN_XBEE_RX, PIN_XBEE_TX); // RX, TX
RTC_DS3231 rtc;
AdaptiveTXWSN adaptiveTX;
DataPayload payload;

// --- CONFIGURACIÓN DE TRANSMISIÓN ADAPTATIVA ---
// ¡Ajústalos a tus necesidades!
const float VOLTAJE_ALTO   = 15.00f;
const float VOLTAJE_MEDIO  = 12.00f;
const float VOLTAJE_CORTE  = 10.50f;
const float HISTERESIS_PCT = 0.03f;

// Periodos en milisegundos
const uint32_t PERIODO_ALTO_MS  = 5000;    // 5 segundos
const uint32_t PERIODO_MEDIO_MS = 7000;   // 15 segundos
const uint32_t PERIODO_BAJO_MS  = 10000;  // 2 minutos

void setup() {
  Serial.begin(9600);
  xbeeSerial.begin(9600);
  
  Serial.println("Iniciando Nodo Emisor (version simple)...");

  // 1. Inicializar RTC
  if (!rtc.begin()) {
    Serial.println("No se pudo encontrar el RTC. Verifique conexiones.");
    while (1);
  }
  // Descomenta la siguiente linea UNA VEZ para ajustar la hora del RTC
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // 2. Inicializar tu librería de transmisión adaptativa
  AdaptiveTXWSN::Cfg config;
  config.pinAdcBateria = PIN_BATERIA_ADC;
  config.divisorRArriba_k = 100.0f;
  config.divisorRAbajo_k  = 33.3f;
  
  adaptiveTX.begin(config, VOLTAJE_ALTO, VOLTAJE_MEDIO, VOLTAJE_CORTE, HISTERESIS_PCT,
                   PERIODO_ALTO_MS, PERIODO_MEDIO_MS, PERIODO_BAJO_MS);
  
  Serial.println("Configuracion de transmision adaptativa lista.");
}

void loop() {
  // tick() decide si es momento de enviar
  if (adaptiveTX.tick()) {
    Serial.println("¡Hora de transmitir!");
    
    // 1. Recolectar todos los datos
    collectSensorData();

    // 2. Enviar datos por XBee
    sendData();
    
    // 3. Imprimir estado actual en el monitor serie
    Serial.print("  > Voltaje Bateria: "); Serial.print(payload.batteryVoltage); Serial.println(" V");
    Serial.print("  > Nivel Energia: "); Serial.println(payload.energyLevel);
    Serial.print("  > Proximo envio en: "); Serial.print(adaptiveTX.currentPeriod() / 1000); Serial.println(" s\n");
  }
}

void collectSensorData() {
  DateTime now = rtc.now();
  
  payload.unixTime = now.unixtime();
  payload.voltageAC = readACVoltage();
  payload.currentAC = readACCurrent();
  payload.powerApparent = payload.voltageAC * payload.currentAC;
  payload.batteryVoltage = adaptiveTX.lastVolts(); // La librería ya midió el voltaje
  payload.energyLevel = (uint8_t)adaptiveTX.level();
}

void sendData() {
  // Enviamos la estructura completa como un bloque de bytes
  xbeeSerial.write((uint8_t*)&payload, sizeof(payload));
  Serial.println("Paquete de datos enviado por XBee.");
}

// =================================================================
// ===           ¡¡¡IMPORTANTE: NECESITAS CALIBRAR ESTO!!!         ===
// =================================================================
float readACVoltage() {
  // Esta función es un EJEMPLO. Debes calibrarla.
  int rawValue = analogRead(PIN_SENSOR_VOLTAJE);
  float factorCalibracion = 0.25; // ¡¡AJUSTA ESTE VALOR!!
  return rawValue * factorCalibracion;
}

float readACCurrent() {
  // Esta función es un EJEMPLO para el ACS712. Debes calibrarla.
  int rawValue = analogRead(PIN_SENSOR_CORRIENTE);
  int offset = 512; // ¡¡AJUSTA ESTE VALOR!! (Debería ser la lectura con 0A)
  float sensitivity = 0.185; // (V/A) para el ACS712-05B.
  float voltage = (rawValue - offset) * (5.0 / 1024.0);
  return voltage / sensitivity;
}