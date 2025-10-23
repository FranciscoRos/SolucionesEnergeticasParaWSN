/*
 * ======================================================================
 * ==      SKETCH RECEPTOR UNIVERSAL (MODO BINARIO con CodecWSN)       ==
 * ======================================================================
 * Este sketch usa el Parser de CodecWSN para decodificar frames binarios
 * robustos recibidos por LoRa o XBee.
 */

// --- LIBRERÍAS ---
#include <SPI.h>
#include "UniversalRadioWSN.h"
#include <CodecWSN.h> // Librería para decodificación binaria
#include <EEPROM.h>   // Librería para memoria no volátil

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
//#define USE_XBEE 
#define USE_LORA 
//#define USE_NRF // Asegúrate que coincida con el Emisor

// ======================= CONFIGURACIÓN Y VARIABLES GLOBALES =======================
// Pines para XBee (usando Serial2 en ESP32)
#define RXD2 16
#define TXD2 17

RadioInterface* radio;     // Puntero a la interfaz.
WSNFrame::Parser miParser; // Objeto para decodificar frames.
uint32_t mensajesRecibidos;

// --- Configuración específica por radio ---
#if defined(USE_NRF)
  // Direcciones para NRF24L01 (Invertidas respecto al emisor)
  const byte nrfReadAddress[6] = "00001";   // Dirección de lectura (la de escritura del emisor)
  const byte nrfWriteAddress[6] = "00002"; // Dirección de escritura (la de lectura del emisor)
#endif

// ======================= SETUP =======================
void setup() {
  Serial.begin(115200); // Se usa 115200 para el receptor (ESP32/PC)
  while (!Serial);
  Serial.println("\n--- RECEPTOR UNIVERSAL INICIADO (MODO BINARIO receptor Energy) ---");

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
    configLora.csPin           = 5;  // Pin CS en ESP32
    configLora.irqPin          = 2;  // Pin IRQ en ESP32
    configLora.resetPin        = -1;
    radio = new LoraRadio(configLora);
  #elif defined(USE_NRF)
    Serial.println("NRF24L01");
    NrfConfig configNrf;
    configNrf.cePin = 4; // Pin CE en ESP32
    configNrf.csnPin = 5; // Pin CSN en ESP32
    configNrf.writeAddress = nrfWriteAddress; // Para responder "ON"
    configNrf.readAddress = nrfReadAddress;   // Para recibir datos
    configNrf.channel = 108;
    configNrf.dataRate = 250; // 250KBPS
    configNrf.paLevel = 0;    // 0 = RF24_PA_MIN
    
    radio = new NrfRadio(configNrf);
  #endif
  
  if (!radio->iniciar()) {
    Serial.println("¡ERROR! Fallo al iniciar el módulo de radio.");
    while (true);
  }
  
  miParser.reset(); // Resetea el parser al inicio
  Serial.println("Módulo de radio inicializado. Esperando datos binarios...");
  
  // --- LECTURA INICIAL DE LA EEPROM ---
  // (Adaptado para ESP32, asumiendo que es el receptor)
  EEPROM.begin(sizeof(mensajesRecibidos));
  EEPROM.get(3, mensajesRecibidos);
  Serial.print("Contador de mensajes recuperado de EEPROM: ");
  Serial.println(mensajesRecibidos);
}

// ======================= LOOP =======================
void loop() {
  if (radio->hayDatosDisponibles()) {
    
    // 1. Leer los bytes crudos de la radio
    uint8_t bufferRecepcion[64]; // Buffer amplio
    size_t bytesLeidos = radio->leer(bufferRecepcion, 64);

    // 2. Alimentar cada byte, uno por uno, al parser
    for (size_t i = 0; i < bytesLeidos; i++) {
      Packet paqueteRecibido;
      
      // feed() devuelve true solo cuando ha recibido un frame completo y válido
      if (WSNFrame::feed(miParser, bufferRecepcion[i], paqueteRecibido)) {
        
        mensajesRecibidos++;
        
        // --- GUARDADO EN EEPROM ---
        EEPROM.put(3, mensajesRecibidos);
        EEPROM.commit(); // Asegura la escritura en ESP32

        // --- IMPRESIÓN EN MONITOR SERIAL ---
        Serial.print("Mensaje #" + String(mensajesRecibidos) + " Recibido -> ");
        Serial.print("ID: "); Serial.print(paqueteRecibido.id);
        Serial.print(" V: "); Serial.print(paqueteRecibido.voltaje / 100.0, 2);
        Serial.print(" I: "); Serial.print(paqueteRecibido.corriente / 1000.0, 3);
        Serial.print(" VBat: ");
        Serial.println(paqueteRecibido.vbat / 100.0, 2);

        // Opcional: Enviar una confirmación ("ON") de vuelta
        #if defined(USE_NRF)
          radio->enviar("ON"); // NRF envía sin '\n'
        #else
          radio->enviar("ON\n"); // LoRa/XBee usan '\n'
        #endif
      }
    }
  }
}