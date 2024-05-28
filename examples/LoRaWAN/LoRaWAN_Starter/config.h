#ifndef _CONFIG_H
#define _CONFIG_H

#include <RadioLib.h>

// how often to send an uplink - consider legal & FUP constraints - see notes
const uint32_t uplinkIntervalSeconds = 5UL * 60UL;    // minutes x seconds

// joinEUI - previous versions of LoRaWAN called this AppEUI
// for development purposes you can use all zeros - see wiki for details
#define RADIOLIB_LORAWAN_JOIN_EUI  0x0000000000000000

// the Device EUI & two keys can be generated on the TTN console 
#ifndef RADIOLIB_LORAWAN_DEV_EUI   // Replace with your Device EUI
#define RADIOLIB_LORAWAN_DEV_EUI   0x---------------
#endif
#ifndef RADIOLIB_LORAWAN_APP_KEY   // Replace with your App Key 
#define RADIOLIB_LORAWAN_APP_KEY   0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x-- 
#endif
#ifndef RADIOLIB_LORAWAN_NWK_KEY   // Put your Nwk Key here
#define RADIOLIB_LORAWAN_NWK_KEY   0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x-- 
#endif

// for the curious, the #ifndef blocks allow for automated testing &/or you can
// put your EUI & keys in to your platformio.ini - see wiki for more tips

// regional choices: EU868, US915, AU915, AS923, IN865, KR920, CN780, CN500
const LoRaWANBand_t Region = EU868;
const uint8_t subBand = 0;  // For US915, change this to 2, otherwise leave on 0

// ============================================================================
// Below is to support the sketch - only make changes if the notes say so ...

// Auto select MCU <-> radio connections
// If you get an error message when compiling, it may be that the 
// pinmap could not be determined - see the notes for more info

// Adafruit
#if defined(ARDUINO_SAMD_FEATHER_M0)
    #pragma message ("Adafruit Feather M0 with RFM95")
    #pragma message ("Link required on board")
    SX1276 radio = new Module(8, 3, 4, 6);


// LilyGo 
#elif defined(ARDUINO_TTGO_LORA32_V1)
  #pragma message ("Using TTGO LoRa32 v1 - no Display")
  SX1276 radio = new Module(18, 26, 14, 33);

#elif defined(ARDUINO_TTGO_LORA32_V2)
   #pragma message ("Using TTGO LoRa32 v2 + Display")
   SX1276 radio = new Module(18, 26, 12, RADIOLIB_NC);

#elif defined(ARDUINO_TTGO_LoRa32_v21new) // T3_V1.6.1
  #pragma message ("Using TTGO LoRa32 v2.1 marked T3_V1.6.1 + Display")
  SX1276 radio = new Module(18, 26, 14, 33);

#elif defined(ARDUINO_TBEAM_USE_RADIO_SX1262)
  #pragma error ("ARDUINO_TBEAM_USE_RADIO_SX1262 awaiting pin map")

#elif defined(ARDUINO_TBEAM_USE_RADIO_SX1276)
  #pragma message ("Using TTGO T-Beam")
  SX1276 radio = new Module(18, 26, 23, 33);


// HelTec: https://github.com/espressif/arduino-esp32/blob/master/variants/heltec_*/pins_arduino.h
#elif defined(ARDUINO_HELTEC_WIFI_LORA_32)
  #pragma message ("Using Heltec WiFi LoRa32")
  SX1276 radio = new Module(18, 26, 14, 33);

#elif defined(ARDUINO_heltec_wifi_lora_32_V2)
  #pragma message ("Using Heltec WiFi LoRa32 v2")
  SX1278 radio = new Module(14, 4, 12, 16);

// Pending verfication of which radio is shipped
// #elif defined(ARDUINO_heltec_wifi_lora_32_V2)
//   #pragma message ("ARDUINO_heltec_wifi_kit_32_V2 awaiting pin map")
//   SX1276 radio = new Module(18, 26, 14, 35);

#elif defined(ARDUINO_heltec_wifi_lora_32_V3)
  #pragma message ("Using Heltec WiFi LoRa32 v3 - Display + USB-C")
  SX1262 radio = new Module(8, 14, 12, 13);
  

// Following not verified  
#elif defined (ARDUINO_heltec_wireless_stick)
  #pragma message ("Using Heltec Wireless Stick")
  SX1278 radio = new Module(14, 4, 12, 16);
  
#elif defined (ARDUINO_HELTEC_WIRELESS_STICK)
  #pragma message ("Using Heltec Wireless Stick")
  SX1276 radio = new Module(18, 26, 14, 35);

#elif defined (ARDUINO_HELTEC_WIRELESS_STICK_V3)
  #pragma message ("Using Heltec Wireless Stick v3")
  SX1262 radio = new Module(8, 14, 12, 13);

#elif defined (ARDUINO_HELTEC_WIRELESS_STICK_LITE)
  #pragma message ("Using Heltec Wireless Stick Lite")
  SX1276 radio = new Module(18, 26, 14, 35);

#elif defined (ARDUINO_HELTEC_WIRELESS_STICK_LITE_V3)
  #pragma message ("Using Heltec Wireless Stick Lite v3")
  SX1262 radio = new Module(34, 14, 12, 13);


