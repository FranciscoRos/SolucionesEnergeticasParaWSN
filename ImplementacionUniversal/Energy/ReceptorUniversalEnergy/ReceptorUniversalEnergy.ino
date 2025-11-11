/*
 * ======================================================================
 * ==       SKETCH RECEPTOR UNIVERSAL (MODO STRING / Energy)           ==
 * ======================================================================
 */

#include <SPI.h>
#include "UniversalRadioWSN.h"
#include <EEPROM.h>

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
//#define USE_XBEE 
//#define USE_LORA
#define USE_NRF 

// ======================= CONFIGURACIÓN Y VARIABLES GLOBALES =======================
#define RXD2 16
#define TXD2 17
#define EEPROM_SIZE 32
#define EEPROM_ADDR 0   // Dirección única para guardar el contador

RadioInterface* radio;
uint32_t mensajesRecibidos = 0;
String bufferReceptor = "";

#if defined(USE_NRF)
  const byte nrfReadAddress[6]  = "00001"; 
  const byte nrfWriteAddress[6] = "00002"; 
#endif

// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("\n--- RECEPTOR UNIVERSAL INICIADO (MODO STRING Energy) ---");

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

  #elif defined(USE_NRF)
    Serial.println("NRF24L01");
    NrfConfig configNrf;
    configNrf.cePin = 4; 
    configNrf.csnPin = 5;
    configNrf.writeAddress = nrfWriteAddress;
    configNrf.readAddress  = nrfReadAddress;
    configNrf.channel = 108;      
    configNrf.dataRate = 250; 
    configNrf.paLevel = 0;
    radio = new NrfRadio(configNrf);
  #endif
  
  if (!radio->iniciar()) {
    Serial.println("¡ERROR! Fallo al iniciar el módulo de radio.");
    while (true);
  }

  Serial.println("Módulo de radio inicializado. Esperando datos...");

  // ==== EEPROM ====
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(EEPROM_ADDR, mensajesRecibidos);
  Serial.print("Contador recuperado de EEPROM: ");
  Serial.println(mensajesRecibidos);
}

// ======================= LOOP =======================
void loop() {

#if defined(USE_NRF)
  if (radio->hayDatosDisponibles()) {
    String linea = radio->leerComoString();
    linea.trim();

    if (linea.length() > 0) {
      mensajesRecibidos++;

      Serial.print("Paquete recibido #");
      Serial.print(mensajesRecibidos);
      Serial.print(": --> ");
      Serial.println(linea);

      // Guardar contador en EEPROM
      EEPROM.put(EEPROM_ADDR, mensajesRecibidos);
      EEPROM.commit();

      radio->enviar("ON");
    }
  }

#else
  if (radio->hayDatosDisponibles()) {
    bufferReceptor += radio->leerComoString();
  }

  int fin = bufferReceptor.indexOf('\n');

  if (fin >= 0) {
    String linea = bufferReceptor.substring(0, fin);
    bufferReceptor.remove(0, fin + 1);
    linea.trim();

    if (linea.length() > 0) {
      mensajesRecibidos++;

      Serial.print("Línea recibida #");
      Serial.print(mensajesRecibidos);
      Serial.print(": --> ");
      Serial.println(linea);

      EEPROM.put(EEPROM_ADDR, mensajesRecibidos);
      EEPROM.commit();

      radio->enviar("ON\n");
    }
  }
#endif
}
