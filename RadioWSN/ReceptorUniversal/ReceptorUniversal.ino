/*
 * ==========================================================
 * ==         SKETCH RECEPTOR UNIVERSAL (VERSIÓN 1.2)       ==
 * ==========================================================
 * VERSIÓN FINAL Y LIMPIA: Se revierte el loop a su estado
 * original usando la función 'leerComoString()', ahora que
 * se ha confirmado que los archivos .h subyacentes son correctos.
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>

// --- LIBRERÍAS DE LOS COMPONENTES MODULARES ---
#include "RadioInterface.h"
#include "LoraRadio.h"

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
#define USE_LORA

// ======================= OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;

// ======================= SETUP =======================
void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("\n--- INICIANDO RECEPTOR UNIVERSAL (v1.2 - FINAL) ---");

  // --- INYECCIÓN DE DEPENDENCIA DEL RADIO ---
  Serial.print("Configurando radio: ");

  #if defined(USE_LORA)
    Serial.println("LoRa");

    // Crear el objeto de configuración para el RECEPTOR LoRa (ESP32)
    LoRaConfig configLora;
    configLora.frequency        = 410E6; 
    configLora.spreadingFactor  = 7;
    configLora.signalBandwidth  = 125E3;
    configLora.codingRate       = 5;
    configLora.syncWord         = 0xF3;
    configLora.txPower          = 20;
    configLora.csPin            = 5;
    configLora.resetPin         = 14;
    configLora.irqPin           = 2;

    radio = new LoraRadio(configLora);
  #endif

  if (!radio->iniciar()) {
    Serial.println("¡¡¡ERROR: Fallo al iniciar el módulo de radio!!!");
    while (true);
  }
  Serial.println("Módulo de radio inicializado y escuchando.");
}

// ======================= LOOP (RESTAURADO A LA VERSIÓN LIMPIA) =======================
void loop() {
  if (radio->hayDatosDisponibles()) {
    
    // Volvemos a usar la función de conveniencia, ahora que sabemos que funciona.
    String datosRecibidos = radio->leerComoString();
    int rssi = radio->obtenerRSSI();

    Serial.print("Paquete recibido con RSSI ");
    Serial.print(rssi);
    Serial.println(" dBm:");
    Serial.print(" > Contenido: '");
    Serial.print(datosRecibidos);
    Serial.println("'");
  }
}
