# Librería EnergyWSN

[![Licencia: LGPL 3.0](https://img.shields.io/badge/Licencia-LGPL%203.0-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0.html)

Una librería de ayuda para la gestión de energía en nodos WSN de bajo consumo, optimizada para arquitecturas **AVR** (Arduino UNO, Nano, Pro Mini).

## 📝 Descripción

Esta librería proporciona funciones de alto nivel para gestionar el ciclo de vida de un nodo sensor en un bucle de bajo consumo:

1.  **Power Gating:** Controla la alimentación de los sensores (encendiéndolos solo cuando es necesario).
2.  **Gestión del Radio:** Pone a dormir y despierta un módulo de radio (LoRa, NRF, etc.) usando una abstracción.
3.  **Sueño del MCU:** Pone el microcontrolador en sueño profundo (`powerDown`) por un período específico.

**Nota importante:** Esta librería depende de `LowPower.h` de RocketScream, por lo que está diseñada específicamente para placas basadas en **AVR** (como Arduino UNO, Nano, Pro Mini, etc.) y no funcionará en placas como ESP32 o SAMD.

## ✨ Características

* **`powerSensors(bool on)`**: Controla un pin (ej. para un MOSFET) para energizar o desenergizar la línea de sensores.
* **`sleepFor_ms(uint32_t ms)`**: Pone el MCU en sueño profundo (ADC y BOD deshabilitados) durante el tiempo especificado.
* **`sleepRadio()` / `wakeRadio()`**: Abstracciones para dormir/despertar el hardware de radio.
* **Agnóstica al Radio:** Funciona con cualquier objeto de radio que implemente la `RadioInterface` (ver librería `UniversalRadioWSN`).

## 📦 Dependencias (¡Importante!)

Esta librería **requiere** las siguientes dos bibliotecas para funcionar:

1.  **[Low-Power](https://github.com/rocketscream/Low-Power)** (por RocketScream):
    * Necesaria para la función `sleepFor_ms()`.
    * *Instalación:* Abre el IDE de Arduino > `Administrador de Bibliotecas` > Buscar e instalar `Low-Power`.
2.  **`UniversalRadioWSN`** (tu otra librería):
    * Necesaria porque esta librería opera sobre la clase base `RadioInterface`.
    * *Instalación:* Debes tener `UniversalRadioWSN` instalada en tu carpeta de librerías.

## 💾 Instalación

1.  Asegúrate de instalar primero las **Dependencias** (`Low-Power` y `UniversalRadioWSN`).
2.  Descarga este repositorio como ZIP.
3.  Ve a `Sketch` > `Incluir Librería` > `Añadir biblioteca .ZIP` y selecciona el archivo que descargaste.

## 🚀 Uso Básico

El siguiente ejemplo muestra un ciclo de trabajo completo: despertar, encender sensores, leer, apagar todo y dormir.

**Nota:** Para que este ejemplo funcione de forma autónoma, hemos creado una clase `DummyRadio` que "simula" ser una radio real implementando la `RadioInterface`.

```cpp
#include <Arduino.h>
#include <LowPower.h>         // 1. Incluir dependencia
#include <UniversalRadioWSN.h> // 2. Incluir dependencia (para la interfaz)
#include <EnergyWSN.h>        // 3. Incluir esta librería

// --- Declaración de una Radio Ficticia (Dummy) ---
// EnergyWSN necesita un puntero a un objeto RadioInterface.
// Para este ejemplo, creamos una clase "Dummy" que simula ser una radio.
// En un proyecto real, aquí usarías tu LoraRadio, NrfRadio, etc.
class DummyRadio : public RadioInterface {
public:
  bool iniciar() { Serial.println("Radio Ficticia: Iniciada"); return true; }
  bool enviar(const uint8_t* b, size_t l) { Serial.println("Radio Ficticia: Enviando datos..."); return true; }
  int hayDatosDisponibles() { return 0; }
  size_t leer(uint8_t* b, size_t l) { return 0; }
  bool dormir() { 
    Serial.println("Radio Ficticia: Durmiendo."); 
    _isSleeping = true; 
    return true; 
  }
  bool despertar() { 
    Serial.println("Radio Ficticia: Despierta."); 
    _isSleeping = false; 
    return true; 
  }
  bool _isSleeping = false;
};

// --- Instancias ---
DummyRadio miRadio; // Instancia de la radio ficticia
EnergyWSN energyManager; // Instancia del gestor de energía

// --- Configuración de Pines ---
const int PIN_PWR_SENSORES = 7; // Pin conectado al Gate de un MOSFET

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Iniciando EnergyWSN - Ejemplo Basico...");

  // 4. Configurar EnergyWSN
  EnergyWSN::Cfg config;
  config.pins.pwrSens = PIN_PWR_SENSORES;
  config.invertPwr = false; // false = HIGH enciende los sensores
  config.bootSleep = true;  // Iniciar con la radio dormida (buena práctica)

  miRadio.iniciar(); // Iniciar el hardware de radio
  
  // 5. Inicializar el gestor, pasando la config y el puntero a la radio
  energyManager.begin(config, &miRadio);
  
  Serial.println("Setup completo. Iniciando loop...");
}

void loop() {
  Serial.println("----------------------");
  Serial.println("Ciclo de trabajo: INICIO");

  // 1. Despertar periféricos
  Serial.println("Paso 1: Despertando radio y sensores...");
  energyManager.wakeRadio();
  energyManager.powerSensors(true); // Encender sensores
  
  // Esperar a que los sensores se estabilicen
  // (En un sketch real, esto podría ser un sleep corto)
  delay(500); 

  // 2. Leer sensores y transmitir
  Serial.println("Paso 2: Leyendo sensores y transmitiendo...");
  // ... tu_codigo_de_lectura_de_sensores_va_aqui ...
  miRadio.enviar((const uint8_t*)"Hola", 4);
  
  // Esperar a que se complete la TX (depende de la radio)
  delay(100); 

  // 3. Apagar periféricos
  Serial.println("Paso 3: Apagando sensores y durmiendo radio...");
  energyManager.powerSensors(false); // Apagar sensores
  energyManager.sleepRadio();

  // 4. Dormir el MCU
  uint32_t tiempoSueno_ms = 8000; // 8 segundos
  Serial.print("Paso 4: MCU durmiendo por ");
  Serial.print(tiempoSueno_ms / 1000);
  Serial.println(" seg...");
  Serial.flush(); // Asegurar que se imprima todo antes de dormir

  energyManager.sleepFor_ms(tiempoSueno_ms);
  
  // -- El MCU se despierta aquí y el loop() comienza de nuevo --
  Serial.println("Ciclo de trabajo: MCU despierto!");
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
* **Omar Tox Dzul** ([@Omar-Tox](https://github.com/Omar-Tox))

Ambos autores merecen igual reconocimiento por su contribución al diseño y desarrollo de este proyecto.
