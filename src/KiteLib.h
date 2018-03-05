#ifndef _KITELIB_H
#define _KITELIB_H

#include "TypeDef.h"
#include "Module.h"

#include "ESP8266.h"
#include "SX1278.h"
#include "SX1272.h"
#include "HC05.h"

#define KITE_CS_A   10
#define KITE_TX_A   9
#define KITE_RX_A   8
#define KITE_CS_B   5
#define KITE_TX_B   7
#define KITE_RX_B   6
#define KITE_INT_0  2
#define KITE_INT_1  3

class KiteShield {
  public:
    KiteShield();
    
    Module* ModuleA;
    Module* ModuleB;
  
  private:
    
  
};

extern KiteShield Kite;

#endif
