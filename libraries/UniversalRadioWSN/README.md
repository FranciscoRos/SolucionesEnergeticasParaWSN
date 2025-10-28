# Librería UniversalRadioWSN

[![Licencia: LGPL 3.0](https://img.shields.io/badge/Licencia-LGPL%203.0-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0.html)


Una interfaz de radio universal para Arduino que abstrae módulos LoRa, NRF24L01 y XBee, permitiendo cambiar de hardware sin reescribir la lógica de la aplicación.

## 📝 Descripción

Esta librería proporciona una **interfaz de abstracción** (una clase base llamada `RadioInterface`) que define un "contrato" común para cualquier módulo de radio.

Tu código principal (el *sketch*) no interactúa directamente con `LoRa.h` o `RF24.h`. En su lugar, opera únicamente usando un puntero a `RadioInterface`.

Esto te permite cambiar el hardware de radio (ej. de un NRF24 a un LoRa) cambiando **solo una o dos líneas de código** en un archivo de configuración, mientras que toda tu lógica principal de `setup()` y `loop()` permanece **idéntica**.

## ✨ Implementaciones Incluidas

Esta librería agrupa la interfaz y tres implementaciones concretas:

* **`RadioInterface.h`**: La clase base abstracta (el "contrato").
* **`LoraRadio.h`**: Implementación para módulos LoRa (ej. SX127x) usando la librería `LoRa` de Sandeep Mistry.
* **`NrfRadio.h`**: Implementación para módulos NRF24L01+ usando la librería `RF24`.
* **`XbeeRadio.h`**: Implementación para módulos XBee (en modo transparente AT) usando cualquier `Stream` (como `HardwareSerial`).

## 📦 Dependencias

Para compilar las implementaciones concretas, esta librería requiere que tengas instaladas las siguientes bibliotecas (puedes instalarlas desde el Administrador de Bibliotecas del IDE):

1.  **[LoRa](https://github.com/sandeepmistry/arduino-LoRa)** (por Sandeep Mistry):
    * Necesaria para `LoraRadio.h`.
2.  **[RF24](https://github.com/nRF24/RF24)** (por TMRh20, nRF24):
    * Necesaria para `NrfRadio.h`.

**Nota:** No necesitas instalar ambas si solo vas a usar una. Por ejemplo, si solo usas `XbeeRadio`, no necesitas instalar `LoRa` ni `RF24`.

## 💾 Instalación

1.  Asegúrate de instalar las **Dependencias** que vayas a necesitar (ej. `LoRa` y/o `RF24`).
2.  Descarga este repositorio como ZIP.
3.  Abre el IDE de Arduino.
4.  Ve a `Sketch` > `Incluir Librería` > `Añadir biblioteca .ZIP`.
5.  Selecciona el archivo ZIP que descargaste.

## 🚀 Concepto de Uso (El Poder de la Abstracción)

El uso de esta librería se divide en dos partes:

1.  **El Sketch Principal (`.ino`)**: Este archivo nunca "sabe" qué radio está conectada. Solo incluye `UniversalRadioWSN.h` y opera sobre un puntero `RadioInterface*`. Su lógica es genérica.
2.  **Un Archivo de Configuración (`config_radio.h`)**: Este es el **único** archivo que modificas para cambiar de hardware. Aquí es donde creas la instancia *concreta* (ej. `LoraRadio` o `NrfRadio`) y la asignas al puntero global `RadioInterface*` que usará el sketch principal.

A continuación, pondremos los ejemplos de código para un emisor y un receptor.

### Código de Emisión (Nodo Sensor)

```cpp
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
```
### Código de Recepción (Coordinador)

```cpp


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

```
## ⚖️ Licencia

Esta librería se distribuye bajo la licencia **LGPL 3.0**. Es gratuita y de código abierto para proyectos personales, educativos y de código abierto.

### Uso Comercial
La licencia LGPL 3.0 tiene ciertas condiciones si se usa en un software comercial de código cerrado.

Si deseas utilizar esta librería en un producto comercial y prefieres evitar las restricciones de la LGPL, por favor, **contáctame en [FranciscoRosalesHuey@gmail.com]** para adquirir una licencia comercial alternativa (tipo MIT) que se adapte a tus necesidades.