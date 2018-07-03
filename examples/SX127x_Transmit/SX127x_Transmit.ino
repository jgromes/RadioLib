/*
 * KiteLib SX127x Transmit Example
 * 
 * This example transmits packets using SX1278 LoRa radio module.
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

// create instance of Packet class with destination address
// "01:23:45:67:89:AB:CD:EF" and data "Hello World !"
Packet pack("01:23:45:67:89:AB:CD:EF", "Hello World!");

void setup() {
  Serial.begin(9600);

  // initialize SX1278 with default settings
  Serial.print(F("[SX1278] Initializing ... "));
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

  // start transmitting the packet
  byte state = lora.transmit(pack);
  
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
