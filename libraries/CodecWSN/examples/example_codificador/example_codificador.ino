#include <Arduino.h>
#include <CodecWSN.h>
// #include <UniversalRadioWSN.h> // ¡Esta librería se usaría aquí!

// LoraRadio miRadio(config); // (Ejemplo de la radio)

void setup() {
  Serial.begin(9600);
  // miRadio.iniciar();
}

void loop() {
  // 1. Crear el paquete (la carga útil)
  Packet miPaquete;
  miPaquete.id = 0x01; // Nodo 1
  miPaquete.voltaje = 1234;  // Representa 12.34V
  miPaquete.corriente = 56;    // Representa 56mA
  miPaquete.vbat = 395;    // Representa 3.95V

  // 2. Crear un buffer para la trama completa
  uint8_t bufferFrame[WSNFrame::FRAME_SIZE]; // FRAME_SIZE es 14 bytes

  // 3. Codificar el paquete en la trama (añade SOF, CRC, etc.)
  WSNFrame::encodeFrameFromPacket(bufferFrame, miPaquete);

  // 4. Enviar la trama binaria (14 bytes)
  // Se puede enviar por Serial...
  Serial.write(bufferFrame, WSNFrame::FRAME_SIZE);
  
  // ...o (más comúnmente) enviarlo por la radio:
  // miRadio.enviar(bufferFrame, WSNFrame::FRAME_SIZE);

  Serial.println("Trama de 14 bytes enviada.");
  delay(5000);
}