/*
  RadioLib LoRaWAN End Device Example

  This example joins a LoRaWAN network and will send
  uplink packets. Before you start, you will have to
  register your device at https://www.thethingsnetwork.org/
  After your device is registered, you can run this example.
  The device will join the network and start uploading data.

  NOTE: LoRaWAN requires storing some parameters persistently!
        RadioLib does this by using EEPROM, by default
        starting at address 0 and using 32 bytes.
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
  //node.wipe();

  // application identifier - in LoRaWAN 1.1, it is also called joinEUI
  // when adding new end device in TTN, you will have to enter this number
  // you can pick any number you want, but it has to be unique
  uint64_t appEUI = 0x12AD1011B0C0FFEE;

  // device identifier - this number can be anything
  // when adding new end device in TTN, you can generate this number,
  // or you can set any value you want, provided it is also unique
  uint64_t devEUI = 0x70B3D57ED005E120;

  // select some encryption keys which will be used to secure the communication
  // there are two of them - network key and application key
  // because LoRaWAN uses AES-128, the key MUST be 16 bytes (or characters) long
  const char nwkKey[] = "topSecretKey1234";
  const char appKey[] = "aDifferentKeyABC";

  // now we can start the activation
  // this can take up to 20 seconds, and requires a LoRaWAN gateway in range
  Serial.print(F("[LoRaWAN] Attempting over-the-air activation ... "));
  state = node.beginOTAA(appEUI, devEUI, (uint8_t*)nwkKey, (uint8_t*)appKey);
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  // after the device has been activated,
  // network can be rejoined after device power cycle
  // by calling "begin"
  /*
    Serial.print(F("[LoRaWAN] Resuming previous session ... "));
    state = node.begin();
    if(state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while(true);
    }
  */
}

// counter to keep track of transmitted packets
int count = 0;

void loop() {
  // send uplink to port 10
  Serial.print(F("[LoRaWAN] Sending uplink packet ... "));
  String str = "Hello World! #" + String(count++);
  int state = node.uplink(str, 10);
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // wait before sending another one
  delay(10000);
}
