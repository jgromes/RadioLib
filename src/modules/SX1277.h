#ifndef _KITELIB_SX1277_H
#define _KITELIB_SX1277_H

#include "TypeDef.h"
#include "SX1278.h"

class SX1277: public SX1278 {
  public:
    // constructor
    SX1277(Module* mod);
    
    // basic methods
    int16_t begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = SX127X_SYNC_WORD, int8_t power = 17, uint8_t currentLimit = 100, uint16_t preambleLength = 8, uint8_t gain = 0);
    
    // configuration methods
    int16_t setFrequency(float freq);
    int16_t setSpreadingFactor(uint8_t sf);
};

#endif
