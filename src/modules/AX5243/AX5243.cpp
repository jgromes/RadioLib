#include "AX5243.h"
#if !defined(RADIOLIB_EXCLUDE_AX5243)

AX5243::AX5243(Module* module) : PhysicalLayer(AX5243_FREQUENCY_STEP_SIZE, AX5243_MAX_PACKET_LENGTH) {
  _mod = module;
}

int16_t AX5243::begin() {
  return(ERR_NONE);
}

#endif
