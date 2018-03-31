#include "KiteLib.h"

ESP8266 wifi = Kite.ModuleA;
SX1278 lora = Kite.ModuleB;

Packet pack;

void setup() {
  Serial.begin(9600);

  Serial.print("[SX1278]  Initializing ... ");
  byte state = lora.begin();
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    while(true);
  }

  Serial.print("[ESP8266] Connecting ... ");
  state = wifi.begin(9600);
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    while(true);
  }
}

void loop() {
  Serial.print("[SX1278]  Waiting for incoming transmission ... ");
  byte state = lora.receive(pack);

  if(state == ERR_NONE) {
    Serial.println("success!");

    char str[24];

    pack.getSourceStr(str);
    Serial.print("[SX1278]  Source:\t");
    Serial.println(str);

    pack.getDestinationStr(str);
    Serial.print("[SX1278]  Destination:\t");
    Serial.println(str);

    Serial.print("[SX1278]  Length:\t");
    Serial.println(pack.length);

    Serial.print("[SX1278]  Data:\t\t");
    Serial.println(pack.data);

    Serial.print("[SX1278]  Datarate:\t");
    Serial.print(lora.dataRate);
    Serial.println(" bps");

    Serial.print("[SX1278]  RSSI:\t\t");
    Serial.print(lora.lastPacketRSSI);
    Serial.println(" dBm");

    /*Serial.print("[ESP266] Sending HTTP POST ...");
    String response;
    int http_code = wifi.HttpPost("http://www.httpbin.org/ip", String(pack.data), response);
    if(http_code == 200) {
      Serial.println("success!");
      Serial.println("[ESP8266] Response:\n");
      Serial.println(response);
    } else {
      Serial.print("failed, code 0x");
      Serial.println(http_code, HEX);
    }*/
    
  } else if(state == ERR_RX_TIMEOUT) {
    Serial.println("timeout!");
    
  } else if(state == ERR_CRC_MISMATCH) {
    Serial.println("CRC error!");
    
  }
}

