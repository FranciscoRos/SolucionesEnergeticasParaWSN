// RadioInterface.h (Híbrida: Binario y String)

#ifndef RADIO_INTERFACE_H
#define RADIO_INTERFACE_H

#include <Arduino.h>

class RadioInterface {
public:
  virtual ~RadioInterface() {} 

  // --- Funciones Fundamentales (Obligatorias para las clases hijas) ---
  
  virtual bool iniciar() = 0;
  
  //función de envío: envía un buffer de bytes.
  virtual bool enviar(const uint8_t* buffer, size_t longitud) = 0;
  
  virtual int hayDatosDisponibles() = 0;
  
  //Función de lectura: lee bytes a un buffer.
  virtual size_t leer(uint8_t* buffer, size_t maxLongitud) = 0;
  

  // --- Funciones de Conveniencia (Opcionales, con implementación por defecto) ---
  
  // Sobrecarga para enviar un String.
  virtual bool enviar(const String& data) {
    // Convierte el String a un buffer de bytes y llama a la función de envío binaria.
    return enviar(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
  }

  // Función para leer directamente como un String.
  virtual String leerComoString() {
    int availableBytes = hayDatosDisponibles();
    if (availableBytes > 0) {
      // Crea un buffer temporal del tamaño justo
      char* tempBuffer = new char[availableBytes + 1];
      
      // Llama a la función de lectura binaria
      size_t bytesLeidos = leer(reinterpret_cast<uint8_t*>(tempBuffer), availableBytes);
      
      // Asegura que el buffer termine en nulo
      tempBuffer[bytesLeidos] = '\0'; 
      
      String result(tempBuffer);
      delete[] tempBuffer; // Libera la memoria
      return result;
    }
    return ""; // Devuelve un String vacío si no hay datos
  }
  
  virtual bool dormir() { return true; }
  virtual bool despertar() { return true; }
};

#endif