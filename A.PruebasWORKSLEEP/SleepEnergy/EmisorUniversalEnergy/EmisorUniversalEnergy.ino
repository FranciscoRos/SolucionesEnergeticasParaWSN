/*
 * ==========================================================
 * ==    SKETCH EMISOR UNIVERSAL (LoRa + XBee + NRF24)     ==
 * ==             MODO SLEEP PROFUNDO (LOW)              ==
 * ==========================================================
 * Este sketch usa UniversalRadioWSN para enviar datos (String)
 * y EnergyWSN para implementar ciclos de sueño (sleep).
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>
#include <UniversalRadioWSN.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "EnergyWSN.h"

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
//#define USE_LORA
//#define USE_XBEE 
#define USE_NRF 

// ======================= 2. CONFIGURACIÓN GENERAL DE PINES =======================
#define RELAY_PIN         4
#define SENSOR_POWER_PIN  7     // Pin que alimenta a los sensores
#define ACS_PIN           A0
#define ZMPT_PIN          A1
#define VBAT_PIN          A2

// --- Pines específicos para XBee (Sleep) ---
#if defined(USE_XBEE)
  #define XBEE_RX_PIN       2
  #define XBEE_TX_PIN       3
  #define XBEE_SLEEP_RQ_PIN 9
  #define XBEE_ON_SLEEP_PIN 10
#endif

// ======================= 3. OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;
EnergyWSN energyManager;
// uint32_t paquetesEnviados; // No se usa
// const unsigned long SLEEP_INTERVAL_MS = ( 1); // No se usa

// --- Configuración específica por radio ---
#if defined(USE_XBEE)
  SoftwareSerial xbeeSerial(XBEE_RX_PIN, XBEE_TX_PIN);
#elif defined(USE_NRF)
  // Direcciones para NRF24L01
  const byte nrfWriteAddress[6] = "00001"; // Dirección del receptor
  const byte nrfReadAddress[6] = "00002";  // Dirección de este emisor
#endif

// ======================= 4. SETUP =======================
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  while(!Serial);
  Serial.println("\n--- INICIANDO EMISOR (MODO SLEEP PROFUNDO) ---");

  // --- Recuperar contador desde la EEPROM ---
  // EEPROM.get(2, paquetesEnviados); // <-- COMENTADO
  // Serial.print("Contador recuperado de EEPROM: ");
  // Serial.println(paquetesEnviados);

  // --- INICIALIZACIÓN DEL MÓDULO DE RADIO ---
  Serial.print("Configurando radio: ");
  #if defined(USE_LORA)
    Serial.println("LoRa");

    LoRaConfig configLora;
    configLora.frequency       = 410E6;
    configLora.spreadingFactor = 7;
    configLora.signalBandwidth = 125E3;
    configLora.codingRate      = 5;
    configLora.syncWord        = 0xF3;
    configLora.txPower         = 20;
    configLora.csPin           = 10;
    configLora.resetPin        = -1;
    configLora.irqPin          = 2;

    radio = new LoraRadio(configLora);
  #elif defined(USE_XBEE)
    Serial.println("XBee");
    xbeeSerial.begin(9600);
    radio = new XBeeRadio(xbeeSerial, 9600, XBEE_SLEEP_RQ_PIN, XBEE_ON_SLEEP_PIN);
  #elif defined(USE_NRF)
    Serial.println("NRF24L01");
    
    NrfConfig configNrf;
    configNrf.cePin = 9;
    configNrf.csnPin = 10; 
    configNrf.writeAddress = nrfWriteAddress;
    configNrf.readAddress = nrfReadAddress;
    configNrf.channel = 108;
    configNrf.dataRate = 250; // 250KBPS
    configNrf.paLevel = 0;    // Potencia mínima
    
    radio = new NrfRadio(configNrf);
  #endif
  
  if (!radio->iniciar()) {
    Serial.println("¡¡¡ERROR: Fallo al iniciar el módulo de radio!!!");
    while (true);
  }
  Serial.println("Módulo de radio inicializado y listo.");

  // --- Configuración del gestor de energía ---
  EnergyWSN::Cfg energyConfig;
  energyConfig.pins.pwrSens = SENSOR_POWER_PIN;
  energyConfig.pins.vbatSense = VBAT_PIN;
  
  // --- INICIO DE LA MODIFICACIÓN ---
  energyConfig.bootSleep = true; // 1. Arrancar dormido
  energyConfig.invertPwr = true; // 2. Usar lógica invertida (HIGH para apagar)
  // --- FIN DE LA MODIFICACIÓN ---
  
  energyManager.begin(energyConfig, radio);
  Serial.println("Gestor de energía inicializado.");
  Serial.println("Sensores y radio están dormidos.");
  Serial.println("Entrando en modo de sueño profundo...");
  delay(100); // Da tiempo al Serial Monitor para imprimir
}

// ======================= 5. LOOP (CICLO DE SUEÑO INFINITO) =======================
void loop() {
  // El MCU se despierta, no hace nada, y se vuelve a dormir.
  // Usamos un número grande; la librería lo dividirá en ciclos de 8s.
  energyManager.sleepFor_ms(1000000);
}

// ======================= 6. FUNCIONES DE LECTURA DE SENSORES =======================
// (No se usan, pero no estorban)
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