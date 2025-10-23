/*
 * ==========================================================
 * ==   SKETCH EMISOR UNIVERSAL (LoRa + XBee Preparado)    ==
 * ==   CON GESTIÓN ADAPTATIVA DE ENERGÍA                  ==
 * ==========================================================
 * Este sketch usa LoRa/XBee y ajusta su frecuencia de envío
 * automáticamente según el nivel de la batería para ahorrar
 * energía, usando la librería AdaptiveTXWSN.
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>
#include <UniversalRadioWSN.h>
#include <SoftwareSerial.h>
#include "AdaptiveTXWSN.h"

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
// Descomenta solo UNA de las siguientes dos líneas.
//#define USE_XBEE
//#define USE_LORA
#define USE_NRF


// --- VERIFICACIÓN DE COMPILACIÓN ---

// ======================= CONFIGURACIÓN GENERAL DE PINES =======================
#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2

// ======================= OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;
AdaptiveTXWSN txManager; // --> CAMBIO: Se crea el objeto para gestionar la energía.
uint32_t paquetesEnviados = 0;

// --> CAMBIO: Se eliminan las variables del temporizador manual.
// unsigned long previousMillis = 0;
// const unsigned long INTERVAL_MS = 3000;

// --- Objeto de puerto serial para el XBee (listo para usarse) ---
#if defined(USE_XBEE)
  SoftwareSerial xbeeSerial(2, 3); // RX Pin = 2, TX Pin = 3

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
  Serial.println("\n--- INICIANDO EMISOR UNIVERSAL ADAPTATIVO ---");

  // --> CAMBIO: Se configura el gestor de energía adaptativo.
  Serial.println("Configurando gestor de energía adaptativo...");
  AdaptiveTXWSN::Cfg configEnergia;

  // -- Configuración específica de la placa (Arduino Uno/Nano 5V) --
  configEnergia.pinAdcBateria        = VBAT_PIN;
  configEnergia.voltajeReferenciaAdc = 5.0f;     // Para Arduino a 5V

  // -- Configuración del divisor de voltaje para la batería --
  // Tu función original `leerVoltajeBateria` multiplicaba por 3.0.
  // Esto equivale a un divisor con R_arriba=20k y R_abajo=10k.
  // (20k + 10k) / 10k = 3.0
  configEnergia.divisorRArriba_k = 100.0f;
  configEnergia.divisorRAbajo_k  = 33.3f;
  
  // -- Umbrales y períodos (AJUSTA ESTO PARA TU BATERÍA) --
  configEnergia.umbralAlto_V   = 4.00f;  // Umbral para considerar batería alta (LiPo)
  configEnergia.umbralMedio_V  = 3.70f;  // Umbral para considerar batería media (LiPo)
  configEnergia.corteVoltaje_V = 3.40f;  // Voltaje de seguridad para dejar de enviar
  configEnergia.periodoAlto_ms = 10000;  // Enviar cada 10 segundos con batería llena
  configEnergia.periodoMedio_ms= 30000;  // Enviar cada 30 segundos con batería media
  configEnergia.periodoBajo_ms = 12000; // Enviar cada 2 minutos con batería baja

  // Pasa la configuración Y todos los umbrales/periodos como argumentos separados
txManager.begin(configEnergia, 
                configEnergia.umbralAlto_V, 
                configEnergia.umbralMedio_V, 
                configEnergia.corteVoltaje_V, 
                configEnergia.fraccionHisteresis, 
                configEnergia.periodoAlto_ms, 
                configEnergia.periodoMedio_ms, 
                configEnergia.periodoBajo_ms);

  // --- INYECCIÓN DE DEPENDENCIA DEL RADIO (Sin cambios) ---
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
    configLora.csPin            = 10;
    configLora.resetPin         = 9;
    configLora.irqPin           = 2;

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
}

// ======================= LOOP =======================
void loop() {
  //txManager decide cuándo enviar
  if (txManager.tick()) {
    float voltage   = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();

    // Obtenemos el voltaje de la batería usando la librería.
    float vbat      = txManager.lastVolts();
    paquetesEnviados++;

    String dataPayload = "N:" + String(paquetesEnviados) +
                         " V:" + String(voltage, 2) +
                         " I:" + String(corriente, 2) +
                         " B:" + String(vbat, 2);
    
    Serial.print("Enviado (Nivel Bateria: " + String(txManager.level()) + "): ");
    Serial.println(dataPayload);

    #if defined(USE_NRF)
      radio->enviar(dataPayload);
    #else
      radio->enviar(dataPayload + "\n");
    #endif

  }

  // --- Recepción de comandos (sin cambios) ---
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
  return ((lectura * 5.0) / 1023.0) * 50.0; // Asume Arduino 5V 10-bit
}

float leerCorrienteACS() {
  int lectura = analogRead(ACS_PIN);
  float volt = (lectura * 5.0) / 1023.0; // Asume Arduino 5V 10-bit
  return (volt - 2.5) / 0.066;
}
