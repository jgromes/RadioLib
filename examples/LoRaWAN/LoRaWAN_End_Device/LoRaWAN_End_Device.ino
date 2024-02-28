/*
  RadioLib LoRaWAN End Device Example

  This example joins a LoRaWAN network and will send
  uplink packets. Before you start, you will have to
  register your device at https://www.thethingsnetwork.org/
  After your device is registered, you can run this example.
  The device will join the network and start uploading data.

  NOTE: LoRaWAN v1.1 requires storing parameters persistently!
        RadioLib does this by using EEPROM (persistent storage), 
        by default starting at address 0 and using 448 bytes.
        If you already use EEPROM in your application,
        you will have to either avoid this range, or change it
        by setting a different start address by changing the value of
        RADIOLIB_HAL_PERSISTENT_STORAGE_BASE macro, either
        during build or in src/BuildOpt.h.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/

  For LoRaWAN details, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/LoRaWAN
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

  // JoinEUI - previous versions of LoRaWAN called this AppEUI
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


  // Manages uplink intervals to the TTN Fair Use Policy
  node.setDutyCycle(true, 1250);

  // Begin the join to the network
  Serial.print(F("[LoRaWAN] Attempting over-the-air activation ... "));
  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  if(state >= RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
    delay(2000);	// small delay between joining and uplink
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }
}   // setup

// counter to keep track of transmitted packets
int count = 0;

void loop() {
  // send uplink to port 10
  Serial.print(F("[LoRaWAN] Sending uplink packet ... "));
  String strUp = "Hello! " + String(count++);
  String strDown;
  int state = node.sendReceive(strUp, 10, strDown);
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("received a downlink!"));

    // print data of the packet (if there are any)
    Serial.print(F("[LoRaWAN] Data:\t\t"));
    if(strDown.length() > 0) {
      Serial.println(strDown);
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
  
  } else if(state == RADIOLIB_ERR_RX_TIMEOUT) {
    Serial.println(F(""));
  
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // on boards that can save to Flash or EEPROMthis saves the session
  // which allows recall of the session after reboot or deepsleep
  node.saveSession();
  
  // wait before sending another packet
  uint32_t minimumDelay = 300000;                 // try to send once every 3 minutes
  uint32_t interval = node.timeUntilUplink();     // calculate minimum duty cycle delay (per FUP & law!)
  uint32_t delayMs = max(interval, minimumDelay); // cannot send faster than duty cycle allows
  
  Serial.print(F("[LoRaWAN] Next uplink in "));
  Serial.print(delayMs/60);
  Serial.println(F("s"));
  
  delay(delayMs);
}   // loop
