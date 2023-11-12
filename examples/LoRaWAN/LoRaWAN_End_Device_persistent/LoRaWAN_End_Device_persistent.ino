/*
  RadioLib LoRaWAN End Device Persistent Example

  This example assumes you have tried one of the OTAA or ABP
  examples and are familiar with the required keys and procedures.
  This example restores and saves a session such that you can use
  deepsleep or survive power cycles. Before you start, you will 
  have to register your device at https://www.thethingsnetwork.org/
  and join the network using either OTAA or ABP.
  Please refer to one of the other LoRaWAN examples for more
  information regarding joining a network.

  NOTE: LoRaWAN requires storing some parameters persistently!
        RadioLib does this by using EEPROM, by default
        starting at address 0 and using 384 bytes.
        If you already use EEPROM in your application,
        you will have to either avoid this range, or change it
        by setting a different start address by changing the value of
        RADIOLIB_HAL_PERSISTENT_STORAGE_BASE macro, either
        during build or in src/BuildOpt.h.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1278 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// RESET pin: 9
// DIO1 pin:  3
SX1278 radio = new Module(10, 2, 9, 3);

// create the node instance on the EU-868 band
// using the radio module and the encryption key
// make sure you are using the correct band
// based on your geographical location!
LoRaWANNode node(&radio, &EU868);

void setup() {
  Serial.begin(9600);

  // initialize SX1278 with default settings
  Serial.print(F("[SX1278] Initializing ... "));
  int state = radio.begin();
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  // first we need to initialize the device storage
  // this will reset all persistently stored parameters
  // NOTE: This should only be done once prior to first joining a network!
  //       After wiping persistent storage, you will also have to reset
  //       the end device in TTN and perform the join procedure again!
  // Here, a delay is added to make sure that during re-flashing
  // the .wipe() is not triggered and the session is lost
  //delay(5000);
  //node.wipe();

  // now we can start the activation
  // Serial.print(F("[LoRaWAN] Attempting over-the-air activation ... "));
  // uint64_t joinEUI = 0x12AD1011B0C0FFEE;
  // uint64_t devEUI = 0x70B3D57ED005E120;
  // uint8_t nwkKey[] = { 0x74, 0x6F, 0x70, 0x53, 0x65, 0x63, 0x72, 0x65,
  //                      0x74, 0x4B, 0x65, 0x79, 0x31, 0x32, 0x33, 0x34 };
  // uint8_t appKey[] = { 0x61, 0x44, 0x69, 0x66, 0x66, 0x65, 0x72, 0x65,
  //                      0x6E, 0x74, 0x4B, 0x65, 0x79, 0x41, 0x42, 0x43 };
  // state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);

  // after the device has been activated,
  // the session can be restored without rejoining after device power cycle
  // on EEPROM-enabled boards by calling "restore"
  Serial.print(F("[LoRaWAN] Resuming previous session ... "));
  state = node.restore();
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

}

// counter to keep track of transmitted packets
int count = 0;

void loop() {
  // send uplink to port 10
  Serial.print(F("[LoRaWAN] Sending uplink packet ... "));
  String strUp = "Hello World! #" + String(count++);
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
    Serial.println(F("no downlink!"));
  
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // on EEPROM enabled boards, you can save the current session
  // by calling "saveSession" which allows retrieving the session after reboot or deepsleep
  node.saveSession();

  // wait before sending another packet
  // alternatively, call a deepsleep function here
  // make sure to send the radio to sleep as well using radio.sleep()
  delay(30000);
}
