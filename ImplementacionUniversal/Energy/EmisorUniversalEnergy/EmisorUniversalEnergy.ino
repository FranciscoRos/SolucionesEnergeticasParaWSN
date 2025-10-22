/*
 * ==========================================================
 * ==    SKETCH EMISOR UNIVERSAL (LoRa + XBee + NRF24)      ==
 * ==          MODIFICADO CON GESTIÓN DE ENERGÍA           ==
 * ==========================================================
 * Este sketch usa la interfaz "RadioInterface" para abstraer
 * el hardware de radio.
 *
 * MODIFICACIÓN: Guarda el contador de paquetes en la EEPROM.
 * VERSIÓN: Corregida para Arduino Nano.
 * NUEVO: Añadido soporte para NRF24L01.
 * MODIFICADO: Añadida gestión de energía con EnergyWSN.
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>
#include <UniversalRadioWSN.h>  // Contiene LoraRadio.h, XbeeRadio.h y NrfRadio.h
#include <SoftwareSerial.h> // Se incluye para la compatibilidad con XBee
#include <EEPROM.h>         // Librería para la memoria no volátil
#include "EnergyWSN.h"      // <-- AÑADIDO: Gestión de energía

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
//#define USE_LORA
//#define USE_XBEE 
#define USE_NRF    

// ======================= CONFIGURACIÓN GENERAL DE PINES =======================
// --- Pines comunes ---
#define RELAY_PIN         4
#define SENSOR_POWER_PIN  7    // <-- AÑADIDO: Pin que alimenta a los sensores
#define ACS_PIN           A0
#define ZMPT_PIN          A1
#define VBAT_PIN          A2

// --- Pines específicos para XBee (Añadidos para sleep) ---
#if defined(USE_XBEE)
  #define XBEE_RX_PIN       2
  #define XBEE_TX_PIN       3
  #define XBEE_SLEEP_RQ_PIN 9   // <-- AÑADIDO
  #define XBEE_ON_SLEEP_PIN 10  // <-- AÑADIDO
#endif

// ======================= OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;
EnergyWSN energyManager;        // <-- AÑADIDO
uint32_t paquetesEnviados;    // Se inicializa desde la EEPROM en setup()
const unsigned long SLEEP_INTERVAL_MS = ( 6* 1000); // <-- AÑADIDO

// --- Configuración específica por radio ---
#if defined(USE_XBEE)
  SoftwareSerial xbeeSerial(XBEE_RX_PIN, XBEE_TX_PIN); // RX Pin = 2, TX Pin = 3
#elif defined(USE_NRF)
  // Direcciones para NRF24L01 (basadas en tus .ino)
  const byte nrfWriteAddress[6] = "00001"; // Dirección del receptor/coordinador
  const byte nrfReadAddress[6] = "00002";  // Dirección ÚNICA para este emisor (para recibir "ON"/"OFF")
#endif

// ======================= SETUP =======================
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  while(!Serial);
  Serial.println("\n--- INICIANDO EMISOR UNIVERSAL (CON GESTIÓN DE ENERGÍA) ---");

  // --- Recuperar contador desde la EEPROM ---
  EEPROM.get(2, paquetesEnviados);
  Serial.print("Contador recuperado de EEPROM: ");
  Serial.println(paquetesEnviados);

  // --- INYECCIÓN DE DEPENDENCIA DEL RADIO ---
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
    xbeeSerial.begin(9600); // Inicia el puerto serial para el XBee
    // Crea la instancia de radio para XBee (MODIFICADO para incluir pines de sleep)
    radio = new XBeeRadio(xbeeSerial, 9600, XBEE_SLEEP_RQ_PIN, XBEE_ON_SLEEP_PIN);
  #elif defined(USE_NRF)
    Serial.println("NRF24L01");
    
    NrfConfig configNrf;
    // Pines para Arduino Nano/Uno (basado en NodoSensorPowerDown.ino)
    configNrf.cePin = 9;
    configNrf.csnPin = 10; 
    configNrf.writeAddress = nrfWriteAddress;
    configNrf.readAddress = nrfReadAddress;
    configNrf.channel = 108;
    // --- VALORES GENÉRICOS ---
    configNrf.dataRate = 250; // 250KBPS
    configNrf.paLevel = 0;    // 0 = RF24_PA_MIN
    
    radio = new NrfRadio(configNrf);
  #endif
  
  if (!radio->iniciar()) {
    Serial.println("¡¡¡ERROR: Fallo al iniciar el módulo de radio!!!");
    while (true);
  }
  Serial.println("Módulo de radio inicializado y listo.");

  // --- Configuración del gestor de energía --- (AÑADIDO)
  EnergyWSN::Cfg energyConfig;
  energyConfig.pins.pwrSens = SENSOR_POWER_PIN;
  energyConfig.pins.vbatSense = VBAT_PIN;
  energyConfig.bootSleep = false;
  
  energyManager.begin(energyConfig, radio);
  Serial.println("Gestor de energía inicializado.");
}

// ======================= LOOP (MODIFICADO CON GESTIÓN DE ENERGÍA) =======================
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

  // --- LÓGICA DE ENVÍO ORIGINAL (CONSERVADA) ---
  Serial.print("Enviado: ");
  Serial.println(dataPayload);
  
  #if defined(USE_NRF)
    // NRF envía paquetes puros, el '\n' no es necesario y gasta 1 byte.
    // ADVERTENCIA: La librería RF24 truncará esto a 32 bytes.
    radio->enviar(dataPayload);
  #else
    // LoRa y XBee (en modo transparente) se benefician del delimitador
    radio->enviar(dataPayload + "\n");
  #endif
  // --- FIN LÓGICA DE ENVÍO ---
    
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