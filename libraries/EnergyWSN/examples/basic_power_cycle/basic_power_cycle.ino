/**
 * @file basic_power_cycle.ino
 * @brief Ejemplo básico de uso de la librería EnergyWSN.
 * @details Este sketch demuestra el ciclo de vida completo de un nodo
 * de bajo consumo:
 * 1. Despertar (MCU y periféricos).
 * 2. Encender sensores (Power Gating).
 * 3. Simular una lectura y transmisión.
 * 4. Apagar sensores y dormir la radio.
 * 5. Poner el MCU en sueño profundo.
 *
 * @note Este ejemplo requiere la librería Low-Power (de RocketScream)
 * y la librería UniversalRadioWSN (para RadioInterface.h).
 */

#include <Arduino.h>
#include <LowPower.h>          // 1. Dependencia para el sueño del MCU (AVR)
#include <UniversalRadioWSN.h> // 2. Dependencia para la clase base RadioInterface.h
#include <EnergyWSN.h>         // 3. ¡Nuestra librería!

// --- Declaración de una Radio Ficticia (Dummy) ---
// EnergyWSN necesita un puntero a un objeto RadioInterface.
// Para este ejemplo, creamos una clase "Dummy" que simula ser una radio.
// Simplemente imprime en el Serial lo que EnergyWSN le ordena hacer.
// En un proyecto real, aquí usarías tu LoraRadio, NrfRadio, etc.
class DummyRadio : public RadioInterface {
public:
  bool iniciar() override { 
    Serial.println("  [DummyRadio]: Iniciada."); 
    return true; 
  }
  
  bool enviar(const uint8_t* b, size_t l) override { 
    Serial.println("  [DummyRadio]: Enviando datos..."); 
    return true; 
  }
  
  int hayDatosDisponibles() override { 
    return 0; // Nunca recibe nada en este ejemplo
  }
  
  size_t leer(uint8_t* b, size_t l) override { 
    return 0; 
  }
  
  bool dormir() override { 
    Serial.println("  [DummyRadio]: Durmiendo."); 
    _isSleeping = true; 
    return true; 
  }
  
  bool despertar() override { 
    Serial.println("  [DummyRadio]: Despierta."); 
    _isSleeping = false; 
    return true; 
  }
  
private:
  bool _isSleeping = false;
};

// --- Instancias Globales ---
DummyRadio miRadioFicticia; // Instancia de la radio ficticia
EnergyWSN  energyManager;     // Instancia del gestor de energía

// --- Configuración de Pines ---
// Pin que controla el VCC de los sensores (ej. a través de un MOSFET)
const int PIN_PWR_SENSORES = 7; 

void setup() {
  Serial.begin(9600);
  while (!Serial); // Esperar a que el monitor serial se conecte
  Serial.println("Iniciando EnergyWSN - Ejemplo Basico de Ciclo de Potencia...");

  // 4. Configurar EnergyWSN
  EnergyWSN::Cfg config;
  config.pins.pwrSens = PIN_PWR_SENSORES; // Pin de control de sensores
  config.invertPwr = false; // false = HIGH enciende los sensores
  config.bootSleep = true;  // Iniciar con la radio dormida (buena práctica)

  // Iniciar el hardware de radio (nuestra radio ficticia)
  miRadioFicticia.iniciar(); 
  
  // 5. Inicializar el gestor, pasando la config y el puntero a la radio
  energyManager.begin(config, &miRadioFicticia);
  
  Serial.println("Setup completo. Iniciando loop...");
}

void loop() {
  Serial.println("----------------------");
  Serial.println("Ciclo de trabajo: INICIO");

  // 1. Despertar periféricos
  Serial.println("Paso 1: Despertando radio y sensores...");
  energyManager.wakeRadio();        // Llama a miRadioFicticia.despertar()
  energyManager.powerSensors(true); // Pone el PIN_PWR_SENSORES en HIGH
  
  // Esperar a que los sensores se estabilicen
  // (En un sketch real, esto podría ser un sleep corto de 15ms)
  delay(500); 

  // 2. Leer sensores y transmitir
  Serial.println("Paso 2: Leyendo sensores y transmitiendo...");
  // ... tu_codigo_de_lectura_de_sensores_va_aqui ...
  
  // Simula el envío de datos
  miRadioFicticia.enviar((const uint8_t*)"Hola", 4); 
  delay(100); // Simula el tiempo que tarda la transmisión

  // 3. Apagar periféricos
  Serial.println("Paso 3: Apagando sensores y durmiendo radio...");
  energyManager.powerSensors(false); // Pone el PIN_PWR_SENSORES en LOW
  energyManager.sleepRadio();        // Llama a miRadioFicticia.dormir()

  // 4. Dormir el MCU
  uint32_t tiempoSueno_ms = 8000; // 8 segundos
  Serial.print("Paso 4: MCU durmiendo por ");
  Serial.print(tiempoSueno_ms / 1000);
  Serial.println(" seg...");
  Serial.flush(); // Asegurar que se imprima todo antes de dormir

  // Esta función bloquea la ejecución y pone el MCU en bajo consumo
  energyManager.sleepFor_ms(tiempoSueno_ms);
  
  // -- El MCU se despierta aquí después de 8 segundos --
  // -- y el loop() comienza de nuevo.                 --
  Serial.println("Ciclo de trabajo: MCU despierto!");
}