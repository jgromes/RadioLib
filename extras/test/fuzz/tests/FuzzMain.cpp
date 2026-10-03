#include <stdint.h>
#include <stddef.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  extern int FuzzLoRaWANDownlink(const uint8_t* data, size_t size);
  FuzzLoRaWANDownlink(data, size);

  extern int FuzzLoRaWANPersistBuffer(const uint8_t* data, size_t size);
  FuzzLoRaWANPersistBuffer(data, size);

  extern int FuzzPager(const uint8_t* data, size_t size);
  FuzzPager(data, size);

  return(0);
}
