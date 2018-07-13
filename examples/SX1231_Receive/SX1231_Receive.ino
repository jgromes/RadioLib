/*
 * KiteLib SX1231 Receive Example
 * 
 * This example receives packets using SX1231 FSK radio module. 
 */

// include the library
#include <KiteLib.h>

// SX1231 module is in slot A on the shield
SX1231 rf = Kite.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SX1231 with default settings
  Serial.print(F("[SX1231] Initializing ... "));
  // carrier frequency:                   434.0 MHz
  // bit rate:                            48.0 kbps
  // Rx bandwidth:                        125.0 kHz
  // frequency deviation:                 50.0 kHz
  // output power:                        13 dBm
  // sync word:                           0x2D  0x01
  byte state = rf.begin();
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }

  // you can change the sync word at runtime
  // sync word can be up to 8 non-zero bytes
  Serial.print(F("[SX1231] Settings sync word ... "));
  uint8_t syncWord[] = {0x01, 0x23};
  // sync word:               0x01  0x23
  // length:                  2
  // tolerated error bits:    0
  state == rf.setSyncWord(syncWord, 2);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else if(state == ERR_INVALID_SYNC_WORD) {
    Serial.println(F("invalid!"));
  }
}

void loop() {
  Serial.print(F("[SX1231] Waiting for incoming transmission ... "));

  // you can receive data as an Arduino String
  String str;
  byte state = rf.receive(str);

  // you can also receive data as byte array
  /*
  byte byteArr[8];
  byte state = rf.receive(byteArr, 8);
  */

  if(state == ERR_NONE) {
    // packet was successfully received
    Serial.println(F("success!"));

    // print the data of the packet
    Serial.print(F("[SX1231] Data:\t\t"));
    Serial.println(str);
    
  } else if(state == ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    Serial.println(F("timeout!"));
    
  } else if(state == ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("CRC error!"));
    
  }
}
