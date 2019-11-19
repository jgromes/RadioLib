/*
   RadioLib CC1101 Receive with Interrupts Example

   This example listens for FSK transmissions and tries to
   receive them. Once a packet is received, an interrupt is
   triggered.

   To successfully receive data, the following settings have to be the same
   on both transmitter and receiver:
    - carrier frequency
    - bit rate
    - frequency deviation
    - sync word

    For full API reference, see the GitHub Pages
    https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// CC1101 has the following connections:
// NSS pin:   10
// GDO0 pin:  2
// GDO2 pin:  3
CC1101 cc = new Module(10, 2, 3);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//CC1101 cc = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize CC1101 with default settings
  Serial.print(F("[CC1101] Initializing ... "));
  // carrier frequency:                   868.0 MHz
  // bit rate:                            4.8 kbps
  // Rx bandwidth:                        325.0 kHz
  // frequency deviation:                 48.0 kHz
  // sync word:                           0xD391
  int state = cc.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set the function that will be called
  // when new packet is received
  cc.setGdo0Action(setFlag);

  // start listening for packets
  Serial.print(F("[CC1101] Starting to listen ... "));
  state = cc.startReceive();
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
  // cc.standby()
  // cc.sleep()
  // cc.transmit();
  // cc.receive();
  // cc.readData();
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
    int state = cc.readData(str);

    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int state = cc.receive(byteArr, 8);
    */

    if (state == ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[CC1101] Received packet!"));

      // print data of the packet
      Serial.print(F("[CC1101] Data:\t\t"));
      Serial.println(str);

      // print RSSI (Received Signal Strength Indicator)
      // of the last received packet
      Serial.print(F("[CC1101] RSSI:\t\t"));
      Serial.print(cc.getRSSI());
      Serial.println(F(" dBm"));

      // print LQI (Link Quality Indicator)
      // of the last received packet, lower is better
      Serial.print(F("[CC1101] LQI:\t\t"));
      Serial.println(cc.getLQI());

    } else if (state == ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("failed, code "));
      Serial.println(state);

    }

    // put module back to listen mode
    cc.startReceive();

    // we're ready to receive more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }

}
