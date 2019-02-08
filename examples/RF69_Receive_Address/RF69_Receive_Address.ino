/*
   RadioLib RF69 Receive with Address Example

   This example receives packets using RF69 FSK radio module.
   Packets can have 1-byte address of the destination node.
   After setting node (or broadcast) address, this node will
   automatically filter out any packets that do not contain
   either node address or broadcast address.
*/

// include the library
#include <RadioLib.h>

// RF69 module is in slot A on the shield
RF69 rf = RadioShield.ModuleA;

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
  // NOTE: calling this method will autmatically enable
  // address filtering (node address only)
  Serial.print(F("[RF69] Setting node address ... "));
  state = rf.setNodeAddress(0x02);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set broadcast address
  // NOTE: calling this method will autmatically enable
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
    state == rf.disableAddressFiltering();
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
  Serial.print(F("[RF69] Waiting for incoming transmission ... "));

  // you can receive data as an Arduino String
  String str;
  int state = rf.receive(str);

  // you can also receive data as byte array
  /*
    byte byteArr[8];
    int state = rf.receive(byteArr, 8);
  */

  if (state == ERR_NONE) {
    // packet was successfully received
    Serial.println(F("success!"));

    // print the data of the packet
    Serial.print(F("[RF69] Data:\t\t"));
    Serial.println(str);
    /*
      for(int i = 0; i < 8; i++) {
      Serial.print("0x");
      Serial.print(byteArr[i], HEX);
      Serial.print(' ');
      }
      Serial.println();
    */

  } else if (state == ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    Serial.println(F("timeout!"));

  } else if (state == ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("CRC error!"));

  }
}
