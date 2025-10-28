#include <SPI.h> // Necesario para LoRa y NRF
#include <UniversalRadioWSN.h>

// 1. Puntero a la interfaz.
//    Este puntero "apuntará" al objeto de radio que creemos en setup().
RadioInterface* radio;

long contadorMensajes = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("--- EMISOR BÁSICO INICIADO ---");

  // 2. Aquí ocurre la magia:
  //    Definimos la configuración para la radio que queremos usar.
  //    En este ejemplo, usamos LoRa.
  
  LoRaConfig configLora;
  configLora.frequency       = 410E6;
  configLora.spreadingFactor = 7;
  configLora.signalBandwidth = 125E3;
  configLora.codingRate      = 5;
  configLora.syncWord        = 0xF3;
  configLora.txPower         = 20;
  configLora.csPin           = 10;
  configLora.resetPin        = -1;
  configLora.irqPin          = 2;
  
  // 3. Creamos la instancia CONCRETA y la asignamos al puntero de INTERFAZ.
  radio = new LoraRadio(configLora);

  /* * === PARA CAMBIAR A NRF24L01 ===
   * Simplemente comenta las 3 líneas de LoRa (config, new) y descomenta estas:
   *
   * const byte nrfAddr[6] = "00001";
   * NrfConfig configNrf;
   * configNrf.cePin = 9;  configNrf.csnPin = 10;
   * configNrf.writeAddress = nrfAddr; configNrf.readAddress = nrfAddr;
   * configNrf.channel = 108; configNrf.dataRate = 250; configNrf.paLevel = 0;
   * radio = new NrfRadio(configNrf);
   */

  // 4. Inicializamos la radio (sin importar cuál sea)
  if (!radio->iniciar()) {
    Serial.println("¡ERROR: Fallo al iniciar el módulo de radio!");
    while (true);
  }
  Serial.println("Radio inicializada correctamente.");
}

void loop() {
  // Esperar 3 segundos
  delay(3000);

  // Incrementar contador y crear un mensaje simple
  contadorMensajes++;
  String mensaje = "Hola Mundo! Mensaje #" + String(contadorMensajes);

  // 5. Enviar el mensaje usando la interfaz.
  //    El código de loop() NO SABE si está usando LoRa, NRF o XBee.
  //    Simplemente funciona.
  
  Serial.print("Enviando: '");
  Serial.print(mensaje);
  Serial.println("'");
  
  radio->enviar(mensaje + "\n"); // Añadimos '\n' como delimitador
}