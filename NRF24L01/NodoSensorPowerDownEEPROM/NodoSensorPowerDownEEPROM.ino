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
RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

// --- Configuración del Reloj (I2C) ---
RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  
  // 1. Iniciar la EEPROM y leer el último dato
  // No es necesario EEPROM.begin() para Arduino Nano/Uno
  Serial.println("Nodo Emisor PowerDown con EEPROM Iniciado");
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

  // Si el reloj perdió energía, ajústalo a la fecha y hora de compilación
  if (rtc.lostPower()) {
    Serial.println("RTC sin energía, ajustando a la hora de compilación.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // 3. Iniciar la Radio
  if (!radio.begin()) {
    Serial.println("Fallo al iniciar la radio. Revise las conexiones.");
    while(1);
  }
  radio.powerDown(); //Se apaga por defecto para ahorrar energía
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  
  Serial.println("Emisor listo para enviar la hora.");
}

void loop() {
  Serial.println("Radio encendida para transmitir...");
  radio.powerUp();
  delay(5); // Pequeña pausa para que el módulo se estabilice

  // 1. Leer la hora actual del RTC
  DateTime now = rtc.now();
  // 2. Formatear la hora en un texto para enviarla
  char mensaje[EEPROM_SIZE];
  sprintf(mensaje, "%02d/%02d/%04d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());

  // 3. Enviar el mensaje a través de la radio
  bool ok = radio.write(&mensaje, sizeof(mensaje));

  if (ok) {
    Serial.print("Enviado: ");
    Serial.println(mensaje);
    
    // --- Guardado en EEPROM ---
    Serial.println("Guardando mensaje en EEPROM...");
    for (int i = 0; i < sizeof(mensaje); i++) {
      EEPROM.update(i, mensaje[i]);
    }
    Serial.println("Mensaje guardado.");

  } else {
    Serial.println("Fallo al enviar.");
  }

  Serial.println("Radio apagada. Entrando en modo de bajo consumo.");
  radio.powerDown();
  
  delay(5000); // Enviar la hora cada 5 segundos
}
