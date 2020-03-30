/*
   RadioLib Si443x Receive with Interrupts Example

   This example listens for FSK transmissions and tries to
   receive them. Once a packet is received, an interrupt is
   triggered.

   Other modules from Si443x/RFM2x family can also be used.

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// Si4432 has the following connections:
// nSEL pin:  10
// nIRQ pin:  2
// SDN pin:   9
Si4432 fsk = new Module(10, 2, 9);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//Si4432 fsk = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize Si4432 with default settings
  Serial.print(F("[Si4432] Initializing ... "));
  // carrier frequency:           434.0 MHz
  // bit rate:                    48.0 kbps
  // frequency deviation:         50.0 kHz
  // Rx bandwidth:                225.1 kHz
  // output power:                11 dBm
  // sync word:                   0x2D  0x01
  int state = fsk.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set the function that will be called
  // when new packet is received
  fsk.setIrqAction(setFlag);

  // start listening for packets
  Serial.print(F("[Si4432] Starting to listen ... "));
  state = fsk.startReceive();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // if needed, 'listen' mode can be disabled by calling
  // any of the following methods:
  //
  // fsk.standby()
  // fsk.sleep()
  // fsk.transmit();
  // fsk.receive();
  // fsk.readData();
}

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }

  // we got a packet, set the flag
  receivedFlag = true;
}

void loop() {
  // check if the flag is set
  if(receivedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    String str;
    int state = fsk.readData(str);

    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int state = fsk.readData(byteArr, 8);
    */

    if (state == ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[Si4432] Received packet!"));

      // print data of the packet
      Serial.print(F("[Si4432] Data:\t\t\t"));
      Serial.println(str);

    } else if (state == ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("failed, code "));
      Serial.println(state);

    }

    // put module back to listen mode
    fsk.startReceive();

    // we're ready to receive more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }

}
