#include <EEPROM.h>       // Librería para la EEPROM
#include <SoftwareSerial.h> // Librería para comunicación serial por software

// --- Definiciones para la EEPROM ---
#define ADDR_ESTADO 0 // Dirección para guardar el estado del relevador

// --- Configuración de Pines ---
SoftwareSerial xbeeSerial(2, 3); // RX, TX para el módulo XBee
#define RELAY_PIN 4
#define ACS_PIN A0
#define ZMPT_PIN A1

// --- Variables Globales ---
unsigned long previousMillis = 0;
const long interval = 3000; // Intervalo de envío de datos (3 segundos)
bool estadoRelevador = LOW; // Variable para el estado actual del relevador (LOW/false = OFF)

void setup() {
  pinMode(RELAY_PIN, OUTPUT);

  // --- Recuperación del estado desde la EEPROM ---
  // Lee el último estado guardado en la memoria EEPROM.
  estadoRelevador = EEPROM.read(ADDR_ESTADO);
  // Aplica el estado recuperado al relevador físico.
  digitalWrite(RELAY_PIN, estadoRelevador);

  xbeeSerial.begin(9600); // Inicia comunicación con XBee
  Serial.begin(9600);     // Inicia comunicación con el monitor serial de la PC

  Serial.println("Nodo esclavo listo");
  Serial.print("Estado inicial del relevador recuperado: ");
  Serial.println(estadoRelevador == HIGH ? "ENCENDIDO" : "APAGADO");
}

void loop() {
  // --- Envío periódico de datos de sensores ---
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float voltage = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();

    // Envía los datos al coordinador a través del XBee
    xbeeSerial.print("V:"); xbeeSerial.print(voltage, 2);
    xbeeSerial.print(" I:"); xbeeSerial.println(corriente, 2);

    // Muestra los datos enviados en el monitor serial para depuración
    Serial.print(" Enviado -> V: "); Serial.print(voltage, 2);
    Serial.print(" | I: "); Serial.println(corriente, 2);
  }

  // --- Escucha de comandos del coordinador ---
  if (xbeeSerial.available()) {
    String comando = xbeeSerial.readStringUntil('\n');
    comando.trim();

    Serial.print("Comando recibido: "); Serial.println(comando);

    bool nuevoEstado = estadoRelevador;

    // Determina el nuevo estado basado en el comando
    if (comando == "ON") {
      nuevoEstado = HIGH;
    } else if (comando == "OFF") {
      nuevoEstado = LOW;
    } else {
      Serial.print("Comando desconocido: "); Serial.println(comando);
      return; // Si el comando no es válido, no hace nada más
    }

    if (nuevoEstado != estadoRelevador) {
      estadoRelevador = nuevoEstado; 
      digitalWrite(RELAY_PIN, estadoRelevador);

      // Guarda el nuevo estado en la EEPROM para persistencia
      EEPROM.write(ADDR_ESTADO, estadoRelevador);

      Serial.print("Relevador ");
      Serial.println(estadoRelevador == HIGH ? "ENCENDIDO" : "APAGADO");
      Serial.println(">> Nuevo estado guardado en EEPROM.");
    }
  }
}

// --- Funciones de Lectura de Sensores ---

// Lectura simple del sensor de voltaje ZMPT101B
float leerVoltajeZMPT() {
  int lectura = analogRead(ZMPT_PIN);
  float voltaje = (lectura * 5.0) / 1023.0;
  return voltaje * 50.0;
}

// Lectura simple del sensor de corriente ACS712
float leerCorrienteACS() {
  int lectura = analogRead(ACS_PIN);
  float voltaje = (lectura * 5.0) / 1023.0;
  float corriente = (voltaje - 2.5) / 0.066;
  return abs(corriente);
}
