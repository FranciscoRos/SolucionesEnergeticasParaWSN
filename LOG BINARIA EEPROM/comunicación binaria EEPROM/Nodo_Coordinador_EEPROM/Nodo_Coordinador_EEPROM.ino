#include <time.h>
#include <EEPROM.h>
#include "CodecWSN.h"

#define RXD2 16
#define TXD2 17
#define BAT_PIN 34 // Pin para medir la batería del propio receptor

#define EEPROM_SIZE 4 // Definimos tamaño de memoria a usar en ESP32

uint32_t paquetesRecibidos = 0; // Contador de paquetes recibidos

// Función para formatear el timestamp recibido
String formatUnixTime(uint32_t unixTime) {
  time_t time_utc = unixTime;
  struct tm *timeinfo;
  char buffer[30];
  timeinfo = gmtime(&time_utc);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}

// Función para leer la batería del nodo receptor
float leerVoltajeBateriaLocal() {
  int lecturaADC = analogRead(BAT_PIN);
  float voltajeADC = (lecturaADC / 4095.0) * 3.3;
  // Ajusta los valores de las resistencias de tu divisor de voltaje aquí
  return voltajeADC * ((10000.0 + 5000.0) / 5000.0); 
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, paquetesRecibidos);

  Serial.println("\n--- NODO RECEPTOR INICIADO ---");
  Serial.print("Total de paquetes recibidos previamente: ");
  Serial.println(paquetesRecibidos);
  Serial.println("| timestamp_unix | fecha_hora_utc      | id_paquete | voltaje | corriente | v_bat_sensor | v_bat_local |");
  Serial.println("|----------------|---------------------|------------|---------|-----------|--------------|-------------|");
}

// --- Lógica de recepción robusta ---
enum class RxState { WAITING_FOR_SYNC_1, WAITING_FOR_SYNC_2, READING_PAYLOAD };
RxState currentState = RxState::WAITING_FOR_SYNC_1;
uint8_t packetBuffer[PACKET_SIZE];
size_t bytesRead = 0;

void processPacket() {
    paquetesRecibidos++;
    EEPROM.put(0, paquetesRecibidos);
    EEPROM.commit(); // Guarda el contador en el ESP32

    Packet p = decodePacket(packetBuffer);

    // Verifica que el syncword sea correcto antes de procesar
    if (p.sync != SYNCWORD) {
        return; // Paquete corrupto, ignorar
    }

    float voltaje   = p.voltaje   / 100.0f;
    float corriente = p.corriente / 1000.0f;
    float vbat_sensor  = p.vbat      / 100.0f;
    float vbat_local   = leerVoltajeBateriaLocal();
    String fechaHoraLegible = formatUnixTime(p.timestamp);
    
    char serialBuffer[200];
    snprintf(serialBuffer, sizeof(serialBuffer), "| %-14lu | %-19s | %-10u | %-7.2f | %-9.3f | %-12.2f | %-11.2f |",
        p.timestamp, fechaHoraLegible.c_str(), p.id, voltaje, corriente, vbat_sensor, vbat_local);
    Serial.println(serialBuffer);

    // Envía un comando de vuelta si es necesario
    // Serial2.println("OK"); 
}

void loop() {
  while (Serial2.available() > 0) {
    uint8_t incomingByte = Serial2.read();
    switch (currentState) {
      case RxState::WAITING_FOR_SYNC_1:
        if (incomingByte == (SYNCWORD & 0xFF)) currentState = RxState::WAITING_FOR_SYNC_2;
        break;
      case RxState::WAITING_FOR_SYNC_2:
        if (incomingByte == (SYNCWORD >> 8)) {
          packetBuffer[0] = (SYNCWORD & 0xFF);
          packetBuffer[1] = (SYNCWORD >> 8);
          bytesRead = 2;
          currentState = RxState::READING_PAYLOAD;
        } else {
          currentState = RxState::WAITING_FOR_SYNC_1;
        }
        break;
      case RxState::READING_PAYLOAD:
        packetBuffer[bytesRead++] = incomingByte;
        if (bytesRead >= PACKET_SIZE) {
          processPacket();
          currentState = RxState::WAITING_FOR_SYNC_1;
        }
        break;
    }
  }
}

