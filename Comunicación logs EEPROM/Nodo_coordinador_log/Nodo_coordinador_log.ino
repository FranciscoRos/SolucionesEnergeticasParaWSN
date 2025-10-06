#include <time.h>
#include <EEPROM.h>

#define RXD2 16
#define TXD2 17
#define BAT_PIN 34 // Pin para medir la batería del propio receptor

#define EEPROM_SIZE 32 // Aumentado para guardar el contador y la estructura del paquete

// Estructura para guardar los datos relevantes de un paquete en la EEPROM
struct LastPacketData {
  uint16_t id;
  float voltaje;
  float corriente;
  float vbat_sensor;
  uint32_t timestamp;
};

uint32_t paquetesRecibidos = 0; // Contador de paquetes recibidos
LastPacketData lastPacket;      // Variable global para guardar los datos del último paquete

// Función para formatear el timestamp recibido
String formatUnixTime(uint32_t unixTime) {
  time_t time_utc = unixTime;
  struct tm *timeinfo;
  char buffer[30];
  timeinfo = gmtime(&time_utc);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}

// Función para leer la batería del nodo receptor
float leerVoltajeBateriaLocal() {
  int lecturaADC = analogRead(BAT_PIN);
  float voltajeADC = (lecturaADC / 4095.0) * 3.3;
  // Ajusta los valores de las resistencias de tu divisor de voltaje aquí
  return voltajeADC * ((10000.0 + 5000.0) / 5000.0); 
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  EEPROM.begin(EEPROM_SIZE);
  // Lee el contador desde la dirección 0
  EEPROM.get(0, paquetesRecibidos);
  // Lee la estructura del último paquete desde la dirección 4
  EEPROM.get(4, lastPacket);

  Serial.println("\n--- NODO RECEPTOR INICIADO (MODO TEXTO) ---");
  Serial.print("Total de paquetes recibidos previamente: ");
  Serial.println(paquetesRecibidos);
  Serial.println("--- Último paquete guardado en EEPROM ---");
  if (lastPacket.id > 0) { // Muestra los datos solo si hay algo guardado
      String fechaHoraLegible = formatUnixTime(lastPacket.timestamp);
      Serial.print("ID: "); Serial.print(lastPacket.id);
      Serial.print(" | Fecha: "); Serial.println(fechaHoraLegible);
      Serial.print("Voltaje: "); Serial.print(lastPacket.voltaje, 2);
      Serial.print(" | Corriente: "); Serial.print(lastPacket.corriente, 3);
      Serial.print(" | V. Bat Sensor: "); Serial.println(lastPacket.vbat_sensor, 2);
  } else {
      Serial.println("No hay datos de paquetes previos en la EEPROM.");
  }
  Serial.println("-----------------------------------------");

  Serial.println("| timestamp_unix | fecha_hora_utc      | id_paquete | voltaje | corriente | v_bat_sensor | v_bat_local |");
  Serial.println("|----------------|---------------------|------------|---------|-----------|--------------|-------------|");
}

void loop() {
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.trim();
    
    // Imprime la cadena de datos cruda para depuración
    Serial.print("RX < ");
    Serial.println(data);

    // --- LÓGICA DE DECODIFICACIÓN REESCRITA CON sscanf ---
    uint16_t id;
    float voltaje;
    float corriente;
    float vbat_sensor;
    uint32_t timestamp;

    const char* format = "N:%hu V:%f I:%f B:%f T:%lu";
    int itemsParsed = sscanf(data.c_str(), format, &id, &voltaje, &corriente, &vbat_sensor, &timestamp);

    if (itemsParsed == 5) {
      // Éxito: Se decodificaron los 5 valores.
      
      // Guarda el contador de paquetes en la dirección 0
      paquetesRecibidos++;
      EEPROM.put(0, paquetesRecibidos);

      // Llena la estructura con los nuevos datos
      lastPacket.id = id;
      lastPacket.voltaje = voltaje;
      lastPacket.corriente = corriente;
      lastPacket.vbat_sensor = vbat_sensor;
      lastPacket.timestamp = timestamp;
      
      // Guarda la estructura completa en la EEPROM en una dirección diferente (4)
      EEPROM.put(4, lastPacket);
      
      // Guarda todos los cambios en la memoria física
      EEPROM.commit();

      String fechaHoraLegible = formatUnixTime(timestamp);
      float vbat_local = leerVoltajeBateriaLocal();
      
      char serialBuffer[200];
      snprintf(serialBuffer, sizeof(serialBuffer), "| %-14lu | %-19s | %-10u | %-7.2f | %-9.3f | %-12.2f | %-11.2f |",
          timestamp, fechaHoraLegible.c_str(), id, voltaje, corriente, vbat_sensor, vbat_local);
      Serial.println(serialBuffer);
    } else {
      // Error: La cadena no coincide con el formato esperado.
      Serial.print("Error: No se pudieron decodificar los datos. Items leídos: ");
      Serial.println(itemsParsed);
    }
  }
}

