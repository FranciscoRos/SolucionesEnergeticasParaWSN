#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include "CodecWSN.h"  // Packet, PACKET_SIZE, WSNFrame::encodeFrameFromPacket, FRAME_SIZE

// XBee en pines digitales (SoftwareSerial)
SoftwareSerial xbeeSerial(2, 3); // RX=D2, TX=D3

// Pines de sensores / actuador (ajústalos a tu hardware real)
#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2

const uint8_t SD_CS = 10;

// Temporización y SD
unsigned long previousMillis = 0;
const unsigned long INTERVAL_MS = 3000;
unsigned long lastFlush = 0;
const unsigned long FLUSH_MS = 30000;
uint32_t paquetesEnviados = 0;
File logFile;

// === Lecturas de ejemplo (ajusta a tu hardware real) ===
float leerVoltajeZMPT() {
  int lectura = analogRead(ZMPT_PIN);
  return ((lectura * 5.0f) / 1023.0f) * 50.0f;}
float leerCorrienteACS() {
  int lectura = analogRead(ACS_PIN);
  float v = (lectura * 5.0f) / 1023.0f;
  return (v - 2.5f) / 0.066f; }
float leerVoltajeBateria() {
  int lectura = analogRead(VBAT_PIN);
  float vEsc = (lectura * 5.0f) / 1023.0f;
  return vEsc * 3.0f; }

String obtenerFechaHora() {
  unsigned long s = millis() / 1000;
  int hh = 12 + (s / 3600) % 12;
  int mm = (s / 60) % 60;
  int ss = s % 60;
  char buf[20];
  sprintf(buf, "2025-09-05 %02d:%02d:%02d", hh, mm, ss);
  return String(buf);}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);      // Consola local
  xbeeSerial.begin(9600);  // Enlace XBee
  
  Serial.print("encendido  Nodo Sensor binario\n");
  // SD para almacenar y verificar pérdidas
  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD no inicializada"));
  } else {
    logFile = SD.open("/llog.txt", FILE_WRITE);
    if (logFile) {
      logFile.println("fecha_hora,id_paquete,voltaje_red,corriente,voltaje_bateria");
      logFile.flush();
    }
  }
}

void loop() {
  unsigned long now = millis();

  // Cada INTERVAL_MS, medimos → paquetizamos → enviamos FRAME BINARIO (con SOF/CRC)
  if (now - previousMillis >= INTERVAL_MS) {
    previousMillis = now;

    const float v_red = leerVoltajeZMPT();  
    const float i     = leerCorrienteACS(); 
    const float vbat  = leerVoltajeBateria();
    paquetesEnviados++;

    // Armar Packet (unidades: voltaje en centésimas, corriente en mA, vbat en centésimas)
    Packet p;
    p.id        = static_cast<uint16_t>(paquetesEnviados & 0xFFFF);
    p.voltaje   = static_cast<int16_t>(v_red * 100.0f);   // centésimas de V
    p.corriente = static_cast<int16_t>(i * 1000.0f);      // mA
    p.vbat      = static_cast<uint16_t>(vbat * 100.0f);   // centésimas de V

    // Codificar a FRAME y ENVIAR EN BINARIO por XBee
    uint8_t frame[WSNFrame::FRAME_SIZE];
    WSNFrame::encodeFrameFromPacket(frame, p);
    xbeeSerial.write(frame, WSNFrame::FRAME_SIZE);

    // (Opcional) Log humano local en el Nano (Serial/SD)
    String linea = obtenerFechaHora() + "," + String(p.id) + "," +
                   String(v_red, 2) + "," + String(i, 3) + "," + String(vbat, 2);
    Serial.println(linea);
    if (logFile) logFile.println(linea);
  }

  // Recibir comandos de texto desde el coordinador (ej. "ON"/"OFF")
  if (xbeeSerial.available()) {
    String comando = xbeeSerial.readStringUntil('\n');
    comando.trim();
    if (comando == "ON")  digitalWrite(RELAY_PIN, HIGH);
    if (comando == "OFF") digitalWrite(RELAY_PIN, LOW);
  }

  // Flush periódico de SD
  if (logFile && millis() - lastFlush >= FLUSH_MS) {
    logFile.flush();
    lastFlush = millis();
  }
}
