/*
 * ==========================================================
 * ==           SKETCH EMISOR UNIVERSAL (VERSIÓN FINAL)     ==
 * ==========================================================
 * Este sketch es un emisor genérico capaz de usar diferentes 
 * módulos de radio (LoRa, XBee, etc.) cambiando una sola 
 * línea de código en la sección de selección de radio.
 * * Utiliza un objeto de configuración que se inyecta en la
 * clase del radio para máxima flexibilidad y limpieza del código.
 *
 * Tareas que realiza:
 * 1. Lee sensores a un intervalo fijo.
 * 2. Envía los datos a través del radio seleccionado.
 * 3. Escucha por comandos entrantes para controlar un relé.
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>

// --- LIBRERÍAS DE LOS COMPONENTES MODULARES ---
// Estos archivos .h deben estar en la misma carpeta que este sketch
#include "RadioInterface.h"
#include "LoRaRadio.h"
// #include "XBeeRadio.h" // Se añadiría para soportar XBee

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
// Para cambiar de radio, comenta una línea y descomenta la otra.
#define USE_LORA
// #define USE_XBEE

// ======================= CONFIGURACIÓN GENERAL DE PINES =======================
#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2

// ======================= OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio; // Puntero genérico a la interfaz del radio.

unsigned long previousMillis = 0;
const unsigned long INTERVAL_MS = 3000;
uint32_t paquetesEnviados = 0;

// ======================= SETUP =======================
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  while(!Serial);
  Serial.println("\n--- INICIANDO EMISOR UNIVERSAL (VERSIÓN FINAL) ---");

  // --- INYECCIÓN DE DEPENDENCIA DEL RADIO ---
  Serial.print("Configurando radio: ");
  
  #if defined(USE_LORA)
    Serial.println("LoRa");

    // 1. Crear el objeto de configuración para el EMISOR LoRa (Arduino Nano)
    LoRaConfig configLora;
    configLora.frequency        = 410E6;
    configLora.txPower          = 20;
    configLora.spreadingFactor  = 7;
    configLora.signalBandwidth  = 125E3;
    configLora.codingRate       = 5;
    configLora.syncWord         = 0xF3;
    // Pines específicos del Arduino Nano
    configLora.csPin            = 10;
    configLora.resetPin         = 9;
    configLora.irqPin           = 2;
    
    // 2. Crear la instancia del radio pasándole el objeto de configuración
    radio = new LoRaRadio(configLora); 
  
  #elif defined(USE_XBEE)
    Serial.println("XBee");
    // Aquí iría la configuración para XBee.
    // Se necesitaría una estructura XBeeConfig y la clase XBeeRadio.
    /*
    XBeeConfig configXBee;
    configXBee.rxPin = 2;
    configXBee.txPin = 3;
    ...
    radio = new XBeeRadio(configXBee);
    */
  #endif
  
  if (!radio->iniciar()) {
    Serial.println("¡¡¡ERROR: Fallo al iniciar el módulo de radio!!!");
    while (true);
  }
  Serial.println("Módulo de radio inicializado y listo.");
}

// ======================= LOOP =======================
void loop() {
  unsigned long currentMillis = millis();

  // --- Tarea 1: Enviar datos a un intervalo fijo ---
  if (currentMillis - previousMillis >= INTERVAL_MS) {
    previousMillis = currentMillis;

    float voltage = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();
    float vbat = leerVoltajeBateria();
    paquetesEnviados++;

    String dataPayload = "N:" + String(paquetesEnviados) +
                         " V:" + String(voltage, 2) +
                         " I:" + String(corriente, 2) +
                         " B:" + String(vbat, 2);

    radio->enviar(dataPayload);
    
    Serial.print("Enviado: ");
    Serial.println(dataPayload);
  }

  // --- Tarea 2: Escuchar comandos entrantes ---
  if (radio->hayDatosDisponibles()) {
    String comando = radio->leerComoString();
    comando.trim();

    Serial.print("Comando recibido: ");
    Serial.println(comando);

    if (comando == "ON") {
      digitalWrite(RELAY_PIN, HIGH);
    } else if (comando == "OFF") {
      digitalWrite(RELAY_PIN, LOW);
    }
  }
}

// ======================= FUNCIONES DE LECTURA DE SENSORES =======================
float leerVoltajeZMPT() {
  int lectura = analogRead(ZMPT_PIN);
  return ((lectura * 5.0) / 1023.0) * 50.0;
}

float leerCorrienteACS() {
  int lectura = analogRead(ACS_PIN);
  float volt = (lectura * 5.0) / 1023.0;
  return (volt - 2.5) / 0.066;
}

float leerVoltajeBateria() {
  int lectura = analogRead(VBAT_PIN);
  float vEsc = (lectura * 5.0) / 1023.0;
  return vEsc * 3.0;
}
