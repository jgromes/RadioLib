#include "SX1276.h"

SX1276::SX1276(Module* mod) : SX1278(mod) {
  
}

uint8_t SX1276::setFrequency(float freq) {
  // check frequency range
  if((freq < 137.0) || (freq > 1020.0)) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  // set frequency
  return(SX1278::setFrequencyRaw(freq));
}
