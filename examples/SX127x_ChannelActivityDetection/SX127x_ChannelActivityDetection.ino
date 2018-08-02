/*
   KiteLib SX127x Channel Activity Detection Example

   This example scans the current LoRa channel and detects 
   valid LoRa preambles. Preamble is the first part of 
   LoRa transmission, so this can be used to check 
   if the LoRa channel is free, or if you should start 
   receiving a message.

   Other modules from SX127x family can also be used.
   SX1272 lora = Kite.ModuleA;
   SX1273 lora = Kite.ModuleA;
   SX1276 lora = Kite.ModuleA;
   SX1277 lora = Kite.ModuleA;
   SX1279 lora = Kite.ModuleA;
*/

// include the library
#include <KiteLib.h>

// SX1278 module is in slot A on the shield
SX1278 lora = Kite.ModuleA;

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
  Serial.print(F("Scanning channel for LoRa preamble ... "));

  // start scanning current channel
  int state = lora.scanChannel();

  if (state == PREAMBLE_DETECTED) {
    // LoRa preamble was detected
    Serial.println(" detected preamble!");

  } else if (state == CHANNEL_FREE) {
    // no preamble was detected, channel is free
    Serial.println(" channel is free!");

  }

  // wait 100 ms before new scan
  delay(100);
}
