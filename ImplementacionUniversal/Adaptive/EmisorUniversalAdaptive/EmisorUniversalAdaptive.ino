/*
 * ==========================================================
 * ==      SKETCH EMISOR ADAPTATIVO (VERSIÓN FINAL)        ==
 * ==========================================================
 * Este sketch integra la librería AdaptiveTXWSN para ajustar
 * la frecuencia de envío de datos según el voltaje de la
 * batería, optimizando el consumo de energía.
 */

// --- LIBRERías DE LA APLICACIÓN ---
#include <SPI.h>
#include <UniversalRadioWSN.h>
#include "AdaptiveTXWSN.h"
// --> CORRECCIÓN 1: SoftwareSerial se incluye aquí, en el ámbito global.
#include <SoftwareSerial.h>

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
// --> CORRECCIÓN 2: Elige SOLO UNO. Comenta la línea que no vayas a usar.
//#define USE_LORA
#define USE_XBEE

// ======================= CONFIGURACIÓN GENERAL DE PINES =======================
#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2

// ======================= OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;
AdaptiveTXWSN adaptiveTimer;

uint32_t paquetesEnviados = 0;

// --> CORRECCIÓN 3: Se declara el objeto SoftwareSerial globalmente para que no se destruya.
//     Solo se usará si USE_XBEE está activo.
#if defined(USE_XBEE)
  SoftwareSerial xbeeSerial(2, 3); // RX Pin = 2, TX Pin = 3
#endif

// ======================= SETUP =======================
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  while(!Serial);
  Serial.println("\n--- INICIANDO EMISOR ADAPTATIVO (VERSIÓN FINAL) ---");

  // Configuración del temporizador adaptativo...
  Serial.println("Configurando temporizador adaptativo...");
  AdaptiveTXWSN::Cfg adaptiveConfig;
  adaptiveConfig.pinAdcBateria = VBAT_PIN;
  adaptiveConfig.voltajeReferenciaAdc = 4.78;
  adaptiveConfig.divisorRArriba_k = 0.1;
  adaptiveConfig.divisorRAbajo_k  = 1.0;
  
  float umbralAlto_V     = 3.9f;
  float umbralMedio_V    = 3.4f;
  float corteVoltaje_V   = 2.0f;
  float histeresis       = 0.05f;
  uint32_t periodoAlto_ms  = 5000;
  uint32_t periodoMedio_ms = 20000;
  uint32_t periodoBajo_ms  = 180000;

  adaptiveTimer.begin(
    adaptiveConfig, umbralAlto_V, umbralMedio_V, corteVoltaje_V, histeresis,
    periodoAlto_ms, periodoMedio_ms, periodoBajo_ms
  );

  // --- INYECCIÓN DE DEPENDENCIA DEL RADIO ---
  Serial.print("Configurando radio: ");
  
  // --> CORRECCIÓN 4: Se usa #elif para asegurar que solo se configure UN radio.
  #if defined(USE_LORA)
    Serial.println("LoRa");
    LoRaConfig configLora;
    configLora.frequency       = 410E6;
    configLora.txPower         = 20;
    configLora.spreadingFactor = 7;
    configLora.signalBandwidth = 125E3;
    configLora.codingRate      = 5;
    configLora.syncWord        = 0xF3;
    configLora.csPin           = 10;
    configLora.resetPin        = 9;
    configLora.irqPin          = 2;
    radio = new LoraRadio(configLora); 
  
  #elif defined(USE_XBEE)
    Serial.println("XBee");
    xbeeSerial.begin(9600);
    radio = new XBeeRadio(xbeeSerial, 9600, -1, -1); // -1 para pines de sleep no usados
  #endif
  
  if (!radio->iniciar()) {
    Serial.println("¡¡¡ERROR: Fallo al iniciar el módulo de radio!!!");
    while (true);
  }
  Serial.println("Módulo de radio inicializado y listo.");
}

// ======================= LOOP =======================
void loop() {
  Serial.println("entramos");
  if (adaptiveTimer.tick()) {
    if (adaptiveTimer.isCutoff()) {
      Serial.println("VOLTAJE DE CORTE ALCANZADO. Transmisión detenida.");
      radio->dormir();
      while(true);
    }

    float voltage = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();
    float vbat = adaptiveTimer.lastVolts(); 
    paquetesEnviados++;

    String dataPayload = "N:" + String(paquetesEnviados) +
                         " V:" + String(voltage, 2) +
                         " I:" + String(corriente, 2) +
                         " B:" + String(vbat, 2);

    radio->enviar(dataPayload);
    
    Serial.print("Enviado (Nivel Bat: " + String(adaptiveTimer.level()) + ", V: " + String(vbat, 2) + "): ");
    Serial.print(dataPayload);
    Serial.println(" | Próximo envío en " + String(adaptiveTimer.currentPeriod() / 1000) + "s");
  }

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