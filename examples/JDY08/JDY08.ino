/*
 * KiteLib JDY08 Example
 * 
 * This example sends data using JDY08 Bluetooth module.
 */

// include the library
#include <KiteLib.h>

// JDY08 module is in slot A on the shield
JDY08 ble = Kite.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize JDY08
  // baudrate:  9600 baud
  ble.begin(9600);
}

void loop() {
  // JDY08 supports all methods of the Serial class
  // read data incoming from Serial port and write them to Bluetooth
  while(Serial.available() > 0) {
    bluetooth.write(Serial.read());
  }
  
  // read data incoming from Bluetooth and write them to Serial port
  while(bluetooth.available() > 0) {
    Serial.write(bluetooth.read());
  }
}
