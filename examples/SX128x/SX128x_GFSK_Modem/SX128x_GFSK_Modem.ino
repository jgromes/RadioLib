/*
   RadioLib SX128x GFSK Modem Example

   This example shows how to use GFSK modem in SX128x chips.

   NOTE: The sketch below is just a guide on how to use
         GFSK modem, so this code should not be run directly!
         Instead, modify the other examples to use GFSK
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
SX1280 gfsk = new Module(10, 2, 3, 9);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1280 lora = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SX1280 with default settings
  Serial.print(F("[SX1280] Initializing ... "));
  // carrier frequency:           2400.0 MHz
  // bit rate:                    800 kbps
  // frequency deviation:         400.0 kHz
  // output power:                10 dBm
  // preamble length:             16 bits
  // coding rate:                 7
  // data shaping:                Gaussian, BT = 0.5
  // sync word:                   0x2D  0x01
  // CRC:                         enabled, CRC16 (CCIT)
  int state = gfsk.beginGFSK();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // if needed, you can switch between LoRa and FSK modes
  //
  // gfsk.begin()       start LoRa mode (and disable GFSK)
  // lora.beginGFSK()   start GFSK mode (and disable LoRa)

  // the following settings can also
  // be modified at run-time
  state = gfsk.setFrequency(2410.5);
  state = gfsk.setBitRate(200);
  state = gfsk.setFrequencyDeviation(100.0);
  state = gfsk.setOutputPower(5);
  state = gfsk.setDataShaping(1.0);
  uint8_t syncWord[] = {0x01, 0x23, 0x45, 0x67, 0x89};
  state = gfsk.setSyncWord(syncWord, 5);
  if (state != ERR_NONE) {
    Serial.print(F("Unable to set configuration, code "));
    Serial.println(state);
    while (true);
  }

  #warning "This sketch is just an API guide! Read the note at line 6."
}

void loop() {
  // GFSK modem can use the same transmit/receive methods
  // as the LoRa modem, even their interrupt-driven versions

  // transmit GFSK packet
  int state = gfsk.transmit("Hello World!");
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    int state = gfsk.transmit(byteArr, 8);
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
  state = gfsk.receive(str);
  /*
    byte byteArr[8];
    int state = gfsk.receive(byteArr, 8);
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
