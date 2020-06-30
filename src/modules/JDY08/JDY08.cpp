#if !defined(_RADIOLIB_JDY08_H) && !defined(RADIOLIB_EXCLUDE_JDY08)
#include "JDY08.h"

JDY08::JDY08(Module* mod) : ISerial(mod) {

}

void JDY08::begin(long speed) {
  // set module properties
  _mod->AtLineFeed = "";
  _mod->baudrate = speed;
  _mod->init(RADIOLIB_USE_UART);
}

#endif
