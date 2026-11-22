// boost test header
#include <boost/test/unit_test.hpp>

#include <cmath>
#include <cstring>
#include <string>

// ADS-B header
#include "protocols/ADSB/ADSB.h"

// ADS-B decoding is pure bit extraction, a null physical layer is enough
class AdsBTestNullPhy: public PhysicalLayer {
  public:
    Module* getMod() override { return(nullptr); }
};

// run decode() over a frame carrying the given message
static ADSBFrame makeFrame(ADSBClient* adsb, uint8_t i0, uint8_t i1, uint8_t i2, const uint8_t msg[7]) {
  uint8_t in[RADIOLIB_ADSB_FRAME_LEN_BYTES] = { 0 };
  in[0] = (17 << 3) | 5;
  in[1] = i0;
  in[2] = i1;
  in[3] = i2;
  memcpy(&in[RADIOLIB_ADSB_FRAME_MESSAGE_POS], msg, RADIOLIB_ADSB_FRAME_MESSAGE_LEN_BYTES);
  ADSBFrame frame;
  BOOST_CHECK_EQUAL(adsb->decode(in, &frame), RADIOLIB_ERR_NONE);
  return(frame);
}

BOOST_AUTO_TEST_SUITE(suite_ADSB)

BOOST_AUTO_TEST_CASE(ADSB_Decode) {
  BOOST_TEST_MESSAGE("--- Test ADS-B frame decoding ---");
  AdsBTestNullPhy phy;
  ADSBClient adsb(&phy);

  // a real-world DF17 identification frame
  uint8_t in[] = { 0x8D, 0x48, 0x40, 0xD6, 0x23, 0x88, 0x40, 0xC0, 0xC4, 0x1E, 0x64, 0x12, 0x5C, 0x43 };
  ADSBFrame frame;
  BOOST_CHECK_EQUAL(adsb.decode(in, &frame), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(frame.downlinkFormat, 17);
  BOOST_CHECK_EQUAL(frame.capability, 5);
  BOOST_CHECK_EQUAL_COLLECTIONS(frame.icao, frame.icao + 3, in + 1, in + 4);
  BOOST_CHECK(frame.messageType == ADSBMessageType::AIRCRAFT_ID);
  BOOST_CHECK_EQUAL_COLLECTIONS(frame.message, frame.message + 7, in + 4, in + 11);

  // the message type lookup must cover every type code 0..31
  for(uint8_t tc = 0; tc < 32; tc++) {
    uint8_t msg[RADIOLIB_ADSB_FRAME_MESSAGE_LEN_BYTES] = { 0 };
    msg[0] = tc << 3;
    memcpy(&in[4], msg, 7);
    BOOST_CHECK_EQUAL(adsb.decode(in, &frame), RADIOLIB_ERR_NONE);
    BOOST_CHECK(static_cast<int>(frame.messageType) >= 0 && static_cast<int>(frame.messageType) <= 8);
  }
}

BOOST_AUTO_TEST_CASE(ADSB_ParseHexId) {
  BOOST_TEST_MESSAGE("--- Test ADS-B ICAO hex ID parsing ---");
  AdsBTestNullPhy phy;
  ADSBClient adsb(&phy);

  const uint8_t in[] = { 0x8D, 0x48, 0x40, 0xD6, 0x23, 0x88, 0x40, 0xC0, 0xC4, 0x1E, 0x64, 0x12, 0x5C, 0x43 };
  ADSBFrame frame;
  BOOST_CHECK_EQUAL(adsb.decode(in, &frame), RADIOLIB_ERR_NONE);

  char id[RADIOLIB_ADSB_HEX_ID_LEN];
  BOOST_CHECK_EQUAL(adsb.parseHexId(&frame, id), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(std::string(id), std::string("4840D6"));
}

BOOST_AUTO_TEST_CASE(ADSB_ParseCallsign) {
  BOOST_TEST_MESSAGE("--- Test ADS-B callsign and category parsing ---");
  AdsBTestNullPhy phy;
  ADSBClient adsb(&phy);

  // identification message carrying "RADIO 1" in the 6-bit ICAO character set
  // (A-Z = 1..26, space = 32, digits = ASCII values)
  uint8_t msg[7] = { 0x21, 0x48, 0x11, 0x09, 0x3E, 0x0C, 0x40 };
  ADSBFrame frame = makeFrame(&adsb, 0x48, 0x40, 0xD6, msg);
  BOOST_CHECK(frame.messageType == ADSBMessageType::AIRCRAFT_ID);

  char callsign[RADIOLIB_ADSB_CALLSIGN_LEN];
  ADSBAircraftCategory cat = ADSBAircraftCategory::NONE;
  BOOST_CHECK_EQUAL(adsb.parseCallsign(&frame, callsign, &cat), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(std::string(callsign), std::string("RADIO 1"));
  BOOST_CHECK(cat == ADSBAircraftCategory::LIGHT);

  // without the category field set, no category is reported
  msg[0] = 0x20;
  frame = makeFrame(&adsb, 0x48, 0x40, 0xD6, msg);
  cat = ADSBAircraftCategory::RESERVED;
  BOOST_CHECK_EQUAL(adsb.parseCallsign(&frame, callsign, &cat), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(std::string(callsign), std::string("RADIO 1"));
  BOOST_CHECK(cat == ADSBAircraftCategory::NONE);

  // type code 1 reports a reserved category
  uint8_t tc1Msg[7] = { 0x09, 0x48, 0x11, 0x09, 0x3E, 0x0C, 0x40 };
  frame = makeFrame(&adsb, 0x48, 0x40, 0xD6, tc1Msg);
  cat = ADSBAircraftCategory::NONE;
  BOOST_CHECK_EQUAL(adsb.parseCallsign(&frame, callsign, &cat), RADIOLIB_ERR_NONE);
  BOOST_CHECK(cat == ADSBAircraftCategory::RESERVED);

  // a frame that is not an identification message is rejected
  const uint8_t velMsg[7] = { 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  frame = makeFrame(&adsb, 0x48, 0x40, 0xD6, velMsg);
  BOOST_CHECK(frame.messageType == ADSBMessageType::AIRBORNE_VEL);
  BOOST_CHECK_EQUAL(adsb.parseCallsign(&frame, callsign), RADIOLIB_ERR_ADSB_INVALID_MSG_TYPE);
}

BOOST_AUTO_TEST_CASE(ADSB_ParseAirbornePosition) {
  BOOST_TEST_MESSAGE("--- Test ADS-B airborne position decoding ---");
  AdsBTestNullPhy phy;
  ADSBClient adsb(&phy);

  int alt = 0;
  float lat = 0.0f, lon = 0.0f;
  bool altGnss = true;

  // reference position near the encoded aircraft position
  adsb.setReferencePosition(51.5f, 100.0f);

  // even-CPR frame encoding latitude 51.47 N, longitude 100.2 E,
  // barometric altitude 36000 ft in 25 ft steps (Q = 1),
  // encoded with an independent spec-side bit packer
  const uint8_t baroMsg[7] = { 0x60, 0xB9, 0x82, 0x50, 0x36, 0x98, 0xBF };
  ADSBFrame frame = makeFrame(&adsb, 0x48, 0x40, 0xD6, baroMsg);
  BOOST_CHECK(frame.messageType == ADSBMessageType::AIRBORNE_POS_ALT_BARO);
  BOOST_CHECK_EQUAL(adsb.parseAirbornePosition(&frame, &alt, &lat, &lon, &altGnss), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(alt, 36000);
  BOOST_CHECK(!altGnss);
  // single-frame CPR relative to the reference position must recover the
  // encoded coordinates to within the CPR quantisation error
  BOOST_CHECK_CLOSE(lat, 51.47f, 0.01f);
  BOOST_CHECK_CLOSE(lon, 100.2f, 0.01f);

  // the same position as a GNSS (TC20) message with 1234 m altitude
  const uint8_t gnssMsg[7] = { 0xA0, 0x4D, 0x22, 0x50, 0x36, 0x98, 0xBF };
  frame = makeFrame(&adsb, 0x48, 0x40, 0xD6, gnssMsg);
  BOOST_CHECK(frame.messageType == ADSBMessageType::AIRBORNE_POS_ALT_GNSS);
  altGnss = false;
  BOOST_CHECK_EQUAL(adsb.parseAirbornePosition(&frame, &alt, &lat, &lon, &altGnss), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(alt, 1234);
  BOOST_CHECK(altGnss);
  BOOST_CHECK_CLOSE(lat, 51.47f, 0.01f);
  BOOST_CHECK_CLOSE(lon, 100.2f, 0.01f);

  // odd-CPR frame (TC11): latitude 30 N, longitude 10 E, 15000 ft baro in
  // 25 ft steps, encoded with an independent spec-side bit packer using the
  // standard odd zone sizes (Dlat = 360/59, Dlon = 360/50 at this latitude)
  {
    const uint8_t oddMsg[7] = { 0x58, 0x51, 0x07, 0xAA, 0xAA, 0xC7, 0x1C };
    ADSBFrame oddFrame = makeFrame(&adsb, 0x48, 0x40, 0xD6, oddMsg);
    BOOST_CHECK(oddFrame.messageType == ADSBMessageType::AIRBORNE_POS_ALT_BARO);

    adsb.setReferencePosition(30.0f, 10.0f);
    BOOST_CHECK_EQUAL(adsb.parseAirbornePosition(&oddFrame, &alt, &lat, &lon, &altGnss), RADIOLIB_ERR_NONE);
    BOOST_CHECK_EQUAL(alt, 15000);
    BOOST_CHECK_CLOSE(lat, 30.0f, 0.01f);
    BOOST_CHECK_CLOSE(lon, 10.0f, 0.01f);
    adsb.setReferencePosition(51.5f, 100.0f);
  }

  // a message type without position is rejected
  const uint8_t idMsg[7] = { 0x20, 0x48, 0x11, 0x09, 0x3E, 0x0C, 0x40 };
  frame = makeFrame(&adsb, 0x48, 0x40, 0xD6, idMsg);
  BOOST_CHECK_EQUAL(adsb.parseAirbornePosition(&frame, &alt, &lat, &lon), RADIOLIB_ERR_ADSB_INVALID_MSG_TYPE);

  // null pointers for any of the outputs are allowed
  BOOST_CHECK_EQUAL(adsb.parseAirbornePosition(&frame, NULL, NULL, NULL, NULL), RADIOLIB_ERR_ADSB_INVALID_MSG_TYPE);
}

BOOST_AUTO_TEST_SUITE_END()
