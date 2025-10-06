#include <SPI.h>
#include <SD.h>

// Pines UART para XBee
#define RXD2 16
#define TXD2 17
const uint8_t SD_CS = 5;

File logFile;
unsigned long lastFlush = 0;
const unsigned long FLUSH_MS = 30000;
uint32_t paquetesRecibidos = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD no inicializada");
  } else {
    logFile = SD.open("/clog.txt", FILE_WRITE);
    if (logFile) logFile.println("fecha_hora,id_paquete,id_nodo,RSSI,estado");
    logFile.flush();
  }
}

void loop() {
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.trim();

    if (data.startsWith("N:")) {
      paquetesRecibidos++;
      int idStart = data.indexOf("N:") + 2;
      int idEnd = data.indexOf(' ', idStart);
      String id_paquete = data.substring(idStart, idEnd);

      String fecha_hora = obtenerFechaHora();
      int rssi = WiFi.RSSI(); // Simula RSSI (puedes usar valor real del XBee)
      String linea = fecha_hora + "," + id_paquete + ",SENSOR1," + String(rssi) + ",OK";

      Serial.println(linea);
      if (logFile) logFile.println(linea);

      // Lógica básica de respuesta
      Serial2.println("ON"); // aquí decides ON/OFF según lógica real
    }
  }

  if (logFile && millis() - lastFlush >= FLUSH_MS) {
    logFile.flush();
    lastFlush = millis();
  }
}

// Simular fecha/hora (usar RTC en caso real)
String obtenerFechaHora() {
  unsigned long segundos = millis() / 1000;
  int hh = 12 + (segundos / 3600) % 12;
  int mm = (segundos / 60) % 60;
  int ss = segundos % 60;
  char buf[20];
  sprintf(buf, "2025-06-03 %02d:%02d:%02d", hh, mm, ss);
  return String(buf);
}
