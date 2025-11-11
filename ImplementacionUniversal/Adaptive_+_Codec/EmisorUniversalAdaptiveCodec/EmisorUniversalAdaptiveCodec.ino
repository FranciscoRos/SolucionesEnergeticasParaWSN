/*
 * =======================================================================
 * ==    SKETCH EMISOR UNIVERSAL INTEGRAL (ADAPTATIVO + BINARIO)        ==
 * =======================================================================
 *
 * Combina dos estrategias:
 * 1. AdaptiveTXWSN: Para gestión de energía y frecuencia de envío
 * automática según el voltaje de la batería.
 * 2. CodecWSN: Para codificación binaria eficiente del payload.
 *
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>
#include <SoftwareSerial.h> 
#include <EEPROM.h>       

#include <UniversalRadioWSN.h>
#include <AdaptiveTXWSN.h>
#include <CodecWSN.h>        

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
//#define USE_LORA
//#define USE_XBEE
#define USE_NRF


// ======================= 2. SELECCIÓN DEL MODO DE PAYLOAD =======================
// Descomenta esta línea para usar CodecWSN o coméntala para usar el payload de String 
#define USE_BINARY_ENCODING


// ======================= CONFIGURACIÓN GENERAL DE PINES =======================
#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2

// ======================= CONFIGURACIÓN DE EEPROM =======================
#define EEPROM_COUNTER_ADDR 3 


// ======================= OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;
AdaptiveTXWSN txManager;     
AdaptiveTXWSN::Cfg configEnergia; 
uint32_t paquetesEnviados = 0; 

// --- Objeto de puerto serial para el XBee ---
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
  Serial.println("\n--- INICIANDO EMISOR UNIVERSAL (ADAPTATIVO + BINARIO) ---");

  Serial.println("Configurando gestor de energía adaptativo...");
  
  configEnergia.pinAdcBateria       = VBAT_PIN;
  configEnergia.voltajeReferenciaAdc = 5.0f;     

  configEnergia.divisorRArriba_k = 0.99f;
  configEnergia.divisorRAbajo_k  = 0.1f; 
  
  // -- Umbrales y períodos --
  configEnergia.umbralAlto_V   = 4.00f;
  configEnergia.umbralMedio_V  = 3.70f;
  configEnergia.corteVoltaje_V = 1.0f;
  configEnergia.periodoAlto_ms = 3000;
  configEnergia.periodoMedio_ms= 5000;
  configEnergia.periodoBajo_ms = 12000;

  // Pasa la configuración
  txManager.begin(configEnergia);

  // --- LECTURA INICIAL DE LA EEPROM ---
  EEPROM.get(EEPROM_COUNTER_ADDR, paquetesEnviados);
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
    configLora.resetPin        = 9;
    configLora.irqPin          = 2;

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
  
  // txManager decide cuándo enviar 
  if (txManager.tick()) {
    
    // 1. Leer los sensores
    float voltage   = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();

    // 2. Obtenemos el voltaje de la batería directamente del gestor
    float vbat      = txManager.lastVolts();
    paquetesEnviados++;

    // 3. Guardar el nuevo contador en EEPROM
    EEPROM.put(EEPROM_COUNTER_ADDR, paquetesEnviados);
    
    // 4. Determinar el intervalo actual
    long currentPeriod_ms = 0;
    String currentLevelStr; // String para almacenar el texto
    int currentLevel = (int)txManager.level(); 

    // Usamos los valores enteros del enum (BATT_HIGH=2, BATT_MID=1, BATT_LOW=0)
    switch (currentLevel) {
      
      case 2: // BATT_HIGH
        currentPeriod_ms = configEnergia.periodoAlto_ms;
        currentLevelStr = "ALTO";
        break;
        
      case 1: // BATT_MID
        currentPeriod_ms = configEnergia.periodoMedio_ms;
        currentLevelStr = "MEDIO";
        break;
        
      case 0: // BATT_LOW
      default: 
        currentPeriod_ms = configEnergia.periodoBajo_ms;
        currentLevelStr = "BAJO";
        break;
    }
    
    float periodInSeconds = currentPeriod_ms / 1000.0;

    // 5. Imprimir el mensaje de estado de batería
    Serial.println("----------------------------------------");
    Serial.print("Nivel Bateria: " + currentLevelStr); 
    Serial.print(" (" + String(vbat, 2) + "V). ");
    Serial.print("Proximo envio en: " + String(periodInSeconds, 0) + " seg.");
    Serial.println();


    // 6. Decidir el formato del payload (Binario o Texto)
    #if defined(USE_BINARY_ENCODING)
      // --- MODO BINARIO (CodecWSN) ---
      
      //Crear el paquete 
      Packet miPaquete;
      miPaquete.id = paquetesEnviados;
      miPaquete.voltaje = (int16_t)(voltage * 100);    
      miPaquete.corriente = (int16_t)(corriente * 1000);
      miPaquete.vbat = (uint16_t)(vbat * 100);

      // Codificar el paquete en un frame binario
      uint8_t frameBuffer[WSNFrame::FRAME_SIZE];
      WSNFrame::encodeFrameFromPacket(frameBuffer, miPaquete);

      // Enviar el frame binario
      radio->enviar(frameBuffer, WSNFrame::FRAME_SIZE);

      // --- IMPRESIÓN EN MONITOR SERIAL (MODO BINARIO) ---
      Serial.print("  -> Enviando Paquete (Binario): "); 
      Serial.print("ID: "); Serial.print(miPaquete.id);
      Serial.print(" V: "); Serial.print(voltage, 2);
      Serial.print(" I: "); Serial.print(corriente, 3);
      Serial.print(" VBat: "); Serial.println(vbat, 2);

    #else
      // --- MODO TEXTO (String) ---
      
      // Crear el payload como String
      String dataPayload = "N:" + String(paquetesEnviados) +
                           " V:" + String(voltage, 2) +
                           " I:" + String(corriente, 2) +
                           " B:" + String(vbat, 2);
      
      Serial.print("  -> Enviando Paquete (ASCII): "); 
      Serial.println(dataPayload);
      
      // Enviar el payload de String
      #if defined(USE_NRF)
        radio->enviar(dataPayload); 
      #else
        radio->enviar(dataPayload + "\n");
      #endif

    #endif
  }

  // --- Recepción de comandos ---
  if (radio->hayDatosDisponibles()) {
    String comando = radio->leerComoString();
    comando.trim();

    if (comando.length() > 0) {
      Serial.print("Comando recibido: ");
      Serial.println(comando);

      if (comando == "ON") {
        digitalWrite(RELAY_PIN, HIGH);
      } else if (comando == "OFF") {
        digitalWrite(RELAY_PIN, LOW);
      }
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