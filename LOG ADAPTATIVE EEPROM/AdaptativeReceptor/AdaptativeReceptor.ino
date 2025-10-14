#include <HardwareSerial.h>
#include <Wire.h>
#include <RTClib.h>

#include "DataPayload.h" // La misma estructura de datos

// --- PINES HARDWARE (Según tu esquema) ---
// El ESP32 usa Serial2 en los pines GPIO 16 (RX) y 17 (TX)
// No es necesario definirlos, solo llamar a Serial2.begin()
#define RXD2 16
#define TXD2 17

// --- OBJETOS GLOBALES ---
DataPayload receivedPayload;
RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  
  // Iniciar Serial2 para la comunicación con el XBee
  // Formato: begin(baud-rate, protocol, RX pin, TX pin)
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Serial.println("\nIniciando Nodo Receptor...");

  // Iniciar I2C y RTC local
  if (!rtc.begin()) {
    Serial.println("No se pudo encontrar el RTC local.");
  }
}

void loop() {
  // Comprobar si ha llegado un paquete completo de datos
  if (Serial2.available() >= sizeof(receivedPayload)) {
    // Leer los bytes directamente en la estructura
    Serial2.readBytes((uint8_t*)&receivedPayload, sizeof(receivedPayload));

    Serial.println("----------------------------------------");
    Serial.println("Paquete de datos recibido:");

    // Convertir Unix time a formato legible
    DateTime timestamp(receivedPayload.unixTime);
    char buffer[32];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
            timestamp.year(), timestamp.month(), timestamp.day(),
            timestamp.hour(), timestamp.minute(), timestamp.second());
    
    Serial.print("  > Marca de Tiempo: "); Serial.println(buffer);
    Serial.print("  > Voltaje AC:      "); Serial.print(receivedPayload.voltageAC, 2); Serial.println(" V");
    Serial.print("  > Corriente AC:    "); Serial.print(receivedPayload.currentAC, 2); Serial.println(" A");
    Serial.print("  > Potencia Ap.:    "); Serial.print(receivedPayload.powerApparent, 2); Serial.println(" VA");
    
    Serial.println("  --- Datos del Nodo Remoto ---");
    Serial.print("  > Voltaje Bateria: "); Serial.print(receivedPayload.batteryVoltage, 2); Serial.println(" V");
    
    String nivel = "";
    switch(receivedPayload.energyLevel) {
      case 0: nivel = "BAJO"; break;
      case 1: nivel = "MEDIO"; break;
      case 2: nivel = "ALTO"; break;
      default: nivel = "Desconocido"; break;
    }
    Serial.print("  > Nivel de Energia:  "); Serial.println(nivel);
    Serial.println("----------------------------------------\n");
  }
}