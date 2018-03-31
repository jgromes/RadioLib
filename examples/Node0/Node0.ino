#include "KiteLib.h"

HC05 bluetooth = Kite.ModuleA;
RF69 rf = Kite.ModuleB;

void setup() {
  Serial.begin(9600);

  bluetooth.begin(9600);
  Serial.println("[HC05] Port open!");

  Serial.print("[RF69] Initializing ... ");
  byte state = rf.begin();
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    while(true);
  }

  Serial.println("[HC05] Waiting for incoming data ... ");
}

void loop() {
  bool receivedFlag = false;
  String receivedString;

  while(bluetooth.available() > 0) {
    char receivedCharacter = bluetooth.read();
    Serial.print("[HC05] ");
    Serial.print(receivedCharacter);
    Serial.print("\t 0x");
    Serial.println(receivedCharacter, HEX);
    if(receivedCharacter != '\n') {
      receivedString += receivedCharacter;
    } else {
      Serial.print("[HC05] Received string: ");
      Serial.println(receivedString);
      receivedFlag = true;
      break;
    }
  }

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
    Serial.println("[HC05] Waiting for incoming data ... ");
  }
}

