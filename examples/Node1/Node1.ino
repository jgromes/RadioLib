#include "KiteLib.h"

XBee bee = Kite.ModuleA;
RF69 rf = Kite.ModuleB;

Packet pack;

void setup() {
  Serial.begin(9600);

  Serial.print("[XBee] Initializing ... ");
  byte state = bee.begin(9600);
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    while(true);
  }

  Serial.print("[XBee] Setting PAN ID ... ");
  state = bee.setPanId("0123456789ABCDEF");
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    while(true);
  }

  Serial.print("[XBee] Setting destination address ... ");
  state = bee.setDestinationAddress("0013A200", "40A58A5D");
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    while(true);
  }

  Serial.print("[RF69] Initializing ... ");
  state = rf.begin();
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    while(true);
  }
}

void loop() {
  Serial.print("[RF69] Waiting for incoming transmission ... ");
  byte state = rf.receive(pack);
  bee.println("Hello World!");

  if(state == ERR_NONE) {
    Serial.println("success!");

    char str[24];

    pack.getSourceStr(str);
    Serial.print("[RF69] Source:\t\t");
    Serial.println(str);

    pack.getDestinationStr(str);
    Serial.print("[RF69] Destination:\t");
    Serial.println(str);

    Serial.print("[RF69] Length:\t\t");
    Serial.println(pack.length);

    Serial.print("[RF69] Data:\t\t");
    Serial.println(pack.data);

    Serial.print("[XBee] Sending packet ... ");
    bee.println(pack.data);
    Serial.println("done!");
    
  } else if(state == ERR_RX_TIMEOUT) {
    Serial.println("timeout!");
    
  } else if(state == ERR_CRC_MISMATCH) {
    Serial.println("CRC error!");
    
  }
}

