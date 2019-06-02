/*
   RadioLib HC05 Example

   This example sends data using HC05 Bluetooth module.
   HC05 works exactly like a Serial line, data are sent to the paired device.
   The default pairing code for HC05 is 1234 or 1111.

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// HC05 has the following connections:
// TX pin: 9
// RX pin: 8
HC05 bluetooth = new Module(9, 8);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//HC05 bluetooth = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize HC05
  // baudrate:  9600 baud
  bluetooth.begin(9600);
}

void loop() {
  // HC05 supports all methods of the Serial class
  // read data incoming from Serial port and write them to Bluetooth
  while (Serial.available() > 0) {
    bluetooth.write(Serial.read());
  }

  // read data incoming from Bluetooth and write them to Serial port
  while (bluetooth.available() > 0) {
    Serial.write(bluetooth.read());
  }
}
