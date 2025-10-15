#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include "RTClib.h"

// --- Configuración de la Radio (SPI) ---
RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

// --- Configuración del Reloj (I2C) ---
RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  
  // 1. Iniciar el RTC
  if (!rtc.begin()) {
    Serial.println("No se pudo encontrar el RTC!");
    while (1);
  }

  // Si el reloj perdió energía, ajústalo a la fecha y hora de compilación
  if (rtc.lostPower()) {
    Serial.println("RTC sin energía, ajustando a la hora de compilación.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // 2. Iniciar la Radio
  radio.begin();
  radio.powerDown(); //Se apaga por defecto para ahorrar energía
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  
  Serial.println("Emisor listo para enviar la hora.");
}

void loop() {
  Serial.println("Radio encendida");
  radio.powerUp();
  delay(5);

  // 1. Leer la hora actual del RTC
  DateTime now = rtc.now();
  // 2. Formatear la hora en un texto para enviarla
  char mensaje[50];
  sprintf(mensaje, "%02d/%02d/%04d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());

  // 3. Enviar el mensaje a través de la radio
  bool ok = radio.write(&mensaje, sizeof(mensaje));

  if (ok) {
    Serial.print("Enviado: ");
    Serial.println(mensaje);
  } else {
    Serial.println("Fallo al enviar.");
  }
  Serial.println("Radio apagada");
  radio.powerDown();
  
  delay(5000); // Enviar la hora cada 5 segundos
}