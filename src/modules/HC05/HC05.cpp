#include "HC05.h"

HC05::HC05(Module* mod) : ISerial(mod) {
  
}

void HC05::begin(long speed) {
  // set module properties
  _mod->baudrate = speed;
  _mod->init(RADIOLIB_USE_UART, RADIOLIB_INT_NONE);
}
