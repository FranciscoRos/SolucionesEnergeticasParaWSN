/**
 * ======================================================================
 * ==        SKETCH RECEPTOR UNIVERSAL (XBee + LoRa Preparado)         ==
 * ======================================================================
 * Este sketch es la versión básica de un receptor, compatible tanto con
 * XBee como con LoRa, gracias a la librería UniversalRadioWSN.
 *
 * Para cambiar de módulo, solo comenta una línea y descomenta la otra
 * en la sección "SELECCIÓN DEL MÓDULO DE RADIO".
 */

// --- 1. LIBRERÍAS ---
#include <SPI.h> // Incluimos SPI porque es necesario para LoRa
#include "UniversalRadioWSN.h" // Nuestra librería principal

// --- 2. SELECCIÓN DEL MÓDULO DE RADIO ---
#define USE_XBEE // <-- MODO ACTUAL
//#define USE_LORA // <-- Descomenta esta línea para usar LoRa

// --- 3. CONFIGURACIÓN DE PINES ---
// Pines para XBee (usando Serial2 en ESP32)
#define RXD2 16
#define TXD2 17

// --- 4. OBJETOS GLOBALES ---
RadioInterface* radio; // Puntero a la interfaz. No le importa si es XBee o LoRa.

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
    configLora.resetPin        = -1;   // ¡IMPORTANTE! No especificaste un pin de RESET. 
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
}

// ======================= LOOP =======================
// ¡OBSERVA! El loop no necesita ningún cambio.
// Funciona exactamente igual para XBee y para LoRa.
void loop() {
  if (radio->hayDatosDisponibles()) {
    
    String datosRecibidos = radio->leerComoString();
    datosRecibidos.trim();

    if (datosRecibidos.length() > 0) {
      Serial.print("Línea recibida: --> ");
      Serial.println(datosRecibidos);

      // Usamos la interfaz para enviar una respuesta
      radio->enviar("ON\n");
    }
  }
}