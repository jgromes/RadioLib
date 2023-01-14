/*
   RadioLib STM32WLx Channel Activity Detection Example

   This example uses STM32WLx to scan the current LoRa
   channel and detect ongoing LoRa transmissions.
   Unlike SX127x CAD, SX126x/STM32WLx can detect any part
   of LoRa transmission, not just the preamble.

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// no need to configure pins, signals are routed to the radio internally
STM32WLx radio = new STM32WLx_Module();


void setup() {
  Serial.begin(9600);

  // initialize STM32WLx with default settings
  Serial.print(F("[STM32WLx] Initializing ... "));
  int state = radio.begin(868.0);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop() {
  Serial.print(F("[STM32WLx] Scanning channel for LoRa transmission ... "));

  // start scanning current channel
  int state = radio.scanChannel();

  if (state == RADIOLIB_LORA_DETECTED) {
    // LoRa preamble was detected
    Serial.println(F("detected!"));

  } else if (state == RADIOLIB_CHANNEL_FREE) {
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
