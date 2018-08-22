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
  Serial.print("[SX1278] Initializing ... ");
  // carrier frequency:           434.4 MHz
  // bit rate:                    48.0 kbps
  // frequency deviation:         50.0 kHz
  // Rx bandwidth:                125.0 kHz
  // output power:                13 dBm
  // current limit:               100 mA
  // sync word:                   0x2D  0x01
  int state = fsk.beginFSK(434.4);
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code ");
    Serial.println(state);
    while(true);
  }

  // initialize RTTY client
  Serial.print("[RTTY] Initializing ... ");
  state = rtty.begin(434.4, 183, 45);
  // low frequency:               434.4 MHz
  // frequency shift:             183 Hz (must be divisible by 61!)
  // baud rate:                   45 baud
  // data bits:                   8
  // stop bits:                   1
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code ");
    Serial.println(state);
    while(true);
  }
}

void loop() {
  Serial.println("Sending RTTY data ... ");
  
  // send 500 ms high frequency beep
  rtty.leadIn(500);

  // RTTYClient supports all methods of the Serial class
  String aStr = "Arduino String";
  rtty.print(aStr);
  rtty.println(aStr);
  
  const char cStr[] = "C-string";
  rtty.print(cStr);
  rtty.println(cStr);

  char c = 'c';
  rtty.print(c);
  rtty.println(c);
  
  byte b = 0xAA;
  rtty.print(b, HEX);
  rtty.println(b, HEX);

  int i = 1000;
  rtty.print(i);
  rtty.println(i);

  float f = 3.1415;
  rtty.print(f, 3);
  rtty.println(f, 3);

  // turn transmitter off
  fsk.standby();

  Serial.println("done!");

  // wait for a second before transmitting again
  delay(1000);
}
