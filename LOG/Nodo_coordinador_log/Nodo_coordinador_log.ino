#include <SPI.h>
#include <SD.h>

// UART para XBee (ESP32)
#define RXD2 16
#define TXD2 17

// Chip Select de la SD en ESP32 (ajústalo a tu placa)
const uint8_t SD_CS = 5;

File logFile;
unsigned long lastFlush = 0;
const unsigned long FLUSH_MS = 30000;
uint32_t paquetesRecibidos = 0;

// ---- Utilidades ----
String obtenerFechaHora() {
  unsigned long segundos = millis() / 1000;
  int hh = 12 + (segundos / 3600) % 12;
  int mm = (segundos / 60) % 60;
  int ss = segundos % 60;
  char buf[20];
  sprintf(buf, "2025-06-03 %02d:%02d:%02d", hh, mm, ss);
  return String(buf);
}

// Busca "clave:" y obtiene el valor hasta el siguiente espacio o fin de línea.
// Devuelve true si encontró y pudo convertir a float (o a unsigned long si isInt=true).
bool extraerValor(const String& data, const char* clave, float& outVal) {
  int k = data.indexOf(clave);
  if (k < 0) return false;
  k += strlen(clave);
  // saltar espacios
  while (k < (int)data.length() && data[k] == ' ') k++;
  int end = data.indexOf(' ', k);
  if (end < 0) end = data.length();
  String token = data.substring(k, end);
  token.trim();
  if (token.length() == 0) return false;
  outVal = token.toFloat();
  return true;
}

bool extraerIdPaquete(const String& data, unsigned long& outId) {
  int k = data.indexOf("N:");
  if (k < 0) return false;
  k += 2;
  while (k < (int)data.length() && data[k] == ' ') k++;
  int end = data.indexOf(' ', k);
  if (end < 0) end = data.length();
  String token = data.substring(k, end);
  token.trim();
  if (token.length() == 0) return false;

  // toInt() retorna long con base 10
  long v = token.toInt();
  if (v < 0) return false;
  outId = (unsigned long)v;
  return true;
}

// Parseo robusto del frame "N: V: I: B:" en cualquier orden.
// Devuelve true si pudo extraer todos.
bool parseFrame(const String& data, unsigned long& id, float& vred, float& iamp, float& vbat) {
  bool ok = true;
  ok &= extraerIdPaquete(data, id);
  ok &= extraerValor(data, "V:", vred);
  ok &= extraerValor(data, "I:", iamp);
  ok &= extraerValor(data, "B:", vbat);
  return ok;
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Serial.println("Nodo coordinador Log iniciado");
  if (!SD.begin(SD_CS)) {
    Serial.println("[SD] SD no inicializada");
  } else {
    logFile = SD.open("/clog.txt", FILE_WRITE);
    if (logFile) {
      // Mantengo tu encabezado original (usa la batería del sensor recibida en B:)
      logFile.println("fecha_hora,id_paquete,id_nodo,RSSI,estado,voltaje_bateria");
      logFile.flush();
    }
  }

  // Encabezado bonito para la consola
  Serial.println();
  Serial.println(F("==== COORDINADOR ESP32 (XBee) ===="));
  Serial.println(F("Esperando tramas del sensor (formato: N:<id> V:<Vr> I:<I> B:<Vbat>)"));
  Serial.println();
  Serial.println(F("Fecha/Hora            Nodo      pkt     Vred[V]   I[A]    Vbat[V]   RSSI  Estado"));
  Serial.println(F("--------------------  --------  ------  --------  ------  --------  ----  --------"));
}

void loop() {
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.trim();
    if (data.length() > 0) {
      paquetesRecibidos++;

      unsigned long id = 0;
      float vred = NAN, iamp = NAN, vbat = NAN;

      bool ok = parseFrame(data, id, vred, iamp, vbat);
      int rssi = -1;             // XBee en modo transparente no entrega RSSI por UART
      const char* estado = ok ? "OK" : "PARSE_ERR";
      String fecha_hora = obtenerFechaHora();

      // ---- Impresión bonita en consola ----
      // Alinear columnas manualmente
      char lineaBonita[160];
      // %-20s fecha fija, %-8s nodo, %6lu pkt, %8.2f, %6.2f, %8.2f, %4d RSSI, %-8s estado
      snprintf(lineaBonita, sizeof(lineaBonita),
               "%-20s  %-8s  %6lu  %8.2f  %6.2f  %8.2f  %4d  %-8s",
               fecha_hora.c_str(),
               "SENSOR1",
               id,
               ok ? vred : 0.0,
               ok ? iamp : 0.0,
               ok ? vbat : 0.0,
               rssi,
               estado);
      Serial.println(lineaBonita);

      // ---- Registro a SD (mantengo tu CSV original) ----
      // Usamos la batería del sensor (B:) para el campo "voltaje_bateria"
      if (logFile) {
        String lineaCSV = fecha_hora + "," + String(id) + ",SENSOR1," +
                          String(rssi) + "," + estado + "," + String(ok ? vbat : 0.0, 2);
        logFile.println(lineaCSV);
      }

      // ---- Comando al sensor (opcional) ----
      // Si no quieres encender siempre el relé, comenta la siguiente línea.
      Serial2.println("ON");
    }
  }

  if (logFile && millis() - lastFlush >= FLUSH_MS) {
    logFile.flush();
    lastFlush = millis();
  }
}
