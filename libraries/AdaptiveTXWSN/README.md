# Librería AdaptiveTXWSN

[![Licencia: LGPL 3.0](https://img.shields.io/badge/Licencia-LGPL%203.0-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0.html)

Una librería de Arduino para ajustar adaptativamente el período de transmisión de un nodo WSN (Red de Sensores Inalámbricos) basado en el voltaje de su batería.

## 📝 Descripción

Esta librería permite a un nodo sensor (ej. Arduino Nano, Pro Mini) ajustar su período de transmisión para ahorrar energía cuando el nivel de batería es bajo. Monitorea el voltaje, lo clasifica en niveles (ALTO, MEDIO, BAJO) usando umbrales con histéresis, y determina el intervalo de tiempo adecuado para la próxima transmisión.

## ✨ Características

* **Medición de Batería:** Lee el voltaje desde un pin ADC usando un divisor de voltaje.
* **Inyección de Voltaje:** Permite "inyectar" un voltaje medido externamente (ej. con un PMIC o ADC externo) usando `setBatteryVolts()`.
* **Niveles de Energía:** Clasifica el estado de la batería en `BATT_HIGH`, `BATT_MID`, y `BATT_LOW`.
* **Histéresis:** Evita el "rebote" o cambios rápidos de estado cuando el voltaje está cerca de un umbral.
* **Corte de Batería:** Desactiva la transmisión por debajo de un umbral de voltaje crítico (`isCutoff()`).
* **Configurable:** Todos los pines, umbrales, períodos y resistencias del divisor son configurables.

## 📦 Dependencias

Esta librería es autónoma y no tiene dependencias externas, solo requiere el core de Arduino (`<Arduino.h>`).

## 💾 Instalación

1.  Abre el IDE de Arduino.
2.  Ve a `Sketch` > `Incluir Librería` > `Administrar Bibliotecas...`.
3.  Busca `AdaptiveTXWSN` y haz clic en "Instalar".
4.  (Alternativamente) Descarga este repositorio como ZIP y ve a `Sketch` > `Incluir Librería` > `Añadir biblioteca .ZIP`.

## 🚀 Uso Básico

El siguiente ejemplo configura la librería para medir una batería LiPo (conectada a `A0`) y ajusta el período de envío.

```cpp
#include <Arduino.h>
#include <AdaptiveTXWSN.h> // 1. Incluir la librería

// Crear una instancia del temporizador adaptativo
AdaptiveTXWSN txTimer;

void setup() {
  Serial.begin(9600);
  Serial.println("Iniciando AdaptiveTXWSN...");

  // 2. Definir la configuración
  AdaptiveTXWSN::Cfg config;

  // --- Configuración del Divisor de Voltaje ---
  // (Vin) -> [R Arriba] -> (A0) -> [R Abajo] -> (GND)
  config.pinAdcBateria       = A0;
  config.divisorRArriba_k    = 100.0f; // Resistencia de 100k
  config.divisorRAbajo_k     = 33.0f;  // Resistencia de 33k
  config.voltajeReferenciaAdc = 5.0f;   // Para Arduino Nano/UNO a 5V
  
  // --- Umbrales de Voltaje (para LiPo) ---
  config.umbralAlto_V     = 3.90f; // Por encima de 3.9V = ALTO
  config.umbralMedio_V    = 3.60f; // Entre 3.6V y 3.9V = MEDIO
  config.corteVoltaje_V   = 3.40f; // Por debajo de 3.4V = CORTE (no enviar)

  // --- Períodos de Transmisión ---
  config.periodoAlto_ms   = 5000;   // 5 segundos (batería llena)
  config.periodoMedio_ms  = 15000;  // 15 segundos (batería media)
  config.periodoBajo_ms   = 120000; // 2 minutos (batería baja)

  // 3. Inicializar la librería
  txTimer.begin(config);
}

void loop() {
  
  // 4. Llamar a tick() en cada ciclo del loop
  //    Devolverá 'true' cuando sea momento de transmitir.
  
  if (txTimer.tick()) {
    
    // ¡Es hora de transmitir!
    // Aquí iría tu código para leer sensores y enviar datos.
    
    Serial.print("Transmitiendo... ");
    Serial.print("VBat: ");
    Serial.print(txTimer.lastVolts()); // Obtener último voltaje medido
    Serial.print("V, Nivel: ");
    
    switch(txTimer.level()) { // Obtener nivel actual
      case AdaptiveTXWSN::BATT_HIGH: Serial.print("ALTO"); break;
      case AdaptiveTXWSN::BATT_MID:  Serial.print("MEDIO"); break;
      case AdaptiveTXWSN::BATT_LOW:  Serial.print("BAJO"); break;
    }
    
    Serial.print(", Prox. TX en: ");
    Serial.print(txTimer.currentPeriod() / 1000); // Obtener período actual
    Serial.println(" seg");

  } else {
    
    // Si la batería está en corte, tick() siempre devuelve 'false'.
    if (txTimer.isCutoff()) {
      Serial.println("Batería en corte. No se transmitirá.");
      // Aquí podrías poner el MCU en sueño profundo permanente
      delay(10000);
    }
    
    // No es hora de transmitir, el MCU puede dormir
    // o hacer otras tareas de baja prioridad.
    // delay(10); // Evitar spam en el monitor serial
  }
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
