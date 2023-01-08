#include "AX5243.h"
#if !defined(RADIOLIB_EXCLUDE_AX5X43)

AX5243::AX5243(Module* mod) : AX5x43(mod) {

}

int16_t AX5243::begin(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength) {
  int16_t state = AX5x43::begin(freq, br, freqDev, rxBw, power, preambleLength);
  return(state);
}

#endif
