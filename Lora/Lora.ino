/*
 * ==========================================================
 * ==        RECEPTOR LORA BÁSICO (ESP32) - REVISADO       ==
 * ==========================================================
 * Este sketch es un receptor simple. Su única función es
 * escuchar paquetes de LoRa y mostrarlos en el monitor serie
 * junto con la intensidad de la señal (RSSI).
 *
 * Conexiones (ESP32 - LoRa):
 * - 3.3V -> VCC
 * - GND  -> GND
 * - G23  -> MOSI
 * - G19  -> MISO
 * - G18  -> SCK
 * - G5   -> NSS/CS
 * - G14  -> RESET
 * - G2   -> DIO0
 */
#include <SPI.h>
#include <LoRa.h> // Incluye la librería LoRa directamente

// Define la frecuencia de LoRa para tu región (debe coincidir con el emisor)
const long LORA_FREQUENCY = 915E6;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("\n--- Receptor LoRa Básico Iniciado ---");

  // Configura los pines para el LoRa en el ESP32
  LoRa.setPins(5, 14, 2); // LoRa.setPins(csPin, resetPin, irqPin);

  // Intenta inicializar el módulo LoRa
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("¡Error al iniciar el módulo LoRa!");
    while (1);
  }

  // ====================================================================================
  // ==         PARÁMETROS DE COMUNICACIÓN LoRa (¡DEBEN COINCIDIR CON EL EMISOR!)        ==
  // ====================================================================================

  // 1. FACTOR DE PROPAGACIÓN (Spreading Factor - SF)
  // Rango: 6 a 12. Debe ser el mismo que en el emisor.
  LoRa.setSpreadingFactor(7);

  // 2. ANCHO DE BANDA DE LA SEÑAL (Signal Bandwidth)
  // Valores comunes: 62.5E3, 125E3, 250E3. Debe ser el mismo que en el emisor.
  LoRa.setSignalBandwidth(125E3);

  // 3. TASA DE CODIFICACIÓN (Coding Rate)
  // Rango: 5 a 8 (representa 4/5, 4/6, etc.). Debe ser el mismo que en el emisor.
  LoRa.setCodingRate4(5);

  // 4. PALABRA DE SINCRONIZACIÓN (Sync Word) - "CANAL PRIVADO"
  // Un byte de 0x00 a 0xFF. Solo escuchará a los emisores con la misma palabra.
  LoRa.setSyncWord(0xF3);

  // ====================================================================================

  Serial.println("Módulo LoRa inicializado y escuchando...");
}

void loop() {
  // Intenta parsear un paquete entrante
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    // Se recibió un paquete
    Serial.print("Paquete recibido con RSSI ");
    Serial.print(LoRa.packetRssi()); // Imprime la intensidad de la señal
    Serial.println(" dBm:");

    String datosRecibidos = "";
    // Lee el paquete mientras haya bytes disponibles
    while (LoRa.available()) {
      datosRecibidos += (char)LoRa.read();
    }

    // Imprime el contenido del paquete
    Serial.print(" > Contenido: '");
    Serial.print(datosRecibidos);
    Serial.println("'");
  }
}

