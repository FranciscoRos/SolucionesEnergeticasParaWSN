#include <EEPROM.h>

// IMPORTANTE: Usa el mismo tamaño que en tu programa receptor.
#define EEPROM_SIZE 32

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Esperar a que el puerto serie se conecte
  }

  Serial.println("Iniciando reseteo de EEPROM...");

  // Inicia la EEPROM con el tamaño definido
  EEPROM.begin(EEPROM_SIZE);

  // Recorre cada una de las celdas de la memoria que hemos reservado
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }

  // Guarda los cambios en la memoria flash
  if (EEPROM.commit()) {
    Serial.println("¡EEPROM reseteada con éxito!");
  } else {
    Serial.println("Error al guardar los cambios en la EEPROM.");
  }

  Serial.println("Por favor, vuelve a cargar tu programa principal (Receptor).");
}

void loop() {
  // No es necesario hacer nada en el loop.
}

