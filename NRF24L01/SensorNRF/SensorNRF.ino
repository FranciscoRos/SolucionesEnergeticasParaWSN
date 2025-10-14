#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Configuración de los pines CE y CSN para el Nano
RF24 radio(9, 10); // CE, CSN

// Dirección del "pipe" o tubería por donde se comunicarán. Debe ser la misma en ambos nodos.
const byte address[6] = "00001";

void setup() {
  Serial.begin(9600); 
  Serial.println("Nodo Emisor");

  radio.begin();
  radio.printDetails();

  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  
  // Configura el pipe para la escritura (transmisión)
  radio.openWritingPipe(address);
  
  // Establece el nivel de potencia al mínimo para pruebas cercanas.
  // Puedes usar RF24_PA_MAX para mayor alcance.
  radio.setPALevel(RF24_PA_MIN);
  
  // Detiene la escucha para prepararse a enviar datos
  radio.stopListening();
}

void loop() {
  const char text[] = "Hola Mundo!";
  
  bool ok = radio.write(&text, sizeof(text));
  
  if (ok) {
    Serial.println("Mensaje enviado con éxito.");
  } else {
    Serial.println("Fallo al enviar el mensaje.");
  }
  
  delay(1000);
}