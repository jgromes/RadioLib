// boost test header
#include <boost/test/unit_test.hpp>

// Pager header
#include "protocols/Pager/Pager.h"

// POCSAG BCD codec does not touch the radio, a null physical layer is enough
class TestNullPhy: public PhysicalLayer {
  public:
    Module* getMod() override { return(nullptr); }
};

BOOST_AUTO_TEST_SUITE(suite_Pager)

BOOST_AUTO_TEST_CASE(Pager_EncodeBCD) {
  BOOST_TEST_MESSAGE("--- Test POCSAG BCD encoding ---");
  TestNullPhy phy;
  PagerClient pager(&phy);

  // digits map to their values
  for(char c = '0'; c <= '9'; c++) {
    BOOST_CHECK_EQUAL(pager.encodeBCD(c), (uint8_t)(c - '0'));
  }

  // the six special POCSAG numerics
  BOOST_CHECK_EQUAL(pager.encodeBCD('*'), (uint8_t)0x0A);
  BOOST_CHECK_EQUAL(pager.encodeBCD('U'), (uint8_t)0x0B);
  BOOST_CHECK_EQUAL(pager.encodeBCD(' '), (uint8_t)0x0C);
  BOOST_CHECK_EQUAL(pager.encodeBCD('-'), (uint8_t)0x0D);
  BOOST_CHECK_EQUAL(pager.encodeBCD(')'), (uint8_t)0x0E);
  BOOST_CHECK_EQUAL(pager.encodeBCD('('), (uint8_t)0x0F);
}

BOOST_AUTO_TEST_CASE(Pager_DecodeBCD) {
  BOOST_TEST_MESSAGE("--- Test POCSAG BCD decoding ---");
  TestNullPhy phy;
  PagerClient pager(&phy);

  // digits
  for(uint8_t b = 0; b <= 9; b++) {
    BOOST_CHECK_EQUAL(pager.decodeBCD(b), (char)('0' + b));
  }

  // the six special POCSAG numerics
  BOOST_CHECK_EQUAL(pager.decodeBCD(0x0A), '*');
  BOOST_CHECK_EQUAL(pager.decodeBCD(0x0B), 'U');
  BOOST_CHECK_EQUAL(pager.decodeBCD(0x0C), ' ');
  BOOST_CHECK_EQUAL(pager.decodeBCD(0x0D), '-');
  BOOST_CHECK_EQUAL(pager.decodeBCD(0x0E), ')');
  BOOST_CHECK_EQUAL(pager.decodeBCD(0x0F), '(');

  // encode/decode round trip for all valid symbols
  const char* symbols = "0123456789*U -()";
  for(const char* p = symbols; *p != '\0'; p++) {
    BOOST_CHECK_EQUAL(pager.decodeBCD(pager.encodeBCD(*p)), *p);
  }
}

BOOST_AUTO_TEST_SUITE_END()
