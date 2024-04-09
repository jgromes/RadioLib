/*
  RadioLib LoRaWAN Starter Example

  This example joins a LoRaWAN network and will send
  uplink packets. Before you start, you will have to
  register your device at https://www.thethingsnetwork.org/
  After your device is registered, you can run this example.
  The device will join the network and start uploading data.

  Running this examples REQUIRES you to check "Resets DevNonces"
  on your LoRaWAN dashboard. Refer to the network's 
  documentation on how to do this.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/

  For LoRaWAN details, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/LoRaWAN

*/

#include "config.h"

void setup() {
  Serial.begin(115200);
  while(!Serial);
  delay(5000);  // Give time to switch to the serial monitor
  Serial.println(F("\nSetup ... "));

  Serial.println(F("Initialise the radio"));
  int state = radio.begin();
  debug(state != RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);

  Serial.println(F("Join ('login') to the LoRaWAN Network"));
  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey, true);
  debug(state < RADIOLIB_ERR_NONE, F("Join failed"), state, true);

  Serial.println(F("Ready!\n"));
}

void loop() {
  Serial.println(F("Sending uplink"));

  // This is the place to gather the sensor inputs
  // Instead of reading any real sensor, we just generate some random numbers as example
  uint8_t value1 = radio.random(100);
  uint16_t value2 = radio.random(2000);

  // Build payload byte array
  uint8_t uplinkPayload[3];
  uplinkPayload[0] = value1;
  uplinkPayload[1] = highByte(value2);   // See notes for high/lowByte functions
  uplinkPayload[2] = lowByte(value2);
  
  // Perform an uplink
  int state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload));    
  debug((state != RADIOLIB_LORAWAN_NO_DOWNLINK) && (state != RADIOLIB_ERR_NONE), F("Error in sendReceive"), state, false);

  Serial.print(F("Uplink complete, next in "));
  Serial.print(uplinkIntervalSeconds);
  Serial.println(F(" seconds"));
  
  // Wait until next uplink - observing legal & TTN FUP constraints
  delay(uplinkIntervalSeconds * 1000UL);  // delay needs milli-seconds
}
