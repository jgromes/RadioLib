#include <Arduino.h>

#include "config.h"

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("\nSetup ... "));

  Serial.println(F("Initalise the radio"));
  int state = radio.begin();
  debug(state != RADIOLIB_ERR_NONE, F("Initalise radio failed"), state, true);

  Serial.println(F("Join ('login') to the LoRaWAN Network"));
  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  debug(state < RADIOLIB_ERR_NONE, F("Join failed"), state, true);

  Serial.println(F("Ready!\n"));
}


void loop() {
  Serial.print(F("Sending uplink #"));

  // Read some inputs
  uint8_t Digital1 = digitalRead(2);
  uint16_t Analog1 = analogRead(A0);

  // Build payload byte array
  uint8_t uplinkPayload[3];
  uplinkPayload[0] = Digital1;
  uplinkPayload[1] = highByte(Analog1);   // See notes for high/lowByte functions
  uplinkPayload[2] = lowByte(Analog1);
  
  // Perform an uplink
  int state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload));    
  debug((state != RADIOLIB_ERR_RX_TIMEOUT) && (state != RADIOLIB_ERR_NONE), F("Error in sendReceive"), state, false);
  
  // Complete serial line with the uplink counter
  Serial.println(node.getFcntUp());

  // Wait until next uplink - observing legal & TTN FUP constraints
  delay(uplinkInterval);
}
