#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include "RTClib.h"

// --- Configuración de la Radio (SPI) ---
RF24 radio(9, 10); // CE, CSN
// Direcciones: [0] para escribir, [1] para leer (invertido respecto al receptor)
const byte address[2][6] = {"00001", "00002"};

// --- Configuración del Reloj (I2C) ---
RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  
  // 1. Iniciar el RTC
  if (!rtc.begin()) {
    Serial.println("No se pudo encontrar el RTC!");
    while (1);
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // 2. Iniciar la Radio
  radio.begin();
  radio.powerDown(); // Iniciar con la radio apagada
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MIN);

  // IMPORTANTE: Configurar ambos pipes
  radio.openWritingPipe(address[0]);  // Pipe para ENVIAR la hora
  radio.openReadingPipe(1, address[1]); // Pipe para RECIBIR el ping

  Serial.println("Emisor Configurado");
}

void loop() {
  // === FASE DE ENVÍO ===
  radio.powerUp();
  delay(5); // Esperar a que el oscilador se estabilice

  radio.stopListening(); // Salir del modo escucha para poder escribir

  // Formatear la hora
  DateTime now = rtc.now();
  char mensaje[50];
  sprintf(mensaje, "%02d/%02d/%04d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());

  // Enviar el mensaje
  if (radio.write(&mensaje, sizeof(mensaje))) {
    Serial.print("Enviado: ");
    Serial.println(mensaje);
  } else {
    Serial.println("Fallo al enviar.");
  }

  // === FASE DE ESCUCHA ===
  radio.startListening(); // Entrar en modo escucha para recibir el "Ping"
  
  unsigned long started_waiting_at = millis();
  bool timeout = false;
  // Escuchar por un breve momento (ej. 100ms)
  while (!radio.available() && !timeout) {
    if (millis() - started_waiting_at > 100) {
      timeout = true;
    }
  }

  if (!timeout) {
    char ping[32] = "";
    radio.read(&ping, sizeof(ping));
    Serial.print("Ping del receptor: "); Serial.println(ping);
  }
  
  // === FASE DE APAGADO ===
  Serial.println("Radio apagada. Durmiendo...");
  radio.powerDown();
  
  delay(4000); // Esperar 4 segundos
}