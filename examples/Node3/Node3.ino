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
  
  Serial.print(F("[SX1278]  Initializing ... "));
  byte state = lora.begin();
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
    ledsLow();
    delay(BLINK_DELAY);
    ledsHigh();
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    ledsLow();
    digitalWrite(LED_START_ERROR, HIGH);
    while(true);
  }
  
  Serial.print(F("[ESP8266] Connecting ... "));
  state = wifi.begin(9600);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
    ledsLow();
    delay(BLINK_DELAY);
    ledsHigh();
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    ledsLow();
    digitalWrite(LED_START_ERROR, HIGH);
    while(true);
  }
  
  Serial.print(F("[ESP8266] Joining AP ... "));
  state = wifi.join("Tenda", "Student20-X13");
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
    ledsLow();
    delay(BLINK_DELAY);
    ledsHigh();
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    ledsLow();
    digitalWrite(LED_START_ERROR, HIGH);
    while(true);
  }

  Serial.print(F("[ESP8266] Connecting to MQTT broker ... "));
  state = wifi.MqttConnect("broker.shiftr.io", "Node3", "7dfeba8b", "3b0bb0efc0916009");
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    ledsLow();
    digitalWrite(LED_START_ERROR, HIGH);
    while(true);
  }

  ledsLow();
  digitalWrite(LED_START_OK, HIGH);
}

void loop() {
  Serial.print(F("[SX1278]  Waiting for incoming transmission ... "));
  byte state = lora.receive(pack);

  if(state == ERR_NONE) {
    digitalWrite(LED_RECEIVING, HIGH);
    Serial.println(F("success!"));

    char str[24];

    pack.getSourceStr(str);
    Serial.print(F("[SX1278]  Source:\t"));
    Serial.println(str);

    pack.getDestinationStr(str);
    Serial.print(F("[SX1278]  Destination:\t"));
    Serial.println(str);

    Serial.print(F("[SX1278]  Length:\t"));
    Serial.println(pack.length);

    Serial.print(F("[SX1278]  Data:\t\t"));
    Serial.println(pack.data);

    Serial.print(F("[SX1278]  Datarate:\t"));
    Serial.print(lora.dataRate);
    Serial.println(F(" bps"));

    Serial.print(F("[SX1278]  RSSI:\t\t"));
    Serial.print(lora.lastPacketRSSI);
    Serial.println(F(" dBm"));

    digitalWrite(LED_RECEIVING, LOW);

    Serial.print(F("[ESP8266] Publishing MQTT message ... "));
    digitalWrite(LED_TRANSMITING, HIGH);
    byte state = wifi.MqttPublish("Kite", pack.data);
    if(state == ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code 0x"));
      Serial.println(state, HEX);
    }
    digitalWrite(LED_TRANSMITING, LOW);
    
  } else if(state == ERR_RX_TIMEOUT) {
    Serial.println(F("timeout!"));
    
  } else if(state == ERR_CRC_MISMATCH) {
    Serial.println(F("CRC error!"));
    
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
