/*
 * =================================================================
 * ==   SKETCH EMISOR UNIVERSAL CON GESTIÓN DE ENERGÍA (HÍBRIDO)   ==
 * =================================================================
 * FUSIONA:
 * 1. La capacidad de cambiar entre LoRa y XBee con una sola línea.
 * 2. La gestión de energía con la librería EnergyWSN para dormir
 * el microcontrolador, la radio y los sensores.
 * 3. El guardado del contador de paquetes en la memoria EEPROM.
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>
#include <UniversalRadioWSN.h>
#include <SoftwareSerial.h>
#include "EnergyWSN.h"
#include <EEPROM.h>

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
// Descomenta la línea del radio que quieres usar y comenta la otra.
#define USE_LORA
//#define USE_XBEE

// ======================= 2. CONFIGURACIÓN GENERAL DE PINES =======================
// --- Pines comunes ---
#define RELAY_PIN         4
#define SENSOR_POWER_PIN  7    // Pin que alimenta a los sensores (ZMPT, ACS).
#define ACS_PIN           A0
#define ZMPT_PIN          A1
#define VBAT_PIN          A2

// --- Pines específicos para LoRa ---
#if defined(USE_LORA)
  #define LORA_CS_PIN     10
  #define LORA_IRQ_PIN    2
  #define LORA_RST_PIN    -1 // -1 si no se usa
#endif

// --- Pines específicos para XBee ---
#if defined(USE_XBEE)
  #define XBEE_RX_PIN       2
  #define XBEE_TX_PIN       3
  #define XBEE_SLEEP_RQ_PIN 9
  #define XBEE_ON_SLEEP_PIN 10
#endif

// ======================= 3. OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;
EnergyWSN energyManager;
uint32_t paquetesEnviados; // Se inicializa desde la EEPROM
const unsigned long SLEEP_INTERVAL_MS = ( 6* 1000); // Tiempo que dormirá el micro

// --- Objetos específicos del radio (se compilan condicionalmente) ---
#if defined(USE_XBEE)
  SoftwareSerial xbeeSerial(XBEE_RX_PIN, XBEE_TX_PIN);
#endif

// ======================= 4. SETUP =======================
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  while(!Serial);
  Serial.println("\n--- INICIANDO EMISOR HÍBRIDO CON GESTIÓN DE ENERGÍA ---");

  // --- Recuperar contador desde la EEPROM ---
  EEPROM.get(1, paquetesEnviados);
  Serial.print("Contador recuperado de EEPROM: ");
  Serial.println(paquetesEnviados);

  // --- INYECCIÓN DE DEPENDENCIA DEL RADIO (Lógica Universal) ---
  Serial.print("Configurando radio: ");
  #if defined(USE_LORA)
    Serial.println("LoRa");
    LoRaConfig configLora;
    configLora.frequency        = 410E6;
    configLora.spreadingFactor  = 7;
    configLora.signalBandwidth  = 125E3;
    configLora.codingRate       = 5;
    configLora.syncWord         = 0xF3;
    configLora.txPower          = 20;
    configLora.csPin            = LORA_CS_PIN;
    configLora.resetPin         = LORA_RST_PIN;
    configLora.irqPin           = LORA_IRQ_PIN;
    radio = new LoraRadio(configLora);
    
  #elif defined(USE_XBEE)
    Serial.println("XBee");
    xbeeSerial.begin(9600);
    radio = new XBeeRadio(xbeeSerial, 9600, XBEE_SLEEP_RQ_PIN, XBEE_ON_SLEEP_PIN);
  #endif
  
  if (!radio->iniciar()) {
    Serial.println("¡¡¡ERROR: Fallo al iniciar el módulo de radio!!!");
    while (true);
  }
  Serial.println("Módulo de radio inicializado.");

  // --- Configuración del gestor de energía ---
  EnergyWSN::Cfg energyConfig;
  energyConfig.pins.pwrSens = SENSOR_POWER_PIN;
  energyConfig.pins.vbatSense = VBAT_PIN;
  energyConfig.bootSleep = false;
  
  energyManager.begin(energyConfig, radio);
  Serial.println("Gestor de energía inicializado.");
}

// ======================= 5. LOOP =======================
void loop() {
  Serial.println("\n---------------------------------");
  Serial.println("Iniciando ciclo de medición y envío.");

  // 1. Despertar la radio
  Serial.print("Despertando radio... ");
  if (energyManager.wakeRadio()) Serial.println("OK."); else Serial.println("FALLO.");

  // 2. Energizar sensores
  energyManager.powerSensors(true);
  Serial.println("Sensores energizados. Esperando estabilización...");
  //delay(200);

  // 3. Leer sensores
  float voltage = leerVoltajeZMPT();
  float corriente = leerCorrienteACS();
  float vbat = leerVoltajeBateria();
  paquetesEnviados++;

  // 4. Construir y enviar payload
  String dataPayload = "N:" + String(paquetesEnviados) +
                       " V:" + String(voltage, 2) +
                       " I:" + String(corriente, 2) +
                       " B:" + String(vbat, 2);

  radio->enviar(dataPayload + "\n");
  Serial.print("Paquete enviado: ");
  Serial.println(dataPayload);
  //delay(2000);

  // Guardar el contador en la EEPROM
  EEPROM.put(1, paquetesEnviados);

  // 5. Escuchar por comandos un corto tiempo
  Serial.print("Escuchando comandos por 500ms... ");
  long tiempoInicioEscucha = millis();
  bool comandoRecibido = false;
  while (millis() - tiempoInicioEscucha < 500) {
    if (radio->hayDatosDisponibles()) {
      String comando = radio->leerComoString();
      comando.trim();
      Serial.print("\nComando recibido: '"); Serial.print(comando); Serial.println("'");
      if (comando == "ON") {
        digitalWrite(RELAY_PIN, HIGH);
        comandoRecibido = true;
      } else if (comando == "OFF") {
        digitalWrite(RELAY_PIN, LOW);
        comandoRecibido = true;
      }
    }
  }
  if (!comandoRecibido) Serial.println("Ninguno.");

  // 6. Apagar todo
  energyManager.powerSensors(false);
  Serial.println("Sensores desenergizados.");
  Serial.print("Poniendo radio a dormir... ");
  if (energyManager.sleepRadio()) Serial.println("OK."); else Serial.println("FALLO.");

  // 7. Poner el microcontrolador a dormir
  Serial.print("Durmiendo el microcontrolador por "); Serial.print(SLEEP_INTERVAL_MS / 1000); Serial.println(" segundos...");
  delay(100);
  
  energyManager.sleepFor_ms(SLEEP_INTERVAL_MS);
}

// ======================= 6. FUNCIONES DE LECTURA DE SENSORES =======================
float leerVoltajeZMPT() {
  return ((analogRead(ZMPT_PIN) * 5.0) / 1023.0) * 50.0;
}

float leerCorrienteACS() {
  float volt = (analogRead(ACS_PIN) * 5.0) / 1023.0;
  return (volt - 2.5) / 0.066;
}

float leerVoltajeBateria() {
  float vEsc = (analogRead(VBAT_PIN) * 5.0) / 1023.0;
  return vEsc * 3.0;
}