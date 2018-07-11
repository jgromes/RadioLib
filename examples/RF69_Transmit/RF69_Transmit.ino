/*
 * KiteLib RF69 Transmit Example
 * 
 * This example transmits packets using RF69 FSK radio module.
 */

// include the library
#include <KiteLib.h>

// RF69 module is in slot A on the shield
RF69 rf = Kite.ModuleA;

// create instance of Packet class with destination address
// "01:23:45:67:89:AB:CD:EF" and data "Hello World !"
Packet pack("01:23:45:67:89:AB:CD:EF", "Hello World!");

void setup() {
  Serial.begin(9600);

  // initialize RF69 with default settings
  Serial.print(F("[RF69] Initializing ... "));
  // carrier frequency:                   434.0 MHz
  // bit rate:                            48.0 kbps
  // Rx bandwidth:                        125.0 kHz
  // frequency deviation:                 50.0 kHz
  // output power:                        13 dBm
  byte state = rf.begin();
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }
}

void loop() {
  Serial.print(F("[RF69] Transmitting packet ... "));

  // start transmitting the packet
  byte state = rf.transmit(pack);
  
  if(state == ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(" success!");
    
  } else if(state == ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(" too long!");
    
  }

  // wait for a second before transmitting again
  delay(1000);
}
