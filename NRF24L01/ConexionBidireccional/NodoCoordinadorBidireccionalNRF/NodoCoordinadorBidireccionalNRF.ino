#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>

// Configuración de los pines CE y CSN para el ESP32
RF24 radio(4, 5); // CE, CSN

// Dirección del pipe. [0] para leer y [1] para escribir
const byte address[2][6] = {"00001","00002"};

unsigned long ultimoEnvio = 0;
const long intervalo = 5000;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Nodo Receptor ESP32 Iniciado");

  radio.begin();
  //radio.printDetails();

  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MIN);

  //Configuración de PIPES
  radio.openReadingPipe(1, address[0]);
  radio.openWritingPipe(address[1]);

  radio.startListening();
  Serial.println("Escuchando mensajes...");
}

void loop() {
  // Comprueba si hay datos disponibles para leer
  if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text));    
    Serial.print("Mensaje recibido: ");
    Serial.println(text);
    ultimoEnvio = millis(); //ultimoEnvio = 0
  }

  //Revisar si ya pasaron 5 segundos sin recibir nada
  if(millis() - ultimoEnvio > intervalo) {
    radio.stopListening(); // Pone el módulo en modo Standby-I

    Serial.println("No se recibieron datos. Enviando ping");
    const char text[] = "Ping";
    bool ok = radio.write(&text, sizeof(text));

    if (!ok) {
      Serial.println("Fallo al enviar el PING. Revisé el emisor");
    }

    radio.startListening();
    ultimoEnvio = millis();
  }
}