#include "HC05.h"

HC05::HC05(Module* module) {
  _mod = module;
}

void HC05::begin(long speed) {
  _mod->baudrate = speed;
  _mod->init(USE_UART, INT_NONE);
}
