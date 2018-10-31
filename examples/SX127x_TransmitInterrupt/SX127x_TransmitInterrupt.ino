/*
   KiteLib Transmit with Inerrupts Example

   This example transmits LoRa packets with one second delays
   between them. Each packet contains up to 256 bytes
   of data, in the form of:
    - Arduino String
    - null-terminated char array (C-string)
    - arbitrary binary data (byte array)

   Other modules from SX127x/RFM9x family can also be used.
*/

// include the library
#include <KiteLib.h>

// SX1278 module is in slot A on the shield
SX1278 lora = Kite.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SX1278 with default settings
  Serial.print(F("Initializing ... "));
  // carrier frequency:                   434.0 MHz
  // bandwidth:                           125.0 kHz
  // spreading factor:                    9
  // coding rate:                         7
  // sync word:                           0x12
  // output power:                        17 dBm
  // current limit:                       100 mA
  // preamble length:                     8 symbols
  // amplifier gain:                      0 (automatic gain control)
  int state = lora.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set the function that will be called 
  // when packet transmission is finished
  lora.setDio0Action(setFlag);

  // start transmitting the first packet
  Serial.print(F("Sending first packet ... "));

  // you can transmit C-string or Arduino string up to
  // 256 characters long
  state = lora.startTransmit("Hello World!");

  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56,
                      0x78, 0xAB, 0xCD, 0xEF};
    state = lora.transmit(byteArr, 8);
  */
  
  if (state != ERR_NONE) {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

// flag to indicate that a packet was received
volatile bool transmittedFlag = false;

void setFlag(void) {
  // packet transmission is finished, set the flag
  transmittedFlag = true;
}

void loop() {
  // check if the previous transmission finished
  if(transmittedFlag) {
    Serial.println(F("Packet transmission finished!"));

    // wait one second before next transmission
    delay(1000);

    // send another packet
    Serial.print(F("Sending another packet ... "));

    // you can transmit C-string or Arduino string up to
    // 256 characters long
    int state = lora.startTransmit("Hello World!");
  
    // you can also transmit byte array up to 256 bytes long
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x56,
                        0x78, 0xAB, 0xCD, 0xEF};
      int state = lora.transmit(byteArr, 8);
    */

    // NOTE: when using interrupt-driven transmit method,
    //       it is not possible to automatically measure
    //       transmission data rate using getDataRate()
    
    if (state != ERR_NONE) {
      Serial.print(F("failed, code "));
      Serial.println(state);
    }
  }

}
