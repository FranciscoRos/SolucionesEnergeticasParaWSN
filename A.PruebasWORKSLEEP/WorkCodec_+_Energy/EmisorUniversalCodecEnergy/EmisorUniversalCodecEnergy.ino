/*
 * =======================================================================
 * ==      SKETCH EMISOR UNIVERSAL (Energy + Codec)                     ==
 * =======================================================================
 * Este sketch usa CodecWSN para enviar datos binarios y EnergyWSN
 * para implementar ciclos de sueño (sleep).
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#include <EnergyWSN.h>
#include <CodecWSN.h>
#include <UniversalRadioWSN.h>

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
#define USE_LORA
//#define USE_XBEE 
//#define USE_NRF 

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
uint32_t paquetesEnviados;
const unsigned long SLEEP_INTERVAL_MS = ( 1); 

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
  Serial.println("\n--- INICIANDO EMISOR (ENERGY + CODEC) ---");

  // --- Recuperar contador desde la EEPROM ---
  EEPROM.get(2, paquetesEnviados);
  Serial.print("Contador recuperado de EEPROM: ");
  Serial.println(paquetesEnviados);

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
  energyConfig.bootSleep = false;
  
  energyManager.begin(energyConfig, radio);
  Serial.println("Gestor de energía inicializado.");
}

// ======================= 5. LOOP (CICLO DE SUEÑO) =======================
void loop() {
  Serial.println("\n---------------------------------");
  Serial.println("Iniciando ciclo de medición y envío.");
  
  // 1. Despertar radio y sensores
  energyManager.wakeRadio();
  energyManager.powerSensors(true);
  Serial.println("Radio y sensores energizados.");
  //delay(200); // Espera de estabilización

  // 2. Leer sensores
  float voltage = leerVoltajeZMPT();
  float corriente = leerCorrienteACS();
  float vbat = leerVoltajeBateria();
  paquetesEnviados++;

  // 3. Construir paquete de datos binario
  Packet miPaquete;
  miPaquete.id = paquetesEnviados;
  miPaquete.voltaje = (int16_t)(voltage * 100);     
  miPaquete.corriente = (int16_t)(corriente * 1000); 
  miPaquete.vbat = (uint16_t)(vbat * 100);         

  // 4. Codificar y enviar el frame binario
  uint8_t frameBuffer[WSNFrame::FRAME_SIZE];
  WSNFrame::encodeFrameFromPacket(frameBuffer, miPaquete);
  
  Serial.print("Enviando Paquete Binario -> ");
  Serial.print("ID: "); Serial.print(miPaquete.id);
  Serial.print(" V: "); Serial.print(voltage, 2);
  Serial.print(" I: "); Serial.print(corriente, 3);
  Serial.print(" VBat: "); Serial.println(vbat, 2);

  radio->enviar(frameBuffer, WSNFrame::FRAME_SIZE);
    
  // 5. Guardar contador en EEPROM
  EEPROM.put(2, paquetesEnviados);
  
  // 6. Escuchar por comandos entrantes (ventana de 500ms)
  Serial.print("Escuchando comandos por 500ms... ");
  long tiempoInicioEscucha = millis();
  bool comandoRecibido = false;
  
  #if defined(USE_NRF)
    // NRF usa paquetes discretos
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
  #else
    // LoRa/XBee usan streaming (buscan '\n')
    String bufferComando = "";
    while (millis() - tiempoInicioEscucha < 500) {
      if (radio->hayDatosDisponibles()) {
        bufferComando += radio->leerComoString();
      }
      int fin = bufferComando.indexOf('\n');
      if (fin >= 0) {
        String comando = bufferComando.substring(0, fin);
        bufferComando = bufferComando.substring(fin + 1);
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
  #endif

  if (!comandoRecibido) Serial.println("Ninguno.");

  // 7. Apagar periféricos
  energyManager.powerSensors(false);
  energyManager.sleepRadio();
  Serial.println("Radio y sensores dormidos.");

  // 8. Poner el microcontrolador a dormir
  Serial.print("Durmiendo MCU por "); Serial.print(SLEEP_INTERVAL_MS / 1000);
  Serial.println(" segundos...");
  delay(100); // Da tiempo al Serial Monitor para imprimir
  
  energyManager.sleepFor_ms(SLEEP_INTERVAL_MS);
}

// ======================= 6. FUNCIONES DE LECTURA DE SENSORES =======================
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