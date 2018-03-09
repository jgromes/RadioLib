#ifndef _KITELIB_XBEE_H
#define _KITELIB_XBEE_H

#include "Module.h"
#include "TypeDef.h"

class XBee {
  public:
    XBee(Module* module);
    
    void begin(long speed);
    uint8_t send(uint32_t destinationAddressHigh, uint32_t destinationAddressLow, const char* data);
  private:
    Module* _mod;
};

#endif
