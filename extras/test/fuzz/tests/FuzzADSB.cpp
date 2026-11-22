#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>

#include <RadioLib.h>

// ADS-B decoding is pure bit extraction, a null physical layer is enough
class FuzzNullPhy: public PhysicalLayer {
  public:
    Module* getMod() override { return(nullptr); }
};

// decode arbitrary 14-byte ADS-B extended squitter frames and run all parsers
// on the result - every output must stay within its domain
extern "C" int FuzzADSB(const uint8_t* data, size_t size) {
  static FuzzNullPhy* phy = nullptr;
  static ADSBClient* adsb = nullptr;
  if(!adsb) {
    phy = new FuzzNullPhy();
    adsb = new ADSBClient(phy);
    adsb->setReferencePosition(0.0f, 0.0f);
  }

  for(size_t off = 0; off + RADIOLIB_ADSB_FRAME_LEN_BYTES <= size; off += RADIOLIB_ADSB_FRAME_LEN_BYTES) {
    const uint8_t* in = &data[off];
    ADSBFrame frame;
    assert(adsb->decode(in, &frame) == RADIOLIB_ERR_NONE);

    // the basic fields are plain bit extractions
    assert(frame.downlinkFormat == (in[0] >> 3));
    assert(frame.capability == (in[0] & 0x07));
    for(int i = 0; i < RADIOLIB_ADSB_FRAME_ICAO_LEN_BYTES; i++) {
      assert(frame.icao[i] == in[RADIOLIB_ADSB_FRAME_ICAO_POS + i]);
    }
    // the message type lookup must never read out of the 32-entry table
    int type = static_cast<int>(frame.messageType);
    assert(type >= 0 && type <= 8);

    // ICAO address is rendered as 6 upper-case hex characters
    char id[RADIOLIB_ADSB_HEX_ID_LEN];
    assert(adsb->parseHexId(&frame, id) == RADIOLIB_ERR_NONE);
    assert(strlen(id) == 6);
    for(int i = 0; i < 6; i++) {
      assert(isxdigit(id[i]));
    }

    // callsign is only available on identification messages
    char callsign[RADIOLIB_ADSB_CALLSIGN_LEN];
    ADSBAircraftCategory cat = ADSBAircraftCategory::NONE;
    int16_t state = adsb->parseCallsign(&frame, callsign, &cat);
    if(frame.messageType != ADSBMessageType::AIRCRAFT_ID) {
      assert(state == RADIOLIB_ERR_ADSB_INVALID_MSG_TYPE);
    } else {
      assert((state == RADIOLIB_ERR_NONE) || (state == RADIOLIB_ERR_ADSB_INVALID_CATEGORY));
      assert(strlen(callsign) <= RADIOLIB_ADSB_CALLSIGN_LEN - 1);
    }

    // position is only available on airborne position messages
    int alt = 0;
    float lat = 0.0f, lon = 0.0f;
    bool altGnss = false;
    state = adsb->parseAirbornePosition(&frame, &alt, &lat, &lon, &altGnss);
    if((frame.messageType == ADSBMessageType::AIRBORNE_POS_ALT_BARO) ||
       (frame.messageType == ADSBMessageType::AIRBORNE_POS_ALT_GNSS)) {
      assert(state == RADIOLIB_ERR_NONE);
      // the 12-bit altitude field is bounded, and the CPR solution around the
      // (0, 0) reference point must be a finite number
      assert(alt > -2000 && alt < 410000);
      assert(isfinite(lat) && fabsf(lat) < 1000.0f);
      assert(isfinite(lon) && fabsf(lon) < 1000.0f);
      assert(altGnss == (frame.messageType == ADSBMessageType::AIRBORNE_POS_ALT_GNSS));
    } else {
      assert(state == RADIOLIB_ERR_ADSB_INVALID_MSG_TYPE);
    }
  }

  return(0);
}
