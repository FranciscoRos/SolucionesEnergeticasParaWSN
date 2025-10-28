#include <Wire.h>
#include <DHT.h> // O la librería para tu sensor de temp

// ... (elimina #include <Adafruit_INA219.h>)

// Pines para los ACS712
#define PANEL_CURRENT_PIN A2
#define NODO_CURRENT_PIN  A3

// Pines para los Divisores de Voltaje
#define BATERIA_VOLTAGE_PIN A0
#define PANEL_VOLTAGE_PIN A1

// CALIBRACIÓN DEL ACS712 (esto es lo más importante)
// Necesitas medir el valor que da el pin OUT cuando NO pasa corriente.
// Debe ser ~2.5V, que en el ADC es ~512.
const int ZERO_CURRENT_OFFSET = 512; 

// Sensibilidad del tu ACS712 (revisa tu modelo)
// 5A:  185mV por Amperio (0.185V)
// 20A: 100mV por Amperio (0.100V)
// 30A: 66mV por Amperio (0.066V)
const float SENSITIVIDAD_MV_POR_AMP = 100.0; // Ejemplo para el de 20A

void setup() {
  Serial.begin(115200);
  // ... (otros setup)
  Serial.println("Timestamp,Voltaje_Panel,Corriente_Panel_mA,Voltaje_Bateria,Corriente_Nodo_mA,Irradiancia_Solar,Temperatura");
}

float leerCorrienteACS(int pin) {
  int sensorValue = analogRead(pin);
  
  // Convertir lectura (0-1023) a voltaje (0V-5V)
  float voltajeSalida = sensorValue * (5.0 / 1023.0);
  
  // Convertir voltaje de salida a Corriente en Amperios
  // (Voltaje - Offset_de_2.5V) / Sensibilidad
  float corriente_A = (voltajeSalida - 2.5) / (SENSITIVIDAD_MV_POR_AMP / 1000.0);
  
  return corriente_A * 1000.0; // Devolver en mA
}

float leerVoltajeDivisor(int pin, float factorDivisor) {
  // factorDivisor = (R1 + R2) / R2. 
  // Ej: R1=10K, R2=10K -> (10+10)/10 = 2.0
  int sensorValue = analogRead(pin);
  float voltajeMedido = sensorValue * (5.0 / 1023.0);
  return voltajeMedido * factorDivisor;
}

void loop() {
  // 1. Leer Panel
  // Factor 4.0 es un ejemplo si usas R1=30K, R2=10K para medir ~20V
  float voltajePanel = leerVoltajeDivisor(PANEL_VOLTAGE_PIN, 4.0); 
  float corrientePanel_mA = leerCorrienteACS(PANEL_CURRENT_PIN);

  // 2. Leer Consumo Nodo
  float corrienteNodo_mA = leerCorrienteACS(NODO_CURRENT_PIN);
  
  // 3. Leer Voltaje Batería
  // Factor 2.0 es ejemplo si usas R1=10K, R2=10K para medir ~5V
  float voltajeBateria = leerVoltajeDivisor(BATERIA_VOLTAGE_PIN, 2.0); 

  // ... (Leer LDR y Temperatura) ...
  
  // Imprimir datos
  Serial.print(millis());
  Serial.print(",");
  Serial.print(voltajePanel);
  Serial.print(",");
  Serial.print(corrientePanel_mA);
  Serial.print(",");
  Serial.print(voltajeBateria);
  Serial.print(",");
  Serial.print(corrienteNodo_mA);
  // ... (imprimir resto de datos)
  Serial.println();
  
  delay(5000); 
}