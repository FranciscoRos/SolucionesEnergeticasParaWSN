#include <HardwareSerial.h>
#include <EEPROM.h>

#include "DataPayload.h"

// --- OBJETOS GLOBALES ---
DataPayload receivedPayload;
uint32_t lastReceivedID = 0;

#define EEPROM_SIZE 12

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  
  Serial.println("\nIniciando Nodo Receptor con EEPROM (sin RTC)...");

  // Inicializar la EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Leer el último ID guardado
  EEPROM.get(0, lastReceivedID);
  
  Serial.print("Ultimo ID recibido antes de este reinicio: ");
  Serial.println(lastReceivedID);
  Serial.println("----------------------------------------\n");
}

void loop() {
  if (Serial2.find('$')) {
    delay(50);

    if (Serial2.available() >= sizeof(receivedPayload)) {
      Serial2.readBytes((uint8_t*)&receivedPayload, sizeof(receivedPayload));

      if (receivedPayload.messageID > lastReceivedID) {
        lastReceivedID = receivedPayload.messageID;
        EEPROM.put(0, lastReceivedID);
        EEPROM.commit();
      }

      Serial.println("----------------------------------------");
      Serial.println("Paquete de datos recibido:");
      Serial.print("  > ID de Mensaje:      "); Serial.println(receivedPayload.messageID);
      Serial.print("  > Ultimo ID guardado: "); Serial.println(lastReceivedID);
      Serial.print("  > Voltaje AC:         "); Serial.print(receivedPayload.voltageAC, 2); Serial.println(" V");
      Serial.print("  > Corriente AC:       "); Serial.print(receivedPayload.currentAC, 2); Serial.println(" A");
      Serial.print("  > Potencia Ap.:       "); Serial.print(receivedPayload.powerApparent, 2); Serial.println(" VA");
      
      Serial.println("  --- Datos del Nodo Remoto ---");
      Serial.print("  > Voltaje Bateria:    "); Serial.print(receivedPayload.batteryVoltage, 2); Serial.println(" V");
      
      String nivel = "";
      switch(receivedPayload.energyLevel) {
        case 0: nivel = "BAJO"; break;
        case 1: nivel = "MEDIO"; break;
        case 2: nivel = "ALTO"; break;
        default: nivel = "Desconocido"; break;
      }
      Serial.print("  > Nivel de Energia:     "); Serial.println(nivel);
      Serial.println("----------------------------------------\n");
    }
  }
}