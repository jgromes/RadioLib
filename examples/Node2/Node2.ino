#include "KiteLib.h"

XBee bee = Kite.ModuleA;
SX1278 lora = Kite.ModuleB;

void setup() {
  Serial.begin(9600);

  Serial.print("[XBee]   Initializing ... ");
  byte state = bee.begin(9600);
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    while(true);
  }

  Serial.print("[XBee]   Setting destination address ... ");
  state = bee.setDestinationAddress("0013A200", "40A58A5D");
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    while(true);
  }

  Serial.print("[SX1278] Initializing ... ");
  state = lora.begin();
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    while(true);
  }

  Serial.println("[XBee]   Waiting for incoming data ... ");
}

void loop() {
  bool receivedFlag = false;
  String receivedString;

  while(bee.available() > 0) {
    char receivedCharacter = bee.read();
    Serial.print("[XBee]   ");
    Serial.print(receivedCharacter);
    Serial.print("\t 0x");
    Serial.println(receivedCharacter, HEX);
    if(receivedCharacter != '\n') {
      receivedString += receivedCharacter;
    } else {
      Serial.print("[XBee]   Received string: ");
      Serial.println(receivedString);
      receivedFlag = true;
      break;
    }
  }
  
  if(receivedFlag) {
    receivedFlag = false;
    Packet pack("01:23:45:67:89:AB:CD:EF", receivedString.c_str());
    Serial.print("[SX1278] Transmitting packet ... ");
    byte state = lora.transmit(pack);
    if(state == ERR_NONE) {
      Serial.println("success!");
    } else {
      Serial.print("failed, code 0x");
      Serial.println(state, HEX);
    }
  }
}

