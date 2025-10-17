/*
 * ==========================================================
 * ==     SKETCH EMISOR UNIVERSAL (LoRa + XBee Preparado)    ==
 * ==========================================================
 * Este sketch usa LoRa por defecto, pero tiene todo el código
 * necesario para cambiar a XBee con solo modificar una línea.
 */

// --- LIBRERÍAS DE LA APLICACIÓN ---
#include <SPI.h>
#include <UniversalRadioWSN.h>
#include <SoftwareSerial.h> // Se incluye para la compatibilidad con XBee

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
  Serial.println("\n--- INICIANDO EMISOR UNIVERSAL ---");

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
    // --- ESTE BLOQUE ESTÁ LISTO PERO INACTIVO ---
    Serial.println("XBee");
    xbeeSerial.begin(9600); // Inicia el puerto serial para el XBee
    // Crea la instancia de radio para XBee
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
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= INTERVAL_MS) {
    previousMillis = currentMillis;

    float voltage = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();
    float vbat = leerVoltajeBateria();
    paquetesEnviados++;

    String dataPayload = "N:" + String(paquetesEnviados) +
                         " V:" + String(voltage, 2) +
                         " I:" + String(corriente, 2) +
                         " B:" + String(vbat, 2);

    radio->enviar(dataPayload + "\n");
    
    Serial.print("Enviado: ");
    Serial.println(dataPayload);
  }

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