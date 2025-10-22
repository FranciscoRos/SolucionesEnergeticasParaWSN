/*
 * ==========================================================
 * ==          SKETCH RECEPTOR UNIVERSAL (XBEE)            ==
 * ==========================================================
 * Este sketch está configurado para usar un módulo XBee
 * en un ESP32 a través del puerto Serial2 (RX2=16, TX2=17).
 */

// --- LIBRERÍAS ---
#include <UniversalRadioWSN.h>

// --- SELECCIÓN DEL MÓDULO DE RADIO ---
#define USE_LORA
//#define USE_XBEE

// --- OBJETOS Y VARIABLES GLOBALES ---
RadioInterface* radio;

// --- SETUP ---
void setup() {
  // Usar 115200 para la comunicación con la PC
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n--- INICIANDO RECEPTOR UNIVERSAL (XBEE) ---");

  Serial.print("Configurando radio: ");
  #if defined(USE_XBEE)
    Serial.println("XBee");
    // El puerto Serial2 del ESP32 se comunica con el XBee a 9600
    Serial2.begin(9600);
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

  if (!radio->iniciar()) {
    Serial.println("¡ERROR: Fallo al iniciar el módulo de radio!");
    while (true);
  }
  Serial.println("Módulo de radio inicializado y escuchando.");
}

// --- LOOP ---
void loop() {
  if (radio->hayDatosDisponibles()) {
    String datosRecibidos = radio->leerComoString();
    datosRecibidos.trim();
    Serial.print("Data > "); Serial.println(datosRecibidos);

    if (datosRecibidos.length() > 0) {
      Serial.print("Paquete recibido:");
      Serial.print(" > Contenido: '");
      Serial.print(datosRecibidos);
      Serial.println("'");
    } else {
      Serial.println("Paquete detectado, pero estaba vacío.");
    }
  }
}