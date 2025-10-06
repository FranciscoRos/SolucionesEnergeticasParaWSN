#define RXD2 16  // Conectado al TX del XBee
#define TXD2 17  // Conectado al RX del XBee

float voltaje = 0.0;
float corriente = 0.0;

void setup() {
  Serial.begin(115200); // Comunicación con PC
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // UART2 para XBee
  Serial.println("Nodo Coordinador ESP32 iniciado");
  Serial.println("encendido  Nodo Coordinador Energy\n");
}

void loop() {
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.trim(); // Limpia espacios y saltos

    // Validar que sea un paquete válido tipo: V:xx.xx I:yy.yy
    if (data.indexOf('V') != -1 && data.indexOf('I') != -1) {
      Serial.println(" Datos recibidos: " + data);

      int idxV = data.indexOf('V');
      int idxI = data.indexOf('I');

      float v = data.substring(idxV + 2, idxI - 1).toFloat();
      float i = data.substring(idxI + 2).toFloat();

      voltaje = v;
      corriente = i;

      // Lógica de decisión
      if (voltaje < 190 || voltaje > 250 || corriente < 0.1) {
        Serial2.println("OFF");
        Serial.println(">>  Enviando comando: OFF");
      } else {
        Serial2.println("ON");
        Serial.println(">>  Enviando comando: ON");
      }
    } else {
      // Si no es un dato con formato válido, lo ignoramos
    }
  }

  delay(1000); // Espera para evitar saturar la UART
}

