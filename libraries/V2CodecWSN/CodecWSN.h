#pragma once
#include <stdint.h>

// Palabra de sincronización para asegurar que siempre leemos un paquete completo.
const uint16_t SYNCWORD = 0xABCD;

// Estructura del paquete de datos que se enviará y se guardará en EEPROM.
// Total: 14 bytes (2 sync + 2 id + 2 voltaje + 2 corriente + 2 vbat + 4 timestamp)
struct Packet {
  uint16_t sync;      // Siempre será 0xABCD
  uint16_t id;
  int16_t  voltaje;   // centésimas de volt
  int16_t  corriente; // mA (signed)
  uint16_t vbat;      // centésimas de volt
  uint32_t timestamp; // Timestamp de Unix (segundos desde 1970-01-01)
};

constexpr size_t PACKET_SIZE = sizeof(Packet);

// --- Codificador: Convierte la estructura a un arreglo de bytes ---
inline void encodePacket(uint8_t *buf, const Packet &p) {
  const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(&p);
  for(size_t i = 0; i < PACKET_SIZE; ++i) {
    buf[i] = byte_ptr[i];
  }
}

// --- Decodificador: Convierte un arreglo de bytes de vuelta a la estructura ---
inline Packet decodePacket(const uint8_t *buf) {
  Packet p;
  uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(&p);
  for(size_t i = 0; i < PACKET_SIZE; ++i) {
    byte_ptr[i] = buf[i];
  }
  return p;
}