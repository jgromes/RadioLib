#include "JDY08.h"

JDY08::JDY08(Module* module) {
  _mod = module;
}

void JDY08::begin(long speed) {
  // set module properties
  _mod->AtLineFeed = "";
  _mod->baudrate = speed;
  _mod->init(USE_UART, INT_NONE);
}
