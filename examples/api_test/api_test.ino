#include "KiteLib.h"

ESP8266 wifi = Kite.ModuleA;
//SX1278 lora = Kite.ModuleB;
//HC05 bluetooth = Kite.ModuleB;

//Packet pack("01:23:45:67:89:AB:CD:EF", "Hello World! (by LoRa)");

void setup() {
  Serial.begin(9600);

  Serial.print("[ESP8266] Connecting ... ");
  uint8_t state = wifi.begin(9600);
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code ");
    Serial.println(state, HEX);
  }

  Serial.print("[ESP8266] Joining AP ... ");
  state = wifi.join("Tenda", "Student20-X13");
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code ");
    Serial.println(state, HEX);
  }

  Serial.print("[ESP8266] Sending GET request ... ");
  String response;
  uint16_t http_code = wifi.HttpGet("http://www.httpbin.org/ip", response);
  if(http_code == 200) {
    Serial.println("success!");
    Serial.println("[ESP8266] Response:\n");
    Serial.println(response);
  } else {
    Serial.print("failed, code ");
    Serial.println(http_code, HEX);
  }

  //lora.begin();

  //bluetooth.begin(9600);
}

void loop() {
  //wifi.send("GET / HTTP/1.1\r\nHost: www.google.com\r\nConnection: close\r\n\r\n");
  
  //lora.transmit(pack);

  //bluetooth.println("Hello World! (by Blueooth)");
  
  delay(1000);
}

