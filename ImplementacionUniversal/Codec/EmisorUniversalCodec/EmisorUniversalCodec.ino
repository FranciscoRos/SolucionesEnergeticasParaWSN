/*
 * =======================================================================
 * ==     SKETCH EMISOR UNIVERSAL (MODO BINARIO con CodecWSN)           ==
 * =======================================================================
 * Este sketch usa la librería CodecWSN para enviar datos de sensores
 * de forma eficiente y robusta. Es compatible con LoRa y XBee.
 */

// --- LIBRERías DE LA APLICACIÓN ---
#include <SPI.h>
#include <UniversalRadioWSN.h>
#include <SoftwareSerial.h> // Se incluye para la compatibilidad con XBee
#include <CodecWSN.h>       // <-- ¡LIBRERÍA PARA CODIFICACIÓN BINARIA!

// ======================= 1. SELECCIÓN DEL MÓDULO DE RADIO =======================
//#define USE_LORA
#define USE_XBEE // <-- Descomenta esta línea para usar XBee

// ======================= CONFIGURACIÓN GENERAL DE PINES =======================
#define RELAY_PIN 4
#define ACS_PIN   A0
#define ZMPT_PIN  A1
#define VBAT_PIN  A2

// ======================= OBJETOS Y VARIABLES GLOBALES =======================
RadioInterface* radio;
unsigned long previousMillis = 0;
const unsigned long INTERVAL_MS = 3000;
uint32_t paquetesEnviados = 0;
// --- Objeto de puerto serial para el XBee (listo para usarse) ---
#if defined(USE_XBEE)
  SoftwareSerial xbeeSerial(2, 3); // RX Pin = 2, TX Pin = 3
#endif

// ======================= SETUP =======================
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  while(!Serial);
  Serial.println("\n--- INICIANDO EMISOR UNIVERSAL (MODO BINARIO) ---");

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
    configLora.resetPin         = -1;
    configLora.irqPin           = 2;

    radio = new LoraRadio(configLora);

  #elif defined(USE_XBEE)
    Serial.println("XBee");
    xbeeSerial.begin(9600); // Inicia el puerto serial para el XBee
    radio = new XBeeRadio(xbeeSerial, 9600, -1, -1);
  #endif
  
  if (!radio->iniciar()) {
    Serial.println("¡¡¡ERROR: Fallo al iniciar el módulo de radio!!!");
    while (true);
  }
  Serial.println("Módulo de radio inicializado y listo.");
}

// ======================= LOOP =======================
void loop() {
  // --- Envío de datos de sensores cada INTERVAL_MS ---
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= INTERVAL_MS) {
    previousMillis = currentMillis;

    // 1. Leer los sensores (sin cambios)
    float voltage = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();
    float vbat = leerVoltajeBateria();
    paquetesEnviados++;

    // 2. Crear un paquete de datos binario
    Packet miPaquete;
    miPaquete.id = paquetesEnviados;
    miPaquete.voltaje = (int16_t)(voltage * 100);       // Guarda 120.55V como 12055
    miPaquete.corriente = (int16_t)(corriente * 1000);  // Guarda 1.25A como 1250 (mA)
    miPaquete.vbat = (uint16_t)(vbat * 100);          // Guarda 4.15V como 415

    // 3. Crear un buffer para el frame final (14 bytes)
    uint8_t frameBuffer[WSNFrame::FRAME_SIZE];

    // 4. Codificar el paquete en el frame (con SOF, LEN, CRC, etc.)
    WSNFrame::encodeFrameFromPacket(frameBuffer, miPaquete);

    // 5. Enviar el frame binario
    radio->enviar(frameBuffer, WSNFrame::FRAME_SIZE);
    
    // --- IMPRESIÓN MODIFICADA ---
    Serial.print("Enviando -> ");
    Serial.print("ID: "); Serial.print(miPaquete.id);
    Serial.print(" V: "); Serial.print(voltage, 2);
    Serial.print(" I: "); Serial.print(corriente, 3);
    Serial.print(" VBat: "); Serial.println(vbat, 2);
  }

  // --- Recepción de comandos (sin cambios) ---
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

float leerVoltajeBateria() {
  int lectura = analogRead(VBAT_PIN);
  float vEsc = (lectura * 5.0) / 1023.0;
  return vEsc * 3.0;
}