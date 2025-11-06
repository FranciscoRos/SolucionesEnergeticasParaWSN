/*
 * ==========================================================
 * ==   SKETCH EMISOR UNIVERSAL (LoRa + XBee + NRF24L01)   ==
 * ==   CON GESTIÓN ADAPTATIVA DE ENERGÍA                 ==
 * ==========================================================
 * Este sketch usa LoRa/XBee/NRF24L01 y ajusta su frecuencia de envío
 * automáticamente según el nivel de la batería para ahorrar
 * energía, usando la librería AdaptiveTXWSN.
 */

// --- LIBRERIAS DE LA APLICACIÓN ---
#include <SPI.h>
#include <UniversalRadioWSN.h>
#include <SoftwareSerial.h>
#include <AdaptiveTXWSN.h>

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
//#define USE_XBEE
//#define USE_LORA
#define USE_NRF



// ======================= CONFIGURACIÓN GENERAL DE PINES =======================
#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2

// ======================= OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;
AdaptiveTXWSN txManager; 
uint32_t paquetesEnviados = 0;



// --- Objeto de puerto serial para el XBee (listo para usarse) ---
#if defined(USE_XBEE)
  SoftwareSerial xbeeSerial(2, 3); // RX Pin = 2, TX Pin = 3

#elif defined(USE_NRF)
  const byte nrfWriteAddress[6] = "00001";
  const byte nrfReadAddress[6] = "00002";
#endif

// --- INICIO DE CÓDIGO AÑADIDO (MÉTRICAS DE LOOP) ---
unsigned long lastPrintTime = 0; // Para imprimir el tiempo cada X ms
const unsigned long printInterval = 2000; // Imprimir cada 2 segundos
unsigned long totalLoopTime_us = 0; // Acumulador de tiempo de loop (en microsegundos)
unsigned long loopCount = 0; // Contador de loops
// --- FIN DE CÓDIGO AÑADIDO ---


// ======================= SETUP =======================
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  while(!Serial);
  Serial.println("\n--- INICIANDO EMISOR UNIVERSAL ADAPTATIVO ---");


  Serial.println("Configurando gestor de energía adaptativo...");
  AdaptiveTXWSN::Cfg configEnergia;


  configEnergia.pinAdcBateria       = VBAT_PIN;
  configEnergia.voltajeReferenciaAdc = 5.0f;     // Para Arduino a 5V

  configEnergia.divisorRArriba_k = 0.1f;
  configEnergia.divisorRAbajo_k  = 0.99f;
  
  // -- Umbrales y períodos  --
  configEnergia.umbralAlto_V   = 4.00f;  
  configEnergia.umbralMedio_V  = 2.0f;  
  configEnergia.corteVoltaje_V = 1.0f;  
  configEnergia.periodoAlto_ms = 1;  
  configEnergia.periodoMedio_ms= 1;  
  configEnergia.periodoBajo_ms = 1; 

  // Pasa la configuración 
  txManager.begin(configEnergia);

  // --- INYECCIÓN DE DEPENDENCIA DEL RADIO ---
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

  // --- CÓDIGO AÑADIDO (MÉTRICAS DE LOOP) ---
  lastPrintTime = millis(); // Inicializa el temporizador de impresión
  // --- FIN DE CÓDIGO AÑADIDO ---
}

// ======================= LOOP =======================
void loop() {
  // --- INICIO DE CÓDIGO AÑADIDO (MÉTRICAS DE LOOP) ---
  unsigned long startTime_us = micros(); // 1. Registrar inicio (alta precisión)
  // --- FIN DE CÓDIGO AÑADIDO ---


  //txManager decide cuándo enviar
    float voltage   = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();

    // Obtenemos el voltaje de la batería usando la librería.
    float vbat      = txManager.lastVolts();
    paquetesEnviados++;

    String dataPayload = "N:" + String(paquetesEnviados) +
                         " V:" + String(voltage, 2) +
                         " I:" + String(corriente, 2) +
                         " B:" + String(vbat, 2);
    
    //Serial.print("Enviado (Nivel Bateria: " + String(txManager.level()) + "): ");
    //Serial.println(dataPayload);

    #if defined(USE_NRF)
      radio->enviar(dataPayload);
    #else
      radio->enviar(dataPayload + "\n");
    #endif

  

  // --- Recepción de comandos ---
  if (radio->hayDatosDisponibles()) {
    String comando = radio->leerComoString();
    comando.trim();

    Serial.print("Comando recibido: ");
    Serial.println(comando);

      digitalWrite(RELAY_PIN, HIGH);

  }


  // --- INICIO DE CÓDIGO AÑADIDO (MÉTRICAS DE LOOP) ---
  
  // 2. Acumular métricas
  totalLoopTime_us += (micros() - startTime_us);
  loopCount++;

  // 3. Imprimir promedio cada 'printInterval' milisegundos
  if (millis() - lastPrintTime >= printInterval) {
    
    if (loopCount > 0) { // Evitar división por cero
      // Calcular el tiempo promedio en milisegundos
      float avgTime_ms = (float)totalLoopTime_us / loopCount / 1000.0; 
      
      Serial.print("[METRICA] Loops en " + String(printInterval) + "ms: " + String(loopCount));
      Serial.print(" | Tiempo loop (prom): ");
      Serial.print(avgTime_ms, 4); // Imprimir con 4 decimales
      Serial.println(" ms");
    }
    
    // Reiniciar contadores para el próximo intervalo
    lastPrintTime = millis();
    totalLoopTime_us = 0;
    loopCount = 0;
  }
  // --- FIN DE CÓDIGO AÑADIDO ---
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