/*
   RadioLib Si443x Receive Example

   This example receives packets using Si443x FSK radio module.
   To successfully receive data, the following settings have to be the same
   on both transmitter and receiver:
    - carrier frequency
    - bit rate
    - frequency deviation
    - sync word

   Other modules from Si443x/RFM2x family can also be used.

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// Si4432 has the following connections:
// nSEL pin:  10
// nIRQ pin:  2
// SDN pin:   9
Si4432 fsk = new Module(10, 2, 9);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//Si4432 fsk = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize Si4432 with default settings
  Serial.print(F("[Si4432] Initializing ... "));
  // carrier frequency:           434.0 MHz
  // bit rate:                    48.0 kbps
  // frequency deviation:         50.0 kHz
  // Rx bandwidth:                225.1 kHz
  // output power:                11 dBm
  // sync word:                   0x2D  0x01
  int state = fsk.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop() {
  Serial.print(F("[Si4432] Waiting for incoming transmission ... "));

  // you can receive data as an Arduino String
  String str;
  int state = fsk.receive(str);

  // you can also receive data as byte array
  /*
    byte byteArr[8];
    int state = rf.receive(byteArr, 8);
  */

  if (state == ERR_NONE) {
    // packet was successfully received
    Serial.println(F("success!"));

    // print the data of the packet
    Serial.print(F("[Si4432] Data:\t\t"));
    Serial.println(str);

  } else if (state == ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    Serial.println(F("timeout!"));

  } else if (state == ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("CRC error!"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);

  }
}
