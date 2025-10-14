/*
 * ==========================================================
 * ==   SKETCH EMISOR UNIVERSAL (ACTIVADO PARA LORA SIN SD)  ==
 * ==========================================================
 * Versión simplificada que elimina la funcionalidad de la
 * tarjeta SD para resolver un conflicto de pines en el bus SPI.
 *
 * Tareas que realiza:
 * 1. Lee sensores a un intervalo fijo.
 * 2. Envía los datos en formato de texto a través del radio LoRa.
 * 3. Escucha comandos ("ON"/"OFF") para controlar un relé.
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
// SPI.h sigue siendo necesaria porque el módulo LoRa la utiliza.
#include <SPI.h>

// --- LIBRERÍAS DE LOS COMPONENTES MODULARES ---
#include "RadioInterface.h"
#include "XBeeRadio.h"
#include "LoRaRadio.h"

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
#define USE_LORA


// ======================= 2. CONFIGURACIÓN ESPECIALIZADA POR RADIO =======================
#ifdef USE_XBEE
  // --- Configuración para XBee ---
  #include <SoftwareSerial.h>
  const int PIN_XBEE_RX = 2;
  const int PIN_XBEE_TX = 3;
  const int PIN_XBEE_SLEEP_RQ = 7;
  const int PIN_XBEE_ON_SLEEP = 8;
  SoftwareSerial radioSerial(PIN_XBEE_RX, PIN_XBEE_TX);

#elif defined(USE_LORA)
  // --- Configuración para LoRa ---
  const long LORA_FREQUENCY = 915E6;
  // --- Recordatorio de Pines de Conexión (Arduino Nano -> LoRa) ---
  // D11 -> MOSI
  // D12 -> MISO
  // D13 -> SCK
  // D10 -> NSS / CS (Chip Select)
  // D9  -> RESET
  // D2  -> DIO0
#endif


// ======================= CONFIGURACIÓN GENERAL DE PINES =======================
#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2
// La definición de SD_CS ha sido eliminada.


// ======================= OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio; // Puntero a la interfaz.

unsigned long previousMillis = 0;
const unsigned long INTERVAL_MS = 3000;
uint32_t paquetesEnviados = 0;
// Las variables 'logFile', 'lastFlush' y 'FLUSH_MS' han sido eliminadas.


// ======================= SETUP =======================
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  Serial.println("\n--- INICIANDO NODO SENSOR UNIVERSAL (VERSIÓN SIN SD) ---");

  // --- El bloque de inicialización de la tarjeta SD ha sido eliminado. ---

  // --- INYECCIÓN DE DEPENDENCIA DEL RADIO ---
  Serial.print("Configurando radio: ");
  #ifdef USE_LORA
    Serial.println("LoRa");
    radio = new LoRaRadio(LORA_FREQUENCY);
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

    String loraData = "N:" + String(paquetesEnviados) +
                      " V:" + String(voltage, 2) +
                      " I:" + String(corriente, 2) +
                      " B:" + String(vbat, 2);

    // Envía los datos por el radio
    radio->enviar(loraData + "\n");
    
    // Imprime los datos en el monitor serie para depuración
    Serial.print("Enviado: ");
    Serial.println(loraData);
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

  // --- La tarea 3 (guardar en SD) ha sido eliminada. ---
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

String obtenerFechaHora() {
  // Esta función ahora solo sería útil si quisieras enviar la hora por LoRa.
  unsigned long segundos = millis() / 1000;
  int hh = (segundos / 3600) % 24;
  int mm = (segundos / 60) % 60;
  int ss = segundos % 60;
  char buf[20];
  sprintf(buf, "%02d:%02d:%02d", hh, mm, ss);
  return String(buf);
}
