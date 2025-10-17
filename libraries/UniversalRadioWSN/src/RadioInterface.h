#ifndef RADIO_INTERFACE_H
#define RADIO_INTERFACE_H

#include <Arduino.h>

class RadioInterface {
public:
  virtual ~RadioInterface() {} 

  // --- Funciones Fundamentales (Obligatorias para las clases hijas) ---
  
  virtual bool iniciar() = 0;
  
  virtual bool enviar(const uint8_t* buffer, size_t longitud) = 0;
  
  virtual int hayDatosDisponibles() = 0;
  
  virtual size_t leer(uint8_t* buffer, size_t maxLongitud) = 0;

  // --- Funciones de Conveniencia y Estado (Con implementación por defecto) ---
  
  /**
   * @brief Obtiene el Indicador de Fuerza de la Señal Recibida (RSSI) del último paquete.
   * @return El valor del RSSI en dBm, o 0 si no es soportado por el radio.
   */
  virtual int obtenerRSSI() { return 0; }

  virtual bool dormir() { return true; }
  
  virtual bool despertar() { return true; }

  // --- Sobrecargas para facilitar el uso ---

  virtual bool enviar(const String& data) {
    return enviar(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
  }

    // Lee los datos disponibles directamente como un String (VERSIÓN CORREGIDA)
  virtual String leerComoString() {
    uint8_t buffer[256];
    size_t longitud = leer(buffer, 255); // Llama a la función 'leer' que implementa LoraRadio
    buffer[longitud] = '\0'; // Asegura terminación nula
    return String(reinterpret_cast<char*>(buffer));
  }

};

#endif

