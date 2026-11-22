// boost test header
#include <boost/test/unit_test.hpp>

// convolutional code header
#include "utils/ConvCode.h"

#include <string.h>

// independent reference convolutional encoder, built from the generator polynomials
// of the LR-FHSS convolutional code: rate 1/3, constraint length 7,
// generators (0155, 0117, 0127) in octal (Semtech AN1900);
// the rate 1/2 code used here has generators (035, 023).
// generator bit 0 corresponds to the current input bit, bit n to n-th previous bit.
static int rlb_test_parity(uint32_t x) {
  x ^= x >> 16; x ^= x >> 8; x ^= x >> 4; x ^= x >> 2; x ^= x >> 1;
  return(x & 1);
}

static size_t rlb_test_convEncode(const uint8_t* in, size_t inBits, uint8_t* out, uint8_t rate) {
  static const uint32_t gens1_3[3] = { 0155, 0117, 0127 };
  static const uint32_t gens1_2[2] = { 035, 023 };
  const uint32_t* gens = (rate == 2) ? gens1_2 : gens1_3;
  uint8_t numGens = (rate == 2) ? 2 : 3;

  uint32_t reg = 0;
  size_t bitCount = 0;
  uint32_t word = 0;
  for(size_t i = 0; i < inBits; i++) {
    uint8_t bit = (in[i / 8] >> (7 - (i % 8))) & 1;
    reg = ((reg << 1) | bit) & 0x7F;
    for(uint8_t g = 0; g < numGens; g++) {
      word = (word << 1) | (uint32_t)rlb_test_parity(reg & gens[g]);
      bitCount++;
      if(bitCount % 8 == 0) {
        *out++ = (uint8_t)word;
      }
    }
  }
  if(bitCount % 8) {
    *out++ = (uint8_t)(word << (8 - (bitCount % 8)));
  }
  return(bitCount);
}

BOOST_AUTO_TEST_SUITE(suite_ConvCode)

BOOST_AUTO_TEST_CASE(ConvCode_ZeroInput) {
  BOOST_TEST_MESSAGE("--- Test convolutional encoding of an all-zero input ---");
  RadioLibConvCode conv;
  uint8_t in[2] = { 0x00, 0x00 };
  uint8_t out[16];
  size_t outBits = 0;

  // rate 1/3: 16 input bits -> 48 coded bits, all zero
  conv.begin(3);
  BOOST_CHECK_EQUAL(conv.encode(in, 16, out, &outBits), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(outBits, (size_t)48);
  static const uint8_t expRate3[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  BOOST_CHECK_EQUAL_COLLECTIONS(out, out + sizeof(expRate3), expRate3, expRate3 + sizeof(expRate3));

  // rate 1/2: 16 input bits -> 32 coded bits, all zero
  conv.begin(2);
  BOOST_CHECK_EQUAL(conv.encode(in, 16, out, &outBits), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(outBits, (size_t)32);
  static const uint8_t expRate2[] = { 0x00, 0x00, 0x00, 0x00 };
  BOOST_CHECK_EQUAL_COLLECTIONS(out, out + sizeof(expRate2), expRate2, expRate2 + sizeof(expRate2));
}

BOOST_AUTO_TEST_CASE(ConvCode_KnownVectors) {
  BOOST_TEST_MESSAGE("--- Test convolutional encoding against regression vectors ---");
  RadioLibConvCode conv;
  const uint8_t in[4] = { 0x00, 0xFF, 0x5A, 0xC3 };
  uint8_t out[64];
  size_t outBits = 0;

  // reference values verified against the independent encoder below
  conv.begin(3);
  BOOST_CHECK_EQUAL(conv.encode(in, 32, out, &outBits), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(outBits, (size_t)96);
  static const uint8_t expRate3[12] = {
    0x00, 0x00, 0x00, 0xF1, 0xD8, 0x3F, 0x13, 0xA2, 0x93, 0xA5, 0x51, 0x63,
  };
  BOOST_CHECK_EQUAL_COLLECTIONS(out, out + sizeof(expRate3), expRate3, expRate3 + sizeof(expRate3));

  conv.begin(2);
  BOOST_CHECK_EQUAL(conv.encode(in, 32, out, &outBits), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(outBits, (size_t)64);
  static const uint8_t expRate2[8] = {
    0x00, 0x00, 0xE2, 0x55, 0x82, 0x0C, 0x80, 0x7E,
  };
  BOOST_CHECK_EQUAL_COLLECTIONS(out, out + sizeof(expRate2), expRate2, expRate2 + sizeof(expRate2));
}

BOOST_AUTO_TEST_CASE(ConvCode_MatchesReference) {
  BOOST_TEST_MESSAGE("--- Test convolutional encoding against independent reference encoder ---");
  RadioLibConvCode conv;
  uint8_t in[31];
  for(size_t i = 0; i < sizeof(in); i++) {
    in[i] = (uint8_t)(i * 41 + 7);
  }
  uint8_t got[128], ref[128];
  size_t outBits = 0;

  for(uint8_t rate : { (uint8_t)2, (uint8_t)3 }) {
    // test all input bit lengths 0..8*len - exercise partial-byte flushing
    for(size_t inBits = 0; inBits <= 8 * sizeof(in); inBits++) {
      conv.begin(rate);
      BOOST_CHECK_EQUAL(conv.encode(in, inBits, got, &outBits), RADIOLIB_ERR_NONE);
      BOOST_CHECK_EQUAL(outBits, inBits * rate);
      size_t refBits = rlb_test_convEncode(in, inBits, ref, rate);
      BOOST_CHECK_EQUAL(refBits, outBits);
      // only compare full output bytes, the trailing partial byte may contain padding
      size_t fullBytes = outBits / 8;
      BOOST_CHECK_EQUAL_COLLECTIONS(got, got + fullBytes, ref, ref + fullBytes);
    }
  }
}

BOOST_AUTO_TEST_CASE(ConvCode_NullPointerChecks) {
  BOOST_TEST_MESSAGE("--- Test convolutional encoding null pointer handling ---");
  RadioLibConvCode conv;
  uint8_t in[2] = { 0x00, 0x00 };
  uint8_t out[16];
  size_t outBits = 0;
  conv.begin(3);
  BOOST_CHECK(conv.encode(NULL, 16, out, &outBits) != RADIOLIB_ERR_NONE);
  BOOST_CHECK(conv.encode(in, 16, NULL, &outBits) != RADIOLIB_ERR_NONE);
}

BOOST_AUTO_TEST_SUITE_END()
