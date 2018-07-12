/*
 * KiteLib SX127x Transmit Example
 * 
 * This example transmits packets using SX1278 LoRa radio module.
 * Each packet contains up to 256 bytes of data, in the form of:
 *  - Arduino String
 *  - null-terminated char array (C-string)
 *  - arbitrary binary data (byte array)
 * 
 * Other modules from SX127x family can also be used.
 * SX1272 lora = Kite.ModuleA;
 * SX1273 lora = Kite.ModuleA;
 * SX1276 lora = Kite.ModuleA;
 * SX1277 lora = Kite.ModuleA;
 * SX1279 lora = Kite.ModuleA;
 */

// include the library
#include <KiteLib.h>

// SX1278 module is in slot A on the shield
SX1278 lora = Kite.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SX1278 with default settings
  Serial.print(F("[SX1278] Initializing ... "));
  // carrier frequency:                   434.0 MHz
  // bandwidth:                           125.0 kHz
  // spreading factor:                    9
  // coding rate:                         7
  // sync word:                           0x12
  // output power:                        17 dBm
  byte state = lora.begin();
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }
}

void loop() {
  Serial.print(F("[SX1278] Transmitting packet ... "));

  // you can transmit C-string or Arduino string up to 256 characters long
  byte state = lora.transmit("Hello World!");

  // you can also transmit byte array up to 256 bytes long
  /*
  byte byteArr[] = {0x01, 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD, 0xEF};
  byte state = lora.transmit(byteArr, 8);
  */
  
  if(state == ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(" success!");
    
  } else if(state == ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(" too long!");
    
  } else if(state == ERR_TX_TIMEOUT) {
    // timeout occured while transmitting packet
    Serial.println(" timeout!");
    
  }

  // wait for a second before transmitting again
  delay(1000);
}
