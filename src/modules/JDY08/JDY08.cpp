#include "JDY08.h"

JDY08::JDY08(Module* mod) : ISerial(mod) {
  
}

void JDY08::begin(long speed) {
  // set module properties
  _mod->AtLineFeed = "";
  _mod->baudrate = speed;
  _mod->init(RADIOLIB_USE_UART, RADIOLIB_INT_NONE);
}
