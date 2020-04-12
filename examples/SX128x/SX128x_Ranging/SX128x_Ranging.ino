/*
   RadioLib SX128x Ranging Example

   This example performs ranging exchange between two
   SX1280 LoRa radio modules. Ranging allows to measure
   distance between the modules using time-of-flight
   measurement.

   Only SX1280 and SX1282 support ranging!

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1280 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
SX1280 lora = new Module(10, 2, 3, 9);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1280 lora = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SX1280 with default settings
  Serial.print(F("[SX1280] Initializing ... "));
  // carrier frequency:           2400.0 MHz
  // bandwidth:                   812.5 kHz
  // spreading factor:            9
  // coding rate:                 7
  // output power:                10 dBm
  // preamble length:             12 symbols
  // CRC:                         enabled
  int state = lora.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop() {
  Serial.print(F("[SX1280] Ranging ... "));

  // start ranging exchange
  // range as master:             true
  // slave address:               0x12345678
  int state = lora.range(true, 0x12345678);

  // the other module must be configured as slave with the same address
  /*
    int state = lora.range(false, 0x12345678);
  */

  if (state == ERR_NONE) {
    // ranging finished successfully
    Serial.println(F("success!"));
    Serial.print(F("[SX1280] Distance:\t\t\t"));
    Serial.print(lora.getRangingResult());
    Serial.println(F(" meters"));

  } else if (state == ERR_RANGING_TIMEOUT) {
    // timed out waiting for ranging packet
    Serial.println(F("timed out!"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);

  }

  // wait for a second before ranging again
  delay(1000);
}
