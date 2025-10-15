#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <EEPROM.h>

// --- Configuración de la EEPROM ---
// Se define un tamaño para guardar los datos. 32 bytes para el mensaje.
#define EEPROM_SIZE 32

// --- Configuración de los pines CE y CSN para el ESP32 ---
RF24 radio(4, 5); // CE, CSN

// Dirección del pipe. Debe ser la misma que en el emisor.
const byte address[6] = "00001";

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Nodo Coordinador ESP32 con EEPROM Iniciado");

  // Iniciar la EEPROM con el tamaño definido
  EEPROM.begin(EEPROM_SIZE);
  
  // Leer y mostrar el último mensaje guardado en la EEPROM al arrancar
  char ultimoMensaje[EEPROM_SIZE] = "";
  for (int i = 0; i < EEPROM_SIZE; i++) {
    ultimoMensaje[i] = EEPROM.read(i);
  }
  Serial.print("Ultimo mensaje guardado en EEPROM: ");
  Serial.println(ultimoMensaje);

  // Iniciar el módulo NRF24L01
  if (!radio.begin()) {
    Serial.println(F("El módulo de radio no responde, verifique las conexiones."));
    while (1) {} // Bucle infinito si falla
  }

  // Descomentar la siguiente línea para ver detalles de la configuración de la radio
  // radio.printDetails();

  // --- Configuración del módulo de radio ---
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  
  radio.startListening(); // Poner el módulo en modo escucha
  Serial.println("Escuchando mensajes...");
}

void loop() {
  // Comprueba si hay datos disponibles para leer
  if (radio.available()) {
    char text[32] = ""; // Crea un buffer para almacenar el texto recibido
    radio.read(&text, sizeof(text));
    
    Serial.print("Mensaje recibido: ");
    Serial.println(text);

    // --- Guardado en EEPROM ---
    Serial.println("Guardando mensaje en EEPROM...");
    for (int i = 0; i < sizeof(text); i++) {
      EEPROM.write(i, text[i]);
    }
    // Es crucial llamar a commit() para que los cambios se guarden en la memoria flash del ESP32
    if (EEPROM.commit()) {
      Serial.println("Mensaje guardado exitosamente en EEPROM.");
    } else {
      Serial.println("Error al guardar en EEPROM.");
    }
  }
}
