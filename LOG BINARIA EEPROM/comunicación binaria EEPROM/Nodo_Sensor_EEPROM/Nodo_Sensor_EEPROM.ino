#include <Wire.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>      // <-- Librería para memoria persistente
#include "RTClib.h"
#include "CodecWSN.h"    // <-- Nuestra librería de comunicación

SoftwareSerial xbeeSerial(2, 3); // RX=D2, TX=D3

#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2

unsigned long previousMillis = 0;
const unsigned long INTERVAL_MS = 3000;
uint16_t paquetesEnviados = 0; // Contador de paquetes

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

  // Lee el último contador guardado desde la EEPROM
  EEPROM.get(0, paquetesEnviados); 
  
  Serial.println("\n--- NODO EMISOR INICIADO ---");
  Serial.print("Continuando desde el ID de paquete: ");
  Serial.println(paquetesEnviados);
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
    float corriente = leerCorrienteACS();
    float vbat      = leerVoltajeBateria();

    Packet p;
    p.sync      = SYNCWORD;
    p.id        = paquetesEnviados;
    p.voltaje   = (int16_t)(voltage * 100);
    p.corriente = (int16_t)(corriente * 1000);
    p.vbat      = (uint16_t)(vbat * 100);
    p.timestamp = nowRTC.unixtime();

    // Guarda el nuevo contador en la EEPROM
    EEPROM.put(0, paquetesEnviados);

    // Muestra los datos en la tabla del Monitor Serie
    char serialBuffer[100], voltage_s[8], corriente_s[10], vbat_s[8];
    dtostrf(voltage, 7, 2, voltage_s);
    dtostrf(corriente, 9, 3, corriente_s);
    dtostrf(vbat, 7, 2, vbat_s);
    snprintf(serialBuffer, sizeof(serialBuffer), "| %-4u | %s | %s | %s | %-12lu |",
        p.id, voltage_s, corriente_s, vbat_s, p.timestamp);
    Serial.println(serialBuffer);

    // Envía el paquete binario por XBee
    uint8_t buf[PACKET_SIZE];
    encodePacket(buf, p);
    xbeeSerial.write(buf, PACKET_SIZE);
  }

  // Comandos de retorno (sin cambios)
  if (xbeeSerial.available()) {
    String comando = xbeeSerial.readStringUntil('\n');
    comando.trim();
    if (comando == "ON")  digitalWrite(RELAY_PIN, HIGH);
    if (comando == "OFF") digitalWrite(RELAY_PIN, LOW);
  }
}