#ifndef _KITELIB_SX1279_H
#define _KITELIB_SX1279_H

#include "TypeDef.h"
#include "SX1278.h"

class SX1279: public SX1278 {
  public:
    // constructor
    SX1279(Module* mod);
    
    // configuration methods
    uint8_t setFrequency(float freq);
};

#endif
