/**
 * @file UniversalRadioWSN.h
 * @brief Archivo maestro de inclusión para la librería UniversalRadioWSN.
 * @details Este es el único archivo que se necesita incluir.
 * Agrupa todas las interfaces y clases de radio
 * concretas (LoRa, NRF, Xbee).
 *
 * Al incluir este archivo, tienes acceso a:
 * - RadioInterface (La clase base abstracta)
 * - LoraRadio (Implementación para LoRa)
 * - NrfRadio (Implementación para NRF24L01)
 * - XbeeRadio (Implementación para Xbee)
 */

#ifndef UNIVERSAL_RADIO_WSN_H
#define UNIVERSAL_RADIO_WSN_H

// Estos archivos incluyen todas las clases de la librería.
#include "RadioInterface.h"
#include "LoraRadio.h"
#include "XbeeRadio.h"
#include "NrfRadio.h" 

#endif 
