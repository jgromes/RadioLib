/*
   KiteLib SX1231 Transmit Example

   This example transmits packets using SX1231 FSK radio module.
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
  int state = rf.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // you can change the sync word at runtime
  // sync word can be up to 8 non-zero bytes
  Serial.print(F("[SX1231] Settings sync word ... "));
  uint8_t syncWord[] = {0x01, 0x23};
  // sync word:               0x01  0x23
  // length:                  2
  // tolerated error bits:    0
  state == rf.setSyncWord(syncWord, 2);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else if (state == ERR_INVALID_SYNC_WORD) {
    Serial.println(F("invalid!"));
  }
}

void loop() {
  Serial.print(F("[SX1231] Transmitting packet ... "));

  // you can transmit C-string or Arduino string up to 256 characters long
  int state = rf.transmit("Hello World!");

  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD, 0xEF};
    int state = rf.transmit(byteArr, 8);
  */

  if (state == ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(" success!");

  } else if (state == ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(" too long!");

  }

  // wait for a second before transmitting again
  delay(1000);
}
