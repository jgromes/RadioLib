#ifndef _KITELIB_HC05_H
#define _KITELIB_HC05_H

#include "ISerial.h"

class HC05: public ISerial {
  public:
    HC05(Module* module);
    
    void begin(long speed);
};

#endif
