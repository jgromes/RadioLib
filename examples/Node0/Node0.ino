#include "KiteLib.h"

#define LED_START_ERROR   A1
#define LED_START_OK      A0
#define LED_RECEIVING     A3
#define LED_TRANSMITING   A2

#define BLINK_DELAY       250

HC05 bluetooth = Kite.ModuleA;
RF69 rf = Kite.ModuleB;

void setup() {
  Serial.begin(9600);

  pinMode(LED_START_ERROR, OUTPUT);
  pinMode(LED_START_OK, OUTPUT);
  pinMode(LED_RECEIVING, OUTPUT);
  pinMode(LED_TRANSMITING, OUTPUT);

  ledsHigh();

  bluetooth.begin(9600);
  Serial.println("[HC05] Port open!");

  Serial.print("[RF69] Initializing ... ");
  byte state = rf.begin();
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

  Serial.println("[HC05] Waiting for incoming data ... ");
}

void loop() {
  bool receivedFlag = false;
  String receivedString;

  while(bluetooth.available() > 0) {
    digitalWrite(LED_RECEIVING, HIGH);
    char receivedCharacter = bluetooth.read();
    Serial.print("[HC05] ");
    Serial.print(receivedCharacter);
    Serial.print("\t 0x");
    Serial.println(receivedCharacter, HEX);
    digitalWrite(LED_RECEIVING, LOW);
    if(receivedCharacter != '\n') {
      receivedString += receivedCharacter;
    } else {
      Serial.print("[HC05] Received string: ");
      Serial.println(receivedString);
      receivedFlag = true;
      break;
    }
  }

  if(receivedFlag) {
    digitalWrite(LED_TRANSMITING, HIGH);
    receivedFlag = false;
    Packet pack("01:23:45:67:89:AB:CD:EF", receivedString.c_str());
    Serial.print("[RF69] Transmitting packet ... ");
    byte state = rf.transmit(pack);
    if(state == ERR_NONE) {
      Serial.println("success!");
    } else {
      Serial.print("failed, code 0x");
      Serial.println(state, HEX);
    }
    digitalWrite(LED_TRANSMITING, LOW);
    Serial.println("[HC05] Waiting for incoming data ... ");
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

