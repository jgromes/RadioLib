#ifndef _KITELIB_SX1273_H
#define _KITELIB_SX1273_H

#include "TypeDef.h"
#include "SX1272.h"

class SX1273: public SX1272 {
  public:
    // constructor
    SX1273(Module* mod);
    
    // configuration methods
    int16_t setSpreadingFactor(uint8_t sf);
};

#endif
