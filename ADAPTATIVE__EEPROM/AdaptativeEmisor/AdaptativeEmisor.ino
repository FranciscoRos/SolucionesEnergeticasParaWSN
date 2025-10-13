#include <SoftwareSerial.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h> // Librería para la memoria EEPROM

#include "AdaptiveTXWSN.h"
#include "DataPayload.h"

// --- PINES HARDWARE ---
const int PIN_XBEE_RX = 2;
const int PIN_XBEE_TX = 3;
const int PIN_RELAY = 4;
const int PIN_SENSOR_CORRIENTE = A0;
const int PIN_SENSOR_VOLTAJE = A1;
const int PIN_BATERIA_ADC = A2;

// --- OBJETOS GLOBALES ---
SoftwareSerial xbeeSerial(PIN_XBEE_RX, PIN_XBEE_TX);
RTC_DS3231 rtc;
AdaptiveTXWSN adaptiveTX;
DataPayload payload;
uint32_t messageCounter = 0; // Contador global para el ID del mensaje

// --- CONFIGURACIÓN DE TRANSMISIÓN ADAPTATIVA ---
const float VOLTAJE_ALTO   = 15.00f;
const float VOLTAJE_MEDIO  = 12.00f;
const float VOLTAJE_CORTE  = 10.50f;
const float HISTERESIS_PCT = 0.03f;

const uint32_t PERIODO_ALTO_MS  = 5000;
const uint32_t PERIODO_MEDIO_MS = 7000;
const uint32_t PERIODO_BAJO_MS  = 10000;

void setup() {
  Serial.begin(9600);
  xbeeSerial.begin(9600);
  
  Serial.println("Iniciando Nodo Emisor con EEPROM...");

  // 1. Leer el último ID guardado en la EEPROM
  // La dirección 0 es donde guardaremos nuestro contador.
  EEPROM.get(0, messageCounter);
  messageCounter++; // Incrementamos para empezar con el siguiente ID
  
  Serial.print("ID de mensaje inicial: ");
  Serial.println(messageCounter);

  // 2. Inicializar RTC
  if (!rtc.begin()) {
    Serial.println("No se pudo encontrar el RTC.");
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // 3. Inicializar tu librería
  AdaptiveTXWSN::Cfg config;
  config.pinAdcBateria = PIN_BATERIA_ADC;
  config.divisorRArriba_k = 100.0f;
  config.divisorRAbajo_k  = 33.3f;
  
  adaptiveTX.begin(config, VOLTAJE_ALTO, VOLTAJE_MEDIO, VOLTAJE_CORTE, HISTERESIS_PCT,
                   PERIODO_ALTO_MS, PERIODO_MEDIO_MS, PERIODO_BAJO_MS);
  
  Serial.println("Configuracion lista.");
}

void loop() {
  if (adaptiveTX.tick()) {
    Serial.println("\n¡Hora de transmitir!");
    
    collectSensorData();
    sendData();

    // 3. Guardar el ID actual en la EEPROM para el próximo reinicio
    EEPROM.put(0, messageCounter);
    
    // 4. Incrementar el contador para el siguiente mensaje
    messageCounter++;
    
    Serial.print("  > ID de Mensaje: "); Serial.println(payload.messageID);
    Serial.print("  > Voltaje Bateria: "); Serial.print(payload.batteryVoltage); Serial.println(" V");
    Serial.print("  > Nivel Energia: "); Serial.println(payload.energyLevel);
    Serial.print("  > Proximo envio en: "); Serial.print(adaptiveTX.currentPeriod() / 1000); Serial.println(" s");
  }
}

void collectSensorData() {
  DateTime now = rtc.now();
  
  payload.messageID = messageCounter; // Asignar el ID actual al payload
  payload.unixTime = now.unixtime();
  payload.voltageAC = readACVoltage();
  payload.currentAC = readACCurrent();
  payload.powerApparent = payload.voltageAC * payload.currentAC;
  payload.batteryVoltage = adaptiveTX.lastVolts();
  payload.energyLevel = (uint8_t)adaptiveTX.level();
}

void sendData() {
  xbeeSerial.write('$'); // Marcador de inicio para sincronización
  xbeeSerial.write((uint8_t*)&payload, sizeof(payload));
  Serial.println("Paquete de datos enviado por XBee.");
}

float readACVoltage() {
  int rawValue = analogRead(PIN_SENSOR_VOLTAJE);
  float factorCalibracion = 0.25;
  return rawValue * factorCalibracion;
}

float readACCurrent() {
  int rawValue = analogRead(PIN_SENSOR_CORRIENTE);
  int offset = 512;
  float sensitivity = 0.185;
  float voltage = (rawValue - offset) * (5.0 / 1024.0);
  return voltage / sensitivity;
}