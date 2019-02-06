/*
   KiteLib CC1101 Transmit Example

   This example transmits packets using RF69 FSK radio module.
*/

// include the library
#include <KiteLib.h>

// CC1101 is in slot A on the shield
CC1101 cc = Kite.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize CC1101
  Serial.print(F("[CC1101] Initializing ... "));
  // carrier frequency:                   868.0 MHz
  // bit rate:                            115.2 kbps
  // Rx bandwidth:                        203 kHz
  // frequency deviation:                 48.0 kHz
  int state = cc.begin(434);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }
}

void loop() {
  Serial.print(F("[CC1101] Transmitting packet ... "));

  // you can transmit C-string or Arduino string up to 255 characters long
  int state = cc.transmit("Hello World!");

  // you can also transmit byte array up to 255 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    int state = cc.transmit(byteArr, 8);
  */

  if (state == ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(" success!");

  } else if (state == ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 255 bytes
    Serial.println(" too long!");

  }

  // wait for a second before transmitting again
  delay(1000);
}
