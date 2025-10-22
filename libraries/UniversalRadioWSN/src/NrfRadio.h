#ifndef NRF_RADIO_H
#define NRF_RADIO_H

#include "RadioInterface.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

/**
 * @struct NrfConfig
 * @brief Almacena los parámetros de configuración para el módulo NRF24L01.
 */
struct NrfConfig {
  uint8_t cePin;
  uint8_t csnPin;
  const byte* writeAddress; 
  const byte* readAddress;  
  uint8_t channel;
  rf24_datarate_e dataRate;
  rf24_pa_dbm_e paLevel;
};

/**
 * @class NrfRadio
 * @brief Implementación de RadioInterface para módulos NRF24L01 usando la librería RF24.
 */
class NrfRadio : public RadioInterface {
private:
  RF24 _radio;
  NrfConfig _config;

public:
  /**
   * @brief Constructor que configura el objeto RF24 con sus pines.
   */
  NrfRadio(const NrfConfig& config)
    : _radio(config.cePin, config.csnPin),
      _config(config) {}

  virtual ~NrfRadio() {}

  /**
   * @brief Inicializa el hardware NRF24L01 con la configuración proporcionada.
   */
  bool iniciar() override {
    if (!_radio.begin()) {
      return false; //Fallo
    }

    _radio.setChannel(_config.channel);
    _radio.setDataRate(_config.dataRate);
    _radio.setPALevel(_config.paLevel);
    
    //payloads dinámicos
    _radio.enableDynamicPayloads();

    // Configurar pipes para comunicación bidireccional
    _radio.openWritingPipe(_config.writeAddress);
    _radio.openReadingPipe(1, _config.readAddress); 

    // Por defecto, nos ponemos en modo escucha
    _radio.startListening();
    return true;
  }

  /**
   * @brief Envía un bloque de datos.
   * @return "true" si el envío fue exitoso, "false" en caso contrario.
   */
  bool enviar(const uint8_t* buffer, size_t longitud) override {
    _radio.stopListening(); 
    
    bool ok = _radio.write(buffer, longitud);
    
    _radio.startListening(); 
    return ok;
  }

  /**
   * @brief Comprueba si hay un paquete disponible y devuelve su tamaño.
   * @return El tamaño del payload dinámico recibido, o 0 si no hay nada.
   */
  int hayDatosDisponibles() override {
    if (_radio.available()) {
      return _radio.getDynamicPayloadSize();
    }
    return 0;
  }

  /**
   * @brief Lee un paquete disponible en el buffer.
   * @return El número de bytes leídos.
   */
  size_t leer(uint8_t* buffer, size_t maxLongitud) override {
    size_t payloadSize = _radio.getDynamicPayloadSize();
    if (payloadSize == 0) return 0;

    size_t bytesALeer = min(payloadSize, maxLongitud);
    _radio.read(buffer, bytesALeer);
    
    return bytesALeer;
  }

  /**
   * @brief Pone el módulo NRF24L01 en modo de bajo consumo.
   */
  bool dormir() override {
    _radio.powerDown();
    return true;
  }

  /**
   * @brief Saca al módulo NRF24L01 del modo de bajo consumo.
   */
  bool despertar() override {
    _radio.powerUp();
    
    
    delay(5); 
    return true;
  }
};

#endif 