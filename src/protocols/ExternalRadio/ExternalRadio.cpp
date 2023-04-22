#include "ExternalRadio.h"

#if defined(RADIOLIB_BUILD_ARDUINO)
ExternalRadio::ExternalRadio() : PhysicalLayer(1, 0) {
  mod = new Module(RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC);
}
#endif

ExternalRadio::ExternalRadio(RadioLibHal *hal) : PhysicalLayer(1, 0) {
  mod = new Module(hal, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC);
}

Module* ExternalRadio::getMod() {
  return(mod);
}