#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <RadioLib.h>

// the SSDV receiver path does not touch the radio, a null physical layer is enough
class FuzzNullPhy: public PhysicalLayer {
  public:
    Module* getMod() override { return(nullptr); }
};

// feed arbitrary 256-byte packets into the SSDV receiver:
// CRC/RS validation, header parsing and the JPEG reassembly state machine
// must all survive fully untrusted input
extern "C" int FuzzSSDV(const uint8_t* data, size_t size) {
  static FuzzNullPhy* phy = nullptr;
  static SSDVClient* ssdv = nullptr;
  static uint8_t* jpegBuf = nullptr;
  const size_t jpegBufLen = 32768;
  if(!ssdv) {
    phy = new FuzzNullPhy();
    ssdv = new SSDVClient(phy);
    assert(ssdv->begin("TEST") == RADIOLIB_ERR_NONE);
    assert(ssdv->beginDecoder(jpegBuf = new uint8_t[jpegBufLen], jpegBufLen) == RADIOLIB_ERR_NONE);
  }

  // process the input as a stream of packets
  for(size_t off = 0; off + RADIOLIB_SSDV_PACKET_LEN <= size; off += RADIOLIB_SSDV_PACKET_LEN) {
    uint8_t packet[RADIOLIB_SSDV_PACKET_LEN];
    memcpy(packet, &data[off], RADIOLIB_SSDV_PACKET_LEN);

    // CRC check + Reed-Solomon correction must not crash and report sane error counts
    int16_t errors = -1;
    (void)SSDVClient::isValidPacket(packet, &errors);
    if(errors >= 0) {
      // RS(255,223) can never correct more than 16 symbol errors
      assert(errors <= 16);
    }

    // header fields must always be parsed into their valid domain
    SSDVPacketInfo_t info;
    SSDVClient::parseHeader(packet, &info);
    assert(strlen(info.callsign) <= RADIOLIB_SSDV_CALLSIGN_MAX_LEN);
    assert(info.quality <= RADIOLIB_SSDV_QUALITY_MAX);
    assert(info.mcuMode <= 3);

    // feed the packet into the JPEG reassembler
    int16_t res = ssdv->feedPacket(packet);
    if(res == RADIOLIB_SSDV_EOI) {
      uint8_t* jpegPtr = nullptr;
      size_t jpegLen = 0;
      int16_t state = ssdv->getJpeg(&jpegPtr, &jpegLen);
      if(state == RADIOLIB_ERR_NONE) {
        assert(jpegPtr != nullptr);
        // a completed image always carries the EOI marker at the end
        assert(jpegLen >= 2);
        assert(jpegPtr[jpegLen - 2] == 0xFF);
        assert(jpegPtr[jpegLen - 1] == 0xD9);
      }
      assert(ssdv->resetDecoder() == RADIOLIB_ERR_NONE);
    } else if(res < 0) {
      // decode failed - resync so that later packets still exercise the decoder
      assert(ssdv->resetDecoder() == RADIOLIB_ERR_NONE);
    }
  }

  return(0);
}
