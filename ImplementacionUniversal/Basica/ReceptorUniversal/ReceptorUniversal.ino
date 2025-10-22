/**
 * ======================================================================
 * ==      SKETCH RECEPTOR UNIVERSAL (XBee + LoRa Preparado)         ==
 * ======================================================================
 * Este sketch es la versión básica de un receptor, compatible tanto con
 * XBee como con LoRa, gracias a la librería UniversalRadioWSN.
 *
 * Para cambiar de módulo, solo comenta una línea y descomenta la otra
 * en la sección "SELECCIÓN DEL MÓDULO DE RADIO".
 *
 * MODIFICACIÓN: Guarda el contador de paquetes recibidos en la EEPROM.
 * CORRECCIÓN: Se implementó un buffer para ensamblar mensajes fragmentados
 * y procesarlos solo cuando están completos.
 */

// --- 1. LIBRERÍAS ---
#include <SPI.h>              // Incluimos SPI porque es necesario para LoRa
#include "UniversalRadioWSN.h" // Nuestra librería principal
#include <EEPROM.h>           // <-- AÑADIDO: Librería para la memoria no volátil

// --- 2. SELECCIÓN DEL MÓDULO DE RADIO ---
#define USE_XBEE // <-- MODO ACTUAL
//#define USE_LORA // <-- Descomenta esta línea para usar LoRa

// --- 3. CONFIGURACIÓN DE PINES ---
// Pines para XBee (usando Serial2 en ESP32)
#define RXD2 16
#define TXD2 17

// --- 4. OBJETOS GLOBALES ---
RadioInterface* radio;   // Puntero a la interfaz. No le importa si es XBee o LoRa.
uint32_t mensajesRecibidos; // Contador de mensajes recibidos

// ***** INICIO DE LA CORRECCIÓN *****
// Buffer para acumular los datos que llegan del radio.
// Esto soluciona el problema de recibir mensajes fragmentados.
String bufferReceptor = "";
// ***** FIN DE LA CORRECCIÓN *****

// ======================= SETUP =======================
void setup() {
  // Inicia la comunicación con la computadora
  Serial.begin(115200);
  while (!Serial);

  Serial.println("\n--- RECEPTOR UNIVERSAL INICIADO ---");

  // --- INICIALIZACIÓN DEL MÓDULO DE RADIO ---
  // El compilador elegirá cuál de estos bloques de código usar
  // basado en la definición (USE_XBEE o USE_LORA) que esté activa.
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
    configLora.csPin           = 5;   // Tu pin NSS va aquí
    configLora.irqPin          = 2;   // Tu pin DIO0 va aquí
    configLora.resetPin        = -1;  // ¡IMPORTANTE! No especificaste un pin de RESET. 
                                      // Usualmente es el pin 9 o 4. Verifica tu cableado.
    // Creamos el objeto LoraRadio con su configuración
    radio = new LoraRadio(configLora);
  #endif
  
  // Este código es común para AMBOS módulos.
  // Llama al método 'iniciar()' del objeto que se haya creado.
  if (!radio->iniciar()) {
    Serial.println("¡ERROR! Fallo al iniciar el módulo de radio.");
    while (true); // Detiene la ejecución
  }
  
  Serial.println("Módulo de radio inicializado. Esperando datos...");

  // --- LECTURA INICIAL DE LA EEPROM ---
  EEPROM.begin(sizeof(mensajesRecibidos)); // Prepara la EEPROM
  EEPROM.get(0, mensajesRecibidos);        // Lee el contador desde la dirección 0
  Serial.print("Contador de mensajes recibidos recuperado de EEPROM: ");
  Serial.println(mensajesRecibidos);
}

// ======================= LOOP (CORREGIDO) =======================
void loop() {
  // ***** INICIO DE LA CORRECCIÓN *****
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
    //    Esto deja el buffer listo para el siguiente mensaje.
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

      // Usamos la interfaz para enviar una respuesta
      radio->enviar("ON\n");
    }
  }
  // ***** FIN DE LA CORRECCIÓN *****
}