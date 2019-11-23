/*
   RadioLib CC1101 Settings Example

   This example shows how to change all the properties of RF69 radio.
   RadioLib currently supports the following settings:
    - pins (SPI slave select, digital IO 0, digital IO 1)
    - carrier frequency
    - bit rate
    - receiver bandwidth
    - allowed frequency deviation
    - output power during transmission
    - sync word

    For full API reference, see the GitHub Pages
    https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// CC1101 has the following connections:
// NSS pin:   10
// GDO0 pin:  2
// GDO2 pin:  3
CC1101 cc1 = new Module(10, 2, 3);

// second CC1101 has different connections:
// NSS pin:   9
// GDO0 pin:  4
// GDO2 pin:  5
CC1101 cc2 = new Module(9, 4, 5);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//CC1101 cc3 = RadioShield.ModuleB;

void setup() {
  Serial.begin(9600);

  // initialize CC1101 with default settings
  Serial.print(F("[CC1101] Initializing ... "));
  // carrier frequency:                   868.0 MHz
  // bit rate:                            4.8 kbps
  // Rx bandwidth:                        325.0 kHz
  // frequency deviation:                 48.0 kHz
  // sync word:                           0xD391
  int state = cc1.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // initialize CC1101 with non-default settings
  Serial.print(F("[CC1101] Initializing ... "));
  // carrier frequency:                   434.0 MHz
  // bit rate:                            32.0 kbps
  // Rx bandwidth:                        250.0 kHz
  // frequency deviation:                 60.0 kHz
  // sync word:                           0xD391
  state = cc2.begin(434.0, 32.0, 250.0, 60.0);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // you can also change the settings at runtime
  // and check if the configuration was changed successfully

  // set carrier frequency to 433.5 MHz
  if (cc1.setFrequency(433.5) == ERR_INVALID_FREQUENCY) {
    Serial.println(F("[CC1101] Selected frequency is invalid for this module!"));
    while (true);
  }

  // set bit rate to 100.0 kbps
  state = cc1.setBitRate(100.0);
  if (state == ERR_INVALID_BIT_RATE) {
    Serial.println(F("[CC1101] Selected bit rate is invalid for this module!"));
    while (true);
  } else if (state == ERR_INVALID_BIT_RATE_BW_RATIO) {
    Serial.println(F("[CC1101] Selected bit rate to bandwidth ratio is invalid!"));
    Serial.println(F("[CC1101] Increase receiver bandwidth to set this bit rate."));
    while (true);
  }

  // set receiver bandwidth to 250.0 kHz
  if (cc1.setRxBandwidth(250.0) == ERR_INVALID_RX_BANDWIDTH) {
    Serial.println(F("[CC1101] Selected receiver bandwidth is invalid for this module!"));
    while (true);
  }

  // set allowed frequency deviation to 10.0 kHz
  if (cc1.setFrequencyDeviation(10.0) == ERR_INVALID_FREQUENCY_DEVIATION) {
    Serial.println(F("[CC1101] Selected frequency deviation is invalid for this module!"));
    while (true);
  }

  // set output power to 5 dBm
  if (cc1.setOutputPower(5) == ERR_INVALID_OUTPUT_POWER) {
    Serial.println(F("[CC1101] Selected output power is invalid for this module!"));
    while (true);
  }

  // 2 bytes can be set as sync word
  if (cc1.setSyncWord(0x01, 0x23) == ERR_INVALID_SYNC_WORD) {
    Serial.println(F("[CC1101] Selected sync word is invalid for this module!"));
    while (true);
  }

}

void loop() {
  // nothing here
}
