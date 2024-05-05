/*
  RadioLib LR11x0 WiFi scan Blocking Example

  This example performs a passive scan of WiFi networks.
  The scan shows basic information about the networks,
  such as the frequency, country code and SSID.

  Other modules from LR11x0 family can also be used.

  Using blocking scan is not recommended, as depending
  on the scan settings, the program may be blocked
  for several seconds! Instead, interrupt scan is recommended.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#lr11x0---wifi-scan

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// LR1110 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
LR1110 radio = new Module(10, 2, 3, 9);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//LR1110 radio = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize LR1110 with default settings
  Serial.print(F("[LR1110] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop() {
  Serial.print(F("[LR1110] Running WiFi scan ... "));

  // scan all WiFi signals with default scan configuration
  uint8_t count = 0;
  int state = radio.wifiScan('*', &count);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));

    // print the table header
    Serial.print(F("[LR1110] Reading "));
    Serial.print(count);
    Serial.println(F(" scan results:"));
    Serial.println(F(" # | WiFi type\t| Frequency\t| MAC Address\t    | Country\t| RSSI [dBm]\t| SSID"));

    // read all results one by one
    // this result type contains the most information, including the SSID
    LR11x0WifiResultExtended_t result;
    for(int i = 0; i < count; i++) {
      if(i < 10) { Serial.print(" "); } Serial.print(i); Serial.print(" | ");
      state = radio.getWifiScanResult(&result, i);
      if(state != RADIOLIB_ERR_NONE) {
        Serial.print(F("Failed to read result, code "));
        Serial.println(state);
        continue;
      }

      // print the basic information
      Serial.print(F("802.11")); Serial.print(result.type); Serial.print("\t| ");
      Serial.print(result.channelFreq); Serial.print(" MHz\t| ");

      // print MAC address
      for(int j = 0; j < 6; j++) {
        if(result.mac[j] < 0x10) { Serial.print("0"); }
        Serial.print(result.mac[j], HEX);
        if(j < 5) { Serial.print(":"); }
      }
      Serial.print(" | ");

      // print the two-letter country code
      String country = result.countryCode;
      Serial.print(country);
      Serial.print("  \t| ");

      // print the RSSI
      Serial.print(result.rssi);
      Serial.print("\t| ");
      
      // print the network SSID
      Serial.println((char*)result.ssid);
      
    }

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);

  }

  // wait for a second before scanning again
  delay(1000);
}
