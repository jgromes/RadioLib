/*
   RadioLib SX126x FSK Modem Example

   This example shows how to use FSK modem in SX126x chips.

   NOTE: The sketch below is just a guide on how to use
         FSK modem, so this code should not be run directly!
         Instead, modify the other examples to use FSK
         modem and use the appropriate configuration
         methods.

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1262 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// BUSY pin:  9
SX1262 fsk = new Module(10, 2, 9);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1262 fsk = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SX1262 FSK modem with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  // carrier frequency:           434.0 MHz
  // bit rate:                    48.0 kbps
  // frequency deviation:         50.0 kHz
  // Rx bandwidth:                156.2 kHz
  // output power:                14 dBm
  // current limit:               60.0 mA
  // preamble length:             16 bits
  // data shaping:                Gaussian, BT = 0.5
  // TCXO voltage:                1.6 V (set to 0 to not use TCXO)
  // sync word:                   0x2D  0x01
  // CRC:                         enabled, CRC16 (CCIT)
  int state = fsk.beginFSK();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // if needed, you can switch between LoRa and FSK modes
  //
  // lora.begin()       start LoRa mode (and disable FSK)
  // lora.beginFSK()    start FSK mode (and disable LoRa)

  // the following settings can also
  // be modified at run-time
  state = fsk.setFrequency(433.5);
  state = fsk.setBitRate(100.0);
  state = fsk.setFrequencyDeviation(10.0);
  state = fsk.setRxBandwidth(250.0);
  state = fsk.setOutputPower(10.0);
  state = fsk.setCurrentLimit(100.0);
  state = fsk.setDataShaping(1.0);
  uint8_t syncWord[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
  state = fsk.setSyncWord(syncWord, 8);
  if (state != ERR_NONE) {
    Serial.print(F("Unable to set configuration, code "));
    Serial.println(state);
    while (true);
  }

  // FSK modem on SX126x can handle the sync word setting in bits, not just
  // whole bytes. The value used is left-justified.
  // This makes same result as fsk.setSyncWord(syncWord, 8):
  state = fsk.setSyncBits(syncWord, 64);
  // This will use 0x012 as sync word (12 bits only):
  state = fsk.setSyncBits(syncWord, 12);

  // FSK modem allows advanced CRC configuration
  // Default is CCIT CRC16 (2 bytes, initial 0x1D0F, polynomial 0x1021, inverted)
  // Set CRC to IBM CRC (2 bytes, initial 0xFFFF, polynomial 0x8005, non-inverted)
  state = fsk.setCRC(2, 0xFFFF, 0x8005, false);
  // set CRC length to 0 to disable CRC

  #warning "This sketch is just an API guide! Read the note at line 6."
}

void loop() {
  // FSK modem can use the same transmit/receive methods
  // as the LoRa modem, even their interrupt-driven versions

  // transmit FSK packet
  int state = fsk.transmit("Hello World!");
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56,
                      0x78, 0xAB, 0xCD, 0xEF};
    int state = lora.transmit(byteArr, 8);
  */
  if (state == ERR_NONE) {
    Serial.println(F("[SX1262] Packet transmitted successfully!"));
  } else if (state == ERR_PACKET_TOO_LONG) {
    Serial.println(F("[SX1262] Packet too long!"));
  } else if (state == ERR_TX_TIMEOUT) {
    Serial.println(F("[SX1262] Timed out while transmitting!"));
  } else {
    Serial.println(F("[SX1262] Failed to transmit packet, code "));
    Serial.println(state);
  }

  // receive FSK packet
  String str;
  state = fsk.receive(str);
  /*
    byte byteArr[8];
    int state = lora.receive(byteArr, 8);
  */
  if (state == ERR_NONE) {
    Serial.println(F("[SX1262] Received packet!"));
    Serial.print(F("[SX1262] Data:\t"));
    Serial.println(str);
  } else if (state == ERR_RX_TIMEOUT) {
    Serial.println(F("[SX1262] Timed out while waiting for packet!"));
  } else {
    Serial.println(F("[SX1262] Failed to receive packet, code "));
    Serial.println(state);
  }

  // FSK modem has built-in address filtering system
  // it can be enabled by setting node address, broadcast
  // address, or both
  //
  // to transmit packet to a particular address,
  // use the following methods:
  //
  // fsk.transmit("Hello World!", address);
  // fsk.startTransmit("Hello World!", address);

  // set node address to 0x02
  state = fsk.setNodeAddress(0x02);
  // set broadcast address to 0xFF
  state = fsk.setBroadcastAddress(0xFF);
  if (state != ERR_NONE) {
    Serial.println(F("[SX1262] Unable to set address filter, code "));
    Serial.println(state);
  }

  // address filtering can also be disabled
  // NOTE: calling this method will also erase previously set
  //       node and broadcast address
  /*
    state = fsk.disableAddressFiltering();
    if (state != ERR_NONE) {
      Serial.println(F("Unable to remove address filter, code "));
    }
  */
}
