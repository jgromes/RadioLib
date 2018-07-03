#ifndef _KITELIB_SX1273_H
#define _KITELIB_SX1273_H

#include "TypeDef.h"
#include "SX1272.h"

class SX1273: public SX1272 {
  public:
    SX1273(Module* mod);
  
  private:
    uint8_t config(uint32_t bw, uint8_t sf, uint8_t cr, float freq, uint8_t syncWord);
};

#endif
