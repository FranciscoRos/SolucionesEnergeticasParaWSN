#include <SPI.h>
#include <SD.h>
#include "CodecWSN.h"  // Packet, decodePacketFast, WSNFrame::{Parser, feed, FRAME_SIZE}

// UART para XBee (ESP32)
#define RXD2 16
#define TXD2 17
const uint8_t SD_CS = 5;

// (Opcional) ADC batería local del coordinador
const uint8_t BAT_PIN = 34;

File logFile;
unsigned long lastFlush = 0;
const unsigned long FLUSH_MS = 30000;

String obtenerFechaHora() {
  unsigned long s = millis() / 1000;
  int hh = 12 + (s / 3600) % 12;
  int mm = (s / 60) % 60;
  int ss = s % 60;
  char buf[20];
  sprintf(buf, "2025-06-05 %02d:%02d:%02d", hh, mm, ss);
  return String(buf);
}

float leerVoltajeBateriaLocal() {
  // ESP32 ADC: 0..4095 → 0..3.3V (si no reconfiguraste atenuación)
  int lecturaADC = analogRead(BAT_PIN);
  float vADC = (lecturaADC / 4095.0f) * 3.3f;
  // Ejemplo: divisor 10k:5k (ajusta a tu hardware real)
  return vADC * ((10000.0f + 5000.0f) / 5000.0f);
}

// --- Parser global (o estático) para mantener estado entre llamadas
WSNFrame::Parser gParser;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD no inicializada");
  } else {
    logFile = SD.open("/clog.txt", FILE_WRITE);
    if (logFile) {
      // Cabecera acorde a lo que DECODEAMOS del frame binario:
      logFile.println("fecha_hora,id_paquete,voltaje,corriente,voltaje_bateria");
      logFile.flush();
    }
  }
  Serial.print("encendido  Nodo Coordinador binario\n");
}

void loop() {
  // Lee TODOS los bytes disponibles y alimenta el parser
  while (Serial2.available()) {
    uint8_t b = uint8_t(Serial2.read());

    Packet rx;
    const bool ok = WSNFrame::feed(gParser, b, rx);
    if (!ok) continue;  // aún no se completó un frame válido

    // Si ok == true, rx ya contiene el Packet decodificado y verificado por CRC
    const float voltaje = rx.voltaje / 100.0f;       // V
    const float corriente = rx.corriente / 1000.0f;  // A
    const float vbat = rx.vbat / 100.0f;             // V

    // Línea CSV
    String linea = obtenerFechaHora() + "," + String(rx.id) + "," + String(voltaje, 2) + "," + String(corriente, 3) + "," + String(vbat, 2);

    Serial.println(linea);
    if (logFile) logFile.println(linea);

    // (Opcional) responder comando de control (texto)
    Serial2.println("ON");
  }

  // Flush periódico de SD
  if (logFile && millis() - lastFlush >= FLUSH_MS) {
    logFile.flush();
    lastFlush = millis();
  }
}
