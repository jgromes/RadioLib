#ifndef _KITELIB_SX1231_H
#define _KITELIB_SX1231_H

#include "TypeDef.h"
#include "Module.h"
#include "RF69.h"

#define SX1231_CHIP_REVISION_2_A                      0x21
#define SX1231_CHIP_REVISION_2_B                      0x22
#define SX1231_CHIP_REVISION_2_C                      0x23

//SX1231 specific register map
#define SX1231_REG_TEST_OOK                           0x6E

//SX1231_REG_TEST_OOK
#define SX1231_OOK_DELTA_THRESHOLHD                   0x0C

class SX1231: public RF69  {
  public:
    // constructor
    SX1231(Module* mod);
    
    // basic methods
    uint8_t begin(float freq = 434.0, float br = 48.0, float rxBw = 125.0, float freqDev = 50.0, int8_t power = 13);
  
  private:
    uint8_t _chipRevision;
};

#endif
