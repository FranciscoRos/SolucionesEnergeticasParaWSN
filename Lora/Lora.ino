/*
 * ==========================================================
 * ==         SKETCH RECEPTOR UNIVERSAL (VERSIÓN FINAL)     ==
 * ==========================================================
 * Este sketch es un receptor genérico capaz de usar diferentes
 * módulos de radio (LoRa, XBee, etc.) cambiando una sola
 * línea de código en la sección de selección de radio.
 * * Utiliza un objeto de configuración que se inyecta en la
 * clase del radio para máxima flexibilidad y limpieza del código.
 *
 * Tareas que realiza:
 * 1. Inicializa el radio seleccionado con su configuración.
 * 2. Escucha por paquetes entrantes y los muestra en el monitor
 * serie junto a su intensidad de señal (RSSI).
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>

// --- LIBRERÍAS DE LOS COMPONENTES MODULARES ---
// Estos archivos .h deben estar en la misma carpeta que este sketch
#include "RadioInterface.h"
#include "LoraRadio.h"
// #include "XbeeRadio.h" // Se añadiría para soportar XBee

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
#define USE_LORA
// #define USE_XBEE

// ======================= OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio; // Puntero genérico a la interfaz del radio.

// ======================= SETUP =======================
void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("\n--- INICIANDO RECEPTOR UNIVERSAL (VERSIÓN FINAL) ---");

  // --- INYECCIÓN DE DEPENDENCIA DEL RADIO ---
  Serial.print("Configurando radio: ");

  #if defined(USE_LORA)
    Serial.println("LoRa");

    // 1. Crear el objeto de configuración para el RECEPTOR LoRa (ESP32)
    LoRaConfig configLora;
    // Parámetros de comunicación (¡DEBEN COINCIDIR EXACTAMENTE CON EL EMISOR!)
    configLora.frequency        = 915E6;
    configLora.spreadingFactor  = 7;
    configLora.signalBandwidth  = 125E3;
    configLora.codingRate       = 5;
    configLora.syncWord         = 0xF3;
    configLora.txPower          = 20; // No se usa para recibir, pero se define por consistencia
    // Pines específicos del ESP32
    configLora.csPin            = 5;
    configLora.resetPin         = 14;
    configLora.irqPin           = 2;

    // 2. Crear la instancia del radio LoRa
    radio = new LoraRadio(configLora);

  #elif defined(USE_XBEE)
    Serial.println("XBee");
    // Aquí iría la configuración para XBee.
    /*
    XBeeConfig configXBee;
    configXBee.rxPin = 16;
    configXBee.txPin = 17;
    ...
    radio = new XbeeRadio(configXBee);
    */
  #endif

  if (!radio->iniciar()) {
    Serial.println("¡¡¡ERROR: Fallo al iniciar el módulo de radio!!!");
    while (true);
  }
  Serial.println("Módulo de radio inicializado y escuchando.");
}

// ======================= LOOP =======================
void loop() {
  // El loop es ahora completamente genérico.
  // Funciona igual sin importar qué radio esté definido arriba.
  if (radio->hayDatosDisponibles()) {

    String datosRecibidos = radio->leerComoString();
    int rssi = radio->obtenerRSSI(); // Usamos la función de la interfaz

    Serial.print("Paquete recibido con RSSI ");
    Serial.print(rssi);
    Serial.println(" dBm:");
    Serial.print(" > Contenido: '");
    Serial.print(datosRecibidos);
    Serial.println("'");
  }
}
