#ifndef _KITELIB_SX1276_H
#define _KITELIB_SX1276_H

#include "TypeDef.h"
#include "SX1278.h"

class SX1276: public SX1278 {
  public:
    // constructor
    SX1276(Module* mod);
    
    // configuration methods
    int16_t setFrequency(float freq);
};

#endif
