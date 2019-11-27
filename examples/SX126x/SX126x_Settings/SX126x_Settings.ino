/*
   RadioLib SX126x Settings Example

   This example shows how to change all the properties of LoRa transmission.
   RadioLib currently supports the following settings:
    - pins (SPI slave select, DIO1, DIO2, BUSY pin)
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word
    - output power during transmission
    - CRC
    - preamble length
    - TCXO voltage
    - DIO2 RF switch control

   Other modules from SX126x family can also be used.

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1262 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// BUSY pin:  9
SX1262 loraSX1262 = new Module(10, 2, 9);

// SX12628 has different connections:
// NSS pin:   8
// DIO1 pin:  4
// BUSY pin:  6
SX1268 loraSX1268 = new Module(8, 4, 6);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1261 loraSX1261 = RadioShield.ModuleB;

void setup() {
  Serial.begin(9600);

  // initialize SX1268 with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  // carrier frequency:           434.0 MHz
  // bandwidth:                   125.0 kHz
  // spreading factor:            9
  // coding rate:                 7
  // sync word:                   0x1424 (private network)
  // output power:                14 dBm
  // current limit:               60 mA
  // preamble length:             8 symbols
  // TCXO voltage:                1.6 V (set to 0 to not use TCXO)
  // CRC:                         enabled
  int state = loraSX1262.begin();
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
  Serial.print(F("[SX1268] Initializing ... "));
  // carrier frequency:           915.0 MHz
  // bandwidth:                   500.0 kHz
  // spreading factor:            6
  // coding rate:                 5
  // sync word:                   0x3444 (public network)
  // output power:                2 dBm
  // current limit:               50 mA
  // preamble length:             20 symbols
  // CRC:                         enabled
  state = loraSX1268.begin(915.0, 500.0, 6, 5, 0x3444, 50, 20);
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
  if (loraSX1262.setFrequency(433.5) == ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected frequency is invalid for this module!"));
    while (true);
  }

  // set bandwidth to 250 kHz
  if (loraSX1262.setBandwidth(250.0) == ERR_INVALID_BANDWIDTH) {
    Serial.println(F("Selected bandwidth is invalid for this module!"));
    while (true);
  }

  // set spreading factor to 10
  if (loraSX1262.setSpreadingFactor(10) == ERR_INVALID_SPREADING_FACTOR) {
    Serial.println(F("Selected spreading factor is invalid for this module!"));
    while (true);
  }

  // set coding rate to 6
  if (loraSX1262.setCodingRate(6) == ERR_INVALID_CODING_RATE) {
    Serial.println(F("Selected coding rate is invalid for this module!"));
    while (true);
  }

  // set LoRa sync word to 0x1234
  if (loraSX1262.setSyncWord(0x1234) != ERR_NONE) {
    Serial.println(F("Unable to set sync word!"));
    while (true);
  }

  // set output power to 10 dBm (accepted range is -17 - 22 dBm)
  if (loraSX1262.setOutputPower(10) == ERR_INVALID_OUTPUT_POWER) {
    Serial.println(F("Selected output power is invalid for this module!"));
    while (true);
  }

  // set over current protection limit to 80 mA (accepted range is 45 - 240 mA)
  // NOTE: set value to 0 to disable overcurrent protection
  if (loraSX1262.setCurrentLimit(80) == ERR_INVALID_CURRENT_LIMIT) {
    Serial.println(F("Selected current limit is invalid for this module!"));
    while (true);
  }

  // set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
  if (loraSX1262.setPreambleLength(15) == ERR_INVALID_PREAMBLE_LENGTH) {
    Serial.println(F("Selected preamble length is invalid for this module!"));
    while (true);
  }

  // disable CRC
  if (loraSX1262.setCRC(false) == ERR_INVALID_CRC_CONFIGURATION) {
    Serial.println(F("Selected CRC is invalid for this module!"));
    while (true);
  }

  // Some SX126x modules have TCXO (temperature compensated crystal
  // oscillator). To configure TCXO reference voltage,
  // the following method can be used.
  if (loraSX1262.setTCXO(2.4) == ERR_INVALID_TCXO_VOLTAGE) {
    Serial.println(F("Selected TCXO voltage is invalid for this module!"));
    while (true);
  }

  // Some SX126x modules use DIO2 as RF switch. To enable
  // this feature, the following method can be used.
  // NOTE: As long as DIO2 is configured to control RF switch,
  //       it can't be used as interrupt pin!
  if (loraSX1262.setDio2AsRfSwitch() != ERR_NONE) {
    Serial.println(F("Failed to set DIO2 as RF switch!"));
    while (true);
  }

  Serial.println(F("All settings succesfully changed!"));
}

void loop() {
  // nothing here
}
