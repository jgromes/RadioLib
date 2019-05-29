/*
   RadioLib nRF24 Transmit Example
*/

// include the library
#include <RadioLib.h>

// nRF24 is in slot A on the shield
nRF24 nrf = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize nRF24
  Serial.print(F("[nRF24] Initializing ... "));
  // carrier frequency:           2400 MHz
  // data rate:                   1000 kbps
  // address width:               5 bytes
  int state = nrf.begin();
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  // set receive pipe 0 address
  // NOTE: address width in bytes MUST be equal to the 
  //       width set in begin() or setAddressWidth()
  //       methods (5 by default)
  Serial.print(F("[nRF24] Setting address for receive pipe 0 ... "));
  byte receiveAddr0[] = {0x05, 0x06, 0x07, 0x08, 0x09};
  state = nrf.setReceivePipe(0, receiveAddr0);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }
  
  // set receive pipe 1 - 5 address
  // NOTE: unlike receive pipe 0, pipes 1 - 5 are only
  //       distinguished by their least significant byte,
  //       the upper bytes will be the same!
  Serial.print(F("[nRF24] Setting addresses for receive pipes 1 - 5 ... "));
  byte receiveAddr1[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xE1};
  // set pipe 1 address
  state = nrf.setReceivePipe(1, receiveAddr1);
  // set the addresses for rest of pipes
  state |= nrf.setReceivePipe(2, 0xE2);
  state |= nrf.setReceivePipe(3, 0xE3);
  state |= nrf.setReceivePipe(4, 0xE4);
  state |= nrf.setReceivePipe(5, 0xE5);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  // pipes 1 - 5 are automatically enabled upon address
  // change, but can be disabled manually
  Serial.print(F("[nRF24] Disabling pipes 2 - 5 ... "));
  state = nrf.disablePipe(2);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }
}

void loop() {
  Serial.print(F("[nRF24] Transmitting packet ... "));

  // set transmit address
  // NOTE: address width in bytes MUST be equal to the 
  //       width set in begin() or setAddressWidth()
  //       methods (5 by default)
  byte addr[] = {0x00, 0x01, 0x02, 0x03, 0x04};

  // you can transmit C-string or Arduino string up to
  // 32 characters long
  int state = nrf.transmit("Hello World!", addr);

  if (state == ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(" success!");

  } else if (state == ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(" too long!");

  }

  // wait for a second before transmitting again
  delay(1000);
}
