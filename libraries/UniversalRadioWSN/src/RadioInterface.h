#ifndef RADIO_INTERFACE_H
#define RADIO_INTERFACE_H

#include <Arduino.h>

/**
 * @class RadioInterface
 * @brief Define la interfaz abstracta para cualquier módulo de radio en una red de sensores.
 * 
 * Esta clase define un conjunto de funciones comunes para inicializar, enviar, recibir datos y
 * gestionar el estado de energía del módulo de radio.
 */
class RadioInterface {
public:
  /**
   * @brief Destructor virtual para asegurar la correcta limpieza de las clases derivadas.
   */
  virtual ~RadioInterface() {} 

  // --- Funciones Fundamentales ---
  
  /**
   * @brief Inicializa el hardware y la configuración del módulo de radio.
   * @return "true" si la inicialización fue exitosa, "false" en caso contrario.
   */
  virtual bool iniciar() = 0;
  
  /**
   * @brief Envía un bloque de datos binarios a través del radio.
   * @param buffer Puntero al array de bytes que se va a enviar.
   * @param longitud Número de bytes a enviar desde el buffer.
   * @return "true" si los datos se enviaron correctamente, "false" si hubo un error.
   */
  virtual bool enviar(const uint8_t* buffer, size_t longitud) = 0;
  
  /**
   * @brief Comprueba si hay datos disponibles para leer en el buffer de recepción del radio.
   * @return El número de bytes disponibles para ser leídos. Devuelve 0 si no hay datos.
   */
  virtual int hayDatosDisponibles() = 0;
  
  /**
   * @brief Lee los datos recibidos del radio y los almacena en un buffer proporcionado.
   * @param buffer Puntero al buffer donde se guardarán los datos leídos.
   * @param maxLongitud El tamaño máximo del buffer para evitar desbordamientos.
   * @return El número de bytes que se leyeron y guardaron en el buffer.
   */
  virtual size_t leer(uint8_t* buffer, size_t maxLongitud) = 0;

  // --- Funciones de Conveniencia y Estado ---
  
  /**
   * @brief Obtiene el Indicador de Fuerza de la Señal Recibida (RSSI) del último paquete.
   * @return El valor del RSSI en dBm. Devuelve 0 si el módulo no soporta esta función.
   */
  virtual int obtenerRSSI() { return 0; }

  /**
   * @brief Pone el módulo de radio en modo de bajo consumo (dormir).
   * @return "true" si la operación fue exitosa o si no es soportada (comportamiento por defecto).
   */
  virtual bool dormir() { return true; }
  
  /**
   * @brief Saca al módulo de radio del modo de bajo consumo (despertar).
   * @return "true" si la operación fue exitosa o si no es soportada (comportamiento por defecto).
   */
  virtual bool despertar() { return true; }

  // --- Sobrecargas para facilitar el uso ---

  /**
   * @brief Envía un objeto String a través del radio. 
   * @param data El objeto String que se va a enviar.
   * @return "true" si el envío fue exitoso, "false" en caso contrario.
   */
  virtual bool enviar(const String& data) {
    return enviar(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
  }

  /**
   * @brief Lee los datos disponibles del radio y los devuelve como un objeto String.
   * @note Esta implementación usa un buffer estático de tamaño fijo (256 bytes).
   *       Para paquetes más grandes, se debe usar la función "leer()" directamente.
   * @return Un objeto String con los datos leídos, o un String vacío si no había datos.
   */
  virtual String leerComoString() {
    uint8_t buffer[256];
    // Se reserva 1 byte para el terminador nulo '\0'.
    size_t longitud = leer(buffer, 255); 
    buffer[longitud] = '\0'; // Asegura terminación nula
    return String(reinterpret_cast<char*>(buffer));
  }

};

#endif