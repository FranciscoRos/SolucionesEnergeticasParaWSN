#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <EEPROM.h>

// --- Configuración de la EEPROM ---
// Se define un tamaño para guardar los datos. 32 bytes para el mensaje.
#define EEPROM_SIZE 32

// --- Configuración de los pines CE y CSN para el ESP32 ---
RF24 radio(4, 5); // Pines CE, CSN

// Dirección del pipe. [0] para leer y [1] para escribir
const byte address[2][6] = {"00001", "00002"};

// Variables para el control de tiempo del "ping"
unsigned long ultimoEnvio = 0;
const long intervalo = 5000; // 5 segundos

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Nodo Receptor ESP32 con EEPROM Iniciado");

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

  // --- Configuración del módulo de radio ---
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MIN);

  // --- Configuración de los PIPES de comunicación ---
  radio.openReadingPipe(1, address[0]); // Pipe para recibir datos
  radio.openWritingPipe(address[1]);  // Pipe para enviar datos

  radio.startListening(); // Poner el módulo en modo escucha
  Serial.println("Escuchando mensajes...");
}

void loop() {
  // Comprueba si hay datos disponibles para leer
  if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text));
    
    Serial.print("Mensaje recibido: ");
    Serial.println(text);

    // --- Guardado en EEPROM ---
    Serial.println("Guardando mensaje en EEPROM...");
    for (int i = 0; i < sizeof(text); i++) {
      EEPROM.write(i, text[i]);
    }
    // Es crucial llamar a commit() para que los cambios se guarden en la memoria flash
    if (EEPROM.commit()) {
      Serial.println("Mensaje guardado exitosamente en EEPROM.");
    } else {
      Serial.println("Error al guardar en EEPROM.");
    }

    ultimoEnvio = millis(); // Reinicia el contador para el ping
  }

  // Revisa si han pasado 5 segundos sin recibir nada para enviar un "ping"
  if (millis() - ultimoEnvio > intervalo) {
    radio.stopListening(); // Pone el módulo en modo Standby-I para poder enviar

    Serial.println("No se recibieron datos. Enviando ping de confirmacion.");
    const char text[] = "Ping";
    bool ok = radio.write(&text, sizeof(text));

    if (ok) {
      Serial.println("Ping enviado correctamente.");
    } else {
      Serial.println("Fallo al enviar el PING. Revise el emisor.");
    }

    radio.startListening(); // Vuelve a poner el módulo en modo escucha
    ultimoEnvio = millis(); // Reinicia el contador
  }
}
