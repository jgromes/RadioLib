/*
   RadioLib RF69 Transmit with Interrupts Example

   This example transmits FSK packets with one second delays
   between them. Each packet contains up to 64 bytes
   of data, in the form of:
    - Arduino String
    - null-terminated char array (C-string)
    - arbitrary binary data (byte array)

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// RF69 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// DIO1 pin:  3
RF69 rf = new Module(10, 2, 3);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//RF69 rf = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize RF69 with default settings
  Serial.print(F("[RF69] Initializing ... "));
  // carrier frequency:                   434.0 MHz
  // bit rate:                            48.0 kbps
  // Rx bandwidth:                        125.0 kHz
  // frequency deviation:                 50.0 kHz
  // output power:                        13 dBm
  // sync word:                           0x2D01
  int state = rf.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set the function that will be called
  // when packet transmission is finished
  rf.setDio0Action(setFlag);

  // start transmitting the first packet
  Serial.print(F("[RF69] Sending first packet ... "));

  // you can transmit C-string or Arduino string up to
  // 64 characters long
  state = rf.startTransmit("Hello World!");

  // you can also transmit byte array up to 64 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56,
                      0x78, 0xAB, 0xCD, 0xEF};
    state = rf.transmit(byteArr, 8);
  */

  if (state != ERR_NONE) {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

// flag to indicate that a packet was received
volatile bool transmittedFlag = false;

void setFlag(void) {
  // packet transmission is finished, set the flag
  transmittedFlag = true;
}

void loop() {
  // check if the previous transmission finished
  if(transmittedFlag) {
    Serial.println(F("packet transmission finished!"));

    // wait one second before next transmission
    delay(1000);

    // send another packet
    Serial.print(F("[RF69] Sending another packet ... "));

    // you can transmit C-string or Arduino string up to
    // 64 characters long
    int state = rf.startTransmit("Hello World!");

    // you can also transmit byte array up to 256 bytes long
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x56,
                        0x78, 0xAB, 0xCD, 0xEF};
      int state = rf.transmit(byteArr, 8);
    */

    if (state != ERR_NONE) {
      Serial.print(F("failed, code "));
      Serial.println(state);
    }
  }

}
