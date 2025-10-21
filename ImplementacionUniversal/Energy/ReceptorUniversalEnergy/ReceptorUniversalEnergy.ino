/**
 * ======================================================================
 * ==      SKETCH RECEPTOR UNIVERSAL (XBee + LoRa Preparado)         ==
 * ======================================================================
 */

// --- 1. LIBRERÍAS ---
#include <SPI.h>
#include "UniversalRadioWSN.h"
#include <EEPROM.h>

// --- 2. SELECCIÓN DEL MÓDULO DE RADIO ---
// Asegúrate de que esta línea coincida con la del emisor.
#define USE_LORA
//#define USE_XBEE

// --- 3. CONFIGURACIÓN DE PINES ---
#if defined(USE_XBEE)
  #define RXD2 16
  #define TXD2 17
#endif

// --- 4. OBJETOS GLOBALES ---
RadioInterface* radio;
uint32_t mensajesRecibidos;

// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("\n--- RECEPTOR UNIVERSAL INICIADO ---");

  // --- INICIALIZACIÓN DEL MÓDULO DE RADIO ---
  Serial.print("Configurando radio: ");
  
  #if defined(USE_XBEE)
    Serial.println("XBee");
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    radio = new XBeeRadio(Serial2, 9600, -1, -1);
    
  #elif defined(USE_LORA)
    Serial.println("LoRa");
    SPI.begin();
    LoRaConfig configLora;
    configLora.frequency       = 410E6;
    configLora.spreadingFactor = 7;
    configLora.signalBandwidth = 125E3;
    configLora.codingRate      = 5;
    configLora.syncWord        = 0xF3;
    configLora.txPower         = 20;
    configLora.csPin           = 5;
    configLora.irqPin          = 2;
    configLora.resetPin        = -1;
    radio = new LoraRadio(configLora);
  #endif
  
  if (!radio->iniciar()) {
    Serial.println("¡ERROR! Fallo al iniciar el módulo de radio.");
    while (true);
  }
  
  Serial.println("Módulo de radio inicializado. Esperando datos...");
  EEPROM.begin(sizeof(mensajesRecibidos));
  EEPROM.get(0, mensajesRecibidos);
  Serial.print("Contador de mensajes recuperado de EEPROM: ");
  Serial.println(mensajesRecibidos);
}

// ======================= LOOP =======================
void loop() {
  if (radio->hayDatosDisponibles()) {
    String datosRecibidos = radio->leerComoString();
    datosRecibidos.trim();
    
    if (datosRecibidos.length() > 0) {
      mensajesRecibidos++;
      
      Serial.print("Línea recibida #" + String(mensajesRecibidos) + ": --> ");
      Serial.println(datosRecibidos);
      
      EEPROM.put(0, mensajesRecibidos);
      EEPROM.commit();
      
      // Opcional: Enviar una respuesta de confirmación.
      // radio->enviar("OK\n");
    }
  }
}