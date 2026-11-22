// boost test header
#include <boost/test/unit_test.hpp>

// Utils header
#include "utils/Utils.h"

#include <string.h>

BOOST_AUTO_TEST_SUITE(suite_Utils)

BOOST_AUTO_TEST_CASE(Utils_Reflect) {
  BOOST_TEST_MESSAGE("--- Test rlb_reflect against known bit reflections ---");

  // single bit moves to the top
  BOOST_CHECK_EQUAL(rlb_reflect(0x01, 8), (uint32_t)0x80);
  // 0xD5 = 0b11010101 -> 0b10101011 = 0xAB
  BOOST_CHECK_EQUAL(rlb_reflect(0xD5, 8), (uint32_t)0xAB);
  // 0x1234 = 0001 0010 0011 0100 -> 0010 1100 0100 1000 = 0x2C48
  BOOST_CHECK_EQUAL(rlb_reflect(0x1234, 16), (uint32_t)0x2C48);
  // reflection is its own inverse
  BOOST_CHECK_EQUAL(rlb_reflect(rlb_reflect(0xDEADBEEF, 32), 32), (uint32_t)0xDEADBEEF);
  BOOST_CHECK_EQUAL(rlb_reflect(rlb_reflect(0xABCD, 12), 12), (uint32_t)0xABCD & 0x0FFF);
  // zero and zero-bit edge cases
  BOOST_CHECK_EQUAL(rlb_reflect(0, 32), (uint32_t)0);
  BOOST_CHECK_EQUAL(rlb_reflect(0xFF, 0), (uint32_t)0);
  // only the lowest "bits" bits are reflected
  BOOST_CHECK_EQUAL(rlb_reflect(0x0F, 4), (uint32_t)0x0F);
  BOOST_CHECK_EQUAL(rlb_reflect(0x06, 4), (uint32_t)0x06);
}

BOOST_AUTO_TEST_CASE(Utils_Scrambler) {
  BOOST_TEST_MESSAGE("--- Test rlb_scrambler LFSR scrambler ---");

  uint8_t orig[64];
  for(size_t i = 0; i < sizeof(orig); i++) {
    orig[i] = (uint8_t)(i * 7 + 3);
  }

  // G3RUH scrambling actually transforms the data
  uint8_t buf[64];
  memcpy(buf, orig, sizeof(buf));
  rlb_scrambler(buf, sizeof(buf), RADIOLIB_SCRAMBLER_G3RUH_POLY, RADIOLIB_SCRAMBLER_G3RUH_INIT, true);
  BOOST_CHECK(memcmp(buf, orig, sizeof(buf)) != 0);

  // descrambling recovers the original data
  rlb_scrambler(buf, sizeof(buf), RADIOLIB_SCRAMBLER_G3RUH_POLY, RADIOLIB_SCRAMBLER_G3RUH_INIT, false);
  BOOST_CHECK(memcmp(buf, orig, sizeof(buf)) == 0);

  // a zero polynomial disables scrambling entirely
  uint8_t z[16];
  memset(z, 0x5A, sizeof(z));
  rlb_scrambler(z, sizeof(z), 0, 0x1234, true);
  for(size_t i = 0; i < sizeof(z); i++) {
    BOOST_CHECK_EQUAL(z[i], (uint8_t)0x5A);
  }

  // all-zero input with zero seed stays zero
  uint8_t zero[8];
  memset(zero, 0x00, sizeof(zero));
  rlb_scrambler(zero, sizeof(zero), RADIOLIB_SCRAMBLER_G3RUH_POLY, 0, true);
  for(size_t i = 0; i < sizeof(zero); i++) {
    BOOST_CHECK_EQUAL(zero[i], (uint8_t)0x00);
  }
}

BOOST_AUTO_TEST_SUITE_END()
