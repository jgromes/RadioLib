/*
   RadioLib SX128x Channel Activity Detection Example

   This example uses SX1280 to scan the current LoRa
   channel and detect ongoing LoRa transmissions.

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
  Serial.print(F("[SX1280] Scanning channel for LoRa transmission ... "));

  // start scanning current channel
  int state = lora.scanChannel();

  if (state == LORA_DETECTED) {
    // LoRa preamble was detected
    Serial.println(F("detected!"));

  } else if (state == CHANNEL_FREE) {
    // no preamble was detected, channel is free
    Serial.println(F("channel is free!"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);

  }

  // wait 100 ms before new scan
  delay(100);
}
