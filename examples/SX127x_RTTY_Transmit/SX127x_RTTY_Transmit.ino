/*
   KiteLib SX127x RTTY Transmit Example

   This example sends RTTY message using SX1278's
   FSK modem.

   Other modules from SX127x family can also be used.
   SX1272 lora = Kite.ModuleA;
   SX1273 lora = Kite.ModuleA;
   SX1276 lora = Kite.ModuleA;
   SX1277 lora = Kite.ModuleA;
   SX1279 lora = Kite.ModuleA;
*/

// include the library
#include <KiteLib.h>

// SX1278 module is in slot A on the shield
SX1278 fsk = Kite.ModuleA;

// create RTTY client instance using the FSK module
RTTYClient rtty(&fsk);

void setup() {
  Serial.begin(9600);

  // initialize SX1278
  Serial.print(F("[SX1278] Initializing ... "));
  // carrier frequency:           434.0 MHz
  // bit rate:                    48.0 kbps
  // frequency deviation:         50.0 kHz
  // Rx bandwidth:                125.0 kHz
  // output power:                13 dBm
  // current limit:               100 mA
  // sync word:                   0x2D  0x01
  int state = fsk.beginFSK();
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  // initialize RTTY client
  // NOTE: RTTY frequency shift MUST be divisible by 61 Hz!
  Serial.print(F("[RTTY] Initializing ... "));
  // low frequency:               434.0 MHz
  // frequency shift:             183 Hz
  // baud rate:                   45 baud
  // data bits:                   8
  // stop bits:                   1
  state = rtty.begin(434, 183, 45);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }
}

void loop() {
  Serial.print(F("[RTTY] Sending RTTY data ... "));
  
  // send out idle condition for 500 ms
  rtty.idle();
  delay(500);

  // RTTYClient supports all methods of the Serial class

  // Arduino String class
  String aStr = "Arduino String";
  rtty.println(aStr);

  // character array (C-string)
  rtty.println("C-string");

  // character
  rtty.println('c');
  
  // byte
  // formatting DEC/HEX/OCT/BIN is supported for
  // any integer type (byte/int/long)
  rtty.println(255, HEX);

  // integer number
  int i = 1000;
  rtty.println(i);

  // floating point number
  float f = -3.1415;
  rtty.println(f, 3);

  // turn transmitter off
  fsk.standby();

  Serial.println(F("done!"));

  // wait for a second before transmitting again
  delay(1000);
}
