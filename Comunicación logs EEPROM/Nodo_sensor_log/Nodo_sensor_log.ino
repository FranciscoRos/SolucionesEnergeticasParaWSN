#include <Wire.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "RTClib.h"

SoftwareSerial xbeeSerial(2, 3); // RX=D2, TX=D3

#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2

// Estructura para guardar los datos del último paquete en la EEPROM
struct LastPacketData {
  uint16_t id;
  float voltaje;
  float corriente;
  float vbat;
  uint32_t timestamp;
};

unsigned long previousMillis = 0;
const unsigned long INTERVAL_MS = 3000;
uint16_t paquetesEnviados = 0; // Contador de paquetes
LastPacketData lastPacket;      // Variable para guardar los datos del último paquete

RTC_DS3231 rtc;

// --- Sensores (sin cambios) ---
float leerVoltajeZMPT() {
  int lectura = analogRead(ZMPT_PIN);
  return ((lectura * 5.0) / 1023.0) * 50.0;
}
float leerCorrienteACS() {
  int lectura = analogRead(ACS_PIN);
  float volt = (lectura * 5.0) / 1023.0;
  return (volt - 2.5) / 0.066;
}
float leerVoltajeBateria() {
  int lectura = analogRead(VBAT_PIN);
  float vEsc = (lectura * 5.0) / 1023.0;
  return vEsc * 3.0;
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  xbeeSerial.begin(9600);
  Wire.begin();

  if (!rtc.begin()) {
    Serial.println("ERROR: No se encuentra el RTC");
    while (1);
  }
  
  // Descomentar solo una vez para ajustar la hora del RTC
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Lee el último contador y el último paquete desde la EEPROM
  EEPROM.get(0, paquetesEnviados);
  EEPROM.get(2, lastPacket); // Se lee desde una dirección diferente
  
  Serial.println("\n--- NODO EMISOR INICIADO (MODO TEXTO) ---");
  Serial.print("Continuando desde el ID de paquete: ");
  Serial.println(paquetesEnviados);

  Serial.println("--- Último paquete guardado en EEPROM ---");
  if (lastPacket.id > 0) { // Muestra los datos solo si hay algo guardado
      Serial.print("ID: "); Serial.print(lastPacket.id);
      Serial.print(" | Timestamp: "); Serial.println(lastPacket.timestamp);
      Serial.print("Voltaje: "); Serial.print(lastPacket.voltaje, 2);
      Serial.print(" | Corriente: "); Serial.print(lastPacket.corriente, 3);
      Serial.print(" | V. Bat: "); Serial.println(lastPacket.vbat, 2);
  } else {
      Serial.println("No hay datos de paquetes previos en la EEPROM.");
  }
  Serial.println("-----------------------------------------");

  Serial.println("| ID   | Voltaje | Corriente | V. Bat  | Timestamp    |");
  Serial.println("|------|---------|-----------|---------|--------------|");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= INTERVAL_MS) {
    previousMillis = currentMillis;

    paquetesEnviados++; // Incrementa el contador

    DateTime nowRTC = rtc.now();
    float voltage   = leerVoltajeZMPT();
    float corriente = abs(leerCorrienteACS()); // Se usa abs() para evitar negativos
    float vbat      = leerVoltajeBateria();

    // Llena la estructura con los nuevos datos
    lastPacket.id = paquetesEnviados;
    lastPacket.voltaje = voltage;
    lastPacket.corriente = corriente;
    lastPacket.vbat = vbat;
    lastPacket.timestamp = nowRTC.unixtime();

    // Guarda el contador y la estructura en la EEPROM
    EEPROM.put(0, paquetesEnviados);
    EEPROM.put(2, lastPacket);

    // Muestra los datos en la tabla del Monitor Serie
    char serialBuffer[100], voltage_s[8], corriente_s[10], vbat_s[8];
    dtostrf(voltage, 7, 2, voltage_s);
    dtostrf(corriente, 9, 3, corriente_s);
    dtostrf(vbat, 7, 2, vbat_s);
    snprintf(serialBuffer, sizeof(serialBuffer), "| %-4u | %s | %s | %s | %-12lu |",
        paquetesEnviados, voltage_s, corriente_s, vbat_s, nowRTC.unixtime());
    Serial.println(serialBuffer);

    // Construye la cadena de texto para enviar
    String dataToSend = "N:" + String(paquetesEnviados) +
                        " V:" + String(voltage, 2) +
                        " I:" + String(corriente, 3) +
                        " B:" + String(vbat, 2) +
                        " T:" + String(nowRTC.unixtime());

    // Envía la cadena de texto por XBee
    xbeeSerial.println(dataToSend);
    Serial.print("TX > ");
    Serial.println(dataToSend);
    Serial.println("----------------------------------------------------------");
  }

  // Comandos de retorno (sin cambios)
  if (xbeeSerial.available()) {
    String comando = xbeeSerial.readStringUntil('\n');
    comando.trim();
    if (comando == "ON")  digitalWrite(RELAY_PIN, HIGH);
    if (comando == "OFF") digitalWrite(RELAY_PIN, LOW);
  }
}

