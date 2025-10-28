/*
 * ======================================================================
 * ==      SKETCH RECEPTOR UNIVERSAL ENERGY(XBee + LoRa + NRF24)            ==
 * ======================================================================
 * Este sketch es la versión básica de un receptor, compatible con
 * todos los radios definidos en UniversalRadioWSN, se usará para utilizar en el Energy.
 *
 */

// --- LIBRERÍAS ---
#include <SPI.h>
#include <UniversalRadioWSN.h> 
#include <EEPROM.h>            

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
//#define USE_XBEE 
#define USE_LORA 
//#define USE_NRF 

// ======================= CONFIGURACIÓN Y VARIABLES GLOBALES ======================= 
#define RXD2 16
#define TXD2 17

RadioInterface* radio;      // Puntero a la interfaz.
uint32_t mensajesRecibidos; // Contador de mensajes recibidos
String bufferReceptor = ""; // Buffer para acumular datos 

// --- Configuración específica por radio ---
#if defined(USE_NRF)
  // Direcciones para NRF24L01
  const byte nrfReadAddress[6] = "00001"; // Dirección de lectura
  const byte nrfWriteAddress[6] = "00002"; // Dirección de escritura (respuesta)
#endif

// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("\n--- RECEPTOR UNIVERSAL ENERGY INICIADO ---");

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
    Serial.println("¡ERROR! Fallo al iniciar el módulo de radio.");
    while (true);
  }
  
  Serial.println("Módulo de radio inicializado. Esperando datos...");

  // --- LECTURA INICIAL DE LA EEPROM ---
  EEPROM.begin(sizeof(mensajesRecibidos));
  EEPROM.get(3, mensajesRecibidos);        
  Serial.print("Contador de mensajes recibidos recuperado de EEPROM: ");
  Serial.println(mensajesRecibidos);
}

// ======================= LOOP =======================
void loop() {

#if defined(USE_NRF)
  // --- LÓGICA PARA NRF24L01  ---
  if (radio->hayDatosDisponibles()) {
    String lineaCompleta = radio->leerComoString();
    lineaCompleta.trim();

    if (lineaCompleta.length() > 0) {
      mensajesRecibidos++;

      Serial.print("Paquete recibido #" + String(mensajesRecibidos) + ": --> ");
      Serial.println(lineaCompleta);

      // --- GUARDADO EN EEPROM ---
      EEPROM.get(3,  mensajesRecibidos);
      EEPROM.commit();

      radio->enviar("ON");
    }
  }

#else
  // --- LÓGICA PARA LoRa y XBee (Streaming con '\n') ---
  if (radio->hayDatosDisponibles()) {
    bufferReceptor += radio->leerComoString();
  }

  // Busca un mensaje completo  
  int indiceFinDeLinea = bufferReceptor.indexOf('\n');

  if (indiceFinDeLinea >= 0) {
    String lineaCompleta = bufferReceptor.substring(0, indiceFinDeLinea);
    bufferReceptor = bufferReceptor.substring(indiceFinDeLinea + 1);

    lineaCompleta.trim();
    if (lineaCompleta.length() > 0) {
      mensajesRecibidos++;

      Serial.print("Línea recibida #" + String(mensajesRecibidos) + ": --> ");
      Serial.println(lineaCompleta);

      // --- GUARDADO EN EEPROM --- 
      EEPROM.put(6, mensajesRecibidos);
      EEPROM.commit();

      radio->enviar("ON\n");
    }
  }
#endif
}