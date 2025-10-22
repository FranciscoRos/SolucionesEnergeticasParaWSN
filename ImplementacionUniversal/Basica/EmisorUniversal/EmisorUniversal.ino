/*
 * ==========================================================
 * ==    SKETCH EMISOR UNIVERSAL (LoRa + XBee + NRF24)      ==
 * ==========================================================
 * Este sketch usa la interfaz "RadioInterface" para abstraer
 * el hardware de radio.
 *
 * MODIFICACIÓN: Guarda el contador de paquetes en la EEPROM.
 * VERSIÓN: Corregida para Arduino Nano.
 * NUEVO: Añadido soporte para NRF24L01.
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>
#include <UniversalRadioWSN.h>  // Contiene LoraRadio.h, XbeeRadio.h y NrfRadio.h
#include <SoftwareSerial.h> // Se incluye para la compatibilidad con XBee
#include <EEPROM.h>         // Librería para la memoria no volátil
// #include <RF24.h>        // <-- ELIMINADO: Ya no es necesario aquí

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
#define USE_LORA
//#define USE_XBEE 
//#define USE_NRF    

// ======================= CONFIGURACIÓN GENERAL DE PINES =======================
#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2

// 
// ======================= OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;

unsigned long previousMillis = 0;
const unsigned long INTERVAL_MS = 3000;
uint32_t paquetesEnviados;
// Se inicializa desde la EEPROM en setup()

// --- Configuración específica por radio ---
#if defined(USE_XBEE)
  SoftwareSerial xbeeSerial(2, 3);
// RX Pin = 2, TX Pin = 3
#elif defined(USE_NRF)
  // Direcciones para NRF24L01 (basadas en tus .ino)
  const byte nrfWriteAddress[6] = "00001";
// Dirección del receptor/coordinador
  const byte nrfReadAddress[6] = "00002";
// Dirección ÚNICA para este emisor (para recibir "ON"/"OFF")
#endif

// ======================= SETUP =======================
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  while(!Serial);
  Serial.println("\n--- INICIANDO EMISOR UNIVERSAL ---");

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
    // --- ESTE BLOQUE ESTÁ LISTO PERO INACTIVO ---
    Serial.println("XBee");
    xbeeSerial.begin(9600);
// Inicia el puerto serial para el XBee
    // Crea la instancia de radio para XBee
    radio = new XBeeRadio(xbeeSerial, 9600, -1, -1);
  
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

  // --- LECTURA INICIAL DE LA EEPROM ---
  // EEPROM.begin(...);
// <-- ELIMINADO: No es necesario para Arduino Nano
  EEPROM.get(1, paquetesEnviados);
// Lee el valor guardado en la dirección 0
  Serial.print("Contador recuperado de EEPROM: ");
  Serial.println(paquetesEnviados);
}

// ======================= LOOP =======================
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= INTERVAL_MS) {
    previousMillis = currentMillis;

    // radio->despertar();
// <-- Opcional: si se usa radio->dormir() abajo

    float voltage = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();
    float vbat = leerVoltajeBateria();
    paquetesEnviados++;

    String dataPayload = "N:" + String(paquetesEnviados) +
                         " V:" + String(voltage, 2) +
                         " I:" + String(corriente, 2) +
                         " B:" + String(vbat, 
2);
    
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
    
    // --- GUARDADO EN EEPROM ---
    EEPROM.put(1, paquetesEnviados);
// Guarda el nuevo valor del contador
    // EEPROM.commit();
// <-- ELIMINADO: No es necesario para Arduino Nano
    
    // radio->dormir();
// <-- Opcional: poner a dormir el radio aquí
  }

  // Comprueba si hay comandos entrantes (p.ej. "ON" / "OFF")
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