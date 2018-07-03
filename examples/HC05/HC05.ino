/*
 * KiteLib HC05 Example
 * 
 * This example sends data using HC05 Bluetooth module.
 * HC05 works exactly like a Serial line, data are sent to the paired device.
 * The default pairing code for HC05 is 1234.
 */

// include the library
#include <KiteLib.h>

// HC05 module is in slot A on the shield
HC05 bluetooth = Kite.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize HC05 with baudrate 9600 baud
  bluetooth.begin(9600);
}

void loop() {
  // read data incoming from Serial port and write them to bluetooth
  // HC05 supports all methods of the Serial class
  while(Serial.available() > 0) {
    bluetooth.write(Serial.read());
  }
}
