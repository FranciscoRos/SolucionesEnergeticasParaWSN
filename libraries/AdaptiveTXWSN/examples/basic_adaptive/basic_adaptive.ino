/**
 * @file basic_usage.ino
 * @brief Ejemplo básico de uso de la librería AdaptiveTXWSN.
 * @details Este sketch demuestra cómo configurar la librería para leer
 * el voltaje de una batería (ej. LiPo) desde un pin ADC (A0)
 * y usar el temporizador adaptativo para imprimir un mensaje
 * en el monitor serial en intervalos variables.
 */

#include <Arduino.h>
#include <AdaptiveTXWSN.h> // 1. Incluir la librería

// Crear una instancia del temporizador adaptativo
AdaptiveTXWSN txTimer;

void setup() {
  Serial.begin(9600);
  while (!Serial); // Esperar a que el monitor serial se conecte
  Serial.println("Iniciando AdaptiveTXWSN - Ejemplo Basico...");

  // 2. Definir la configuración
  AdaptiveTXWSN::Cfg config;

  // --- Configuración del Divisor de Voltaje ---
  // Asume: (Batería+) -> [100k] -> (A0) -> [33k] -> (GND)
  config.pinAdcBateria       = A0;
  config.divisorRArriba_k    = .01f; // No incluimos resistencias
  config.divisorRAbajo_k     = .99f;  // basta con 0 y 1 para evitar división por cero
  config.voltajeReferenciaAdc = 5.0f;   // Para Arduino Nano/UNO a 5V (usar 3.3f para placas de 3.3V)
  
  // --- Umbrales de Voltaje (ejemplo para LiPo) ---
  config.umbralAlto_V     = 3.90f; // Por encima de 3.9V = ALTO
  config.umbralMedio_V    = 3.60f; // Entre 3.6V y 3.9V = MEDIO
  config.corteVoltaje_V   = 3.40f; // Por debajo de 3.4V = CORTE (no enviar)

  // --- Períodos de Transmisión ---
  config.periodoAlto_ms   = 5000;   // 5 segundos (batería llena)
  config.periodoMedio_ms  = 15000;  // 15 segundos (batería media)
  config.periodoBajo_ms   = 120000; // 2 minutos (batería baja)

  // 3. Inicializar la librería
  txTimer.begin(config);
  
  Serial.println("Configuracion cargada. Iniciando loop...");
}

void loop() {
  
  // 4. Llamar a tick() en cada ciclo del loop
  //    Devolverá 'true' cuando sea momento de transmitir.
  
  if (txTimer.tick()) {
    
    // Transmitimos los datos
    
    Serial.print("EVENTO: Transmitir. ");
    Serial.print("VBat: ");
    Serial.print(txTimer.lastVolts()); // Obtener último voltaje medido
    Serial.print("V, Nivel: ");
    
    switch(txTimer.level()) { // Obtener nivel actual
      case AdaptiveTXWSN::BATT_HIGH: Serial.print("ALTO"); break;
      case AdaptiveTXWSN::BATT_MID:  Serial.print("MEDIO"); break;
      case AdaptiveTXWSN::BATT_LOW:  Serial.print("BAJO"); break;
    }
    
    Serial.print(", Proxima TX en: ");
    Serial.print(txTimer.currentPeriod() / 1000); // Obtener período actual
    Serial.println(" seg");

  } else {
    
    // Si la batería está en corte, tick() siempre devuelve 'false'.
    if (txTimer.isCutoff()) {
      Serial.println("ESTADO: Bateria en corte. No se transmitira.");
      delay(10000); // Simula una espera larga
    }
    
    // No es hora de transmitir.
    // En un nodo real, aquí es donde implementas el modo de sueño
    // por un corto período para ahorrar energía.
    // delay(10); 
  }
}