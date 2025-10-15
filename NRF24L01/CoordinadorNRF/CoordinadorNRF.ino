#include <SPI.h>
#include <nRF24L01.h>

// Configuración de los pines CE y CSN para el ESP32
RF24 radio(4, 5); // CE, CSN

// Dirección del pipe. Debe ser la misma que en el emisor.
const byte address[6] = "00001";

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Nodo Receptor ESP32 Iniciado");

  radio.begin();
  radio.printDetails();

  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  Serial.println("Escuchando mensajes...");
}

void loop() {
  // Comprueba si hay datos disponibles para leer
  if (radio.available()) {
    char text[32] = ""; // Crea un buffer para almacenar el texto recibido
    radio.read(&text, sizeof(text));
    
    Serial.print("Mensaje recibido: ");
    Serial.println(text);
  }
}