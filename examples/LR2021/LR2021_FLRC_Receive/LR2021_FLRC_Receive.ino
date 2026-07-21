/*
  RadioLib LR2021 FLRC Receive Example

  This example receives packets using LR2021 FLRC modem.
  FLRC (Fast Long Range Communication) provides high data rates
  up to 2600 kbps, making it suitable for high-throughput applications.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#lr2021---flrc-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// LR2021 has the following connections:
// NSS pin:   10
// IRQ pin:   2
// NRST pin:  3
// BUSY pin:  9
LR2021 radio = new Module(10, 2, 3, 9);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

void setup() {
  Serial.begin(9600);

  // before calling begin(), correct crystal has to be selected
  // some LR2021 have a TCXO which needs 1.6V reference
  // set to 0 if your radio has an XTAL
  radio.tcxoVoltage = 1.6;

  // LR2021 allows to use any DIO pin as the interrupt
  // as an example, we set DIO10 to be the IRQ
  // this has to be done prior to calling begin()!
  radio.irqDioNum = 10;

  // initialize LR2021 with FLRC modem
  // FLRC provides data rates up to 2600 kbps
  // Parameters: frequency (MHz), bit rate (kbps), coding rate,
  //             output power (dBm), preamble length (bits)
  Serial.print(F("[LR2021] Initializing ... "));
  int state = radio.beginFLRC(434.0, 650, RADIOLIB_LR2021_FLRC_CR_2_3, 10, 32);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // start listening for FLRC packets
  Serial.print(F("[LR2021] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

void loop() {
  // wait for packet to arrive
  int state = radio.readData();

  if (state == RADIOLIB_ERR_NONE) {
    // packet was successfully received
    Serial.print(F("[LR2021] Received packet!"));

    // print data
    String str;
    radio.readData(str);
    Serial.print(F("  Data:"));
    Serial.print(str);

    // print RSSI
    Serial.print(F("  RSSI:"));
    Serial.print(radio.getRSSI());
    Serial.print(F(" dBm"));

    Serial.println();

  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    // normal behavior when no transmitter is in range

  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    // packet was received, but CRC check failed
    Serial.println(F("[LR2021] CRC error!"));

  } else {
    // some other error occurred
    Serial.print(F("[LR2021] Failed, code "));
    Serial.println(state);

  }

  // put the module back in listen mode
  radio.startReceive();
}
