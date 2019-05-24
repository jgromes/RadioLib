/*
   RadioLib CC1101 Receive Example

   This example receives packets using CC1101 FSK radio
   module.
*/

// include the library
#include <RadioLib.h>

// CC1101 is in slot A on the shield
CC1101 cc = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize CC1101 with default settings
  Serial.print(F("[CC1101] Initializing ... "));
  // carrier frequency:                   868.0 MHz
  // bit rate:                            4.8 kbps
  // Rx bandwidth:                        325.0 kHz
  // frequency deviation:                 48.0 kHz
  // sync word:                           0xD391
  int state = cc.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop() {
  Serial.print(F("[CC1101] Waiting for incoming transmission ... "));

  // you can receive data as an Arduino String
  String str;
  int state = cc.receive(str);

  // you can also receive data as byte array
  /*
    byte byteArr[8];
    int state = cc.receive(byteArr, 8);
  */

  if (state == ERR_NONE) {
    // packet was successfully received
    Serial.println(F("success!"));

    // print the data of the packet
    Serial.print(F("[CC1101] Data:\t\t"));
    Serial.println(str);

    // print RSSI (Received Signal Strength Indicator)
    // of the last received packet
    Serial.print(F("[CC1101] RSSI:\t\t"));
    Serial.print(cc.getRSSI());
    Serial.println(F(" dBm"));

    // print LQI (Link Quality Indicator)
    // of the last received packet, lower is better
    Serial.print(F("[CC1101] LQI:\t\t"));
    Serial.println(cc.getLQI());

  } else if (state == ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("CRC error!"));

  }
}
