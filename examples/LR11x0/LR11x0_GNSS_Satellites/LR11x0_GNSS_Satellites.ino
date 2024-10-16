/*
  RadioLib LR11x0 GNSS Satellites Example

  This example performs GNSS scans and shows the satellites
  currently in view. It is mostly useful to verify
  visibility and antenna setup.

  NOTE: This example will only work for LR11x0 devices
        with sufficiently recent firmware!
        LR1110: 4.1
        LR1120: 2.1
        If your device firmware reports older firmware,
        update it using the LR11x0_Firmware_Update example.

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

// structure to save information about the GNSS scan result
LR11x0GnssResult_t gnssResult;

void setup() {
  Serial.begin(9600);

  // initialize LR1110 with default settings
  Serial.print(F("[LR1110] Initializing ... "));
  int state = radio.beginGNSS(RADIOLIB_LR11X0_GNSS_CONSTELLATION_GPS);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // check the firmware version
  Serial.print(F("[LR1110] Checking firmware version ... "));
  state = radio.isGnssScanCapable();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("check passed!"));
  } else {
    Serial.println(F("check failed, firmware update needed."));
    while (true) { delay(10); }
  }
}

void loop() {
  Serial.print(F("[LR1110] Running GNSS scan ... "));
  int state = radio.gnssScan(&gnssResult);
  if(state != RADIOLIB_ERR_NONE) {
    // some error occurred
    Serial.print(F("failed, code "));
    Serial.print(state);
    Serial.print(F(" (demodulator error "));
    Serial.print(RADIOLIB_GET_GNSS_DEMOD_ERROR(state));
    Serial.println(F(")"));
    
  } else  {
    Serial.println(F("success!"));

    // print the table header
    Serial.print(F("[LR1110] Detected "));
    Serial.print(gnssResult.numSatsDet);
    Serial.println(F(" satellite(s):"));
    Serial.println(F(" # | ID | C/N0 [dB]\t| Doppler [Hz]"));

    // read all results at once
    LR11x0GnssSatellite_t satellites[32];
    state = radio.getGnssSatellites(satellites, gnssResult.numSatsDet);
    if(state != RADIOLIB_ERR_NONE) {
      Serial.print(F("Failed to read results, code "));
      Serial.println(state);
    } else {
      // print all the results
      for(int i = 0; i < gnssResult.numSatsDet; i++) {
        if(i < 10) { Serial.print(" "); } Serial.print(i); Serial.print(" | ");
        Serial.print(satellites[i].svId); Serial.print(" | ");
        Serial.print(satellites[i].c_n0); Serial.print("\t\t| ");
        Serial.println(satellites[i].doppler);

      }
    
    }
    
  }

  // wait for a second before scanning again
  delay(1000);
}
