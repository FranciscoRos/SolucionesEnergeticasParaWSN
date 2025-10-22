/**
 * ======================================================================
 * ==     SKETCH RECEPTOR UNIVERSAL (XBee + LoRa + NRF24)            ==
 * ======================================================================
 * Este sketch es la versión básica de un receptor, compatible con
 * todos los radios definidos en UniversalRadioWSN.
 *
 * Para cambiar de módulo, solo comenta una línea y descomenta la otra
 * en la sección "SELECCIÓN DEL MÓDULO DE RADIO".
 *
 * MODIFICACIÓN: Guarda el contador de paquetes recibidos en la EEPROM.
 * CORRECCIÓN: Se implementó un buffer para XBee/LoRa.
 * NUEVO: Añadida lógica para NRF24L01 (modo paquete).
 */

// --- 1. LIBRERÍAS ---
#include <SPI.h>              // Incluimos SPI porque es necesario para LoRa y NRF
#include "UniversalRadioWSN.h" // Nuestra librería principal
#include <EEPROM.h>           // <-- AÑADIDO: Librería para la memoria no volátil
// #include <RF24.h>          // <-- ELIMINADO: Ya no es necesario aquí

// --- 2. SELECCIÓN DEL MÓDULO DE RADIO ---
//#define USE_XBEE 
#define USE_LORA 
//#define USE_NRF // <-- MÓDULO NRF24L01 ACTIVADO

// 
// --- 3. CONFIGURACIÓN DE PINES Y DIRECCIONES ---
// Pines para XBee (usando Serial2 en ESP32)
#define RXD2 16
#define TXD2 17

#if defined(USE_NRF)
  // Direcciones para NRF24L01 (basadas en tus .ino)
  const byte nrfReadAddress[6] = "00001";
// Dirección principal para RECIBIR datos [cite: 2, 8, 24]
  const byte nrfWriteAddress[6] = "00002";
// Dirección para ENVIAR respuestas (al Emisor 1)
#endif

// --- 4. OBJETOS GLOBALES ---
RadioInterface* radio;    // Puntero a la interfaz.
uint32_t mensajesRecibidos; // Contador de mensajes recibidos

// Buffer para acumular los datos que llegan del radio (SOLO para LoRa/XBee)
String bufferReceptor = "";

// ======================= SETUP =======================
void setup() {
  // Inicia la comunicación con la computadora
  Serial.begin(115200);
  while (!Serial);

  Serial.println("\n--- RECEPTOR UNIVERSAL INICIADO ---");

  // --- INICIALIZACIÓN DEL MÓDULO DE RADIO ---
  Serial.print("Configurando radio: ");
  
  #if defined(USE_XBEE)
    Serial.println("XBee");
    // Inicia el puerto físico que usará el XBee
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
// Crea el objeto XBeeRadio y lo asigna a nuestra interfaz
    radio = new XBeeRadio(Serial2, 9600, -1, -1);
  
  #elif defined(USE_LORA)
    Serial.println("LoRa");
    SPI.begin(); // LoRa necesita que el bus SPI esté activo

    // Creamos la estructura de configuración para LoRa
    LoRaConfig configLora;
    configLora.frequency       = 410E6;
    configLora.spreadingFactor = 7;
    configLora.signalBandwidth = 125E3;
    configLora.codingRate      = 5;
    configLora.syncWord        = 0xF3;
    configLora.txPower         = 20; 
    
    // --- PINES ACTUALIZADOS SEGÚN TU HARDWARE ---
    configLora.csPin           = 5;
// Tu pin NSS va aquí
    configLora.irqPin          = 2;
// Tu pin DIO0 va aquí
    configLora.resetPin        = -1;
    
    // Creamos el objeto LoraRadio con su configuración
    radio = new LoraRadio(configLora);
    
  #elif defined(USE_NRF)
    Serial.println("NRF24L01");
    
    NrfConfig configNrf;
    // Pines para ESP32 (basado en CoordinadorNRF.ino)
    configNrf.cePin = 4; 
    configNrf.csnPin = 5;
    configNrf.writeAddress = nrfWriteAddress; // Para responder "ON"
    configNrf.readAddress = nrfReadAddress;
// Para recibir datos
    configNrf.channel = 108;           
    
    // --- VALORES GENÉRICOS ---
    configNrf.dataRate = 250; // 250KBPS
    configNrf.paLevel = 0;    // 0 = RF24_PA_MIN
    
    radio = new NrfRadio(configNrf);
    
  #endif
  
  // Este código es común para TODOS los módulos.
  if (!radio->iniciar()) {
    Serial.println("¡ERROR! Fallo al iniciar el módulo de radio.");
    while (true);
// Detiene la ejecución
  }
  
  Serial.println("Módulo de radio inicializado. Esperando datos...");

  // --- LECTURA INICIAL DE LA EEPROM ---
  // Asumiendo ESP32 (basado en el uso de EEPROM.commit y Serial2)
  EEPROM.begin(sizeof(mensajesRecibidos));
// Prepara la EEPROM
  EEPROM.get(1, mensajesRecibidos);        // Lee el contador desde la dirección 0
  Serial.print("Contador de mensajes recibidos recuperado de EEPROM: ");
  Serial.println(mensajesRecibidos);
}

