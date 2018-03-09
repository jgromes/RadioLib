#include "KiteLib.h"

XBee bee = Kite.ModuleA;
RF69 rf = Kite.ModuleB;

Packet pack;

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
  
}

