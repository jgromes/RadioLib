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
  // bandwidth:                   125.0 kHz
  // spreading factor:            9
  // coding rate:                 7
  // sync word:                   0x12
  // output power:                17 dBm
  // current limit:               100 mA
  // preamble length:             8 symbols
  // amplifier gain:              0 (automatic gain control)
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
  // send 500 ms high frequency beep
  rtty.leadIn(500);

  // RTTYClient supports all methods of the Serial class
  // send the string "Hello World!", followed by
  // carriage return and line feed chracters
  rtty.println("Hello World!");

  // turn transmitter off
  fsk.standby();

  // wait for a second before transmitting again
  delay(1000);
}
