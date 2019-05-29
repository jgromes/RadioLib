/*
   RadioLib Pager (POCSAG) Transmit Example

   This example sends POCSAG messages using SX1278's
   FSK modem.

   Other modules that can be used for POCSAG:
    - SX127x/RFM9x
    - RF69
    - SX1231
    - CC1101
    - SX126x
*/

// include the library
#include <RadioLib.h>

// SX1278 module is in slot A on the shield
SX1278 fsk = RadioShield.ModuleA;

// create Pager client instance using the FSK module
PagerClient pager(&fsk);

void setup() {
  Serial.begin(9600);

  // initialize SX1278
  Serial.print(F("[SX1278] Initializing ... "));
  // carrier frequency:           434.0 MHz
  // bit rate:                    48.0 kbps
  // frequency deviation:         50.0 kHz
  // Rx bandwidth:                125.0 kHz
  // output power:                13 dBm
  // current limit:               100 mA
  // sync word:                   0x2D  0x01
  int state = fsk.beginFSK();
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  // initalize Pager client
  Serial.print(F("[Pager] Initializing ... "));
  // base (center) frequency:     434.0 MHz
  // speed:                       1200 bps
  state = pager.begin(434.0, 1200);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }
}

void loop() {
  // transmit numeric (BCD) message to the destination pager
  Serial.print(F("[Pager] Transmitting message ... "));
  int state = pager.transmit("0123456789*U -()", 0x01234567);
  // NOTE: Only characters 0123456789*U-() and space 
  //       can be sent in a BCD message! To send ASCII
  //       characters, you have to set encoding to ASCII.
  /*
    int state = pager.transmit("Hello World!", 0x01234567, ASCII);
  */
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // wait for a second before transmitting again
  delay(1000);
}
