#include "KiteLib.h"

RF69 rf = Kite.ModuleA;
ESP8266 wifi = Kite.ModuleB;

Packet pack;

void setup() {
  Serial.begin(9600);

  Serial.print("[RF69] Initializing ... ");
  byte state = rf.begin();
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
  Serial.print("[RF69] Waiting for incoming transmission ... ");
  byte state = rf.receive(pack);

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

    Serial.print("[ESP266] Sending HTTP POST ...");
    String response;
    int http_code = wifi.HttpPost("http://www.httpbin.org/ip", String(pack.data), response);
    if(http_code == 200) {
      Serial.println("success!");
      Serial.println("[ESP8266] Response:\n");
      Serial.println(response);
    } else {
      Serial.print("failed, code 0x");
      Serial.println(http_code, HEX);
    }

    if(state == ERR_NONE) {
      Serial.println("success!");
    }
    
  } else if(state == ERR_RX_TIMEOUT) {
    Serial.println("timeout!");
    
  } else if(state == ERR_CRC_MISMATCH) {
    Serial.println("CRC error!");
    
  }
}

