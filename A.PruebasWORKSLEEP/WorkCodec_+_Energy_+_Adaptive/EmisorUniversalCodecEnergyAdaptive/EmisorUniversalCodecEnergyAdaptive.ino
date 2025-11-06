/*
 * =======================================================================
 * ==       SKETCH EMISOR UNIVERSAL (Energy + Adaptive + Codec)       ==
 * ==         MODIFICADO PARA MODO DE TRABAJO INFINITO (HIGH)         ==
 * =======================================================================
 * Este sketch usa:
 * 1. UniversalRadioWSN: Para abstraer el hardware de radio (LoRa/XBee/NRF).
 * 2. CodecWSN: Para codificación binaria eficiente del payload.
 * 3. AdaptiveTXWSN: Para calcular un período de envío VARIABLE según la batería.
 * 4. EnergyWSN: Para apagar periféricos y poner el MCU en sueño profundo (sleep).
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>
#include <SoftwareSerial.h> 
#include <EEPROM.h>  

#include <UniversalRadioWSN.h>       
#include <EnergyWSN.h>       
#include <AdaptiveTXWSN.h>   
#include <CodecWSN.h>        

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
//#define USE_LORA
//#define USE_XBEE 
#define USE_NRF 

// --- INICIO BLOQUE MÉTRICAS (DEFINICIONES) ---
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 2000; // Imprimir cada 2 segundos
unsigned long totalLoopTime_us = 0;
unsigned long loopCount = 0;
// --- FIN BLOQUE MÉTRICAS ---
// ======================= 2. CONFIGURACIÓN GENERAL DE PINES =======================
#define RELAY_PIN         4   
#define SENSOR_POWER_PIN  7   // Pin que alimenta a los sensores (controlado por EnergyWSN)
#define ACS_PIN           A0  
#define ZMPT_PIN          A1  
#define VBAT_PIN          A2   // Pin de sensado de batería (usado por AdaptiveTXWSN)

// --- Pines específicos para XBee (Sleep) ---
#if defined(USE_XBEE)
  #define XBEE_RX_PIN       2   
  #define XBEE_TX_PIN       3   
  #define XBEE_SLEEP_RQ_PIN 9   
  #define XBEE_ON_SLEEP_PIN 10  
#endif

// ======================= 3. CONFIGURACIÓN DE EEPROM =======================
#define EEPROM_COUNTER_ADDR 3 // (Ya no se usa)

// ======================= 4. OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;
EnergyWSN energyManager;      // Gestiona el "cómo" dormir (hardware sleep)
AdaptiveTXWSN txManager;      // Gestiona el "cuánto" dormir (tiempo adaptativo)
AdaptiveTXWSN::Cfg configEnergia; // Configuración para el txManager (movido a global)
uint32_t paquetesEnviados = 0;     

// --- Configuración específica por radio ---
#if defined(USE_XBEE)
  SoftwareSerial xbeeSerial(XBEE_RX_PIN, XBEE_TX_PIN);
#elif defined(USE_NRF)
  // Direcciones para NRF24L01
  const byte nrfWriteAddress[6] = "00001"; // Dirección del receptor
  const byte nrfReadAddress[6] = "00002";  // Dirección de este emisor
#endif

// ======================= 5. SETUP =======================
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  while(!Serial);
  Serial.println("\n--- INICIANDO EMISOR (MODO TRABAJO INFINITO) ---");

  // --- Recuperar contador desde la EEPROM ---
  // EEPROM.get(EEPROM_COUNTER_ADDR, paquetesEnviados); // <-- COMENTADO
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

  // --- CONFIGURACIÓN DEL GESTOR DE TIEMPO ADAPTATIVO (AdaptiveTXWSN) ---
  Serial.println("Configurando gestor de tiempo adaptativo...");
  
  configEnergia.pinAdcBateria       = VBAT_PIN;
  
  configEnergia.voltajeReferenciaAdc = 5.0f; 
  configEnergia.divisorRArriba_k = 0.0f;
  configEnergia.divisorRAbajo_k  = 0.1f;    
  
  configEnergia.umbralAlto_V   = 4.00f;   
  configEnergia.umbralMedio_V  = 3.3f;   
  configEnergia.corteVoltaje_V = 2.0f;   
  configEnergia.periodoAlto_ms = 1;   
  configEnergia.periodoMedio_ms= 1;    
  configEnergia.periodoBajo_ms = 1; 

  // --- INICIALIZACIÓN DEL GESTOR DE TIEMPO ---
  txManager.begin(configEnergia);

  // --- CONFIGURACIÓN DEL GESTOR DE ENERGÍA (EnergyWSN) ---
  EnergyWSN::Cfg energyConfig;
  energyConfig.pins.pwrSens = SENSOR_POWER_PIN;
  energyConfig.pins.vbatSense = -1; // No usado (AdaptiveTXWSN lo gestiona)
  energyConfig.bootSleep = false;
  
  // *** INICIO CORRECCIÓN DE LÓGICA ***
  // Le decimos a la librería que tu hardware usa lógica invertida (LOW=ON, HIGH=OFF)
  energyConfig.invertPwr = true; 
  // *** FIN CORRECCIÓN DE LÓGICA ***
  
  energyManager.begin(energyConfig, radio);
  Serial.println("Gestor de energía (sleep) inicializado.");

  // --- INICIO BLOQUE MÉTRICAS (SETUP) ---
  lastPrintTime = millis();
  // --- FIN BLOQUE MÉTRICAS ---

  // *** ENCENDIDO PERMANENTE ***
  // El begin() los APAGA (porque invertPwr=true), aquí los encendemos permanentemente.
  energyManager.powerSensors(true); // Esto ahora hará un LOW
  Serial.println("Sensores energizados permanentemente.");
}

// ======================= 6. LOOP (CICLO DE TRABAJO INFINITO) =======================
void loop() {
  // --- INICIO BLOQUE MÉTRICAS (LOOP INICIO) ---
  unsigned long startTime_us = micros();
  
  // Serial.println("\n---------------------------------"); // <-- COMENTADO
  // Serial.println("Iniciando ciclo de medición y envío."); // <-- COMENTADO
  
  // --- 1. DESPERTAR RADIO Y SENSORES (CON VERIFICACIÓN) ---
  bool radioDespierta = energyManager.wakeRadio();
  // if (radioDespierta) { // <-- COMENTADO
  //   Serial.println("Radio despertada OK.");
  // } else {
  //   Serial.println("¡¡FALLO al despertar la radio!!");
  // }
  
  // energyManager.powerSensors(true); // Ya no es necesario, se hizo en setup
  // Serial.println("Sensores energizados (comando enviado)."); // <-- COMENTADO

  // --- 2. LEER SENSORES Y ACTUALIZAR ESTADO ADAPTATIVO ---
  float voltage   = leerVoltajeZMPT();
  float corriente = leerCorrienteACS();
  
  // Llamamos a tick() para que lea VBAT_PIN y actualice su estado interno
  txManager.tick(); 
  float vbat = txManager.lastVolts();
  paquetesEnviados++;

  // --- 3. CONSTRUIR PAQUETE BINARIO (CODECWSN) ---
  Packet miPaquete;
  miPaquete.id = paquetesEnviados;
  miPaquete.voltaje = (int16_t)(voltage * 100);     
  miPaquete.corriente = (int16_t)(corriente * 1000); 
  miPaquete.vbat = (uint16_t)(vbat * 100);       

  // --- 4. CODIFICAR Y ENVIAR FRAME BINARIO ---
  uint8_t frameBuffer[WSNFrame::FRAME_SIZE];
  WSNFrame::encodeFrameFromPacket(frameBuffer, miPaquete);
  
  // Serial.print("   -> Enviando Paquete (Binario): "); // <-- COMENTADO
  // Serial.print("ID: "); Serial.print(miPaquete.id); // <-- COMENTADO
  // Serial.print(" V: "); Serial.print(voltage, 2); // <-- COMENTADO
  // Serial.print(" I: "); Serial.print(corriente, 3); // <-- COMENTADO
  // Serial.print(" VBat: "); Serial.println(vbat, 2); // <-- COMENTADO

  radio->enviar(frameBuffer, WSNFrame::FRAME_SIZE); 
    
    
  // --- 5. GUARDAR CONTADOR EN EEPROM ---
  // EEPROM.put(EEPROM_COUNTER_ADDR, paquetesEnviados); // <-- COMENTADO
  
  // --- 6. ESCUCHAR POR COMANDOS (VENTANA DE RECEPCIÓN) ---
  // Serial.print("Escuchando comandos por 500ms... "); // <-- COMENTADO
  // long tiempoInicioEscucha = millis(); // <-- COMENTADO
  // bool comandoRecibido = false; // <-- COMENTADO
  
  // if (radioDespierta) { // <-- COMENTADO
  //   #if defined(USE_NRF)
  //     // ... (todo el bloque de escucha NRF está COMENTADO) ...
  //   #else
  //     // ... (todo el bloque de escucha LoRa/XBee está COMENTADO) ...
  //   #endif
  // } else {
  //    Serial.print("... Escucha omitida (radio no despertó)."); // <-- COMENTADO
  // }
  // if (!comandoRecibido) Serial.println("Ninguno."); // <-- COMENTADO

  // --- 7. APAGAR PERIFÉRICOS ---
  // energyManager.powerSensors(false); // Apagar sensores // <-- COMENTADO
  // Serial.println("Sensores apagados (comando enviado)."); // <-- COMENTADO
  // bool radioDormida = energyManager.sleepRadio(); // <-- COMENTADO
  // if (radioDormida) { // <-- COMENTADO
  //    Serial.println("Radio dormida OK.");
  // } else {
  //    Serial.println("¡¡FALLO al dormir la radio!!");
  // }

  // --- 8. DORMIR EL MCU (TIEMPO ADAPTATIVO) ---
  // ... (todo el bloque de sleep está COMENTADO) ...
  
  // --- INICIO BLOQUE MÉTRICAS (LOOP FINAL) ---
  totalLoopTime_us += (micros() - startTime_us);
  loopCount++;

  if (millis() - lastPrintTime >= printInterval) {
    if (loopCount > 0) {
      float avgTime_ms = (float)totalLoopTime_us / loopCount / 1000.0; 
      
      Serial.print("[METRICA] Loops en ");
      Serial.print(printInterval);
      Serial.print("ms: ");
      Serial.print(loopCount);
      
      Serial.print(" | Tiempo loop (prom): ");
      Serial.print(avgTime_ms, 4);
      Serial.println(" ms");
    }
    lastPrintTime = millis();
    totalLoopTime_us = 0;
    loopCount = 0;
  }
  // --- FIN BLOQUE MÉTRICAS ---
}

// ======================= 7. FUNCIONES DE LECTURA DE SENSORES =======================
float leerVoltajeZMPT() {
  int lectura = analogRead(ZMPT_PIN);
  return ((lectura * 5.0) / 1023.0) * 50.0; 
}

float leerCorrienteACS() {
  int lectura = analogRead(ACS_PIN);
  float volt = (lectura * 5.0) / 1023.0; 
  return (volt - 2.5) / 0.066; 
}