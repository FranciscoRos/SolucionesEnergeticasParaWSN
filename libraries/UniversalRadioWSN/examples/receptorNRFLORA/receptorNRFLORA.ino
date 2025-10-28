
#include <SPI.h> // Necesario para LoRa y NRF
#include <UniversalRadioWSN.h>

// 1. Puntero a la interfaz.
RadioInterface* radio;

void setup() {
  Serial.begin(115200); // Usamos una velocidad alta para el receptor
  while (!Serial);
  Serial.println("--- RECEPTOR BÁSICO INICIADO ---");

  // 2. Definimos la configuración de radio.
  //    Debe coincidir con la configuración del emisor.
  
  LoRaConfig configLora;
  configLora.frequency       = 410E6;
  configLora.spreadingFactor = 7;
  configLora.signalBandwidth = 125E3;
  configLora.codingRate      = 5;
  configLora.syncWord        = 0xF3;
  configLora.txPower         = 20;
  configLora.csPin           = 5;  // <-- Pin CS del receptor
  configLora.irqPin          = 2;  // <-- Pin IRQ del receptor
  configLora.resetPin        = -1;
  
  // 3. Creamos la instancia y la asignamos al puntero
  radio = new LoraRadio(configLora);

  /* * === PARA CAMBIAR A NRF24L01 ===
   * (Asegúrate de que las direcciones de lectura/escritura 
   * estén invertidas respecto al emisor si es necesario)
   *
   * const byte nrfAddr[6] = "00001";
   * NrfConfig configNrf;
   * configNrf.cePin = 4;  configNrf.csnPin = 5; // <-- Pines del receptor
   * configNrf.writeAddress = nrfAddr; configNrf.readAddress = nrfAddr;
   * configNrf.channel = 108; configNrf.dataRate = 250; configNrf.paLevel = 0;
   * radio = new NrfRadio(configNrf);
   */

  // 4. Inicializamos la radio
  if (!radio->iniciar()) {
    Serial.println("¡ERROR: Fallo al iniciar el módulo de radio!");
    while (true);
  }
  Serial.println("Radio inicializada. Esperando mensajes...");
}

void loop() {
  
  // 5. La lógica de recepción universal:
  //    Preguntar si hay un paquete disponible.
  
  if (radio->hayDatosDisponibles()) {
    
    // Si hay, leerlo como un String
    String mensaje = radio->leerComoString();
    mensaje.trim(); // Limpiar espacios en blanco o saltos de línea

    Serial.print("Mensaje Recibido: '");
    Serial.print(mensaje);
    Serial.print("'");
    
    // También podemos obtener el RSSI (si la radio lo soporta)
    Serial.print(" | RSSI: ");
    Serial.println(radio->obtenerRSSI());
  }
  
  // No se necesita delay, la librería maneja la escucha.
}
