/*
 * ======================================================================
 * ==      SKETCH RECEPTOR UNIVERSAL (MODO BINARIO con CodecWSN)       ==
 * ======================================================================
 * Este sketch usa el Parser de CodecWSN para decodificar frames binarios
 * robustos recibidos por LoRa, XBee o NRF.
 *
 * Es compatible con CUALQUIER emisor que envíe paquetes CodecWSN,
 * pero este se usará en el que implementa las 3 librerías
 */

// --- LIBRERÍAS ---
#include <SPI.h>
#include <EEPROM.h>   

#include <UniversalRadioWSN.h>
#include <CodecWSN.h> 

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
#define USE_LORA 
//#define USE_XBEE 
//#define USE_NRF 

// ======================= 2. CONFIGURACIÓN GENERAL DE PINES =======================
#define RXD2 16
#define TXD2 17

// ======================= 3. CONFIGURACIÓN DE EEPROM =======================
#define EEPROM_COUNTER_ADDR 4 

// ======================= 4. OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;     // Puntero a la interfaz.
WSNFrame::Parser miParser; // Objeto para decodificar frames.
uint32_t mensajesRecibidos;

// --- Configuración específica por radio ---
#if defined(USE_NRF)
  const byte nrfReadAddress[6] = "00001"; // Dirección de lectura 
  const byte nrfWriteAddress[6] = "00002"; // Dirección de escritura 
#endif

// ======================= 5. SETUP =======================
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n--- RECEPTOR UNIVERSAL INICIADO (MODO BINARIO)  (ENERGY + ADAPTIVE + CODEC) ---");

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
    configNrf.cePin = 4;  
    configNrf.csnPin = 5;
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
  
  miParser.reset(); 
  Serial.println("Módulo de radio inicializado. Esperando datos binarios...");

  // --- LECTURA INICIAL DE LA EEPROM ---
  EEPROM.begin(sizeof(mensajesRecibidos)); 
  EEPROM.get(EEPROM_COUNTER_ADDR, mensajesRecibidos);
  Serial.print("Contador de mensajes recuperado de EEPROM: ");
  Serial.println(mensajesRecibidos);
}

// ======================= 6. LOOP =======================
void loop() {
  if (radio->hayDatosDisponibles()) {
    
    // 1. Leer los bytes crudos de la radio
    uint8_t bufferRecepcion[64];
    size_t bytesLeidos = radio->leer(bufferRecepcion, 64);

    // 2. Alimentar cada byte, uno por uno, al parser
    for (size_t i = 0; i < bytesLeidos; i++) {
      Packet paqueteRecibido;
      
      if (WSNFrame::feed(miParser, bufferRecepcion[i], paqueteRecibido)) {
        
        mensajesRecibidos++;

        // --- GUARDADO EN EEPROM ---
        EEPROM.put(EEPROM_COUNTER_ADDR, mensajesRecibidos);
        EEPROM.commit(); 
        
        // --- IMPRESIÓN EN MONITOR SERIAL ---
        Serial.print("Mensaje #" + String(mensajesRecibidos) + " Recibido -> ");
        Serial.print("ID: "); Serial.print(paqueteRecibido.id);
        Serial.print(" V: "); Serial.print(paqueteRecibido.voltaje / 100.0, 2);
        Serial.print(" I: "); Serial.print(paqueteRecibido.corriente / 1000.0, 3);
        Serial.print(" VBat: ");
        Serial.println(paqueteRecibido.vbat / 100.0, 2);

        #if defined(USE_NRF)
          radio->enviar("ON"); 
        #else
          radio->enviar("ON\n"); 
        #endif
      }
    }
  }
}