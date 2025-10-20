#include <EEPROM.h>

// No necesitas definir el tamaño para la compilación, 
// pero es buena práctica saber el tamaño de la EEPROM de tu placa.
// Por ejemplo, el Arduino Uno tiene 1024 bytes.
// const int EEPROM_SIZE = 1024; 

void setup() {
  Serial.begin(9600);
  Serial.println("Iniciando reseteo de EEPROM...");

  // Bucle para recorrer toda la memoria EEPROM y escribir un 0 en cada posición.
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }

  Serial.println("EEPROM borrada exitosamente.");
  // El programa puede terminar aquí.
}

void loop() {
  // No es necesario hacer nada en el loop.
}