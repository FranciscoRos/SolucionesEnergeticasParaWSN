/**
 * ======================================================================
 * ==      SKETCH RECEPTOR UNIVERSAL (MODO BINARIO con CodecWSN)       ==
 * ======================================================================
 * Este sketch usa el Parser de CodecWSN para decodificar frames binarios
 * robustos recibidos por LoRa o XBee.
 */

// --- 1. LIBRERÍAS ---
#include <SPI.h>
#include "UniversalRadioWSN.h"
#include <CodecWSN.h> // <-- ¡LIBRERÍA PARA DECODIFICACIÓN BINARIA!
#include <EEPROM.h>   // <-- AÑADIDO: Librería para memoria no volátil

// --- 2. SELECCIÓN DEL MÓDULO DE RADIO ---
//#define USE_XBEE // <-- MODO ACTUAL
#define USE_LORA // <-- Descomenta esta línea para usar LoRa

// --- 3. CONFIGURACIÓN DE PINES ---
// Pines para XBee (usando Serial2 en ESP32)
#define RXD2 16
#define TXD2 17

// --- 4. OBJETOS GLOBALES ---
RadioInterface* radio;     // Puntero a la interfaz.
WSNFrame::Parser miParser; // <-- ¡NUEVO! Objeto para decodificar frames.
uint32_t mensajesRecibidos; // <-- AÑADIDO: Contador de mensajes

// ======================= SETUP =======================
void setup() {
  // Inicia la comunicación con la computadora
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n--- RECEPTOR UNIVERSAL INICIADO (MODO BINARIO) ---");

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
  
  // Llama al método 'iniciar()' del objeto que se haya creado.
  if (!radio->iniciar()) {
    Serial.println("¡ERROR! Fallo al iniciar el módulo de radio.");
    while (true); // Detiene la ejecución
  }
  
  miParser.reset(); // Resetea el parser al inicio
  Serial.println("Módulo de radio inicializado. Esperando datos binarios...");
  
  // --- LECTURA INICIAL DE LA EEPROM ---
  EEPROM.begin(sizeof(mensajesRecibidos)); // <-- AÑADIDO: Prepara la EEPROM
  EEPROM.get(0, mensajesRecibidos);        // <-- AÑADIDO: Lee el contador
  Serial.print("Contador de mensajes recuperado de EEPROM: ");
  Serial.println(mensajesRecibidos);
}

// ======================= LOOP =======================
void loop() {
  // Comprueba si la radio ha recibido CUALQUIER dato binario
  if (radio->hayDatosDisponibles()) {
    
    // 1. Crear un buffer para leer los bytes crudos
    uint8_t bufferRecepcion[64];
    // 2. Leer los bytes crudos de la radio
    size_t bytesLeidos = radio->leer(bufferRecepcion, 64);
    // 3. Alimentar cada byte, uno por uno, al parser
    for (size_t i = 0; i < bytesLeidos; i++) {
      Packet paqueteRecibido;
      // La función 'feed' devuelve 'true' solo cuando ha recibido
      // un frame completo y su CRC es correcto.
      if (WSNFrame::feed(miParser, bufferRecepcion[i], paqueteRecibido)) {
        
        mensajesRecibidos++; // <-- AÑADIDO: Incrementa el contador

        // --- GUARDADO EN EEPROM ---
        EEPROM.put(0, mensajesRecibidos); // <-- AÑADIDO: Guarda el nuevo valor
        EEPROM.commit();                  // <-- AÑADIDO: Asegura la escritura en ESP32

        // --- IMPRESIÓN MODIFICADA ---
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