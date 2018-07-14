/*
 * KiteLib SX127x Settings Example
 * 
 * This example shows how to change all the properties of RF69 radio.
 * KiteLib currently supports the following settings:
 *  - pins (SPI slave select, digital IO 0, digital IO 1)
 *  - carrier frequency
 *  - bit rate
 *  - receiver bandwidth
 *  - allowed frequency deviation
 *  - output power during transmission
 *  - sync word
 */

// include the library
#include <KiteLib.h>

// SX1278 module is in slot A on the shield
RF69 rf1 = Kite.ModuleA;

// if you're not using Kite shield, you can specify
// the connection yourself
// NSS pin:   6
// DIO0 pin:  4
// DIO1 pin:  5
RF69 rf2 = new Module(6, 4, 5);

void setup() {
  Serial.begin(9600);
  
  // initialize RF69 with default settings
  Serial.print(F("[RF69] Initializing ... "));
  // carrier frequency:                   434.0 MHz
  // bit rate:                            48.0 kbps
  // Rx bandwidth:                        125.0 kHz
  // frequency deviation:                 50.0 kHz
  // output power:                        13 dBm
  // sync word:                           0x2D  0x01
  byte state = rf1.begin();
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }

  // initialize RF69 with non-default settings
  Serial.print(F("[RF69] Initializing ... "));
  // carrier frequency:                   868.0 MHz
  // bit rate:                            300.0 kbps
  // Rx bandwidth:                        250.0 kHz
  // frequency deviation:                 60.0 kHz
  // output power:                        17 dBm
  // sync word:                           0x2D  0x01
  state = rf1.begin(868.0, 300.0, 250.0, 60.0, 17);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }

  // you can also change the settings at runtime
  // and check if the configuration was changed successfully

  // set carrier frequency to 433.5 MHz
  if(rf1.setFrequency(433.5) == ERR_INVALID_FREQUENCY) {
    Serial.println("Selected frequency is invalid for this module!");
    while(true);
  }

  // set bit rate to 100.0 kbps
  state = rf1.setBitRate(100.0);
  if(state == ERR_INVALID_BIT_RATE) {
    Serial.println("Selected bit rate is invalid for this module!");
    while(true);
  } else if(state == ERR_INVALID_BIT_RATE_BW_RATIO) {
    Serial.println("Selected bit rate to bandwidth ratio is invalid!");
    Serial.println("Increase receiver bandwidth to set this bit rate.");
    while(true);
  }

  // set receiver bandwidth to 250.0 kHz
  state = rf1.setRxBandwidth(250.0);
  if(state == ERR_INVALID_RX_BANDWIDTH) {
    Serial.println("Selected receiver bandwidth is invalid for this module!");
    while(true);
  } else if(state == ERR_INVALID_BIT_RATE_BW_RATIO) {
    Serial.println("Selected bit rate to bandwidth ratio is invalid!");
    Serial.println("Decrease bit rate to set this receiver bandwidth.");
    while(true);
  }

  // set allowed frequency deviation to 10.0 kHz
  if(rf1.setFrequencyDeviation(10.0) == ERR_INVALID_FREQUENCY_DEVIATION) {
    Serial.println("Selected frequency deviation is invalid for this module!");
    while(true);
  }

  // set output power to 2 dBm
  if(rf1.setOutputPower(2) == ERR_INVALID_OUTPUT_POWER) {
    Serial.println("Selected output power is invalid for this module!");
    while(true);
  }

  // up to 8 bytes can be set as sync word
  // NOTE: sync word must not conatin any zero bytes
  // set sync word to 0x01 0x23 0x45 0x67 0x89 0xAB 0xCD 0xEF
  uint8_t syncWord[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
  if(rf1.setSyncWord(syncWord, 8) == ERR_INVALID_SYNC_WORD) {
    Serial.println("Selected sync word is invalid for this module!");
    while(true);
  }  
}

void loop() {
  // nothing here
}
