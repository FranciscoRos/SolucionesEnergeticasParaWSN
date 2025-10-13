#include <EEPROM.h>

// --- Definiciones para la EEPROM ---
#define EEPROM_SIZE 1  
#define ADDR_ESTADO 0 

// --- Pines para la comunicación Serial ---
#define RXD2 16  // Conectado al TX del XBee
#define TXD2 17  // Conectado al RX del XBee

// --- Variables Globales ---
float voltaje = 0.0;
float corriente = 0.0;
bool estadoActual = false; // Variable para mantener el estado (false=OFF, true=ON)

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // UART2 para XBee
  
  // Inicializamos la EEPROM con el tamaño definido
  EEPROM.begin(EEPROM_SIZE);

  // Leemos el último estado guardado en la EEPROM al arrancar
  estadoActual = EEPROM.read(ADDR_ESTADO);

  Serial.println("Nodo Coordinador ESP32 iniciado");
  Serial.print("Estado recuperado de la EEPROM: ");
  Serial.println(estadoActual ? "ON" : "OFF");
}

void loop() {
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.trim();

    // Validar que sea un paquete válido tipo: V:xx.xx I:yy.yy
    if (data.indexOf('V') != -1 && data.indexOf('I') != -1) {
      Serial.println("📡 Datos recibidos: " + data);

      int idxV = data.indexOf('V');
      int idxI = data.indexOf('I');

      voltaje = data.substring(idxV + 2, idxI - 1).toFloat();
      corriente = data.substring(idxI + 2).toFloat();

      Serial.print(" Voltaje: "); Serial.println(voltaje);
      Serial.print(" Corriente: "); Serial.println(corriente);

      // --- Lógica de decisión ---
      bool nuevoEstado;
      if (voltaje < 190 || voltaje > 250 || corriente < 0.1) {
        nuevoEstado = false; // El nuevo estado debe ser OFF
      } else {
        nuevoEstado = true; // El nuevo estado debe ser ON
      }

      // --- Comprobación y guardado en EEPROM ---
      // Solo enviamos comando y guardamos si el estado ha cambiado
      // para evitar escrituras innecesarias en la EEPROM.
      if (nuevoEstado != estadoActual) {
        estadoActual = nuevoEstado;
        
        String comando = estadoActual ? "ON" : "OFF";
        Serial2.println(comando);
        Serial.println(">> Estado cambió. Enviando comando: " + comando);

        // Guardamos el nuevo estado en la EEPROM
        EEPROM.write(ADDR_ESTADO, estadoActual);
        EEPROM.commit();
        
        Serial.println(">> Nuevo estado guardado en EEPROM.");
      }
    }
  }

  delay(1000);
}
