#ifndef _KITELIB_PHYSICAL_LAYER_H
#define _KITELIB_PHYSICAL_LAYER_H

#include "TypeDef.h"

class PhysicalLayer {
  public:
    // constructor
    PhysicalLayer(float crysFreq, uint8_t divExp) { _crystalFreq = crysFreq; _divExponent = divExp; }
    
    // basic methods
    virtual int16_t transmitDirect(uint32_t FRF = 0) = 0;
    virtual int16_t receiveDirect() = 0;
    virtual int16_t transmit(const char* data, uint8_t addr = 0) = 0;
    virtual int16_t transmit(uint8_t* data, size_t len, uint8_t addr = 0) = 0;
    virtual int16_t receive(uint8_t* data, size_t len) = 0;
    
    // configuration methods
    virtual int16_t setFrequencyDeviation(float freqDev) = 0;
    float getCrystalFreq() { return(_crystalFreq); }
    uint8_t getDivExponent() { return(_divExponent); }

  private:
    float _crystalFreq;
    uint8_t _divExponent;
};

#endif
