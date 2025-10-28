/**
 * @file RadioInterface.h
 * @brief Define la interfaz abstracta (clase base) para cualquier módulo de radio.
 * @details Esta clase actúa como un contrato que cualquier clase de radio (LoRa, NRF, etc.)
 * debe cumplir. Define un conjunto de funciones comunes para inicializar, enviar, recibir datos
 * y gestionar el estado de energía, permitiendo que el resto del código
 * opere con diferentes radios de forma intercambiable.
 */

#ifndef RADIO_INTERFACE_H
#define RADIO_INTERFACE_H

#include <Arduino.h>

/**
 * @class RadioInterface
 * @brief Interfaz abstracta para módulos de radio en una red de sensores.
 * @details Define los métodos puros virtuales que toda clase de radio concreta debe implementar.
 * También proporciona implementaciones por defecto (virtuales) para funciones opcionales
 * y sobrecargas de conveniencia (ej. para Strings).
 */
class RadioInterface {
public:
  /**
   * @brief Destructor virtual.
   * @details Asegura que el destructor de la clase derivada (ej. NrfRadio) se llame
   * correctamente cuando un objeto se elimina a través de un puntero a RadioInterface.
   */
  virtual ~RadioInterface() {} 

  // --- Funciones Fundamentales (Puras Virtuales) ---
  
  /**
   * @brief Inicializa el hardware y la configuración del módulo de radio.
   * @details Debe ser implementado por la clase derivada para configurar pines,
   * SPI/I2C, y parámetros específicos del hardware (frecuencia, potencia, etc.).
   * @return true si la inicialización fue exitosa.
   * @return false si hubo un error (ej. hardware no detectado).
   */
  virtual bool iniciar() = 0;
  
  /**
   * @brief Envía un bloque de datos binarios a través del radio.
   * @details Método puro virtual que debe ser implementado por la clase derivada.
   * @param buffer Puntero al array de bytes (const) que se va a enviar.
   * @param longitud Número de bytes a enviar desde el buffer.
   * @return true si los datos se enviaron (o encolaron para envío) correctamente.
   * @return false si hubo un error (ej. buffer lleno, radio ocupada).
   */
  virtual bool enviar(const uint8_t* buffer, size_t longitud) = 0;
  
  /**
   * @brief Comprueba si hay datos disponibles para leer en el buffer de recepción del radio.
   * @details Método puro virtual. La implementación debe consultar al hardware
   * si se ha recibido un paquete completo.
   * @return El número de bytes disponibles para ser leídos (ej. tamaño del paquete).
   * @return 0 si no hay datos disponibles.
   */
  virtual int hayDatosDisponibles() = 0;
  
  /**
   * @brief Lee los datos recibidos del radio y los almacena en un buffer proporcionado.
   * @details Método puro virtual. Debe llamarse después de que `hayDatosDisponibles()`
   * confirme que hay datos.
   * @param buffer Puntero al buffer (no const) donde se guardarán los datos leídos.
   * @param maxLongitud El tamaño máximo del `buffer` para evitar desbordamientos.
   * @return El número de bytes que se leyeron y guardaron exitosamente en el `buffer`.
   */
  virtual size_t leer(uint8_t* buffer, size_t maxLongitud) = 0;

  // --- Funciones de Conveniencia y Estado (Virtuales con Implementación) ---
  
  /**
   * @brief Obtiene el Indicador de Fuerza de la Señal Recibida (RSSI) del último paquete.
   * @details Implementación virtual (opcional). Las clases derivadas que soporten
   * RSSI (como LoRa) deben sobreescribir esta función.
   * @return El valor del RSSI en dBm.
   * @return 0 por defecto, si el módulo no soporta esta función o no se implementa.
   */
  virtual int obtenerRSSI() { return 0; }

  /**
   * @brief Pone el módulo de radio en modo de bajo consumo (dormir).
   * @details Implementación virtual (opcional). Las clases derivadas deben sobreescribir
   * esta función si el hardware soporta un modo de bajo consumo.
   * @return true por defecto (si no se implementa, se asume que no es necesario o tuvo éxito).
   */
  virtual bool dormir() { return true; }
  
  /**
   * @brief Saca al módulo de radio del modo de bajo consumo (despertar).
   * @details Implementación virtual (opcional).
   * @return true por defecto (si no se implementa, se asume que no es necesario o tuvo éxito).
   */
  virtual bool despertar() { return true; }

  // --- Sobrecargas de Conveniencia (Usan los métodos puros) ---

  /**
   * @brief Envía un objeto String a través del radio. 
   * @details Esta es una función de conveniencia que convierte el String
   * a `const uint8_t*` y llama a la función `enviar()` principal.
   * @param data El objeto String que se va a enviar.
   * @return true si el envío (llamada a `enviar(buffer, longitud)`) fue exitoso.
   * @return false en caso contrario.
   */
  virtual bool enviar(const String& data) {
    // Convierte String a const uint8_t* (reinterpret_cast) y llama al método puro virtual
    return enviar(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
  }

  /**
   * @brief Lee los datos disponibles del radio y los devuelve como un objeto String.
   * @details Esta es una función de conveniencia. Llama a `leer(buffer, max)` y
   * convierte el resultado a un String de Arduino.
   * @note Esta implementación usa un buffer estático de tamaño fijo (256 bytes) en el stack.
   * Se reserva 1 byte para el terminador nulo `\0`.
   * Para paquetes más grandes (> 255 bytes), se debe usar `leer()` directamente.
   * @return Un objeto String con los datos leídos.
   * @return Un String vacío si no había datos.
   */
  virtual String leerComoString() {
    uint8_t buffer[256]; // Buffer local en el stack
    
    // Se reserva 1 byte para el terminador nulo '\0'.
    size_t longitud = leer(buffer, 255); 
    
    buffer[longitud] = '\0'; // Asegura terminación nula para el constructor del String
    
    // Convierte el buffer C a un String de Arduino
    return String(reinterpret_cast<char*>(buffer));
  }

};

#endif // RADIO_INTERFACE_H
