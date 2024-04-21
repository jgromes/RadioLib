/*
  RadioLib LR11x0 Blocking Channel Activity Detection Example

  This example uses LR1110 to scan the current LoRa
  channel and detect ongoing LoRa transmissions.
  Unlike SX127x CAD, LR11x0 can detect any part
  of LoRa transmission, not just the preamble.

  Other modules from LR11x0 family can also be used.

  Using blocking CAD is not recommended, as it will lead
  to significant amount of timeouts, inefficient use of processor
  time and can some miss packets!
  Instead, interrupt CAD is recommended.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#lr11x0---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// LR1110 has the following connections:
// NSS pin:   10
// IRQ pin:   2
// NRST pin:  3
// BUSY pin:  9
LR1110 radio = new Module(10, 2, 3, 9);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//LR1110 radio = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize LR1110 with default settings
  Serial.print(F("[LR1110] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop() {
  Serial.print(F("[LR1110] Scanning channel for LoRa transmission ... "));

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
