#include <CodecWSN.h>
#include <SoftwareSerial.h>

SoftwareSerial xbeeSerial(2, 3); // RX=D2, TX=D3

uint32_t paquetesEnviados = 0;

void setup() {
  Serial.begin(9600);
  xbeeSerial.begin(9600);
}

void loop() {
  // Simulación de lecturas
  float voltage = 5.12;
  float corriente = 0.123;
  float vbat = 7.40;

  Packet p;
  p.id        = ++paquetesEnviados;
  p.voltaje   = (int16_t)(voltage * 100);
  p.corriente = (int16_t)(corriente * 1000);
  p.vbat      = (uint16_t)(vbat * 100);

  uint8_t buf[8];
  encodePacket(buf, p);
  xbeeSerial.write(buf, sizeof(buf));

  Serial.print("Enviado Paquete "); Serial.println(p.id);

  delay(3000);
}
