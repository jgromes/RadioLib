#if !defined(_RADIOLIB_EXTERNAL_RADIO_H)
#define _RADIOLIB_EXTERNAL_RADIO_H

#include "../../TypeDef.h"
#include "../../Module.h"
#if defined(RADIOLIB_BUILD_ARDUINO)
#include "../../ArduinoHal.h"
#endif

#include "../PhysicalLayer/PhysicalLayer.h"

class ExternalRadio: public PhysicalLayer {
  public:
    #if defined(RADIOLIB_BUILD_ARDUINO)
    ExternalRadio();
    #endif
    ExternalRadio(RadioLibHal *hal);

    Module* getMod();
  private:
    Module* mod;
};

#endif