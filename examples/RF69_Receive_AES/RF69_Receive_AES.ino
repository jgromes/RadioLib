/*
   KiteLib RF69 Receive with AES Example

   This example receives packets using RF69 FSK radio module.
   Packets are decrypted using hardware AES.
   NOTE: When using address filtering, the address byte is NOT encrypted!
*/

// include the library
#include <KiteLib.h>

// RF69 module is in slot A on the shield
RF69 rf = Kite.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize RF69 with default settings
  Serial.print(F("[RF69] Initializing ... "));
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

  // set AES key that will be used to decrypt the packet
  // NOTE: the key must be exactly 16 bytes long!
  uint8_t key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                   0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
                  };
  rf.setAESKey(key);

  // enable AES encryption
  rf.enableAES();

  // AES encryption can also be disabled
  /*
    rf.disableAES();
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

  } else if (state == ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    Serial.println(F("timeout!"));

  } else if (state == ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("CRC error!"));

  }
}
