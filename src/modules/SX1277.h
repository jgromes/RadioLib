#ifndef _KITELIB_SX1277_H
#define _KITELIB_SX1277_H

#include "TypeDef.h"
#include "SX1278.h"

class SX1277: public SX1278 {
  public:
    SX1277(Module* mod);
  
  private:
    uint8_t config(uint32_t bw, uint8_t sf, uint8_t cr, float freq, uint8_t syncWord);
};

#endif
