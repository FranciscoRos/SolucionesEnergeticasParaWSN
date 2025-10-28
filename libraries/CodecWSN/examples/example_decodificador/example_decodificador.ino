#include <Arduino.h>
#include <CodecWSN.h> // La única librería que necesitamos

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("\n--- PRUEBA DE BUCLE CERRADO (ENCODE/DECODE) ---");

  // --- 1. CODIFICACIÓN ---
  Serial.println("Paso 1: Codificando un paquete original...");

  Packet paqueteOriginal;
  paqueteOriginal.id = 1;
  paqueteOriginal.voltaje = 1234;   // 12.34V
  paqueteOriginal.corriente = 56;     // 56mA
  paqueteOriginal.vbat = 395;     // 3.95V

  // Buffer para guardar la trama codificada
  uint8_t bufferFrame[WSNFrame::FRAME_SIZE]; // 14 bytes

  // Llama a la función de codificación de TU librería
  WSNFrame::encodeFrameFromPacket(bufferFrame, paqueteOriginal);

  // --- 2. IMPRIMIR EL CODIFICADO ---
  Serial.print("Paso 2: Trama codificada generada (");
  Serial.print(WSNFrame::FRAME_SIZE);
  Serial.print(" bytes):\n  [ ");
  
  // Imprime la trama de 14 bytes que generó tu codificador
  for (size_t i = 0; i < WSNFrame::FRAME_SIZE; i++) {
    if (bufferFrame[i] < 0x10) Serial.print("0"); // Añadir cero inicial
    Serial.print(bufferFrame[i], HEX);
    Serial.print(" ");
  }
  Serial.println("]");


  // --- 3. DECODIFICACIÓN ---
  Serial.println("\nPaso 3: Alimentando esa misma trama al parser...");
  
  WSNFrame::Parser miParser;
  Packet paqueteDecodificado; // Un struct vacío para el resultado
  bool exito = false;

  // Iteramos sobre la trama que ACABAMOS de generar
  for (size_t i = 0; i < WSNFrame::FRAME_SIZE; i++) {
    
    uint8_t byteActual = bufferFrame[i];
    
    // Alimentamos el parser byte por byte
    if (WSNFrame::feed(miParser, byteActual, paqueteDecodificado)) {
      
      // El parser devuelve 'true' solo en el último byte (si el CRC es correcto)
      Serial.println("  -> ¡ÉXITO! El parser reporto un paquete valido.");
      exito = true;
    }
  }

  // --- 4. IMPRIMIR LO DECODIFICADO ---
  Serial.println("\nPaso 4: Verificando los datos decodificados...");
  
  if (exito) {
    Serial.println("  Paquete decodificado:");
    Serial.print("    ID: "); Serial.println(paqueteDecodificado.id);
    Serial.print("    Voltaje: "); Serial.println(paqueteDecodificado.voltaje);
    Serial.print("    Corriente: "); Serial.println(paqueteDecodificado.corriente);
    Serial.print("    VBat: "); Serial.println(paqueteDecodificado.vbat);

    // Verificación final
    if (paqueteDecodificado.id == paqueteOriginal.id &&
        paqueteDecodificado.voltaje == paqueteOriginal.voltaje &&
        paqueteDecodificado.corriente == paqueteOriginal.corriente &&
        paqueteDecodificado.vbat == paqueteOriginal.vbat) 
    {
      Serial.println("\n  VERIFICACION: ¡DATOS COINCIDEN! La libreria funciona.");
    } else {
      Serial.println("\n  VERIFICACION: ¡ERROR! Los datos no coinciden.");
    }
    
  } else {
    Serial.println("  ¡FALLO! El parser nunca devolvio 'true' (Error de CRC).");
  }
  
  Serial.println("\n--- PRUEBA FINALIZADA ---");
}

void loop() {
  // El loop se deja vacío, la prueba solo corre una vez en setup().
}