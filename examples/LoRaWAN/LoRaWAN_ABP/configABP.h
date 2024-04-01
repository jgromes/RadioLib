#ifndef _CONFIG_H
#define _CONFIG_H

#include <RadioLib.h>

// how often to send an uplink - consider legal & FUP constraints - see notes
const uint32_t uplinkIntervalSeconds = 5UL * 60UL;    // minutes x seconds

// device address - either a development address or one assigned
// to the LoRaWAN Service Provider - TTN will generate one for you
#ifndef RADIOLIB_LORAWAN_DEV_ADDR   // Replace with your DevAddr
#define RADIOLIB_LORAWAN_DEV_ADDR   0x------
#endif

#ifndef RADIOLIB_LORAWAN_NWKS_KEY   // Replace with your NwkS Key 
#define RADIOLIB_LORAWAN_NWKS_KEY   0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x-- 
#endif
#ifndef RADIOLIB_LORAWAN_SNWKSINT_KEY   // Replace with your SNwkSInt Key 
#define RADIOLIB_LORAWAN_SNWKSINT_KEY   0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x-- 
#endif
#ifndef RADIOLIB_LORAWAN_NWKSENC_KEY   // Replace with your NwkSEnc Key 
#define RADIOLIB_LORAWAN_NWKSENC_KEY   0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x-- 
#endif
#ifndef RADIOLIB_LORAWAN_APPS_KEY   // Replace with your AppS Key 
#define RADIOLIB_LORAWAN_APPS_KEY   0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x-- 
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
  #pragma message ("TTGO LoRa32 v1 - no Display")
  SX1276 radio = new Module(18, 26, 14, 33);

#elif defined(ARDUINO_TTGO_LORA32_V2)
   #pragma message ("ARDUINO_TTGO_LORA32_V2 + Display")
   SX1276 radio = new Module(18, 26, 12, RADIOLIB_NC);

#elif defined(ARDUINO_TTGO_LoRa32_v21new) // T3_V1.6.1
  #pragma message ("Using TTGO LoRa32 v2.1 marked T3_V1.6.1 + Display")
  SX1276 radio = new Module(18, 26, 14, 33);

#elif defined(ARDUINO_TBEAM_USE_RADIO_SX1262)
  #pragma error ("ARDUINO_TBEAM_USE_RADIO_SX1262 awaiting pin map")

#elif defined(ARDUINO_TBEAM_USE_RADIO_SX1276)
  #pragma message ("Using TTGO LoRa32 v2.1 marked T3_V1.6.1 + Display")
  SX1276 radio = new Module(18, 26, 23, 33);


// Heltec
#elif defined(ARDUINO_HELTEC_WIFI_LORA_32)
  #pragma error ("ARDUINO_HELTEC_WIFI_LORA_32 awaiting pin map")

#elif defined (ARDUINO_heltec_wireless_stick)
  #pragma message ("Using Heltec Wireless Stick")
  SX1278 radio = new Module(14, 4, 12, 16);

#elif defined(ARDUINO_heltec_wifi_lora_32_V2)
  #pragma message ("Using Heltec WiFi LoRa32 v2")
  SX1278 radio = new Module(14, 4, 12, 16);

#elif defined(ARDUINO_heltec_wifi_kit_32_V2)
  #pragma message ("ARDUINO_heltec_wifi_kit_32_V2 awaiting pin map")
  SX1276 radio = new Module(18, 26, 14, 35);

#elif defined(ARDUINO_heltec_wifi_kit_32_V3)
  #pragma message ("Using Heltec WiFi LoRa32 v3 - Display + USB-C")
  SX1262 radio = new Module(8, 14, 12, 13);

#elif defined(ARDUINO_CUBECELL_BOARD)
  #pragma message ("Using TTGO LoRa32 v2.1 marked T3_V1.6.1 + Display")
  SX1262 radio = new Module(RADIOLIB_BUILTIN_MODULE);

#elif defined(ARDUINO_CUBECELL_BOARD_V2)
  #pragma error ("ARDUINO_CUBECELL_BOARD_V2 awaiting pin map")


#else
  #pragma message ("Unknown board - no automagic pinmap available")

  // SX1262  pin order: Module(NSS/CS, DIO1, RESET, BUSY);
  // SX1262 radio = new Module(8, 14, 12, 13);

  // SX1278 pin order: Module(NSS/CS, DIO0, RESET, DIO1);
  // SX1278 radio = new Module(10, 2, 9, 3);

#endif

// copy over the keys in to the something that will not compile if incorrectly formatted
uint32_t devAddr =        RADIOLIB_LORAWAN_DEV_ADDR;
uint8_t NwkSKey[] =     { RADIOLIB_LORAWAN_NWKS_KEY };
uint8_t SNwkSIntKey[] = { RADIOLIB_LORAWAN_SNWKSINT_KEY }; // Previously sNwkSIntKey
uint8_t NwkSEncKey[] =  { RADIOLIB_LORAWAN_NWKSENC_KEY }; // Previously fNwkSIntKey
uint8_t AppSKey[] =     { RADIOLIB_LORAWAN_APPS_KEY };

// create the LoRaWAN node
LoRaWANNode node(&radio, &Region, subBand);

// helper function to display any issues
void debug(bool isFail, const __FlashStringHelper* message, int state, bool Freeze) {
  if (isFail) {
    Serial.print(message);
    Serial.print("(");
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
