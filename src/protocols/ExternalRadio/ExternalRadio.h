#if !defined(_RADIOLIB_EXTERNAL_RADIO_H)
#define _RADIOLIB_EXTERNAL_RADIO_H

#include "../../TypeDef.h"
#include "../../Module.h"

#include "../PhysicalLayer/PhysicalLayer.h"

class ExternalRadio: public PhysicalLayer {
  public:
    ExternalRadio();
    Module* getMod();
  private:
    Module* mod;
};

#endif