/*
   RadioLib XBee Transparent Operation Example

   This example transmits packets using XBee Transparent mode.
   In Transparent mode, two XBee modules act like a Serial line.
   Both modules must have the same PAN ID, and the destination
   addresses have to be set properly.

   IMPORTANT: Before uploading this example, make sure that the XBee modules
   are running AT COORDINATOR and AT ROUTER firmware!

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// XBee has the following connections:
// TX pin:    9
// RX pin:    8
// RESET pin: 3
XBeeSerial bee = new Module(9, 8, 3);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//XBeeSerial bee = RadioShield.ModuleA;

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

  // set PAN ID to 0123456789ABCDEF
  Serial.print(F("[XBee] Setting PAN ID ... "));
  state = bee.setPanId("0123456789ABCDEF");
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set destination address to the address of the second module
  Serial.print(F("[XBee] Setting destination address ... "));
  state = bee.setDestinationAddress("0013A200", "40A58A5D");
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop() {
  // XBeeSerial supports all methods of the Serial class
  // read data incoming from Serial port and write them to XBee
  while (Serial.available() > 0) {
    bee.write(Serial.read());
  }

  // read data incoming from XBee and write them to Serial port
  while (bee.available() > 0) {
    Serial.write(bee.read());
  }
}
