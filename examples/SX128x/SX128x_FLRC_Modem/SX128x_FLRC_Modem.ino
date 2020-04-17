/*
   RadioLib SX128x FLRC Modem Example

   This example shows how to use FLRC modem in SX128x chips.

   NOTE: The sketch below is just a guide on how to use
         FLRC modem, so this code should not be run directly!
         Instead, modify the other examples to use FLRC
         modem and use the appropriate configuration
         methods.

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
SX1280 flrc = new Module(10, 2, 3, 9);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1280 flrc = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SX1280 with default settings
  Serial.print(F("[SX1280] Initializing ... "));
  // carrier frequency:           2400.0 MHz
  // bit rate:                    650 kbps
  // coding rate:                 3
  // output power:                10 dBm
  // preamble length:             16 bits
  // data shaping:                Gaussian, BT = 0.5
  // sync word:                   0x2D  0x01  0x4B  0x1D
  // CRC:                         enabled
  int state = flrc.beginFLRC();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // if needed, you can switch between LoRa and FLRC modes
  //
  // flrc.begin()       start LoRa mode (and disable FLRC)
  // lora.beginFLRC()   start FLRC mode (and disable LoRa)

  // the following settings can also
  // be modified at run-time
  state = flrc.setFrequency(2410.5);
  state = flrc.setBitRate(520);
  state = flrc.setCodingRate(2);
  state = flrc.setOutputPower(5);
  state = flrc.setDataShaping(1.0);
  uint8_t syncWord[] = {0x01, 0x23, 0x45, 0x67};
  state = flrc.setSyncWord(syncWord, 4);
  if (state != ERR_NONE) {
    Serial.print(F("Unable to set configuration, code "));
    Serial.println(state);
    while (true);
  }

  #warning "This sketch is just an API guide! Read the note at line 6."
}

void loop() {
  // FLRC modem can use the same transmit/receive methods
  // as the LoRa modem, even their interrupt-driven versions

  // transmit FLRC packet
  int state = flrc.transmit("Hello World!");
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    int state = flrc.transmit(byteArr, 8);
  */
  if (state == ERR_NONE) {
    Serial.println(F("[SX1280] Packet transmitted successfully!"));
  } else if (state == ERR_PACKET_TOO_LONG) {
    Serial.println(F("[SX1280] Packet too long!"));
  } else if (state == ERR_TX_TIMEOUT) {
    Serial.println(F("[SX1280] Timed out while transmitting!"));
  } else {
    Serial.println(F("[SX1280] Failed to transmit packet, code "));
    Serial.println(state);
  }

  // receive GFSK packet
  String str;
  state = flrc.receive(str);
  /*
    byte byteArr[8];
    int state = flrc.receive(byteArr, 8);
  */
  if (state == ERR_NONE) {
    Serial.println(F("[SX1280] Received packet!"));
    Serial.print(F("[SX1280] Data:\t"));
    Serial.println(str);
  } else if (state == ERR_RX_TIMEOUT) {
    Serial.println(F("[SX1280] Timed out while waiting for packet!"));
  } else {
    Serial.print(F("[SX1280] Failed to receive packet, code "));
    Serial.println(state);
  }
}
