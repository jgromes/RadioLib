#include "KiteLib.h"

HC05 bluetooth = Kite.ModuleA;
SX1272 lora = Kite.ModuleB;

void setup() {
  Serial.begin(9600);

  bluetooth.begin(9600);
  Serial.println("[HC05] Port open!");

  Serial.print("[SX1272] Initializing ... ");
  byte state = lora.begin();
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

  Serial.println("[HC05] Waiting for incoming data ...");
  while(bluetooth.available() > 0) {
    char receivedCharacter = bluetooth.read();
    receivedString += receivedCharacter;
    if(receivedCharacter == '\n') {
      Serial.print("[HC05] Received string: ");
      Serial.println(receivedString);
      receivedFlag = true;
      break;
    }
  }

  if(receivedFlag) {
    receivedFlag = false;
    Packet pack("01:23:45:67:89:AB:CD:EF", receivedString.c_str());
    Serial.print("[SX1272] Transmitting packet ... ");
    byte state = lora.transmit(pack);
    if(state == ERR_NONE) {
      Serial.println("success!");
    } else {
      Serial.print("failed, code 0x");
      Serial.println(state, HEX);
    }
  }
}

