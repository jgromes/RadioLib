#include "ExternalRadio.h"

ExternalRadio::ExternalRadio() : PhysicalLayer(1, 0) {
  mod = new Module(RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC);
}

Module* ExternalRadio::getMod() {
  return(mod);
}