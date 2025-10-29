/*
 * ======================================================================
 * ==      SKETCH RECEPTOR UNIVERSAL (MODO BINARIO con CodecWSN ) PARA CON ADAPTIVE     ==
 * ======================================================================
 * Este sketch usa el Parser de CodecWSN para decodificar frames binarios
 * robustos recibidos por LoRa o XBee.
 */

// --- LIBRERÍAS ---
#include <SPI.h>
#include <UniversalRadioWSN.h>
#include <CodecWSN.h> 
#include <EEPROM.h>   

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
//#define USE_XBEE 
#define USE_LORA 
//#define USE_NRF

// ======================= CONFIGURACIÓN Y VARIABLES GLOBALES =======================
// Pines para XBee 
#define RXD2 16
#define TXD2 17

RadioInterface* radio;     // Puntero a la interfaz.
WSNFrame::Parser miParser; // Objeto para decodificar frames.
uint32_t mensajesRecibidos;

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
  Serial.println("\n--- RECEPTOR UNIVERSAL INICIADO (MODO BINARIO) PARA CON ADAPTIVE  ---");

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
  EEPROM.get(4, mensajesRecibidos);
  Serial.print("Contador de mensajes recuperado de EEPROM: ");
  Serial.println(mensajesRecibidos);
}

// ======================= LOOP =======================
void loop() {
  if (radio->hayDatosDisponibles()) {
    
    // 1. Leer los bytes crudos de la radio
    uint8_t bufferRecepcion[64];
    size_t bytesLeidos = radio->leer(bufferRecepcion, 64);

    // 2. Alimentar cada byte, uno por uno, al parser
    for (size_t i = 0; i < bytesLeidos; i++) {
      Packet paqueteRecibido;
      
      // feed() devuelve true solo cuando ha recibido un frame completo y válido
      if (WSNFrame::feed(miParser, bufferRecepcion[i], paqueteRecibido)) {
        
        mensajesRecibidos++;

        // --- GUARDADO EN EEPROM ---
        EEPROM.put(4, mensajesRecibidos);
        EEPROM.commit(); 

        // --- IMPRESIÓN EN MONITOR SERIAL ---
        Serial.print("Mensaje #" + String(mensajesRecibidos) + " Recibido -> ");
        Serial.print("ID: "); Serial.print(paqueteRecibido.id);
        Serial.print(" V: "); Serial.print(paqueteRecibido.voltaje / 100.0, 2);
        Serial.print(" I: "); Serial.print(paqueteRecibido.corriente / 1000.0, 3);
        Serial.print(" VBat: ");
        Serial.println(paqueteRecibido.vbat / 100.0, 2);

        // Opcional: Enviar una confirmación ("ON") de vuelta
        radio->enviar("ON\n");
      }
    }
  }
}