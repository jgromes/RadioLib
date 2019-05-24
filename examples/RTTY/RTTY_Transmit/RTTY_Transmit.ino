/*
   RadioLib RTTY Transmit Example

   This example sends RTTY message using SX1278's
   FSK modem.

   Other modules that can be used for RTTY:
    - SX1272/73/76/77/79
    - RF69
    - SX1231
    - CC1101
    - SX1262/68
*/

// include the library
#include <RadioLib.h>

// SX1278 module is in slot A on the shield
SX1278 fsk = RadioShield.ModuleA;

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
  // NOTE: RTTY frequency shift will be rounded
  //       to the nearest multiple of frequency step size.
  //       The exact value depends on the module:
  //         SX127x - 61 Hz
  //         RF69 - 61 Hz
  //         CC1101 - 397 Hz
  //         SX126x - 1 Hz
  Serial.print(F("[RTTY] Initializing ... "));
  // low ("space") frequency:     434.0 MHz
  // frequency shift:             183 Hz
  // baud rate:                   45 baud
  // encoding:                    ASCII (7-bit)
  // stop bits:                   1
  state = rtty.begin(434, 183, 45);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  /*
    // RadioLib also provides ITA2 ("Baudot") support
    rtty.begin(434, 183, 45, ITA2);

    // All transmissions in loop() (strings and numbers)
    // will now be encoded using ITA2 code

    // ASCII characters that do not have ITA2 equivalent
    // will be sent as NUL (including lower case letters!)
  */
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

  // character array (C-String)
  rtty.println("C-String");

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
