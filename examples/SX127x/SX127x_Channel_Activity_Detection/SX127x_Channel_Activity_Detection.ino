/*
   RadioLib SX127x Channel Activity Detection Example

   This example scans the current LoRa channel and detects
   valid LoRa preambles. Preamble is the first part of
   LoRa transmission, so this can be used to check
   if the LoRa channel is free, or if you should start
   receiving a message.

   Other modules from SX127x/RFM9x family can also be used.

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1278 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// RESET pin: 9
// DIO1 pin:  3
SX1278 lora = new Module(10, 2, 9, 3);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1278 lora = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SX1278 with default settings
  Serial.print(F("[SX1278] Initializing ... "));
  // carrier frequency:           434.0 MHz
  // bandwidth:                   125.0 kHz
  // spreading factor:            9
  // coding rate:                 7
  // sync word:                   0x12
  // output power:                17 dBm
  // current limit:               100 mA
  // preamble length:             8 symbols
  // amplifier gain:              0 (automatic gain control)
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
  Serial.print(F("[SX1278] Scanning channel for LoRa preamble ... "));

  // start scanning current channel
  int state = lora.scanChannel();

  if (state == PREAMBLE_DETECTED) {
    // LoRa preamble was detected
    Serial.println(F(" detected preamble!"));

  } else if (state == CHANNEL_FREE) {
    // no preamble was detected, channel is free
    Serial.println(F(" channel is free!"));

  }

  // wait 100 ms before new scan
  delay(100);
}
