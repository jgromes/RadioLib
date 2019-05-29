#ifndef _RADIOLIB_SIM800_H
#define _RADIOLIB_SIM800_H

#include "Module.h"

class SIM800 {
  public:
    // constructor
    SIM800(Module* module);
    
    // basic methods
    int16_t begin(long speed, const char* pin = "1234");
    void shutdown();
    int16_t sendSMS(const char* num, const char* msg);
  
  private:
    Module* _mod;
};

#endif
