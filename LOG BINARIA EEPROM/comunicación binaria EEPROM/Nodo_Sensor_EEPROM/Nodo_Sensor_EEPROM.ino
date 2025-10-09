#include <Wire.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "RTClib.h"
#include "CodecWSN.h"

SoftwareSerial xbeeSerial(2, 3); // RX=D2, TX=D3

#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2

unsigned long previousMillis = 0;
const unsigned long INTERVAL_MS = 3000;
uint16_t paquetesEnviados = 0;

RTC_DS3231 rtc;

// --- Helpers ---
// Imprime el buffer binario, separando el SYNCWORD del resto.
void printBufBinary(const uint8_t* b, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    for (int bit = 7; bit >= 0; --bit) {
      Serial.print((b[i] >> bit) & 0x01);
    }
    if ( i == 1) Serial.print(" || "); // Separador visual después del SYNCWORD
    else if (i < n - 1) Serial.print(" ");
  }
  Serial.println();
}

// Obtiene fecha y hora formateada.
String obtenerFechaHoraRTC(const DateTime& dt) {
  char buf[25];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
          dt.year(), dt.month(), dt.day(),
          dt.hour(), dt.minute(), dt.second());
  return String(buf);
}

// --- Sensores (sin cambios) ---
float leerVoltajeZMPT() {
  int lectura = analogRead(ZMPT_PIN);
  return ((lectura * 5.0) / 1023.0) * 50.0; // demo
}
float leerCorrienteACS() {
  int lectura = analogRead(ACS_PIN);
  float volt = (lectura * 5.0) / 1023.0;
  return (volt - 2.5) / 0.066; // demo ACS712-30A
}
float leerVoltajeBateria() {
  int lectura = analogRead(VBAT_PIN);
  float vEsc = (lectura * 5.0) / 1023.0;
  return vEsc * 3.0; // demo divisor
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  xbeeSerial.begin(9600);
  Wire.begin();

  if (!rtc.begin()) {
    Serial.println("Error: No se encuentra el RTC.");
    while (1);
  }
  
  // IMPORTANTE: Descomenta la siguiente línea SOLO UNA VEZ para ajustar la hora del RTC.
  // Después de la primera ejecución, vuelve a comentarla.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  EEPROM.get(0, paquetesEnviados);

  // Encabezado de la tabla para el Monitor Serie
  Serial.println("\n--- NODO EMISOR INICIADO ---");
  Serial.print("ID del útlimo paquete enviado: "); Serial.println(paquetesEnviados);
  Serial.println("| ID   | Voltaje | Corriente | V. Bat  | Timestamp    |");
  Serial.println("|------|---------|-----------|---------|--------------|");

}

void loop() {
  unsigned long now = millis();

  if (now - previousMillis >= INTERVAL_MS) {
    previousMillis = now;

    DateTime nowRTC = rtc.now();
    float voltage   = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();
    float vbat      = leerVoltajeBateria();

    //Incrementar el contador antes de armar el paquete
    paquetesEnviados++;

    // Arma el paquete
    Packet p;
    p.sync      = SYNCWORD;
    p.id        = paquetesEnviados;
    p.voltaje   = (int16_t)(voltage * 100);
    p.corriente = (int16_t)(corriente * 1000);
    p.vbat      = (uint16_t)(vbat * 100);
    p.timestamp = nowRTC.unixtime();

    //Guardamos el contador en le EEPROM
    //.put() para escribir en la memoria
    EEPROM.put(0, paquetesEnviados);


    // --- SALIDA AL MONITOR SERIE (Formato de tabla) ---
    // Se usan buffers para convertir los floats a string de forma segura en Arduino
    char serialBuffer[100];
    char voltage_s[8];
    char corriente_s[10];
    char vbat_s[8];

    // dtostrf(valor_float, ancho_total, decimales, buffer_destino)
    dtostrf(voltage, 7, 2, voltage_s);
    dtostrf(corriente, 9, 3, corriente_s);
    dtostrf(vbat, 7, 2, vbat_s);

    // Se usa snprintf con strings (%s) en lugar de floats (%f)
    snprintf(serialBuffer, sizeof(serialBuffer), "| %-4u | %s | %s | %s | %-12lu |",
        p.id, voltage_s, corriente_s, vbat_s, p.timestamp);
    Serial.println(serialBuffer);

    // Codifica y envía el paquete
    uint8_t buf[PACKET_SIZE];
    encodePacket(buf, p);
    xbeeSerial.write(buf, PACKET_SIZE);

    // Muestra el paquete binario enviado
    Serial.print("TX BIN: ");
    printBufBinary(buf, PACKET_SIZE);
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

