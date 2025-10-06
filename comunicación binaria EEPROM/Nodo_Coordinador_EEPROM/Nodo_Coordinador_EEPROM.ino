#include <SPI.h>
#include <time.h>
#include <EEPROM.h>
#include "CodecWSN.h" // <--- Usará la versión MODIFICADA de 14 bytes

#define RXD2 16
#define TXD2 17

//Definimoa el tamaño de la EEPROM
#define EEPROM_SIZE 4

const uint8_t SD_CS = 5;

unsigned long lastFlush = 0;
const unsigned long FLUSH_MS = 30000;
uint32_t paquetesRecibidos = 0;

String formatUnixTime(uint32_t unixTime) {
  time_t time_utc = unixTime;
  struct tm *timeinfo;
  char buffer[30];
  timeinfo = gmtime(&time_utc);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  //Inicializamos el EEPROM del esp32
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, paquetesRecibidos);

  // Imprime el encabezado de la tabla en el Monitor Serie
  Serial.println();
  Serial.println("| timestamp_unix | fecha_hora_utc      | id_paquete | voltaje | corriente | voltaje_bateria |");
  Serial.println("|----------------|---------------------|------------|---------|-----------|-----------------|");
}

// --- LÓGICA DE RECEPCIÓN ROBUSTA ---
enum class RxState {
  WAITING_FOR_SYNC_1,
  WAITING_FOR_SYNC_2,
  READING_PAYLOAD
};

RxState currentState = RxState::WAITING_FOR_SYNC_1;
uint8_t packetBuffer[PACKET_SIZE];
size_t bytesRead = 0;

// --- CORRECCIÓN APLICADA AQUÍ ---
const uint8_t SYNC_BYTE_1 = static_cast<uint8_t>(SYNCWORD >> 8);      // 0xAB
const uint8_t SYNC_BYTE_2 = static_cast<uint8_t>(SYNCWORD & 0xFF);    // 0xCD <-- CORREGIDO

void processPacket() {
    paquetesRecibidos++;
    EEPROM.put(0, paquetesRecibidos); //Escribimos en el EEPROM
    EEPROM.commit(); //Guardamos los cambios en la EEPROM


    Packet p = decodePacket(packetBuffer);

    float voltaje   = p.voltaje   / 100.0f;
    float corriente = p.corriente / 1000.0f;
    float vbat      = p.vbat      / 100.0f;
    String fechaHoraLegible = formatUnixTime(p.timestamp);
    
    // --- SALIDA AL MONITOR SERIE (nuevo formato de tabla) ---
    char serialBuffer[200];
    // Se usa snprintf para formatear los datos en columnas alineadas
    snprintf(serialBuffer, sizeof(serialBuffer), "| %-14u | %-19s | %-10u | %-7.2f | %-9.3f | %-15.2f |",
        p.timestamp,
        fechaHoraLegible.c_str(),
        p.id,
        voltaje,
        corriente,
        vbat);
    Serial.println(serialBuffer);
}

void loop() {
  while (Serial2.available() > 0) {
    uint8_t incomingByte = Serial2.read();

    switch (currentState) {
      case RxState::WAITING_FOR_SYNC_1:
        if (incomingByte == SYNC_BYTE_1) {
          currentState = RxState::WAITING_FOR_SYNC_2;
        }
        break;

      case RxState::WAITING_FOR_SYNC_2:
        if (incomingByte == SYNC_BYTE_2) {
          packetBuffer[0] = SYNC_BYTE_1;
          packetBuffer[1] = SYNC_BYTE_2;
          bytesRead = 2;
          currentState = RxState::READING_PAYLOAD;
        } else {
          currentState = RxState::WAITING_FOR_SYNC_1;
        }
        break;

      case RxState::READING_PAYLOAD:
        packetBuffer[bytesRead++] = incomingByte;
        if (bytesRead == PACKET_SIZE) {
          processPacket();
          currentState = RxState::WAITING_FOR_SYNC_1;
        }
        break;
    }
  }
}

