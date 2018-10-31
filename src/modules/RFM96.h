#ifndef _KITELIB_RFM96_H
#define _KITELIB_RFM96_H

#include "TypeDef.h"
#include "Module.h"
#include "SX127x.h"
#include "SX1278.h"

// SX127X_REG_VERSION
#define RFM9X_CHIP_VERSION                            0x12  // according to datasheet, this should be 0x11, but all modules seem to have 0x12

class RFM96: public SX1278 {
  public:
    // constructor
    RFM96(Module* mod);
    
    // basic methods
    int16_t begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = SX127X_SYNC_WORD, int8_t power = 17, uint8_t currentLimit = 100, uint16_t preambleLength = 8, uint8_t gain = 0);
    
    // configuration methods
    int16_t setFrequency(float freq);
    
  private:
  
};

using RFM98 = RFM96;

#endif
