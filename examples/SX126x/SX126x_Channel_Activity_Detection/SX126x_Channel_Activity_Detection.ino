/*
   RadioLib SX126x Channel Activity Detection Example

   This example scans the current LoRa channel and detects 
   valid LoRa preambles. Preamble is the first part of 
   LoRa transmission, so this can be used to check 
   if the LoRa channel is free, or if you should start 
   receiving a message.

   Other modules from SX126x family can also be used.
*/

// include the library
#include <RadioLib.h>

// SX1262 module is in slot A on the shield
SX1262 lora = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  // carrier frequency:           434.0 MHz
  // bandwidth:                   125.0 kHz
  // spreading factor:            9
  // coding rate:                 7
  // sync word:                   0x1424 (private network)
  // output power:                14 dBm
  // current limit:               60 mA
  // preamble length:             8 symbols
  // CRC:                         enabled
  int state = lora.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
   
  // NOTE: Some SX126x modules use TCXO
  //       (Temprature-Compensated Crystal Oscillator).
  //       To be able to use these modules, TCXO
  //       control must be enabled by calling
  //       setTCXO() and specifying the reference
  //       voltage.
  
  /*
    Serial.print(F("[SX1262] Setting TCXO reference ... "));
    // enable TCXO
    // reference voltage:           1.6 V
    // timeout:                     5000 us
    state = lora.setTCXO(1.6);
    if (state == ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while (true);
    }
  */
}

void loop() {
  Serial.print(F("[SX1262] Scanning channel for LoRa transmission ... "));

  // start scanning current channel
  int state = lora.scanChannel();

  if (state == LORA_DETECTED) {
    // LoRa preamble was detected
    Serial.println(F(" detected!"));

  } else if (state == CHANNEL_FREE) {
    // no preamble was detected, channel is free
    Serial.println(F(" channel is free!"));

  }

  // wait 100 ms before new scan
  delay(100);
}
