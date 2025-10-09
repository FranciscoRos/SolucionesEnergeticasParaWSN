#pragma once
#include <stdint.h>

struct DataPayload {
  // --- Marca de tiempo ---
  uint32_t unixTime;      // Tiempo desde el RTC

  // --- Mediciones Electricas ---
  float voltageAC;        // Voltaje de linea (V)
  float currentAC;        // Corriente de linea (A)
  float powerApparent;    // Potencia Aparente (VA)

  // --- Estado del Nodo ---
  float batteryVoltage;   // Voltaje de la bateria del nodo (V)
  uint8_t energyLevel;    // Nivel de energia (0:LOW, 1:MID, 2:HIGH)
} __attribute__((packed));