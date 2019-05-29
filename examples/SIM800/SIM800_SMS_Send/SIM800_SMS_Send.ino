/*
   RadioLib SIM800 Send SMS Example
*/

// include the library
#include <RadioLib.h>

// SIM800 module is in slot A on the shield
SIM800 gsm = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SIM800 with default settings
  Serial.print(F("[SIM800] Initializing ... "));
  // baudrate:  9600 baud
  // PIN:       "1234"
  int state = gsm.begin(9600);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop() {
  // send SMS to number 0123456789
  Serial.print(F("[SIM800] Sending SMS ... "));
  int state = gsm.sendSMS("0123456789", "Hello World!");
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // wait 10 seconds before sending again
  delay(10000);
}
