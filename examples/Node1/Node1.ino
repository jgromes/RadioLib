#include "KiteLib.h"

SX1272 lora = Kite.ModuleA;
XBee bee = Kite.ModuleB;

Packet pack;

void setup() {
  Serial.begin(9600);

  Serial.print("[SX1272] Initializing ... ");
  byte state = lora.begin();
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    while(true);
  }

  bee.begin(9600);
  Serial.println("[XBee] Port open!");
}

void loop() {
  Serial.print("[SX1272] Waiting for incoming transmission ... ");
  byte state = lora.receive(pack);

  if(state == ERR_NONE) {
    Serial.println("success!");

    char str[24];

    pack.getSourceStr(str);
    Serial.print("[SX1272] Source:\t\t");
    Serial.println(str);

    pack.getDestinationStr(str);
    Serial.print("[SX1272] Destination:\t");
    Serial.println(str);

    Serial.print("[SX1272] Length:\t\t");
    Serial.println(pack.length);

    Serial.print("[SX1272] Data:\t\t");
    Serial.println(pack.data);

    Serial.print("[SX1272] Datarate:\t");
    Serial.print(lora.dataRate);
    Serial.println(" bps");

    Serial.print("[SX1272] RSSI:\t\t");
    Serial.print(lora.lastPacketRSSI);
    Serial.println(" dBm");

    Serial.print("[XBee] Sending packet ...");
    state = bee.send(0x0013A200, 0x40A58A62, pack.data);
    if(state == ERR_NONE) {
      Serial.println("success!");
    }
    
  } else if(state == ERR_RX_TIMEOUT) {
    Serial.println("timeout!");
    
  } else if(state == ERR_CRC_MISMATCH) {
    Serial.println("CRC error!");
    
  }
}

