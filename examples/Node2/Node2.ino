#include "KiteLib.h"

#define LED_START_ERROR   A1
#define LED_START_OK      A0
#define LED_RECEIVING     A3
#define LED_TRANSMITING   A2

#define BLINK_DELAY       250

XBee bee = Kite.ModuleA;
SX1278 lora = Kite.ModuleB;

void setup() {
  Serial.begin(9600);

  pinMode(LED_START_ERROR, OUTPUT);
  pinMode(LED_START_OK, OUTPUT);
  pinMode(LED_RECEIVING, OUTPUT);
  pinMode(LED_TRANSMITING, OUTPUT);

  ledsHigh();

  Serial.print(F("[XBee]   Initializing ... "));
  byte state = bee.begin(9600);
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

  Serial.print(F("[XBee]   Setting PAN ID ... "));
  state = bee.setPanId("0123456789ABCDEF");
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

  Serial.print(F("[XBee]   Setting destination address ... "));
  state = bee.setDestinationAddress("0013A200", "40A58A5D");
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

  Serial.print(F("[SX1278] Initializing ... "));
  state = lora.begin();
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
  
  Serial.println(F("[XBee]   Waiting for incoming data ... "));
}

void loop() {
  bool receivedFlag = false;
  String receivedString;

  while(bee.available() > 0) {
    digitalWrite(LED_RECEIVING, HIGH);
    char receivedCharacter = bee.read();
    Serial.print(F("[XBee]   "));
    Serial.print(receivedCharacter);
    Serial.print(F("\t 0x"));
    Serial.println(receivedCharacter, HEX);
    digitalWrite(LED_RECEIVING, LOW);
    if((receivedCharacter != '\n') && (receivedCharacter != '\r')) {
      receivedString += receivedCharacter;
    } else if(receivedCharacter != '\n') {
      Serial.print(F("[XBee]   Received string: "));
      Serial.println(receivedString);
      receivedFlag = true;
      break;
    }
  }
  
  if(receivedFlag) {
    digitalWrite(LED_TRANSMITING, HIGH);
    receivedFlag = false;
    Packet pack("01:23:45:67:89:AB:CD:EF", receivedString.c_str());
    Serial.print(F("[SX1278] Transmitting packet ... "));
    byte state = lora.transmit(pack);
    if(state == ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code 0x"));
      Serial.println(state, HEX);
    }
    digitalWrite(LED_TRANSMITING, LOW);
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

