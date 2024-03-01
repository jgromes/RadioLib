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
  Please refer to the 'persistent' example once you are familiar
  with LoRaWAN.
  Running this examples REQUIRES you to check "Resets DevNonces"
  on your LoRaWAN dashboard. Refer to the network's 
  documentation on how to do this.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/

  For LoRaWAN details, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/LoRaWAN


  Last updated 1st March 2024 for RadioLib 6.4.2

*/

// include the library
#include <RadioLib.h>

// SX1262 has the following pin order:
// Module(NSS/CS, DIO1, RESET, BUSY)
SX1262 radio = new Module(8, 14, 12, 13);

// SX1278 has the following pin order:
// Module(NSS/CS, DIO0, RESET, DIO1)
// SX1278 radio = new Module(10, 2, 9, 3);

// create the node instance on the EU-868 band
// using the radio module and the encryption key
// make sure you are using the correct band
// based on your geographical location!
LoRaWANNode node(&radio, &EU868);

// for fixed bands with subband selection
// such as US915 and AU915, you must specify
// the subband that matches the Frequency Plan
// that you selected on your LoRaWAN console
// LoRaWANNode node(&radio, &US915, 2);


void setup() {
  Serial.begin(9600);

  // initialize radio (SX1262 / SX1278 / ... ) with default settings
  Serial.print(F("[Radio] Initializing ... "));
  int state = radio.begin();
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  // JoinEUI - previous versions of LoRaWAN this was AppEUI
  // for development purposes you can use all zeros - see wiki for details
  uint64_t joinEUI = 0x0000000000000000;

  // DevEUI - The device's Extended Unique Identifier
  // TTN will generate one for you
  uint64_t devEUI =  0x----------------;

  // encryption keys used to secure the communication
  // TTN will generate them for you
  // see wiki for details on copying & pasting them
  uint8_t nwkKey[] = { 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--,   
                       0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x-- };
  uint8_t appKey[] = { 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--,   
                       0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x-- };


  // Override the default join rate
  uint8_t joinDR = 3;

  // Begin the join to the network
  Serial.print(F("[LoRaWAN] Attempting over-the-air activation ... "));
  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey, joinDR);
  if(state >= RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
    delay(2000);	// small delay between joining and uplink
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  Serial.print("[LoRaWAN] DevAddr: ");
  Serial.println(node.getDevAddr(), HEX);

  // disable the ADR algorithm (on by default which is preferable)
  node.setADR(false);

  // set a fixed datarate & make it persistent (not normal)
  node.setDatarate(5, true);

  // enable CSMA which tries to minimize packet loss by searching 
  // for a free channel before actually sending an uplink 
  node.setCSMA(6, 2, true);

  // manages uplink intervals to the TTN Fair Use Policy
   node.setDutyCycle(true, 1250);

  // enable the dwell time limits - 400ms is the limit for the US
  node.setDwellTime(true, 400);

} // setup


void loop() {
  int state = RADIOLIB_ERR_NONE;

  // set battery fill level - the LoRaWAN network server
  // may periodically request this information
  // 0 = external power source
  // 1 = lowest (empty battery)
  // 254 = highest (full battery)
  // 255 = unable to measure
  uint8_t battLevel = 146;
  node.setDeviceStatus(battLevel);

  // retrieve the last uplink frame counter
  uint32_t fcntUp = node.getFcntUp();

  Serial.print(F("[LoRaWAN] Sending uplink packet #"));
  Serial.println(fcntUp);
  String strUp = "Hello! " + String(fcntUp);
  
  // send a confirmed uplink to port 10 every 64th frame
  // and also request the LinkCheck and DeviceTime MAC commands
  if(fcntUp % 64 == 0) {
    Serial.print(F("[LoRaWAN] Requesting LinkCheck and DeviceTime"));
    node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_LINK_CHECK);
    node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_DEVICE_TIME);
    state = node.uplink(strUp, 10, true);
  } else {
    state = node.uplink(strUp, 10);
  }
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // after uplink, you must call downlink() to receive any possible reply
  // from the server. This function must be called before the Rx1 delay
  // for the network. Typically this is 5s after end of uplink.
  Serial.println(F("[LoRaWAN] Waiting for downlink ... "));
  String strDown;

  // you can also retrieve additional information about an uplink or 
  // downlink by passing a reference to LoRaWANEvent_t structure
  LoRaWANEvent_t downlinkDetails;
  state = node.downlink(strDown, &downlinkDetails);
  if(state == RADIOLIB_ERR_NONE) {
    // print data of the packet
    Serial.print(F("[LoRaWAN] Data:\t\t"));
    if(strDown.length() > 0) {
      for (uint8_t c = 0; c < strDown.length(); c++) {
        uint8_t value = strDown[c];
        if (value < 10) Serial.print(F("0"));
        Serial.print(value, HEX);
      }
      Serial.println();
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
    Serial.println(downlinkDetails.fcnt);
    Serial.print(F("[LoRaWAN] Port:\t\t"));
    Serial.println(downlinkDetails.port);

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
  
  } else if(state == RADIOLIB_ERR_RX_TIMEOUT) {
    // Not really necessary to report normal operation
  
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // on boards that can save to Flash or EEPROM this saves the session
  // which allows recall of the session after reboot or deepsleep
  node.saveSession();

  // wait before sending another packet
  uint32_t minimumDelay = 3 * 60 * 1000;          // try to send once every 3 minutes
  uint32_t interval = node.timeUntilUplink();     // calculate minimum duty cycle delay (per FUP & law!)
  uint32_t delayMs = max(interval, minimumDelay); // cannot send faster than duty cycle allows

  Serial.print(F("[LoRaWAN] Next uplink in "));
  Serial.print(delayMs/1000);
  Serial.println(F("s"));

  delay(delayMs);
  
}   // loop