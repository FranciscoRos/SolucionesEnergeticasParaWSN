#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <LowPower.h>
#include "EnergyWSN.h"   // Tu librería de ahorro energético

// ---------------- UART hacia XBee ----------------
SoftwareSerial xbeeSerial(2, 3);   // D2=RX, D3=TX

// ---------------- Pines de sensores/actuadores ---
#define RELAY_PIN   4
#define ACS_PIN     A0
#define ZMPT_PIN    A1
#define VBAT_PIN    A2
const uint8_t SD_CS = 10;

// ---------------- Pines de gestión energía --------
const uint8_t PIN_SLEEP_RQ = 8;   // → XBee SLEEP_RQ (3.3V, usa divisor o NPN)
const uint8_t PIN_ON_SLEEP = 9;   // ← XBee ON/SLEEP (entrada 3.3V, sin INPUT_PULLUP)
const uint8_t PIN_PWR_SENS = 7;   // → MOSFET/load-switch de sensores (HIGH=ON)

// ---------------- Instancia de EnergyWSN ----------
EnergyWSN energy;

// ---------------- Variables ----------------------
File logFile;
int paquetesEnviados = 0;
const unsigned long INTERVAL_MS = 2000;   

// ---------------- SETUP --------------------------
void setup() {
  pinMode(5, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  
  xbeeSerial.begin(9600);

  Serial.print("encendido  Nodo Sensor Energy\n");
  // Inicializa SD

  // Inicializa la gestión de energía
  EnergyWSN::Cfg cfg;
  cfg.pins = { PIN_SLEEP_RQ, PIN_ON_SLEEP, PIN_PWR_SENS, -1 }; // no usamos VBAT interno aún
  cfg.invertPwr = true;   // pon true si tu MOSFET se activa en LOW
  cfg.bootSleep = false;    // arranca dormido
  energy.begin(cfg);

  Serial.println("Nodo Esclavo optimizado (AT + EnergyWSN)");
}

// ---------------- LOOP ---------------------------
void loop() {
  digitalWrite(5, HIGH);
  // 1) Despertar XBee + Sensores
 
  if (!energy.wakeRadio(20)) {
  Serial.println("[WARN] XBee no confirmó awake");
} else {
  Serial.println("Xbee on");
}

  energy.powerSensors(true);

  //delay(30);
  

  // 2) Medir
  float voltage   = leerVoltajeZMPT();
  float corriente = leerCorrienteACS();
  float vbat      = leerVoltajeBateria();
  
  energy.powerSensors(false);
  Serial.println("Leímos");

  // 3) Enviar datos al coordinador (AT, delimitador '|')
  paquetesEnviados++;
  xbeeSerial.print(F("Nodo1|"));
  xbeeSerial.print(F("N:")); xbeeSerial.print(paquetesEnviados);
  xbeeSerial.print(F(" V:")); xbeeSerial.print(voltage, 2);
  xbeeSerial.print(F(" I:")); xbeeSerial.print(corriente, 2);
  xbeeSerial.print(F(" B:")); xbeeSerial.println(vbat, 2);

  // Log en SD
  String fecha_hora = obtenerFechaHora();
  
  Serial.print("Paquete "); Serial.print(paquetesEnviados);
  Serial.print(" -> V="); Serial.print(voltage, 2);
  Serial.print(F(" I=")); Serial.print(corriente, 2);
  Serial.print(F(" B=")); Serial.println(vbat, 2);

  // 4) Apagar sensores + dormir XBee
  //delay(5000);
  
  
  //energy.powerSensors(false); PROBAR
  if (!energy.sleepRadio(200)) Serial.println(F("[WARN] XBee no confirmó sleep"));

  // 5) Dormir el micro hasta el siguiente ciclo
  
  digitalWrite(5, LOW);
  energy.sleepFor_ms(INTERVAL_MS);
  
}

// ---------------- FUNCIONES DE MEDICIÓN ----------------
float leerVoltajeZMPT() {
  int lectura = analogRead(ZMPT_PIN);
  float voltaje = lectura * 5.0 / 1023.0;
  return voltaje * 50.0; // calibra según tu módulo
}

float leerCorrienteACS() {
  int lectura = analogRead(ACS_PIN);
  float v = lectura * 5.0 / 1023.0;
  return (v - 2.5) / 0.066; // ACS712 30A
}

float leerVoltajeBateria() {
  int lectura = analogRead(VBAT_PIN);
  float v = lectura * 5.0 / 1023.0;
  return v * 3.0; // tu divisor ≈×3, ajusta factor real
}

String obtenerFechaHora() {
  unsigned long s = millis() / 1000;
  int hh = 12 + (s / 3600) % 12;
  int mm = (s / 60) % 60;
  int ss = s % 60;
  char buf[20];
  sprintf(buf, "2025-06-03 %02d:%02d:%02d", hh, mm, ss);
  return String(buf);
}
