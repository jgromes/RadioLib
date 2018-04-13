#include "KiteLib.h"

#define LED_START_ERROR   A1
#define LED_START_OK      A0
#define LED_RECEIVING     A3
#define LED_TRANSMITING   A2

#define BLINK_DELAY       250

ESP8266 wifi = Kite.ModuleA;
SX1278 lora = Kite.ModuleB;

Packet pack;

void setup() {
  Serial.begin(9600);

  pinMode(LED_START_ERROR, OUTPUT);
  pinMode(LED_START_OK, OUTPUT);
  pinMode(LED_RECEIVING, OUTPUT);
  pinMode(LED_TRANSMITING, OUTPUT);

  ledsHigh();

  Serial.print("[SX1278]  Initializing ... ");
  byte state = lora.begin();
  if(state == ERR_NONE) {
    Serial.println("success!");
    ledsLow();
    delay(BLINK_DELAY);
    ledsHigh();
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    ledsLow();
    digitalWrite(LED_START_ERROR, HIGH);
    while(true);
  }

  Serial.print("[ESP8266] Connecting ... ");
  state = wifi.begin(9600);
  if(state == ERR_NONE) {
    Serial.println("success!");
    ledsLow();
    delay(BLINK_DELAY);
    ledsHigh();
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    ledsLow();
    digitalWrite(LED_START_ERROR, HIGH);
    while(true);
  }

  Serial.print("[ESP8266] Joining AP ... ");
  state = wifi.join("SSID", "PASSWORD");
  if(state == ERR_NONE) {
    Serial.println("success!");
    ledsLow();
    delay(BLINK_DELAY);
    ledsHigh();
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    ledsLow();
    digitalWrite(LED_START_ERROR, HIGH);
    while(true);
  }

  Serial.print("[ESP8266] Connecting to MQTT broker ... ");
  state = wifi.MqttConnect("broker.shiftr.io", "KiteNode3", "KiteNode3", "adea1a9c95ef8cb6");
  if(state == ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code 0x");
    Serial.println(state, HEX);
    ledsLow();
    digitalWrite(LED_START_ERROR, HIGH);
    while(true);
  }

  ledsLow();
  digitalWrite(LED_START_OK, HIGH);
}

void loop() {
  Serial.print("[SX1278]  Waiting for incoming transmission ... ");
  byte state = lora.receive(pack);

  if(state == ERR_NONE) {
    digitalWrite(LED_RECEIVING, HIGH);
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

    digitalWrite(LED_RECEIVING, LOW);

    Serial.print("[ESP8266] Publishing MQTT message ... ");
    digitalWrite(LED_TRANSMITING, HIGH);
    byte state = wifi.MqttPublish("Kite", String(pack.data));
    if(state == ERR_NONE) {
      Serial.println("success!");
    } else {
      Serial.print("failed, code 0x");
      Serial.println(state, HEX);
    }
    digitalWrite(LED_TRANSMITING, LOW);
    
  } else if(state == ERR_RX_TIMEOUT) {
    Serial.println("timeout!");
    
  } else if(state == ERR_CRC_MISMATCH) {
    Serial.println("CRC error!");
    
  }
}

void ledsHigh() {
  digitalWrite(LED_START_ERROR, HIGH);
  digitalWrite(LED_START_OK, HIGH);
  digitalWrite(LED_RECEIVING, HIGH);
  digitalWrite(LED_TRANSMITING, HIGH);
}

void ledsLow() {
  digitalWrite(LED_START_ERROR, LOW);
  digitalWrite(LED_START_OK, LOW);
  digitalWrite(LED_RECEIVING, LOW);
  digitalWrite(LED_TRANSMITING, LOW);
}

