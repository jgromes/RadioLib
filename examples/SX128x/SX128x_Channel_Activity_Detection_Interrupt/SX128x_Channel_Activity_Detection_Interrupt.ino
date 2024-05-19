/*
   RadioLib SX128x Channel Activity Detection Example

   This example uses SX1280 to scan the current LoRa
   channel and detect ongoing LoRa transmissions.

   Other modules from SX128x family can also be used.

   For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx128x---lora-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1280 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
SX1280 radio = new Module(10, 2, 3, 9);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1280 radio = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SX1280 with default settings
  Serial.print(F("[SX1280] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set the function that will be called
  // when LoRa packet or timeout is detected
  radio.setDio1Action(setFlag);

  // start scanning the channel
  Serial.print(F("[SX1280] Starting scan for LoRa preamble ... "));
  state = radio.startChannelScan();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

// flag to indicate that a packet was detected or CAD timed out
volatile bool scanFlag = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // something happened, set the flag
  scanFlag = true;
}

void loop() {
  // check if the flag is set
  if(scanFlag) {
    // reset flag
    scanFlag = false;

    // check CAD result
    int state = radio.getChannelScanResult();

    if (state == RADIOLIB_LORA_DETECTED) {
      // LoRa packet was detected
      Serial.println(F("[SX1280] Packet detected!"));

    } else if (state == RADIOLIB_CHANNEL_FREE) {
      // channel is free
      Serial.println(F("[SX1280] Channel is free!"));

    } else {
      // some other error occurred
      Serial.print(F("[SX1280] Failed, code "));
      Serial.println(state);

    }

    // start scanning the channel again
    Serial.print(F("[SX1280] Starting scan for LoRa preamble ... "));
    state = radio.startChannelScan();
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
    }
  }
}
