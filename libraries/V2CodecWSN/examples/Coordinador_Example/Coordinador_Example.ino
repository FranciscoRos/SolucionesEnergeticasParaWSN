#include <CodecWSN.h>

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600); // ESP32: UART2
}

void loop() {
  if (Serial2.available() >= 8) {
    uint8_t buf[8];
    Serial2.readBytes(buf, 8);

    Packet p = decodePacket(buf);

    float voltaje = p.voltaje / 100.0;
    float corriente = p.corriente / 1000.0;
    float vbat = p.vbat / 100.0;

    Serial.print("ID: "); Serial.print(p.id);
    Serial.print(" | V: "); Serial.print(voltaje, 2);
    Serial.print(" | I: "); Serial.print(corriente, 3);
    Serial.print(" | B: "); Serial.println(vbat, 2);
  }
}
