#include "XBee.h"

XBee::XBee(Module* module) {
  _mod = module;
}

void XBee::begin(long speed) {
  _mod->baudrate = speed;
  _mod->init(USE_UART, INT_NONE);
}

uint8_t XBee::send(uint32_t destinationAddressHigh, uint32_t destinationAddressLow, const char* data) {
  
}
