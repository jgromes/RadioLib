#ifndef _RADIOLIB_H
#define _RADIOLIB_H

/*!
  \mainpage RadioLib Documentation

  Universal wireless communication library for Arduino.

  \par Currently Supported Wireless Modules and Protocols
  - CC1101 FSK module
  - HC05 Bluetooth module
  - JDY08 BLE module
  - RF69 FSK module
  - SX126x LoRa/FSK module
  - SX127x LoRa/FSK module
  - SX1231 FSK module
  - XBee module (S2B)
  - PhysicalLayer protocols
    - RTTY
    - Morse Code
  - TransportLayer protocols
    - HTTP
    - MQTT

  \par Quick Links
  Documentation for most common methods can be found in its reference page (see the list above).\n
  Some methods (mainly configuration) are also overridden in derived classes, such as SX1272, SX1278, RFM96 etc. for SX127x.\n
  \ref status_codes have their own page.\n
  Some modules implement methods of one or more compatibility layers, loosely based on the ISO/OSI model.
  - PhysicalLayer - FSK and LoRa radio modules
  - TransportLayer - Modules with Internet connectivity

  \see https://github.com/jgromes/RadioLib

  \copyright  Copyright (c) 2019 Jan Gromes
*/

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
#include "modules/SX1261.h"
#include "modules/SX1262.h"
#include "modules/SX1268.h"
#include "modules/SX1272.h"
#include "modules/SX1273.h"
#include "modules/SX1276.h"
#include "modules/SX1277.h"
#include "modules/SX1278.h"
#include "modules/SX1279.h"
#include "modules/XBee.h"

// physical layer protocols
#include "protocols/PhysicalLayer.h"
#include "protocols/Morse.h"
#include "protocols/RTTY.h"

// transport layer protocols
#include "protocols/TransportLayer.h"
#include "protocols/HTTP.h"
#include "protocols/MQTT.h"

// RadioShield pin definitions
#define RADIOSHIELD_CS_A   10
#define RADIOSHIELD_RX_A   9
#define RADIOSHIELD_TX_A   8
#define RADIOSHIELD_CS_B   5
#define RADIOSHIELD_RX_B   7
#define RADIOSHIELD_TX_B   6
#define RADIOSHIELD_INT_0  2
#define RADIOSHIELD_INT_1  3

/*!
  \class Radio

  \brief Library control object when using RadioShield.
  Contains two pre-configured "modules", which correspond to the slots on shield.
*/

class Radio {
  public:

    /*!
      \brief Default constructor. Only used to set ModuleA and ModuleB configuration.
    */
    Radio();

    Module* ModuleA;
    Module* ModuleB;

  private:

};

extern Radio RadioShield;

#endif
