/*
  RadioLib SX126x Blocking Transmit Example

  This example transmits packets using LLCC68 LoRa radio module.

  NOTE: LLCC68 modules offer the same features as SX1261 and have the same
        interface. The difference is in the available LoRa settings (not all
        spreading factors are available on LLCC68).
        Please see SX126x examples for full reference.

  WARNING: Often, LLCC68 modules are mislabeled. If you are seeing error -2
           (RADIOLIB_ERR_CHIP_NOT_FOUND) and debug mode shows "SX1261" as
           the version string, use the SX1261 class!

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// LLCC68 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
LLCC68 radio = new Module(10, 2, 3, 9);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

void setup() {
  Serial.begin(9600);

  // initialize LLCC68 with default settings
  Serial.print(F("[LLCC68] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // some modules have an external RF switch
  // controlled via two pins (RX enable, TX enable)
  // to enable automatic control of the switch,
  // call the following method
  // RX enable:   4
  // TX enable:   5
  /*
    radio.setRfSwitchPins(4, 5);
  */
}

// counter to keep track of transmitted packets
int count = 0;

void loop() {
  Serial.print(F("[LLCC68] Transmitting packet ... "));

  // you can transmit C-string or Arduino string up to
  // 256 characters long
  String str = "Hello World! #" + String(count++);
  int state = radio.transmit(str);

  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD, 0xEF};
    int state = radio.transmit(byteArr, 8);
  */

  if (state == RADIOLIB_ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(F("success!"));

    // print measured data rate
    Serial.print(F("[LLCC68] Datarate:\t"));
    Serial.print(radio.getDataRate());
    Serial.println(F(" bps"));

  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));

  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    // timeout occured while transmitting packet
    Serial.println(F("timeout!"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);

  }

  // wait for a second before transmitting again
  delay(1000);
}