// If we don't recognise the board
#else
  #pragma message ("Unknown board - no automagic pinmap available")

  // SX1262  pin order: Module(NSS/CS, DIO1, RESET, BUSY);
  // SX1262 radio = new Module(8, 14, 12, 13);

  // SX1278 pin order: Module(NSS/CS, DIO0, RESET, DIO1);
  // SX1278 radio = new Module(10, 2, 9, 3);
  
  // For Pi Pico + Waveshare HAT - work in progress
  // SX1262 radio = new Module(3, 20, 15, 2, SPI1, RADIOLIB_DEFAULT_SPI_SETTINGS);

#endif

// copy over the EUI's & keys in to the something that will not compile if incorrectly formatted
uint64_t joinEUI =   RADIOLIB_LORAWAN_JOIN_EUI;
uint64_t devEUI  =   RADIOLIB_LORAWAN_DEV_EUI;
uint8_t appKey[] = { RADIOLIB_LORAWAN_APP_KEY };
uint8_t nwkKey[] = { RADIOLIB_LORAWAN_NWK_KEY };

// create the LoRaWAN node
LoRaWANNode node(&radio, &Region, subBand);


// result code to text ...
String stateDecode(const int16_t result) {
  switch (result) {
  case RADIOLIB_ERR_NONE:
    return "ERR_NONE";
  case RADIOLIB_ERR_CHIP_NOT_FOUND:
    return "ERR_CHIP_NOT_FOUND";
  case RADIOLIB_ERR_PACKET_TOO_LONG:
    return "ERR_PACKET_TOO_LONG";
  case RADIOLIB_ERR_RX_TIMEOUT:
    return "ERR_RX_TIMEOUT";
  case RADIOLIB_ERR_CRC_MISMATCH:
    return "ERR_CRC_MISMATCH";
  case RADIOLIB_ERR_INVALID_BANDWIDTH:
    return "ERR_INVALID_BANDWIDTH";
  case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
    return "ERR_INVALID_SPREADING_FACTOR";
  case RADIOLIB_ERR_INVALID_CODING_RATE:
    return "ERR_INVALID_CODING_RATE";
  case RADIOLIB_ERR_INVALID_FREQUENCY:
    return "ERR_INVALID_FREQUENCY";
  case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
    return "ERR_INVALID_OUTPUT_POWER";
  case RADIOLIB_ERR_NETWORK_NOT_JOINED:
	  return "RADIOLIB_ERR_NETWORK_NOT_JOINED";

  case RADIOLIB_ERR_DOWNLINK_MALFORMED:
    return "RADIOLIB_ERR_DOWNLINK_MALFORMED";
  case RADIOLIB_ERR_INVALID_REVISION:
    return "RADIOLIB_ERR_INVALID_REVISION";
  case RADIOLIB_ERR_INVALID_PORT:
    return "RADIOLIB_ERR_INVALID_PORT";
  case RADIOLIB_ERR_NO_RX_WINDOW:
    return "RADIOLIB_ERR_NO_RX_WINDOW";
  case RADIOLIB_ERR_INVALID_CID:
    return "RADIOLIB_ERR_INVALID_CID";
  case RADIOLIB_ERR_UPLINK_UNAVAILABLE:
    return "RADIOLIB_ERR_UPLINK_UNAVAILABLE";
  case RADIOLIB_ERR_COMMAND_QUEUE_FULL:
    return "RADIOLIB_ERR_COMMAND_QUEUE_FULL";
  case RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND:
    return "RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND";
  case RADIOLIB_ERR_JOIN_NONCE_INVALID:
    return "RADIOLIB_ERR_JOIN_NONCE_INVALID";
  case RADIOLIB_ERR_N_FCNT_DOWN_INVALID:
    return "RADIOLIB_ERR_N_FCNT_DOWN_INVALID";
  case RADIOLIB_ERR_A_FCNT_DOWN_INVALID:
    return "RADIOLIB_ERR_A_FCNT_DOWN_INVALID";
  case RADIOLIB_ERR_DWELL_TIME_EXCEEDED:
    return "RADIOLIB_ERR_DWELL_TIME_EXCEEDED";
  case RADIOLIB_ERR_CHECKSUM_MISMATCH:
    return "RADIOLIB_ERR_CHECKSUM_MISMATCH";
  case RADIOLIB_LORAWAN_NO_DOWNLINK:
    return "RADIOLIB_LORAWAN_NO_DOWNLINK";
  case RADIOLIB_LORAWAN_SESSION_RESTORED:
    return "RADIOLIB_LORAWAN_SESSION_RESTORED";
  case RADIOLIB_LORAWAN_NEW_SESSION:
    return "RADIOLIB_LORAWAN_NEW_SESSION";
  case RADIOLIB_LORAWAN_NONCES_DISCARDED:
    return "RADIOLIB_LORAWAN_NONCES_DISCARDED";
  case RADIOLIB_LORAWAN_SESSION_DISCARDED:
    return "RADIOLIB_LORAWAN_SESSION_DISCARDED";
  }
  return "See TypeDef.h";
}

// helper function to display any issues
void debug(bool isFail, const __FlashStringHelper* message, int state, bool Freeze) {
  if (isFail) {
    Serial.print(message);
    Serial.print(" - ");
    Serial.print(stateDecode(state));
    Serial.print(" (");
    Serial.print(state);
    Serial.println(")");
    while (Freeze);
  }
}


// helper function to display a byte array
void arrayDump(uint8_t *buffer, uint16_t len) {
  for(uint16_t c = 0; c < len; c++) {
    char b = buffer[c];
    if(b < 0x10) { Serial.print('0'); }
    Serial.print(b, HEX);
  }
  Serial.println();
}


#endif
