/*
   RadioLib SX128x Settings Example

   This example shows how to change all the properties of LoRa transmission.
   RadioLib currently supports the following settings:
    - pins (SPI slave select, DIO1, DIO2, BUSY pin)
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - output power during transmission
    - CRC
    - preamble length

   Other modules from SX128x family can also be used.

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
SX1280 loraSX1280 = new Module(10, 2, 3, 9);

// SX1280 has the following connections:
// NSS pin:   8
// DIO1 pin:  4
// NRST pin:  5
// BUSY pin:  6
SX1281 loraSX1281 = new Module(8, 4, 5, 6);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1282 loraSX1282 = RadioShield.ModuleB;

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
  int state = loraSX1280.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // initialize the second LoRa instance with
  // non-default settings
  // this LoRa link will have high data rate,
  // but lower range
  Serial.print(F("[SX1281] Initializing ... "));
  // carrier frequency:           2450.0 MHz
  // bandwidth:                   1625.0 kHz
  // spreading factor:            7
  // coding rate:                 5
  // output power:                2 dBm
  // preamble length:             20 symbols
  // CRC:                         enabled
  state = loraSX1281.begin(2450.0, 1625.0, 7, 5, 2, 20);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // you can also change the settings at runtime
  // and check if the configuration was changed successfully

  // set carrier frequency to 2410.5 MHz
  if (loraSX1280.setFrequency(2410.5) == ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected frequency is invalid for this module!"));
    while (true);
  }

  // set bandwidth to 203.125 kHz
  if (loraSX1280.setBandwidth(203.125) == ERR_INVALID_BANDWIDTH) {
    Serial.println(F("Selected bandwidth is invalid for this module!"));
    while (true);
  }

  // set spreading factor to 10
  if (loraSX1280.setSpreadingFactor(10) == ERR_INVALID_SPREADING_FACTOR) {
    Serial.println(F("Selected spreading factor is invalid for this module!"));
    while (true);
  }

  // set coding rate to 6
  if (loraSX1280.setCodingRate(6) == ERR_INVALID_CODING_RATE) {
    Serial.println(F("Selected coding rate is invalid for this module!"));
    while (true);
  }

  // set output power to -2 dBm
  if (loraSX1280.setOutputPower(-2) == ERR_INVALID_OUTPUT_POWER) {
    Serial.println(F("Selected output power is invalid for this module!"));
    while (true);
  }

  // set LoRa preamble length to 16 symbols (accepted range is 2 - 65535)
  if (loraSX1280.setPreambleLength(16) == ERR_INVALID_PREAMBLE_LENGTH) {
    Serial.println(F("Selected preamble length is invalid for this module!"));
    while (true);
  }

  // disable CRC
  if (loraSX1280.setCRC(false) == ERR_INVALID_CRC_CONFIGURATION) {
    Serial.println(F("Selected CRC is invalid for this module!"));
    while (true);
  }

  Serial.println(F("All settings succesfully changed!"));
}

void loop() {
  // nothing here
}
