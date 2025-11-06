/*
 * ==========================================================
 * ==    SKETCH EMISOR UNIVERSAL (LoRa + XBee + NRF24)     ==
 * ==========================================================
 *
 * CORREGIDO:
 * - Añadido ciclo de trabajo de 30s para envío (SEND_INTERVAL_MS)
 * - Añadido ciclo de guardado en EEPROM de 5min (EEPROM_WRITE_INTERVAL_MS)
 * - Corregido error de direcciones de EEPROM (ahora usa EEPROM_ADDR_PAQUETES)
 * - Añadido Serial.print para "ver" la configuración del ciclo de trabajo
 * - Lógica de Relay revertida (enciende con cualquier comando)
 *
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#include <UniversalRadioWSN.h>
// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
//#define USE_LORA
//#define USE_XBEE
#define USE_NRF

// ======================= CONFIGURACIÓN GENERAL DE PINES =======================
#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2

// ======================= CONFIGURACIÓN DE CICLOS Y EEPROM =======================
#define EEPROM_ADDR_PAQUETES 0 

// --- Temporizador para el Ciclo de Trabajo (Envío de Datos) ---
unsigned long previousMillis = 0;
const unsigned long SEND_INTERVAL_MS = 30000; // 30 segundos

// --- Temporizador para el Guardado en EEPROM (Evitar desgaste) ---
unsigned long previousEepromMillis = 0;
const unsigned long EEPROM_WRITE_INTERVAL_MS = 300000; // 5 minutos

// ======================= OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;
uint32_t paquetesEnviados;

// --- Configuración específica por radio ---
#if defined(USE_XBEE)
  SoftwareSerial xbeeSerial(2, 3);
#elif defined(USE_NRF)
  const byte nrfWriteAddress[6] = "00001";
  const byte nrfReadAddress[6] = "00002";
#endif

// ======================= SETUP =======================
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  while(!Serial);
  Serial.println("\n--- INICIANDO EMISOR UNIVERSAL ---");

  Serial.print("Configurando radio: ");

  #if defined(USE_LORA)
    Serial.println("LoRa");
    // ... tu config LoRa ...
    radio = new LoraRadio(configLora);
  #elif defined(USE_XBEE)
    Serial.println("XBee");
    xbeeSerial.begin(9600);
    radio = new XBeeRadio(xbeeSerial, 9600, -1, -1);
  #elif defined(USE_NRF)
    Serial.println("NRF24L01");
    NrfConfig configNrf;
    configNrf.cePin = 9;
    configNrf.csnPin = 10;
    configNrf.writeAddress = nrfWriteAddress;
    configNrf.readAddress = nrfReadAddress;
    configNrf.channel = 108;
    configNrf.dataRate = 250;
    configNrf.paLevel = 0;
    radio = new NrfRadio(configNrf);
  #endif

  if (!radio->iniciar()) {
    Serial.println("¡¡¡ERROR: Fallo al iniciar el módulo de radio!!!");
    while (true);
  }
  Serial.println("Módulo de radio inicializado y listo.");

  // --- LECTURA INICIAL DE LA EEPROM ---
  EEPROM.get(EEPROM_ADDR_PAQUETES, paquetesEnviados);
  Serial.print("Contador recuperado de EEPROM: ");
  Serial.println(paquetesEnviados);

  // <-- NUEVO: Aquí puedes "ver" el ciclo de trabajo configurado
  Serial.println("\n--- CICLOS DE TRABAJO ---");
  Serial.print("Ciclo de envío de datos: ");
  Serial.print(SEND_INTERVAL_MS / 1000);
  Serial.println(" segundos.");
  Serial.print("Ciclo de guardado en EEPROM: ");
  Serial.print(EEPROM_WRITE_INTERVAL_MS / 1000 / 60);
  Serial.println(" minutos.");
  Serial.println("---------------------------\n");
}

void loop() {
  unsigned long currentMillis = millis();

  // ======================= CICLO DE TRABAJO (ENVÍO) =======================
  // Solo se ejecuta 1 vez cada SEND_INTERVAL_MS (30 segundos)
  if (currentMillis - previousMillis >= SEND_INTERVAL_MS) {
    previousMillis = currentMillis;

    float voltage = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();
    float vbat = leerVoltajeBateria();
    paquetesEnviados++;

    String dataPayload = "N:" + String(paquetesEnviados) +
                         " V:" + String(voltage, 2) +
                         " I:" + String(corriente, 2) +
                         " B:" + String(vbat, 2);

    Serial.print("Enviado: "); // <-- Esto también te muestra la parte "activa" del ciclo
    Serial.println(dataPayload);

    #if defined(USE_NRF)
      radio->enviar(dataPayload);
    #else
      radio->enviar(dataPayload + "\n");
    #endif
  }

  // ======================= CICLO DE GUARDADO EN EEPROM =======================
  // Se ejecuta solo 1 vez cada EEPROM_WRITE_INTERVAL_MS (5 minutos)
  if (currentMillis - previousEepromMillis >= EEPROM_WRITE_INTERVAL_MS) {
    previousEepromMillis = currentMillis;
    EEPROM.put(EEPROM_ADDR_PAQUETES, paquetesEnviados);
    Serial.println("-> Contador guardado en EEPROM."); // <-- Confirmación de guardado
  }

  // ======================= RECEPCIÓN DE COMANDOS =======================
  if (radio->hayDatosDisponibles()) {
    String comando = radio->leerComoString();
    comando.trim(); 

    Serial.print("Comando recibido: ");
    Serial.println(comando);
    
    // --- Lógica de Relay Revertida ---
    digitalWrite(RELAY_PIN, HIGH); // <-- MODIFICADO: Se enciende con CUALQUIER comando
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