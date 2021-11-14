#include "AFSK.h"
#if !defined(RADIOLIB_EXCLUDE_AFSK)

AFSKClient::AFSKClient(PhysicalLayer* phy, RADIOLIB_PIN_TYPE pin): _pin(pin) {
  _phy = phy;
}

int16_t AFSKClient::begin() {
  return(_phy->startDirect());
}

int16_t AFSKClient::tone(uint16_t freq, bool autoStart) {
  if(freq == 0) {
    return(RADIOLIB_ERR_INVALID_FREQUENCY);
  }

  if(autoStart) {
    int16_t state = _phy->transmitDirect();
    RADIOLIB_ASSERT(state);
  }

  Module* mod = _phy->getMod();
  mod->tone(_pin, freq);
  return(RADIOLIB_ERR_NONE);
}

int16_t AFSKClient::noTone() {
  Module* mod = _phy->getMod();
  mod->noTone(_pin);
  return(_phy->standby());
}

#endif
