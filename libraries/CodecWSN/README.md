# Librería CodecWSN

[![Licencia: LGPL 3.0](https://img.shields.io/badge/Licencia-LGPL%203.0-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0.html)

Una librería header-only de Arduino para codificar y decodificar un protocolo de paquete binario robusto para Redes de Sensores Inalámbricos (WSN).

## 📝 Descripción

Esta librería define un protocolo de comunicación simple y robusto, ideal para enlaces de radio (como LoRa o NRF24) o conexiones seriales donde la integridad de los datos es crucial.

Se compone de dos partes principales:
1.  **`Packet` (Payload):** Una estructura de datos (`struct`) de 8 bytes fijos que contiene la telemetría del sensor (ID, voltaje, corriente, vbat). Los datos se serializan en formato **Big Endian**.
2.  **`WSNFrame` (Trama):** Un *namespace* que envuelve el `Packet` en una trama (frame) robusta de 14 bytes. Esta trama añade:
    * Un marcador de inicio (`SOF`): `0xAA 0x55`
    * Versión y Longitud.
    * Un checksum **CRC16-CCITT** para validar la integridad del paquete.

La librería proporciona un codificador simple para el emisor y un parser de máquina de estados (State Machine) para el receptor, que permite re-sincronizarse automáticamente si se reciben datos corruptos.

## ✨ Características

* **`Packet` de 8 bytes:** Carga útil fija (ID, voltaje, corriente, vbat).
* **Serialización Big Endian:** Asegura la compatibilidad entre diferentes arquitecturas.
* **`Frame` robusto de 14 bytes:** Estructura: `[SOF(2)][VER(1)][LEN(1)][PAYLOAD(8)][CRC16(2)]`.
* **Verificación de Integridad:** Usa `CRC16-CCITT` (Poly `0x1021`, Init `0xFFFF`) para detectar errores.
* **Codificador (`encodeFrameFromPacket`)**: Función simple para que el nodo emisor cree una trama lista para enviar.
* **Parser (`feed()`)**: Una máquina de estados (FSM) para el nodo receptor que procesa los datos byte por byte, ideal para `Serial.read()` o `radio.leer()`.
* **Librería Header-Only**: No hay archivos `.cpp`, solo necesitas incluir `CodecWSN.h`.

## 📦 Dependencias

Esta librería es autónoma y no tiene dependencias externas, solo requiere el core de Arduino (`<Arduino.h>`) y `<stdint.h>`.

## 💾 Instalación

1.  Descarga este repositorio como ZIP.
2.  Abre el IDE de Arduino.
3.  Ve a `Sketch` > `Incluir Librería` > `Añadir biblioteca .ZIP`.
4.  Selecciona el archivo ZIP que descargaste.

## 🚀 Uso Básico

Esta librería tiene dos casos de uso: el emisor (codifica) y el receptor (decodifica).

### Ejemplo 1: El Emisor (Nodo Sensor)

Este código crea un paquete, lo codifica en una trama de 14 bytes y lo envía.

```cpp
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
```

### Ejemplo 2: El Receptor (Coordinador / Gateway)

Este código escucha bytes entrantes (ej. desde Serial), los alimenta al parser y decodifica el paquete cuando se recibe una trama completa y válida.
```cpp
#include <Arduino.h>
#include <CodecWSN.h>
// #include <UniversalRadioWSN.h> // ¡Esta librería se usaría aquí!

// LoraRadio miRadio(config); // (Ejemplo de la radio)

// 1. Crear una instancia del Parser (mantiene el estado)
WSNFrame::Parser miParser;

// 2. Crear un struct para guardar el paquete decodificado
Packet paqueteRecibido;

void setup() {
  Serial.begin(9600);
  Serial.println("Receptor WSN iniciado. Esperando tramas...");
  // miRadio.iniciar();
  // miRadio.recibir(); // Poner radio en modo recepción si es necesario
}

void loop() {
  
  // Escuchar por bytes entrantes (ya sea de Serial o de la radio)
  while (Serial.available()) {
  // while (miRadio.hayDatosDisponibles()) {
    
    // Leer un solo byte
    uint8_t byteEntrante = Serial.read();
    // uint8_t byteEntrante = miRadio.leerUnByte(); // (Depende de la API de radio)

    // 3. Alimentar el byte al parser
    //    La función devuelve 'true' solo si un paquete completo y válido
    //    fue recibido y decodificado.
    
    if (WSNFrame::feed(miParser, byteEntrante, paqueteRecibido)) {
      
      // ¡ÉXITO! Paquete recibido y CRC válido.
      Serial.println("--- Paquete Valido Recibido ---");
      Serial.print("  ID Nodo: "); Serial.println(paqueteRecibido.id);
      Serial.print("  Voltaje: "); Serial.print(paqueteRecibido.voltaje / 100.0f); Serial.println(" V");
      Serial.print("  Corriente: "); Serial.print(paqueteRecibido.corriente); Serial.println(" mA");
      Serial.print("  VBat: "); Serial.print(paqueteRecibido.vbat / 100.0f); Serial.println(" V");
      Serial.println("---------------------------------");
    }
  }
  
  // El parser se resetea automáticamente después de un paquete
  // exitoso o de un error, listo para buscar el próximo SOF (0xAA 0x55).
}

```
## ⚖️ Licencia

Esta librería se distribuye bajo la licencia **LGPL 3.0**. Es gratuita y de código abierto para proyectos personales, educativos y de código abierto.

### Uso Comercial
La licencia LGPL 3.0 tiene ciertas condiciones si se usa en un software comercial de código cerrado.

Si deseas utilizar esta librería en un producto comercial y prefieres evitar las restricciones de la LGPL, por favor, **contáctame en [FranciscoRosalesHuey@gmail.com]** para adquirir una licencia comercial alternativa (tipo MIT) que se adapte a tus necesidades.

## 👥 Autores

Esta biblioteca fue desarrollada en coautoría y colaboración equitativa por:

* **Francisco Jareth Rosales Huey** ([@FranciscoRos](https://github.com/FranciscoRos))
* **Omar Tox Dzul** ([@xWhiteBerry](https://github.com/xWhiteBerry))

Ambos autores merecen igual reconocimiento por su contribución al diseño y desarrollo de este proyecto.
