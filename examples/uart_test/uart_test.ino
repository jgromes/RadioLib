#include <SoftwareSerial.h>

const int RX_A = 9;
const int TX_A = 8;
const int RX_B = 7;
const int TX_B = 6;

SoftwareSerial module(RX_A, TX_A); // Rx, Tx, Module A
//SoftwareSerial module(RX_B, TX_B); // Rx, Tx, Module B

void setup() {
  Serial.begin(9600);
  module.begin(9600);
}

void loop() {
  while(Serial.available() > 0) {
    module.write(Serial.read());
  }
  
  while(module.available() > 0) {
    Serial.write(module.read());
  }
}
