// boost test header
#include <boost/test/unit_test.hpp>

// BCH header
#include "utils/BCH.h"

#include <string.h>

// independent GF(2^5) arithmetic using the POCSAG primitive polynomial
// x^5 + x^2 + 1 (0x25) - used to verify BCH(31,21) code words via syndrome
// evaluation, without reusing the encoder's internal tables
static int testGfExp[64], testGfLog[64];

static void testGfInit() {
  int x = 1;
  for(int i = 0; i < 5; i++) { testGfExp[i] = x; testGfLog[x] = i; x <<= 1; }
  for(int i = 5; i < 31; i++) {
    x = testGfExp[i - 1] << 1;
    if(x & 0x20) { x ^= 0x25; }
    testGfExp[i] = x; testGfLog[x] = i;
  }
  for(int i = 31; i < 64; i++) { testGfExp[i] = testGfExp[i - 31]; }
}

static int testGfMul(int a, int b) {
  if(a == 0 || b == 0) { return(0); }
  return(testGfExp[testGfLog[a] + testGfLog[b]]);
}

// syndrome S_i = C(alpha^i), code word bit 30 is the x^30 coefficient
static int testSyndrome(uint32_t cw31, int i) {
  int a = testGfExp[i];
  int s = 0;
  for(int b = 30; b >= 0; b--) {
    s = testGfMul(s, a);
    if((cw31 >> b) & 1) { s ^= 1; }
  }
  return(s);
}

// the POCSAG code word layout produced by RadioLibBCH (n = 31, k = 21):
// bits 31..11 hold the data word, bits 10..1 the check bits, bit 0 the overall parity
static void testCheckBchCodeword(uint32_t dataword, uint32_t codeword) {
  // data must be preserved unmodified on bits 31..11
  BOOST_CHECK_EQUAL(codeword & 0xFFFFF800, dataword & 0xFFFFF800);

  // BCH syndromes S_1..S_4 of the 31 data+parity bits must all vanish
  uint32_t cw31 = (codeword >> 1) & 0x7FFFFFFF;
  for(int i = 1; i <= 4; i++) {
    BOOST_CHECK_EQUAL(testSyndrome(cw31, i), 0);
  }

  // overall parity bit makes the code word balanced
  uint32_t ones = 0, v = cw31;
  while(v) { ones += v & 1; v >>= 1; }
  BOOST_CHECK_EQUAL(codeword & 1, ones & 1);
}

BOOST_AUTO_TEST_SUITE(suite_BCH)

BOOST_AUTO_TEST_CASE(BCH_KnownVectors) {
  BOOST_TEST_MESSAGE("--- Test POCSAG BCH(31,21) encoder against regression vectors ---");
  RadioLibBCH bch;
  bch.begin(RADIOLIB_PAGER_BCH_N, RADIOLIB_PAGER_BCH_K, RADIOLIB_PAGER_BCH_PRIMITIVE_POLY);

  // all-zero data word encodes to all zeros
  BOOST_CHECK_EQUAL(bch.encode(0x00000000), (uint32_t)0x00000000);
  // a single data bit produces the generator polynomial itself as check bits
  BOOST_CHECK_EQUAL(bch.encode(0x80000000), (uint32_t)0x80000769);
  // all-ones data word with all-ones check field (fixed point)
  BOOST_CHECK_EQUAL(bch.encode(0xFFFFFFFF), (uint32_t)0xFFFFFFFF);
  BOOST_CHECK_EQUAL(bch.encode(0x12345000), (uint32_t)0x123457F6);
  BOOST_CHECK_EQUAL(bch.encode(0xABC00000), (uint32_t)0xABC003F2);
}

BOOST_AUTO_TEST_CASE(BCH_CodewordValidity) {
  BOOST_TEST_MESSAGE("--- Test POCSAG BCH(31,21) code words against independent syndrome check ---");
  testGfInit();
  RadioLibBCH bch;
  bch.begin(RADIOLIB_PAGER_BCH_N, RADIOLIB_PAGER_BCH_K, RADIOLIB_PAGER_BCH_PRIMITIVE_POLY);

  const uint32_t datawords[] = {
    0x00000000, 0x80000000, 0xFFFFFFFF, 0x7CD80000, 0x7A89C000,
    0x55555000, 0x00000800, 0xDEAD1000, 0x12345800, 0xC0FFEE00 & 0xFFFFF800,
  };
  for(uint32_t dw : datawords) {
    testCheckBchCodeword(dw, bch.encode(dw));
  }

  // sweep single-bit data words: bit 31 down to bit 11
  for(int b = 31; b >= 11; b--) {
    testCheckBchCodeword((uint32_t)1 << b, bch.encode((uint32_t)1 << b));
  }
}

BOOST_AUTO_TEST_SUITE_END()
