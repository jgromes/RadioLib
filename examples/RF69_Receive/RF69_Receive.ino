/*
 * KiteLib RF69 Receive Example
 * 
 * This example receives packets using RF69 FSK radio module. 
 */

// include the library
#include <KiteLib.h>

// RF69 module is in slot A on the shield
RF69 rf = Kite.ModuleA;

// create empty instance of Packet class
Packet pack;

void setup() {
  Serial.begin(9600);

  // initialize RF69 with default settings
  Serial.print(F("[RF69] Initializing ... "));
  // carrier frequency:                   434.0 MHz
  // bit rate:                            48.0 kbps
  // Rx bandwidth:                        125.0 kHz
  // frequency deviation:                 50.0 kHz
  // output power:                        13 dBm
  byte state = rf.begin();
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }
}

void loop() {
  Serial.print(F("[RF69] Waiting for incoming transmission ... "));

  // wait for single packet
  byte state = rf.receive(pack);

  if(state == ERR_NONE) {
    // packet was successfully received
    Serial.println(F("success!"));

    // print the source of the packet
    Serial.print(F("[RF69] Source:\t\t"));
    Serial.println(pack.getSourceStr());

    // print the destination of the packet
    Serial.print(F("[RF69] Destination:\t"));
    Serial.println(pack.getDestinationStr());

    // print the length of the packet
    Serial.print(F("[RF69] Length:\t\t"));
    Serial.println(pack.length);

    // print the data of the packet
    Serial.print(F("[RF69] Data:\t\t"));
    Serial.println(pack.data);
    
  } else if(state == ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    Serial.println(F("timeout!"));
    
  } else if(state == ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("CRC error!"));
    
  }
}
