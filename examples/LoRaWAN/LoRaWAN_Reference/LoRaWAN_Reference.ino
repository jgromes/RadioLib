/*
  RadioLib LoRaWAN End Device Reference Example

  This example joins a LoRaWAN network and will send
  uplink packets. Before you start, you will have to
  register your device at https://www.thethingsnetwork.org/
  After your device is registered, you can run this example.
  The device will join the network and start uploading data.

  Also, most of the possible and available functions are
  shown here for reference.

  LoRaWAN v1.1 requires the use of EEPROM (persistent storage).
  Running this examples REQUIRES you to check "Resets DevNonces"
  on your LoRaWAN dashboard. Refer to the notes or the 
  network's documentation on how to do this.
  To comply with LoRaWAN v1.1's persistent storage, refer to
  https://github.com/radiolib-org/radiolib-persistence

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/

  For LoRaWAN details, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/LoRaWAN

*/

#include "config.h"

// include the library
#include <RadioLib.h>

void setup() {
  Serial.begin(115200);
  while(!Serial);  // Wait for serial to be initialised
  delay(5000);  // Give time to switch to the serial monitor
  Serial.println(F("\nSetup"));

  int16_t state = 0;  // return value for calls to RadioLib

  Serial.println(F("Initialise the radio"));
  state = radio.begin();
  debug(state != RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);

  // Override the default join rate
  uint8_t joinDR = 4;

  // Setup the OTAA session information
  node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);

  Serial.println(F("Join ('login') the LoRaWAN Network"));
  state = node.activateOTAA(joinDR);
  debug(state != RADIOLIB_LORAWAN_NEW_SESSION, F("Join failed"), state, true);

  // Print the DevAddr
  Serial.print("[LoRaWAN] DevAddr: ");
  Serial.println((unsigned long)node.getDevAddr(), HEX);

  // Disable the ADR algorithm (on by default which is preferable)
  node.setADR(false);

  // Set a fixed datarate
  node.setDatarate(4);

  // Manages uplink intervals to the TTN Fair Use Policy
  node.setDutyCycle(true, 1250);

  // Enable the dwell time limits - 400ms is the limit for the US
  node.setDwellTime(true, 400);

  Serial.println(F("Ready!\n"));
}

void loop() {
  int16_t state = RADIOLIB_ERR_NONE;

  // set battery fill level - the LoRaWAN network server
  // may periodically request this information
  // 0 = external power source
  // 1 = lowest (empty battery)
  // 254 = highest (full battery)
  // 255 = unable to measure
  uint8_t battLevel = 146;
  node.setDeviceStatus(battLevel);

  // This is the place to gather the sensor inputs
  // Instead of reading any real sensor, we just generate some random numbers as example
  uint8_t value1 = radio.random(100);
  uint16_t value2 = radio.random(2000);

  // Build payload byte array
  uint8_t uplinkPayload[3];
  uplinkPayload[0] = value1;
  uplinkPayload[1] = highByte(value2);   // See notes for high/lowByte functions
  uplinkPayload[2] = lowByte(value2);

  uint8_t downlinkPayload[10];  // Make sure this fits your plans!
  size_t  downlinkSize;         // To hold the actual payload size received

  // you can also retrieve additional information about an uplink or 
  // downlink by passing a reference to LoRaWANEvent_t structure
  LoRaWANEvent_t uplinkDetails;
  LoRaWANEvent_t downlinkDetails;
  
  uint8_t Port = 10;

  // Retrieve the last uplink frame counter
  uint32_t fcntUp = node.getFCntUp();

  // Send a confirmed uplink every 64th frame
  // and also request the LinkCheck and DeviceTime MAC commands
  if(fcntUp % 64 == 0) {
    Serial.println(F("[LoRaWAN] Requesting LinkCheck and DeviceTime"));
    node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_LINK_CHECK);
    node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_DEVICE_TIME);
    state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload), Port, downlinkPayload, &downlinkSize, true, &uplinkDetails, &downlinkDetails); 
  } else {
    state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload), Port, downlinkPayload, &downlinkSize);    
  }
  debug((state != RADIOLIB_LORAWAN_NO_DOWNLINK) && (state != RADIOLIB_ERR_NONE), F("Error in sendReceive"), state, false);

  // Check if downlink was received
  if(state != RADIOLIB_LORAWAN_NO_DOWNLINK) {
    // Did we get a downlink with data for us
    if(downlinkSize > 0) {
      Serial.println(F("Downlink data: "));
      arrayDump(downlinkPayload, downlinkSize);
    } else {
      Serial.println(F("<MAC commands only>"));
    }

    // print RSSI (Received Signal Strength Indicator)
    Serial.print(F("[LoRaWAN] RSSI:\t\t"));
    Serial.print(radio.getRSSI());
    Serial.println(F(" dBm"));

    // print SNR (Signal-to-Noise Ratio)
    Serial.print(F("[LoRaWAN] SNR:\t\t"));
    Serial.print(radio.getSNR());
    Serial.println(F(" dB"));

    // print frequency error
    Serial.print(F("[LoRaWAN] Frequency error:\t"));
    Serial.print(radio.getFrequencyError());
    Serial.println(F(" Hz"));

    // print extra information about the event
    Serial.println(F("[LoRaWAN] Event information:"));
    Serial.print(F("[LoRaWAN] Confirmed:\t"));
    Serial.println(downlinkDetails.confirmed);
    Serial.print(F("[LoRaWAN] Confirming:\t"));
    Serial.println(downlinkDetails.confirming);
    Serial.print(F("[LoRaWAN] Datarate:\t"));
    Serial.println(downlinkDetails.datarate);
    Serial.print(F("[LoRaWAN] Frequency:\t"));
    Serial.print(downlinkDetails.freq, 3);
    Serial.println(F(" MHz"));
    Serial.print(F("[LoRaWAN] Output power:\t"));
    Serial.print(downlinkDetails.power);
    Serial.println(F(" dBm"));
    Serial.print(F("[LoRaWAN] Frame count:\t"));
    Serial.println(downlinkDetails.fCnt);
    Serial.print(F("[LoRaWAN] Port:\t\t"));
    Serial.println(downlinkDetails.fPort);

    uint8_t margin = 0;
    uint8_t gwCnt = 0;
    if(node.getMacLinkCheckAns(&margin, &gwCnt) == RADIOLIB_ERR_NONE) {
      Serial.print(F("[LoRaWAN] LinkCheck margin:\t"));
      Serial.println(margin);
      Serial.print(F("[LoRaWAN] LinkCheck count:\t"));
      Serial.println(gwCnt);
    }

    uint32_t networkTime = 0;
    uint8_t fracSecond = 0;
    if(node.getMacDeviceTimeAns(&networkTime, &fracSecond, true) == RADIOLIB_ERR_NONE) {
      Serial.print(F("[LoRaWAN] DeviceTime Unix:\t"));
      Serial.println(networkTime);
      Serial.print(F("[LoRaWAN] DeviceTime second:\t1/"));
      Serial.println(fracSecond);
    }
  
  }

  // wait before sending another packet
  uint32_t minimumDelay = uplinkIntervalSeconds * 1000UL;
  uint32_t interval = node.timeUntilUplink();     // calculate minimum duty cycle delay (per FUP & law!)
  uint32_t delayMs = max(interval, minimumDelay); // cannot send faster than duty cycle allows

  Serial.print(F("[LoRaWAN] Next uplink in "));
  Serial.print(delayMs/1000);
  Serial.println(F("s"));

  delay(delayMs);
}
