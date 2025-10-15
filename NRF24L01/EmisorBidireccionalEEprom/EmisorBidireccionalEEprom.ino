#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include "RTClib.h"
#include <EEPROM.h>

// --- Configuración de la EEPROM ---
// Se define un tamaño para guardar los datos. 50 bytes para el mensaje de la hora.
#define EEPROM_SIZE 50

// --- Configuración de la Radio (SPI) ---
RF24 radio(9, 10); // Pines CE, CSN para Arduino (ajustar si es otro microcontrolador)
// Direcciones: [0] para escribir, [1] para leer (invertido respecto al receptor)
const byte address[2][6] = {"00001", "00002"};

// --- Configuración del Reloj (I2C) ---
RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  
  // 1. Iniciar la EEPROM
  EEPROM.begin(EEPROM_SIZE);
  // Leer y mostrar el último mensaje guardado en la EEPROM al arrancar
  char ultimoMensaje[EEPROM_SIZE] = "";
  for (int i = 0; i < EEPROM_SIZE; i++) {
    ultimoMensaje[i] = EEPROM.read(i);
  }
  Serial.print("Ultimo mensaje enviado (guardado en EEPROM): ");
  Serial.println(ultimoMensaje);

  // 2. Iniciar el RTC
  if (!rtc.begin()) {
    Serial.println("No se pudo encontrar el RTC!");
    while (1);
  }
  if (rtc.lostPower()) {
    // Si el RTC perdió energía, se ajusta a la hora de compilación del sketch.
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // 3. Iniciar la Radio
  if (!radio.begin()) {
    Serial.println(F("El módulo de radio no responde, verifique las conexiones."));
    while (1); // Bucle infinito si falla
  }
  radio.powerDown(); // Iniciar con la radio apagada para ahorrar energía
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MIN);

  // IMPORTANTE: Configurar ambos pipes para la comunicación bidireccional
  radio.openWritingPipe(address[0]);   // Pipe para ENVIAR la hora
  radio.openReadingPipe(1, address[1]); // Pipe para RECIBIR el ping de confirmación

  Serial.println("Emisor Configurado y listo.");
}

void loop() {
  // === FASE DE ENVÍO ===
  radio.powerUp(); // Encender la radio
  delay(5); // Esperar un momento a que el oscilador de la radio se estabilice

  radio.stopListening(); // Salir del modo escucha para poder escribir

  // Obtener la fecha y hora actual del RTC
  DateTime now = rtc.now();
  char mensaje[EEPROM_SIZE];
  // Formatear la fecha y hora en un string
  sprintf(mensaje, "%02d/%02d/%04d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());

  // Enviar el mensaje
  if (radio.write(&mensaje, sizeof(mensaje))) {
    Serial.print("Enviado: ");
    Serial.println(mensaje);
    
    // --- Guardado en EEPROM ---
    Serial.println("Guardando mensaje en EEPROM...");
    for (int i = 0; i < sizeof(mensaje); i++) {
      EEPROM.write(i, mensaje[i]);
    }
    if (EEPROM.commit()) {
      Serial.println("Mensaje guardado exitosamente en EEPROM.");
    } else {
      Serial.println("Error al guardar en EEPROM.");
    }

  } else {
    Serial.println("Fallo al enviar el mensaje.");
  }

  // === FASE DE ESCUCHA ===
  radio.startListening(); // Entrar en modo escucha para recibir el "Ping" del receptor
  
  unsigned long started_waiting_at = millis();
  bool timeout = false;
  // Escuchar por un breve momento (ej. 100ms) para no bloquear el loop
  while (!radio.available() && !timeout) {
    if (millis() - started_waiting_at > 100) {
      timeout = true;
    }
  }

  if (!timeout) {
    char ping[32] = "";
    radio.read(&ping, sizeof(ping));
    Serial.print("Confirmacion (Ping) del receptor: "); 
    Serial.println(ping);
  } else {
    Serial.println("No se recibio confirmacion del receptor.");
  }
  
  // === FASE DE APAGADO ===
  Serial.println("Radio apagada. Durmiendo...");
  radio.powerDown(); // Poner la radio en modo de bajo consumo
  
  delay(4000); // Esperar 4 segundos antes del siguiente ciclo
}
