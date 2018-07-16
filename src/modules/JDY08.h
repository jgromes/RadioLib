#ifndef _KITELIB_JDY08_H
#define _KITELIB_JDY08_H

#include "ISerial.h"

class JDY08: public ISerial {
  public:
    // constructor
    JDY08(Module* mod);
    
    // basic methods
    void begin(long speed);
};

#endif
