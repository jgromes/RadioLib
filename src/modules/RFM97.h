#ifndef _KITELIB_RFM97_H
#define _KITELIB_RFM97_H

#include "TypeDef.h"
#include "Module.h"
#include "SX127x.h"
#include "SX1278.h"
#include "RFM95.h"

class RFM97: public RFM95 {
  public:
    // contructor
    RFM97(Module* mod);
    
    // configuration methods
    int16_t setSpreadingFactor(uint8_t sf);
  
  private:
};

#endif
