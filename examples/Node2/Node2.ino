#include "KiteLib.h"

XBee bee = Kite.ModuleA;
RF69 rf = Kite.ModuleB;

void setup() {
  Serial.begin(9600);

  bee.begin(9600);
  Serial.println("[XBee] Port open!");

  Serial.print("[RF69] Initializing ... ");
  byte state = rf.begin();
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    while(true);
  }
}

void loop() {
  bool receivedFlag = false;
  String receivedString;

  Serial.println("[XBee] Waiting for incoming data ...");
  
  if(receivedFlag) {
    receivedFlag = false;
    Packet pack("01:23:45:67:89:AB:CD:EF", receivedString.c_str());
    Serial.print("[RF69] Transmitting packet ... ");
    byte state = rf.transmit(pack);
    if(state == ERR_NONE) {
      Serial.println("success!");
    } else {
      Serial.print("failed, code 0x");
      Serial.println(state, HEX);
    }
  }
}

