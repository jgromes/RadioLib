// boost test header
#include <boost/test/unit_test.hpp>

// ITA2 string header
#include "protocols/Print/ITA2String.h"

#include <string.h>

// ITA2 (ITA-Alphabet No. 5) codes are defined by the standard 5-bit code group:
// the array index in RadioLib's table is the 5-bit code, so expected values below
// correspond to the standard IAR5 code assignment (e.g. E = 00001, T = 10000).

BOOST_AUTO_TEST_SUITE(suite_ITA2)

BOOST_AUTO_TEST_CASE(ITA2_GetBits) {
  BOOST_TEST_MESSAGE("--- Test ITA2 getBits against IAR5 code assignments ---");

  // letters shift (0x1F): E = 00001, T = 10000, space = 00100
  BOOST_CHECK_EQUAL(ITA2String::getBits('E'), (uint16_t)((0x1F << 5) | 1));
  BOOST_CHECK_EQUAL(ITA2String::getBits('T'), (uint16_t)((0x1F << 5) | 16));
  BOOST_CHECK_EQUAL(ITA2String::getBits(' '), (uint16_t)((0x1F << 5) | 4));

  // figures shift (0x1B): '3' shares code 00001 with E, '1' shares 10111 with Q
  BOOST_CHECK_EQUAL(ITA2String::getBits('3'), (uint16_t)((0x1B << 5) | 1));
  BOOST_CHECK_EQUAL(ITA2String::getBits('1'), (uint16_t)((0x1B << 5) | 23));

  // control characters share codes between shifts: CR = 01000, LF = 00010
  BOOST_CHECK_EQUAL(ITA2String::getBits('\r'), (uint16_t)((0x1F << 5) | 8));
  BOOST_CHECK_EQUAL(ITA2String::getBits('\n'), (uint16_t)((0x1F << 5) | 2));

  // the shift field and character field must decompose consistently
  uint16_t code = ITA2String::getBits('5');
  BOOST_CHECK_EQUAL((code >> 5) & 0x1F, (uint16_t)0x1B); // figures shift
  BOOST_CHECK_EQUAL(code & 0x1F, (uint16_t)16); // '5' shares the T code
}

BOOST_AUTO_TEST_CASE(ITA2_ByteArr) {
  BOOST_TEST_MESSAGE("--- Test ITA2 string encoding with shift characters ---");

  // pure letters: no shift characters required
  {
    ITA2String ita2("HELLO");
    uint8_t* arr = ita2.byteArr();
    const uint8_t expected[] = { 20, 1, 18, 18, 24 }; // H E L L O
    BOOST_CHECK_EQUAL(ita2.length(), sizeof(expected));
    BOOST_CHECK_EQUAL_COLLECTIONS(arr, arr + ita2.length(), expected, expected + sizeof(expected));
    delete[] arr;
  }

  // letters followed by figures: FIGS inserted before the digits,
  // LTRS only restored when the next character is a letter or at end of message
  {
    ITA2String ita2("TEST123");
    uint8_t* arr = ita2.byteArr();
    const uint8_t expected[] = { 16, 1, 5, 16, 0x1B, 23, 19, 1, 0x1F }; // T E S T FIGS '1' '2' '3' LTRS
    BOOST_CHECK_EQUAL(ita2.length(), sizeof(expected));
    BOOST_CHECK_EQUAL_COLLECTIONS(arr, arr + ita2.length(), expected, expected + sizeof(expected));
    delete[] arr;
  }

  // letter then figure: LTRS must be restored at end of message
  {
    ITA2String ita2("A3");
    uint8_t* arr = ita2.byteArr();
    const uint8_t expected[] = { 3, 0x1B, 1, 0x1F }; // A FIGS '3' LTRS
    BOOST_CHECK_EQUAL(ita2.length(), sizeof(expected));
    BOOST_CHECK_EQUAL_COLLECTIONS(arr, arr + ita2.length(), expected, expected + sizeof(expected));
    delete[] arr;
  }

  // figure then letter: LTRS restored before the letter
  {
    ITA2String ita2("3A");
    uint8_t* arr = ita2.byteArr();
    const uint8_t expected[] = { 0x1B, 1, 0x1F, 3 }; // FIGS '3' LTRS A
    BOOST_CHECK_EQUAL(ita2.length(), sizeof(expected));
    BOOST_CHECK_EQUAL_COLLECTIONS(arr, arr + ita2.length(), expected, expected + sizeof(expected));
    delete[] arr;
  }
}

BOOST_AUTO_TEST_SUITE_END()
