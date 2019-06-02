/*
   RadioLib JDY08 Example

   This example sends data using JDY08 Bluetooth module.
   JDY08 works exactly like a Serial line, data are sent to the paired device.

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// JDY08 has the following connections:
// TX pin: 9
// RX pin: 8
JDY08 ble = new Module(9, 8);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//JDY08 ble = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize JDY08
  // baudrate:  9600 baud
  ble.begin(9600);
}

void loop() {
  // JDY08 supports all methods of the Serial class
  // read data incoming from Serial port and write them to Bluetooth
  while (Serial.available() > 0) {
    ble.write(Serial.read());
  }

  // read data incoming from Bluetooth and write them to Serial port
  while (ble.available() > 0) {
    Serial.write(ble.read());
  }
}
