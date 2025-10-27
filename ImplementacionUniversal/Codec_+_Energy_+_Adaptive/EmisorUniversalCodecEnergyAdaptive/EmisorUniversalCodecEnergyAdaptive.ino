/*
 * =======================================================================
 * ==    SKETCH EMISOR UNIVERSAL (ENERGY + ADAPTIVE + CODEC)            ==
 * =======================================================================
 * Este sketch usa:
 * 1. UniversalRadioWSN: Para abstraer el hardware de radio (LoRa/XBee/NRF).
 * 2. CodecWSN: Para codificación binaria eficiente del payload.
 * 3. AdaptiveTXWSN: Para calcular un período de envío VARIABLE según la batería.
 * 4. EnergyWSN: Para apagar periféricos y poner el MCU en sueño profundo (sleep).
 * 5. EEPROM: Para persistir el contador de paquetes.
 *
 * *** MODIFICADO: Añade verificación del éxito de wake/sleep de radio/sensores. ***
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>
#include <UniversalRadioWSN.h>
#include <SoftwareSerial.h> // Para compatibilidad con XBee
#include <EEPROM.h>         // Para memoria no volátil
#include "EnergyWSN.h"      // Para gestión de energía (sleep)
#include "AdaptiveTXWSN.h"   // Para gestión de tiempo adaptativo
#include <CodecWSN.h>       // Librería para codificación binaria

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
#define USE_LORA
//#define USE_XBEE 
//#define USE_NRF 

// ======================= 2. CONFIGURACIÓN GENERAL DE PINES =======================
// --- Pines comunes ---
#define RELAY_PIN         4   
#define SENSOR_POWER_PIN  7    // Pin que alimenta a los sensores (controlado por EnergyWSN)
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
// Dirección de memoria para guardar el contador de paquetes
#define EEPROM_COUNTER_ADDR 3 

// ======================= 4. OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;
EnergyWSN energyManager;      // Gestiona el "cómo" dormir (hardware sleep)
AdaptiveTXWSN txManager;      // Gestiona el "cuánto" dormir (tiempo adaptativo)
AdaptiveTXWSN::Cfg configEnergia; // Configuración para el txManager (movido a global)
uint32_t paquetesEnviados;    // Contador de paquetes (se recupera de EEPROM)

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
  Serial.println("\n--- INICIANDO EMISOR UNIVERSAL (ENERGY + ADAPTIVE + CODEC) ---");

  // --- Recuperar contador desde la EEPROM ---
  EEPROM.get(EEPROM_COUNTER_ADDR, paquetesEnviados); // Usamos la dirección 3
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
    // Usamos la inicialización con pines de sleep de EnergyWSN
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
  
  // -- Configuración específica de la placa (Arduino Uno/Nano 5V) --
  configEnergia.pinAdcBateria       = VBAT_PIN;
  
  // --- INICIO DE LA MODIFICACIÓN PARA TESTING (FACTOR 1.0) ---
  // (Para probar sin resistencias, leyendo el voltaje directo del pin)
  configEnergia.voltajeReferenciaAdc = 5.0f; // Mide tu pin 5V real (ej. 4.8V)
  configEnergia.divisorRArriba_k = 0.0f;
  configEnergia.divisorRAbajo_k  = 0.1f;     // Factor 1.0 para leer voltaje directo
  // --- FIN DE LA MODIFICACIÓN PARA TESTING ---
  
  // -- Umbrales y períodos (AJUSTA ESTO PARA TU BATERÍA/PRUEBAS) --
  configEnergia.umbralAlto_V   = 4.00f;   // Umbral para considerar batería alta
  configEnergia.umbralMedio_V  = 3.3f;   // Umbral para considerar batería media
  configEnergia.corteVoltaje_V = 2.0f;   // Voltaje de seguridad
  configEnergia.periodoAlto_ms = 3000;  // Enviar cada 3 seg con batería llena
  configEnergia.periodoMedio_ms= 5000;  // Enviar cada 5 seg con batería media
  configEnergia.periodoBajo_ms = 12000; // Enviar cada 12 seg con batería baja

  // --- INICIALIZACIÓN DEL GESTOR DE TIEMPO ---
  txManager.begin(configEnergia, 
                  configEnergia.umbralAlto_V, 
                  configEnergia.umbralMedio_V, 
                  configEnergia.corteVoltaje_V, 
                  configEnergia.fraccionHisteresis, 
                  configEnergia.periodoAlto_ms, 
                  configEnergia.periodoMedio_ms, 
                  configEnergia.periodoBajo_ms);

  // --- CONFIGURACIÓN DEL GESTOR DE ENERGÍA (EnergyWSN) ---
  EnergyWSN::Cfg energyConfig;
  energyConfig.pins.pwrSens = SENSOR_POWER_PIN;
  energyConfig.pins.vbatSense = -1; // -1 porque AdaptiveTXWSN ya lo está manejando
  energyConfig.bootSleep = false;
  
  energyManager.begin(energyConfig, radio);
  Serial.println("Gestor de energía (sleep) inicializado.");
}

// ======================= 6. LOOP (CICLO DE SUEÑO ADAPTATIVO) =======================
void loop() {
  Serial.println("\n---------------------------------");
  Serial.println("Iniciando ciclo de medición y envío.");
  
  // --- 1. DESPERTAR RADIO Y SENSORES (CON VERIFICACIÓN) ---
  bool radioDespierta = energyManager.wakeRadio();
  if (radioDespierta) {
    Serial.println("Radio despertada OK.");
  } else {
    Serial.println("¡¡FALLO al despertar la radio!!");
    // Podrías añadir lógica aquí para reintentar o abortar
  }
  
  energyManager.powerSensors(true); // Encender sensores
  Serial.println("Sensores energizados (comando enviado).");
  //delay(200); // Opcional: Espera de estabilización

  // --- 2. LEER SENSORES Y ACTUALIZAR ESTADO ADAPTATIVO ---
  float voltage   = leerVoltajeZMPT();
  float corriente = leerCorrienteACS();
  
  // Llamamos a tick() para que lea VBAT_PIN y actualice su estado interno
  txManager.tick(); 
  // Ahora obtenemos el voltaje que acaba de leer
  float vbat = txManager.lastVolts();
  paquetesEnviados++;

  // --- 3. CONSTRUIR PAQUETE BINARIO (CODECWSN) ---
  Packet miPaquete;
  miPaquete.id = paquetesEnviados;
  miPaquete.voltaje = (int16_t)(voltage * 100);     // Guarda 120.55V como 12055
  miPaquete.corriente = (int16_t)(corriente * 1000); // Guarda 1.25A como 1250 (mA)
  miPaquete.vbat = (uint16_t)(vbat * 100);       // Guarda 4.15V como 415

  // --- 4. CODIFICAR Y ENVIAR FRAME BINARIO ---
  uint8_t frameBuffer[WSNFrame::FRAME_SIZE];
  WSNFrame::encodeFrameFromPacket(frameBuffer, miPaquete);
  
  Serial.print("  -> Enviando Paquete (Binario): ");
  Serial.print("ID: "); Serial.print(miPaquete.id);
  Serial.print(" V: "); Serial.print(voltage, 2);
  Serial.print(" I: "); Serial.print(corriente, 3);
  Serial.print(" VBat: "); Serial.println(vbat, 2);

  radio->enviar(frameBuffer, WSNFrame::FRAME_SIZE); 
    
    
  // --- 5. GUARDAR CONTADOR EN EEPROM ---
  EEPROM.put(EEPROM_COUNTER_ADDR, paquetesEnviados);
  
  // --- 6. ESCUCHAR POR COMANDOS (VENTANA DE RECEPCIÓN) ---
  Serial.print("Escuchando comandos por 500ms... ");
  long tiempoInicioEscucha = millis();
  bool comandoRecibido = false;
  
  if (radioDespierta) { // Solo escuchar si la radio está despierta
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
  } else {
     Serial.print("... Escucha omitida (radio no despertó).");
  }

  if (!comandoRecibido) Serial.println("Ninguno.");

  // --- 7. APAGAR PERIFÉRICOS (CON VERIFICACIÓN) ---
  energyManager.powerSensors(false); // Apagar sensores
  Serial.println("Sensores apagados (comando enviado).");

  bool radioDormida = energyManager.sleepRadio();
  if (radioDormida) {
     Serial.println("Radio dormida OK.");
  } else {
     Serial.println("¡¡FALLO al dormir la radio!!");
  }

  // --- 8. DORMIR EL MCU (TIEMPO ADAPTATIVO) ---
  // Obtenemos el próximo período de sueño desde AdaptiveTXWSN
  uint32_t sleepDuration_ms = txManager.currentPeriod(); 
  
  // Imprimimos el estado para depuración
  int currentLevel = (int)txManager.level();
  String currentLevelStr;
  switch (currentLevel) {
    case 2: currentLevelStr = "ALTO"; break;
    case 1: currentLevelStr = "MEDIO"; break;
    default: currentLevelStr = "BAJO"; break;
  }
  
  Serial.print("Nivel Bateria: " + currentLevelStr);
  Serial.print(" (" + String(vbat, 2) + "V). ");
  Serial.print("Proximo envio en: " + String(sleepDuration_ms / 1000.0, 0) + " seg.");
  Serial.println();
  
  delay(100); // Da tiempo al Serial Monitor para imprimir
  
  energyManager.sleepFor_ms(sleepDuration_ms); // Duerme por el tiempo variable
}

// ======================= 7. FUNCIONES DE LECTURA DE SENSORES =======================
// (NOTA: leerVoltajeBateria() se elimina porque AdaptiveTXWSN lo hace automáticamente)

float leerVoltajeZMPT() {
  int lectura = analogRead(ZMPT_PIN);
  return ((lectura * 5.0) / 1023.0) * 50.0; // Asume Arduino 5V 10-bit
}

float leerCorrienteACS() {
  int lectura = analogRead(ACS_PIN);
  float volt = (lectura * 5.0) / 1023.0; // Asume Arduino 5V 10-bit
  return (volt - 2.5) / 0.066; // Asume sensor ACS712 30A (66mV/A)
}