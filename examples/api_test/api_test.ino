#include "KiteLib.h"

ESP8266 wifi = Kite.ModuleA;
SX1278 lora = Kite.ModuleB;
//HC05 bluetooth = Kite.ModuleB;

Packet pack("01:23:45:67:89:AB:CD:EF", "Hello World! (by LoRa)");

void setup() {
  Serial.begin(9600);

  Serial.print("[ESP8266] Connecting ... ");
  byte state = wifi.begin(9600);
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
  }

  Serial.print("[ESP8266] Joining AP ... ");
  state = wifi.join("SSID", "PASSWORD");
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
  }

  Serial.print("[ESP8266] Sending GET request ... ");
  String response;
  int http_code = wifi.HttpGet("http://www.httpbin.org/ip", response);
  if(http_code == 200) {
    Serial.println("success!");
    Serial.println("[ESP8266] Response:\n");
    Serial.println(response);
  } else {
    Serial.print("failed, code 0x");
    Serial.println(http_code, HEX);
  }

  Serial.print("[SX1278] Initializing ... ");
  state = lora.begin();
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
  }

  //bluetooth.begin(9600);
}

void loop() {
  Serial.print("[SX1278] Transmitting packet ... ");
  byte state = lora.transmit(pack);
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
  }

  //bluetooth.println("Hello World! (by Blueooth)");
  
  delay(1000);
}

