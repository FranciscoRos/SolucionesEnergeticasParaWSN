#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>

SoftwareSerial xbeeSerial(2, 3); // RX=D2, TX=D3 para XBee

// Pines ya definidos
#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2
const uint8_t SD_CS = 10;

// Variables
unsigned long previousMillis = 0;
const unsigned long INTERVAL_MS = 3000;
unsigned long lastFlush = 0;
const unsigned long FLUSH_MS = 30000;
uint32_t paquetesEnviados = 0;
File logFile;

// Setup
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  
  Serial.println("Nodo sensor Log iniciado");
  xbeeSerial.begin(9600);

//Inicializamos la SD
  if (!SD.begin(SD_CS)) {
    Serial.println("SD no inicializada");
  } else {
    logFile = SD.open("/llog.txt", FILE_WRITE);
    if (logFile) logFile.println("fecha_hora,id_paquete,voltaje_red,corriente,voltaje_bateria");
    logFile.flush();
  }
}

// Loop
void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= INTERVAL_MS) {
    previousMillis = currentMillis;

    float voltage = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();
    float vbat = leerVoltajeBateria();
    paquetesEnviados++;

    // Línea CSV
    String fecha_hora = obtenerFechaHora();
    String linea = fecha_hora + "," + String(paquetesEnviados) + "," +
                   String(voltage, 2) + "," + String(corriente, 2) + "," +
                   String(vbat, 2);

    // Envío por XBee
    xbeeSerial.print("N:"); xbeeSerial.print(paquetesEnviados);
    xbeeSerial.print(" V:"); xbeeSerial.print(voltage, 2);
    xbeeSerial.print(" I:"); xbeeSerial.print(corriente, 2);
    xbeeSerial.print(" B:"); xbeeSerial.println(vbat, 2);

    // Serial y SD
    Serial.println(linea);
    if (logFile) logFile.println(linea);
  }

  // Comandos recibidos
  if (xbeeSerial.available()) {
    String comando = xbeeSerial.readStringUntil('\n');
    comando.trim();

    if (comando == "ON") digitalWrite(RELAY_PIN, HIGH);
    else if (comando == "OFF") digitalWrite(RELAY_PIN, LOW);
  }

  if (logFile && millis() - lastFlush >= FLUSH_MS) {
    logFile.flush();
    lastFlush = millis();
  }
}

// Funciones sensores existentes
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

// Fecha/hora calculada desde millis
String obtenerFechaHora() {
  unsigned long segundos = millis() / 1000;
  int hh = 12 + (segundos / 3600) % 12;
  int mm = (segundos / 60) % 60;
  int ss = segundos % 60;
  char buf[20];
  sprintf(buf, "2025-06-05 %02d:%02d:%02d", hh, mm, ss);
  return String(buf);
}
