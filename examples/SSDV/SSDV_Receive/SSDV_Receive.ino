/*
  RadioLib SSDV Receive Example

  This example receives SSDV image packets over LoRa, validates each
  one, and prints the decoded header fields to the serial console.

  Optionally — by defining ENABLE_JPEG_RECONSTRUCT below — packets are
  fed into the SSDV decoder to reconstruct a complete JPEG image in RAM.
  When the last packet arrives (EOI), the JPEG length is printed and the
  raw bytes are hex-dumped.

  The radio settings (frequency, bandwidth, spreading factor, coding rate)
  MUST match those used in the SSDV_Transmit example for a successful
  over-the-air link.

  JPEG reconstruction requirements:
    • The userJpegBuf[] array below must be large enough to hold the
      reconstructed JPEG.  Enlarge JPEG_BUF_SIZE for bigger images.
    • Sufficient free heap is needed for the ~2 KB ssdv_t decoder state.
    • Only practical on platforms with ≥ 30 KB free SRAM
      (ESP32, RP2040, STM32, etc.).  Comment out ENABLE_JPEG_RECONSTRUCT
      on AVR Arduinos.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

#include <RadioLib.h>

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

// create SSDV client
// for LoRa we use no-FEC mode (LoRa has its own FEC).
// if you are receiving packets sent in Normal (FEC) mode, change to 'true'.
SSDVClient ssdv(&radio, false);

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

// ── Optional JPEG reconstruction
// uncomment the next line to enable in-RAM JPEG reconstruction.
// see the header comment for RAM requirements.
#define ENABLE_JPEG_RECONSTRUCT

#ifdef ENABLE_JPEG_RECONSTRUCT
// uutput buffer for the reconstructed JPEG.
// 20 KB is sufficient for a 160×120 image at SSDV quality 4.
// increase for larger images (e.g. 60 KB for 320×240).
static const size_t JPEG_BUF_SIZE = 70 * 1024UL;
static uint8_t userJpegBuf[JPEG_BUF_SIZE];

// track the image ID being decoded so we can detect a new image starting.
static int16_t currentImageId = -1;
#endif

// image statistics
static uint16_t packetsReceived  = 0;       // valid packets for this image.
static uint16_t packetsWithErrors = 0;      // packets that needed RS correction.
static uint16_t packetsMissing   = 0;       // gap count between expected/actual IDs.
static uint16_t lastPacketId     = 0xFFFF;  // previous packet_id seen (0xFFFF = none).

void setup() {
  Serial.begin(115200);

  Serial.println(F("[SSDV] Initialising radio..."));

  // radio
  ConfigLoRa_t config;
  config.frequency      = 868.250;
  config.bandwidth      = 62.5;
  int16_t state = radio.begin(config);
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Radio init failed: "));
    Serial.println(state);
    while(1) {
      yield();
    }
  }
  Serial.println(F("Radio OK"));

  // attach the receive interrupt
  radio.setPacketReceivedAction(setFlag);
  // startReceive() puts the radio into continuous receive mode.
  state = radio.startReceive();
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("startReceive() failed: "));
    Serial.println(state);
    while(1) {
      yield();
    }
  }

  // optionally initialise JPEG decoder
#ifdef ENABLE_JPEG_RECONSTRUCT
  state = ssdv.beginDecoder(userJpegBuf, JPEG_BUF_SIZE);
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("beginDecoder() failed (not enough RAM?): "));
    Serial.println(state);
    while(1) {
      yield();
    }
  }
  Serial.println(F("JPEG decoder initialised."));
#endif

  Serial.println(F("Listening for SSDV packets..."));
  Serial.println();
}

void loop() {

  // check if the flag is set
  if(!receivedFlag) {
    return;
  }
  Serial.println(F("Received a packet"));
  receivedFlag = false;

  // read the packet
  uint8_t packet[256];
  int16_t state = ssdv.receiveLoRa(packet);
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("  [RX] Radio error: "));
    Serial.println(state);
    // Restart listening.
    radio.startReceive();
    return;
  }

  // validate the packet
  int rsErrors = 0;
  state = SSDVClient::isValidPacket(packet, &rsErrors);
  if(state != RADIOLIB_ERR_NONE) {
    Serial.println(F("  [RX] Invalid packet — CRC/RS check failed, discarding."));
    radio.startReceive();
    return;
  }

  if(rsErrors > 0) {
    packetsWithErrors++;
    Serial.print(F("  [RS] Corrected "));
    Serial.print(rsErrors);
    Serial.println(F(" symbol error(s)."));
  }

  // parse the header
  ssdv_packet_info_t info;
  SSDVClient::parseHeader(packet, &info);

  packetsReceived++;

  // check for missed packets by comparing with the previous packet_id.
  if(lastPacketId != 0xFFFF && info.packet_id > (uint16_t)(lastPacketId + 1)) {
    uint16_t gap = info.packet_id - lastPacketId - 1;
    packetsMissing += gap;
    Serial.print(F("  [!] Gap detected: "));
    Serial.print(gap);
    Serial.println(F(" packet(s) missing."));
  }
  lastPacketId = info.packet_id;

  // print packet info
  Serial.print(F("  Callsign : "));
  Serial.println(info.callsign_s);

  Serial.print(F("  Image ID : "));
  Serial.println(info.image_id, HEX);

  Serial.print(F("  Packet   : "));
  Serial.print(info.packet_id);
  Serial.print(F("  ("));
  Serial.print(info.width);
  Serial.print(F("x"));
  Serial.print(info.height);
  Serial.print(F("  Q"));
  Serial.print(info.quality);
  Serial.print(F("  MCU "));
  if(info.mcu_id == 0xFFFF) {
    Serial.print(F("none"));
  } else {
    Serial.print(info.mcu_id);
    Serial.print(F("/"));
    Serial.print(info.mcu_count - 1);
  }
  if(info.eoi) {
    Serial.print(F("  EOI"));
  }
  Serial.println();

  Serial.print(F("  Type     : "));
  Serial.println(info.type == SSDV_TYPE_NORMAL ? F("Normal (FEC)") : F("No-FEC"));

  // print radio link quality metrics
  float rssi = radio.getRSSI();
  float snr  = radio.getSNR();
  Serial.print(F("  RSSI     : "));
  Serial.print(rssi);
  Serial.print(F(" dBm   SNR: "));
  Serial.print(snr);
  Serial.println(F(" dB"));

  Serial.println();

  // optional JPEG reconstruction
#ifdef ENABLE_JPEG_RECONSTRUCT

  // if the image ID changed, a new image has started; reset the decoder.
  if(currentImageId != -1 && info.image_id != (uint8_t)currentImageId) {
    Serial.print(F("[JPEG] New image detected (ID 0x"));
    Serial.print(info.image_id, HEX);
    Serial.println(F("). Resetting decoder."));
    ssdv.resetDecoder();
    packetsReceived   = 1;   // this packet is the first of the new image
    packetsWithErrors = (rsErrors > 0) ? 1 : 0;
    packetsMissing    = 0;
    lastPacketId      = info.packet_id;
  }
  currentImageId = info.image_id;

  state = ssdv.feedPacket(packet);

  if(state == RADIOLIB_SSDV_EOI) {
    // iImage complete
    Serial.println(F("[JPEG] Image complete!"));
    Serial.print(F("  Packets received : "));
    Serial.println(packetsReceived);
    Serial.print(F("  RS corrections   : "));
    Serial.println(packetsWithErrors);
    Serial.print(F("  Packets missing  : "));
    Serial.println(packetsMissing);

    uint8_t* jpegPtr = nullptr;
    size_t   jpegLen = 0;
    state = ssdv.getJpeg(&jpegPtr, &jpegLen);

    if(state == RADIOLIB_ERR_NONE) {
      Serial.print(F("  JPEG size        : "));
      Serial.print(jpegLen);
      Serial.println(F(" bytes"));

      // hex-dump the JPEG to Serial
      // A terminal / PC script can capture this output and save it to a
      // .jpg file.  Alternatively, write jpegPtr/jpegLen to SD card here.
      Serial.println(F("--- BEGIN JPEG HEX DUMP ---"));
      for(size_t i = 0; i < jpegLen; i++) {
        if(jpegPtr[i] < 0x10) { Serial.print('0'); }
        Serial.print(jpegPtr[i], HEX);
        if((i % 32) == 31 || i == jpegLen - 1) {
          Serial.println();
        } else {
          Serial.print(' ');
        }
      }
      Serial.println(F("--- END JPEG HEX DUMP ---"));
    } else {
      Serial.print(F("[JPEG] getJpeg() error: "));
      Serial.println(state);
    }

    // reset decoder and statistics for the next image.
    ssdv.resetDecoder();
    packetsReceived   = 0;
    packetsWithErrors = 0;
    packetsMissing    = 0;
    lastPacketId      = 0xFFFF;
    currentImageId    = -1;

  } else if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[JPEG] feedPacket() error: "));
    Serial.println(state);
  }

#endif // ENABLE_JPEG_RECONSTRUCT

  // restart listening
  radio.startReceive();
}
