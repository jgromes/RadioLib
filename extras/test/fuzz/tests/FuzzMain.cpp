#include <stdint.h>
#include <stddef.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  extern int FuzzLoRaWAN(const uint8_t* data, size_t size);
  FuzzLoRaWAN(data, size);

  extern int FuzzPager(const uint8_t* data, size_t size);
  FuzzPager(data, size);

  return(0);
}
