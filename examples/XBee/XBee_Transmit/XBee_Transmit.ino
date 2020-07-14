/*
   RadioLib XBee API Transmit Example

   This example transmits packets using XBee API mode.
   In API mode, many XBee modules can form a mesh network.

   IMPORTANT: Before uploading this example, make sure that the XBee module
   is running API COORDINATOR firmware!

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// XBee has the following connections:
// TX pin:    9
// RX pin:    8
// RESET pin: 3
XBee bee = new Module(9, 8, 3);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//XBee bee = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize XBee module with baudrate 9600
  Serial.print(F("[XBee] Initializing ... "));
  int state = bee.begin(9600);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set PAN ID to 0x0123456789ABCDEF
  Serial.print(F("[XBee] Setting PAN ID ... "));
  uint8_t panId[] = {0x01, 0x23, 0x45, 0x67,
                     0x89, 0xAB, 0xCD, 0xEF};
  state = bee.setPanId(panId);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop() {
  // transmit data to the destination module
  uint8_t dest[] = {0x00, 0x13, 0xA2, 0x00,
                    0x40, 0xA5, 0x8A, 0x6B};
  Serial.print(F("[XBee] Transmitting message ... "));
  int state = bee.transmit(dest, "Hello World!");
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  delay(1000);
}
