#ifndef _KITELIB_HC05_H
#define _KITELIB_HC05_H

#include "ISerial.h"

class HC05: public ISerial {
  public:
    // constructor
    HC05(Module* mod);
    
    // basic methods
    void begin(long speed);
};

#endif
