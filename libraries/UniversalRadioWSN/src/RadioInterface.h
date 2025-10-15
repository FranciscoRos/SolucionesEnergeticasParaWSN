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

  virtual String leerComoString() {
    int availableBytes = hayDatosDisponibles();
    if (availableBytes > 0) {
      char* tempBuffer = new char[availableBytes + 1];
      size_t bytesLeidos = leer(reinterpret_cast<uint8_t*>(tempBuffer), availableBytes);
      tempBuffer[bytesLeidos] = '\0'; 
      String result(tempBuffer);
      delete[] tempBuffer;
      return result;
    }
    return "";
  }
};

#endif

