#include <SoftwareSerial.h>

SoftwareSerial xbeeSerial(2, 3); // RX, TX para XBee

#define RELAY_PIN 4
#define ACS_PIN A0
#define ZMPT_PIN A1

unsigned long previousMillis = 0;
const long interval = 3000; // cada 3 segundos

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Relevador apagado inicialmente

  xbeeSerial.begin(9600);
  Serial.begin(9600);

  Serial.println("Nodo esclavo listo");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float voltage = leerVoltajeZMPT();
    float corriente = leerCorrienteACS();

    // Envío por XBee
    xbeeSerial.print("V:"); xbeeSerial.print(voltage, 2);
    xbeeSerial.print(" I:"); xbeeSerial.println(corriente, 2);

    Serial.print(" Enviado -> V: "); Serial.print(voltage, 2);
    Serial.print(" | I: "); Serial.println(corriente, 2);
  }

  // Escuchar comandos del coordinador
  if (xbeeSerial.available()) {
    String comando = xbeeSerial.readStringUntil('\n');
    comando.trim(); // Elimina espacios

    Serial.print(" Comando recibido: "); Serial.println(comando);

    if (comando == "ON") {
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println(" Relevador ENCENDIDO");
    } else if (comando == "OFF") {
      digitalWrite(RELAY_PIN, LOW);
      Serial.println(" Relevador APAGADO");
    } else {
      Serial.print(" Comando desconocido: "); Serial.println(comando);
    }
  }
}

// Lectura simple del ZMPT101B
float leerVoltajeZMPT() {
  int lectura = analogRead(ZMPT_PIN);
  float voltaje = (lectura * 5.0) / 1023.0;
  return voltaje * 50.0; // Aproximación para escalar a voltaje real
}

// Lectura simple del ACS712
float leerCorrienteACS() {
  int lectura = analogRead(ACS_PIN);
  float voltaje = (lectura * 5.0) / 1023.0;
  float corriente = (voltaje - 2.5) / 0.066; // Para ACS712 30A
  return corriente;
}

