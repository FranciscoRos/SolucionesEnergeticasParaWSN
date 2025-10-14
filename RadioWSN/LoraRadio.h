#pragma once
#include "RadioInterface.h"
#include <LoRa.h> // Necesitas tener instalada la librería "LoRa by Sandeep Mistry"

class LoRaRadio : public RadioInterface {
private:
    long _frequency;

public:
    /**
     * @brief Constructor de la clase LoRaRadio.
     * @param frequency La frecuencia de operación para tu región (ej. 915E6 para América, 868E6 para Europa, 433E6 para Asia).
     */
    LoRaRadio(long frequency) : _frequency(frequency) {}

    /**
     * @brief Inicializa el módulo LoRa con la frecuencia especificada y los pines SPI por defecto.
     * @return true si el módulo se inició correctamente, false en caso contrario.
     */
    bool iniciar() override {
        // La librería LoRa usa los pines SPI estándar por defecto.
        // Si usas pines no estándar, puedes configurarlos con LoRa.setPins(cs, reset, irq);
        // antes de llamar a LoRa.begin().
        if (!LoRa.begin(_frequency)) {
        return false; // Falla si no se puede iniciar
        }
    // AÑADE ESTO PARA ASEGURAR COMPATIBILIDAD
    LoRa.setSpreadingFactor(7);
    LoRa.setSignalBandwidth(125E3);
    return true; // Éxito
    }

    /**
     * @brief Pone el módulo LoRa en modo de bajo consumo.
     * @return Siempre devuelve true.
     */
    bool dormir() override {
        LoRa.sleep();
        return true;
    }

    /**
     * @brief Saca al módulo LoRa del modo de bajo consumo y lo pone en modo de espera (listo para TX/RX).
     * @return Siempre devuelve true.
     */
    bool despertar() override {
        LoRa.idle(); // El modo 'idle' es el modo de espera estándar.
        return true;
    }

    /**
     * @brief Envía un paquete de datos binarios.
     * @param buffer El puntero al arreglo de bytes a enviar.
     * @param longitud El número de bytes a enviar.
     * @return true si el paquete se envió, false si hubo un error.
     */
    bool enviar(const uint8_t* buffer, size_t longitud) override {
        if (LoRa.beginPacket()) {
            LoRa.write(buffer, longitud);
            LoRa.endPacket();
            return true;
        }
        return false;
    }

    /**
     * @brief Comprueba si se ha recibido un nuevo paquete de datos.
     * @return El tamaño en bytes del paquete recibido, o 0 si no hay ninguno.
     */
    int hayDatosDisponibles() override {
        // parsePacket() detecta un nuevo paquete y devuelve su tamaño.
        return LoRa.parsePacket();
    }

    /**
     * @brief Lee los datos de un paquete recibido en un buffer.
     * @param buffer El puntero al arreglo donde se guardarán los datos.
     * @param maxLongitud El tamaño máximo del buffer.
     * @return El número de bytes leídos.
     */
    size_t leer(uint8_t* buffer, size_t maxLongitud) override {
        size_t bytesLeidos = 0;
        while (LoRa.available() && bytesLeidos < maxLongitud) {
            buffer[bytesLeidos] = (uint8_t)LoRa.read();
            bytesLeidos++;
        }
        return bytesLeidos;
    }
};
