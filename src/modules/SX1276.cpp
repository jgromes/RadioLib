#include "SX1276.h"

SX1276::SX1276(Module* mod) : SX1278(mod) {
  
}

uint8_t SX1276::setFrequency(float freq) {
  // check frequency range
  if((freq < 137.0) || (freq > 1020.0)) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  uint8_t state = SX1278::setFrequencyRaw(freq);
  if(state == ERR_NONE) {
    SX127x::_freq = freq;
  }
  return(state);
}
