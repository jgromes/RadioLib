// boost test header
#include <boost/test/unit_test.hpp>

// LoRaWAN header (static helper methods are tested without instantiating a node)
#include "protocols/LoRaWAN/LoRaWAN.h"

#include <string.h>

BOOST_AUTO_TEST_SUITE(suite_LoRaWANHelpers)

BOOST_AUTO_TEST_CASE(LoRaWANHelpers_EndianConversion) {
  BOOST_TEST_MESSAGE("--- Test LoRaWAN ntoh/hton against known frames ---");

  // DevAddr is transmitted little-endian: frame bytes 23 12 00 01 -> 0x01001223
  // (DevAddr field layout from the LoRaWAN specification)
  const uint8_t frame[] = { 0x40, 0x23, 0x12, 0x00, 0x01, 0x00, 0x00, 0x01, 0xAB };
  BOOST_CHECK_EQUAL(LoRaWANNode::ntoh<uint32_t>(&frame[1]), (uint32_t)0x01001223);
  BOOST_CHECK_EQUAL(LoRaWANNode::ntoh<uint16_t>(&frame[1]), (uint16_t)0x1223);
  BOOST_CHECK_EQUAL(LoRaWANNode::ntoh<uint8_t>(&frame[1]), (uint8_t)0x23);

  // explicit size overrides the type width
  BOOST_CHECK_EQUAL(LoRaWANNode::ntoh<uint32_t>(&frame[1], 1), (uint32_t)0x23);
  BOOST_CHECK_EQUAL(LoRaWANNode::ntoh<uint32_t>(&frame[1], 2), (uint32_t)0x1223);

  // hton writes the same layout back
  uint8_t buf[4];
  LoRaWANNode::hton<uint32_t>(buf, 0x01001223);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf, buf + 4, &frame[1], &frame[1] + 4);
  LoRaWANNode::hton<uint16_t>(buf, 0x1223);
  BOOST_CHECK_EQUAL(buf[0], (uint8_t)0x23);
  BOOST_CHECK_EQUAL(buf[1], (uint8_t)0x12);

  // round trip of ntoh(hton(x))
  const uint32_t values[] = { 0x00000000, 0x00000001, 0xDEADBEEF, 0xFFFFFFFF };
  for(uint32_t v : values) {
    LoRaWANNode::hton<uint32_t>(buf, v);
    BOOST_CHECK_EQUAL(LoRaWANNode::ntoh<uint32_t>(buf), v);
  }
}

BOOST_AUTO_TEST_CASE(LoRaWANHelpers_Checksum) {
  BOOST_TEST_MESSAGE("--- Test LoRaWAN XOR checksum ---");

  // 32-bit words are XOR-ed big-endian: 0x00000001 ^ 0x00000300
  const uint8_t buf8[] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00 };
  BOOST_CHECK_EQUAL(LoRaWANNode::checkSum<uint32_t>(buf8, 8), (uint32_t)0x00000301);

  // 16-bit words: 0x0102 ^ 0x0304
  const uint8_t buf4[] = { 0x01, 0x02, 0x03, 0x04 };
  BOOST_CHECK_EQUAL(LoRaWANNode::checkSum<uint16_t>(buf4, 4), (uint16_t)(0x0102 ^ 0x0304));

  // single bytes: 0x01 ^ 0x02 ^ 0x03 ^ 0x04
  BOOST_CHECK_EQUAL(LoRaWANNode::checkSum<uint8_t>(buf4, 4), (uint8_t)(0x01 ^ 0x02 ^ 0x03 ^ 0x04));

  // self-inverse property over a 4-byte aligned buffer:
  // appending the checksum as a big-endian 32-bit word must XOR out to zero
  uint8_t data[36];
  for(size_t i = 0; i < sizeof(data); i++) {
    data[i] = (uint8_t)(i * 13 + 7);
  }
  uint32_t sum = LoRaWANNode::checkSum<uint32_t>(data, sizeof(data));
  uint8_t withSum[sizeof(data) + 4];
  memcpy(withSum, data, sizeof(data));
  for(size_t i = 0; i < 4; i++) {
    withSum[sizeof(data) + i] = (uint8_t)(sum >> (24 - 8 * i));
  }
  BOOST_CHECK_EQUAL(LoRaWANNode::checkSum<uint32_t>(withSum, sizeof(withSum)), (uint32_t)0);
}

BOOST_AUTO_TEST_CASE(LoRaWANHelpers_CheckBufferCommon) {
  BOOST_TEST_MESSAGE("--- Test persistence buffer validation ---");

  // build a valid buffer: data + 32-bit little-endian checksum in the last four bytes
  uint8_t buf[32];
  for(size_t i = 0; i < sizeof(buf) - 4; i++) {
    buf[i] = (uint8_t)(i * 13 + 7);
  }
  uint32_t sum = LoRaWANNode::checkSum<uint32_t>(buf, sizeof(buf) - 4);
  LoRaWANNode::hton<uint32_t>(&buf[sizeof(buf) - 4], sum);
  BOOST_CHECK_EQUAL(LoRaWANNode::checkBufferCommon(buf, sizeof(buf)), RADIOLIB_ERR_NONE);

  // flipping one bit must invalidate the buffer
  buf[5] ^= 0x01;
  BOOST_CHECK(LoRaWANNode::checkBufferCommon(buf, sizeof(buf)) != RADIOLIB_ERR_NONE);
  buf[5] ^= 0x01;

  // an all-ones buffer has a valid XOR signature but no content - must be rejected
  uint8_t blank[32];
  memset(blank, 0xFF, sizeof(blank));
  BOOST_CHECK(LoRaWANNode::checkBufferCommon(blank, sizeof(blank)) != RADIOLIB_ERR_NONE);
  memset(blank, 0x00, sizeof(blank));
  BOOST_CHECK(LoRaWANNode::checkBufferCommon(blank, sizeof(blank)) != RADIOLIB_ERR_NONE);

  // flipping only the stored checksum must also be detected
  buf[31] ^= 0x80;
  BOOST_CHECK(LoRaWANNode::checkBufferCommon(buf, sizeof(buf)) != RADIOLIB_ERR_NONE);
}

BOOST_AUTO_TEST_SUITE_END()
