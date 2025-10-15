#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <EEPROM.h>

// --- Configuración de la EEPROM ---
// Se define un tamaño para guardar los datos. 32 bytes es suficiente para "Hola Mundo!".
#define EEPROM_SIZE 32

// Configuración de los pines CE y CSN para el Nano
RF24 radio(9, 10); // CE, CSN

// Dirección del "pipe" o tubería por donde se comunicarán. Debe ser la misma en ambos nodos.
const byte address[6] = "00001";

void setup() {
  Serial.begin(9600); 
  Serial.println("Nodo Emisor (Sensor) con EEPROM Iniciado");

  // Leer y mostrar el último mensaje guardado en la EEPROM al arrancar
  char ultimoMensaje[EEPROM_SIZE] = "";
  for (int i = 0; i < EEPROM_SIZE; i++) {
    ultimoMensaje[i] = EEPROM.read(i);
  }
  Serial.print("Ultimo mensaje enviado (guardado en EEPROM): ");
  Serial.println(ultimoMensaje);

  if (!radio.begin()) {
    Serial.println(F("El módulo de radio no responde, verifique las conexiones."));
    while (1); // Bucle infinito si falla
  }
  
  // Descomentar la siguiente línea para ver detalles de la configuración de la radio
  // radio.printDetails();

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

    // --- Guardado en EEPROM ---
    Serial.println("Guardando mensaje en EEPROM...");
    for (int i = 0; i < sizeof(text); i++) {
      // Usamos update() para escribir solo si el valor ha cambiado,
      // lo cual alarga la vida útil de la EEPROM.
      EEPROM.update(i, text[i]);
    }
    Serial.println("Mensaje guardado en EEPROM.");

  } else {
    Serial.println("Fallo al enviar el mensaje.");
  }
  
  delay(1000); // Espera un segundo antes de volver a enviar
}
