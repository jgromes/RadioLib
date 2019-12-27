/*
   RadioLib RF69 Transmit to Address Example

   This example transmits packets using RF69 FSK radio module.
   Packets can have 1-byte address of the destination node.
   After setting node (or broadcast) address, this node will
   automatically filter out any packets that do not contain
   either node address or broadcast address.

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// RF69 has the following connections:
// CS pin:    10
// DIO0 pin:  2
// RESET pin: 3
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

  // set node address
  // NOTE: calling this method will automatically enable
  // address filtering (node address only)
  Serial.print(F("[RF69] Setting node address ... "));
  state = rf.setNodeAddress(0x01);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set broadcast address
  // NOTE: calling this method will automatically enable
  // address filtering (node or broadcast address)
  Serial.print(F("[RF69] Setting broadcast address ... "));
  state = rf.setBroadcastAddress(0xFF);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // address filtering can also be disabled
  // NOTE: calling this method will also erase previously set
  // node and broadcast address
  /*
    Serial.print(F("[RF69] Disabling address filtering ... "));
    state = rf.disableAddressFiltering();
    if(state == ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while(true);
    }
  */
}

void loop() {
  Serial.print(F("[RF69] Transmitting packet ... "));

  // transmit C-string or Arduino string to node with address 0x02
  int state = rf.transmit("Hello World!", 0x02);

  // transmit byte array to node with address 0x02
  /*
  byte byteArr[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
  int state = rf.transmit(byteArr, 8, 0x02);
  */

  // transmit C-string or Arduino string in broadcast mode
  /*
    int state = rf.transmit("Hello World!", 0xFF);
  */

  // transmit byte array in broadcast mode
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD, 0xEF};
    int state = rf.transmit(byteArr, 8, 0xFF);
  */

  if (state == ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(F(" success!"));

  } else if (state == ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 64 bytes
    Serial.println(F(" too long!"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);

  }

  // wait for a second before transmitting again
  delay(1000);
}
