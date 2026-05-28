/*
  RadioLib SSDV Transmit Example

  The following example sends SSDV picture over LoRa.

  JPEG constraints (enforced by the SSDV encoder):
    • Width and height must be multiples of 16
    • Baseline DCT (not progressive)
    • Greyscale or YCbCr (not RGB / CMYK)

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

#include <RadioLib.h>
#include "image.h"

// SX1278 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// RESET pin: 9
// DIO1 pin:  3
SX1278 radio = new Module(10, 2, 9, 3);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

// SSDV client
// for LoRa we use no-FEC mode (LoRa has its own FEC).
// set the second argument to 'true' for RTTY / FSK links that need RS-FEC.
SSDVClient ssdv(&radio, false);

void setup() {
  Serial.begin(115200);

  Serial.println(F("[SSDVClient] Initialising radio..."));

  // initialise the radio
  ConfigLoRa_t config;
  config.frequency = 434.250;
  config.bandwidth = 62.5;
  int16_t state = radio.begin(config);

  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Radio init failed: "));
    Serial.println(state);
    while(true) { delay(1000); }
  }
  Serial.println(F("Radio OK"));

  // initialise SSDVClient
  state = ssdv.begin("MYCALL");   // your callsign, ≤ 6 chars, A-Z 0-9 -/.
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("ssdv.begin() failed: "));
    Serial.println(state);
    while(true) { delay(1000); }
  }

  // load and encode the image
  // quality 0-7; 4 = JPEG quality 50 (good balance of size vs. detail)
  state = ssdv.setImage(testJpeg, testJpegLen, 4);
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("setImage() failed: "));
    Serial.println(state);

    while(true) { delay(1000); }
  }

  Serial.print(F("Image encoded. Packets: "));
  Serial.println(ssdv.numPackets());
}

void loop() {
  if(ssdv.isDone()) {
    // image fully transmitted; in a real tracker you would capture a new
    // frame here, or alternate SSDV with other telemetry strings.
    Serial.println(F("All packets sent. Stopping..."));
    ssdv.clearImage();      // or load a new image
    while(1) {
      yield();
    }
  }

  uint16_t packetId = 0;

  // use transmitLoRa() to send 255 bytes (skips 0x55 sync byte).
  // use transmit() instead for RTTY/FSK where the sync byte is needed.
  int16_t result = ssdv.transmitLoRa(&packetId);

  if(result < 0) {
    Serial.print(F("Radio error: "));
    Serial.println(result);

    // simple retry: do nothing and the next loop iteration will retry.
  } else {
    Serial.print(F("Sent packet "));
    Serial.print(packetId);
    Serial.print(F(" / "));
    Serial.print(ssdv.numPackets() - 1);
    Serial.print(F("  ("));
    Serial.print(result);
    Serial.println(F(" sent so far)"));
  }

  delay(2000);  // be aware of dutycycle limits
}