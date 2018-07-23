#ifndef _KITELIB_SX1277_H
#define _KITELIB_SX1277_H

#include "TypeDef.h"
#include "SX1278.h"

class SX1277: public SX1278 {
  public:
    // constructor
    SX1277(Module* mod);
    
    // configuration methods
    int16_t setFrequency(float freq);
    int16_t setSpreadingFactor(uint8_t sf);
};

#endif
