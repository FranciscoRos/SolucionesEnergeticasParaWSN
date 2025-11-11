/*
 * ==========================================================
 * ==  SKETCH RECEPTOR UNIVERSAL (LoRa + XBee + NRF24L01)  ==
 * ==========================================================
 * Este sketch está configurado para usar (LoRa + XBee + NRF24L01)
 * en un ESP32 para recibir comunicación.
 */

// --- LIBRERÍAS ---
#include <SPI.h>
#include <UniversalRadioWSN.h>
#include <EEPROM.h>


// --- SELECCIÓN DEL MÓDULO DE RADIO ---
//#define USE_XBEE
//#define USE_LORA
#define USE_NRF

// ======================= CONFIGURACIÓN Y VARIABLES GLOBALES =======================

// --- OBJETOS Y VARIABLES GLOBALES ---
RadioInterface* radio;
String bufferReceptor = ""; // Buffer para acumular datos recibidos
uint32_t mensajesRecibidos = 0;

#define EEPROM_SIZE 32
#define EEPROM_ADDR 0

#if defined(USE_NRF)
  const byte nrfReadAddress[6] = "00001";
  const byte nrfWriteAddress[6] = "00002";
#elif defined(USE_XBEE)
  #define RXD2 16
  #define TXD2 17
#endif
// --- SETUP ---
void setup() {  
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n--- INICIANDO RECEPTOR UNIVERSAL ADAPTIVE ---");

  Serial.print("Configurando radio: ");
  #if defined(USE_XBEE)
    Serial.println("XBee");
    // El puerto Serial2 del ESP32 se comunica con el XBee a 9600
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
    configNrf.readAddress = nrfReadAddress;
    configNrf.channel = 108;       
    
    configNrf.dataRate = 250; // Data Rate 250KBPS
    configNrf.paLevel = 0;    // Potencia mínima
    
    radio = new NrfRadio(configNrf);
  #endif

  if (!radio->iniciar()) {
    Serial.println("¡ERROR: Fallo al iniciar el módulo de radio!");
    while (true);
  }
  Serial.println("Módulo de radio inicializado y escuchando.");

  // --- LECTURA INICIAL DE LA EEPROM ---
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(EEPROM_ADDR, mensajesRecibidos);       
  Serial.print("Contador de mensajes recibidos recuperado de EEPROM: ");
  Serial.println(mensajesRecibidos);
}

// --- LOOP ---
void loop() {

#if defined(USE_NRF)
  if (radio->hayDatosDisponibles()) {
    String lineaCompleta = radio->leerComoString();
    lineaCompleta.trim();

    if(lineaCompleta.length() >0) {
      mensajesRecibidos++;
      Serial.print("Paquete Recibido #" + String(mensajesRecibidos) + "> ");
      Serial.println(lineaCompleta);

      EEPROM.put(EEPROM_ADDR, mensajesRecibidos);
      EEPROM.commit();

      radio->enviar("ON");
    }
  }
#else
  if (radio->hayDatosDisponibles()) {
    bufferReceptor += radio->leerComoString();
  }

  int indiceFinDeLinea = bufferReceptor.indexOf('\n');

  if(indiceFinDeLinea >= 0) {
    String lineaCompleta = bufferReceptor.substring(0, indiceFinDeLinea);
    bufferReceptor = bufferReceptor.substring(indiceFinDeLinea + 1);
    lineaCompleta.trim();

    if (lineaCompleta.length() > 0) {
      mensajesRecibidos++;
      Serial.print("Paquete Recibido #" + String(mensajesRecibidos) + "> ");
      Serial.println(lineaCompleta);

      // --- GUARDADO EN EEPROM ---
      EEPROM.put(EEPROM_ADDR, mensajesRecibidos);
      EEPROM.commit();

      radio->enviar("ON\n");
    }
  }
#endif
}