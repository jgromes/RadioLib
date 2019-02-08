#ifndef _RADIOLIB_H
#define _RADIOLIB_H

#include "TypeDef.h"
#include "Module.h"

#include "modules/CC1101.h"
#include "modules/ESP8266.h"
#include "modules/HC05.h"
#include "modules/JDY08.h"
#include "modules/RF69.h"
#include "modules/RFM95.h"
#include "modules/RFM96.h"
#include "modules/RFM97.h"
#include "modules/SX1231.h"
#include "modules/SX1272.h"
#include "modules/SX1273.h"
#include "modules/SX1276.h"
#include "modules/SX1277.h"
#include "modules/SX1278.h"
#include "modules/SX1279.h"
#include "modules/XBee.h"

#include "protocols/PhysicalLayer.h"
#include "protocols/RTTY.h"

#include "protocols/TransportLayer.h"
#include "protocols/HTTP.h"
#include "protocols/MQTT.h"

#define RADIOSHIELD_CS_A   10
#define RADIOSHIELD_RX_A   9
#define RADIOSHIELD_TX_A   8
#define RADIOSHIELD_CS_B   5
#define RADIOSHIELD_RX_B   7
#define RADIOSHIELD_TX_B   6
#define RADIOSHIELD_INT_0  2
#define RADIOSHIELD_INT_1  3

class Radio {
  public:
    Radio();
    
    Module* ModuleA;
    Module* ModuleB;
  
  private:
    
  
};

extern Radio RadioShield;

#endif