// ======================= LOOP (ADAPTADO) =======================
void loop() {

#if defined(USE_NRF)
  // --- LÓGICA PARA NRF24L01 (Paquetes discretos) ---
  // NRF no usa streaming ni '\n'.
// Cada lectura es un paquete completo.
  if (radio->hayDatosDisponibles()) {
    String lineaCompleta = radio->leerComoString();
    lineaCompleta.trim();
// Quita espacios/nulos extra

    if (lineaCompleta.length() > 0) {
      mensajesRecibidos++;

      Serial.print("Paquete recibido #" + String(mensajesRecibidos) + ": --> ");
      Serial.println(lineaCompleta);

      // --- GUARDADO EN EEPROM ---
      EEPROM.put(1, mensajesRecibidos);
      EEPROM.commit();
// Guardar en Flash (para ESP32)

      // Usamos la interfaz para enviar una respuesta
      // (El emisor NRF debe estar escuchando en el pipe de respuesta)
      radio->enviar("ON");
// Envía "ON" (sin \n)
    }
  }

#else
  // --- LÓGICA ORIGINAL PARA LoRa y XBee (Streaming con '\n') ---
  // ***** INICIO DE LA CORRECCIÓN ORIGINAL *****
  // Si hay datos disponibles, los leemos y los añadimos a nuestro buffer.
  if (radio->hayDatosDisponibles()) {
    bufferReceptor += radio->leerComoString();
  }

  // Buscamos si en nuestro buffer ya existe un mensaje completo (terminado en '\n').
  int indiceFinDeLinea = bufferReceptor.indexOf('\n');

  // Si se encontró un fin de línea (índice >= 0), procesamos el mensaje.
  if (indiceFinDeLinea >= 0) {
    // 1. Extraemos la línea completa del buffer.
    String lineaCompleta = bufferReceptor.substring(0, indiceFinDeLinea);
    
    // 2. Eliminamos la línea ya procesada (y el carácter '\n') del buffer.
    bufferReceptor = bufferReceptor.substring(indiceFinDeLinea + 1);

    // 3. Procesamos la línea que extrajimos.
    lineaCompleta.trim();
    if (lineaCompleta.length() > 0) {
      mensajesRecibidos++;

      Serial.print("Línea recibida #" + String(mensajesRecibidos) + ": --> ");
      Serial.println(lineaCompleta);

      // --- GUARDADO EN EEPROM ---
      EEPROM.put(0, mensajesRecibidos);
      EEPROM.commit();
// Guardar en Flash (para ESP32)

      // Usamos la interfaz para enviar una respuesta
      radio->enviar("ON\n");
// Envía "ON" (con \n)
    }
  }
  // ***** FIN DE LA CORRECCIÓN ORIGINAL *****
#endif
}