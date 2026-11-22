// boost test header
#include <boost/test/unit_test.hpp>

// Morse header
#include "protocols/Morse/Morse.h"

#include <string.h>

// MorseClient::decode takes a symbol where the first dot/dash is bit 0
// (dot = 0, dash = 1) and len is the number of dots/dashes;
// the guard bit is added by the method itself.
static uint8_t testMorseSymbol(const char* code, uint8_t* len) {
  uint8_t symbol = 0;
  uint8_t l = 0;
  for(; *code != '\0'; code++, l++) {
    if(*code == '-') {
      symbol |= (uint8_t)(1 << l);
    }
  }
  *len = l;
  return(symbol);
}

BOOST_AUTO_TEST_SUITE(suite_Morse)

BOOST_AUTO_TEST_CASE(Morse_Decode) {
  BOOST_TEST_MESSAGE("--- Test Morse decoding against the standard ITU code table ---");

  // the standard ITU Morse alphabet
  struct { char c; const char* code; } table[] = {
    { 'A', ".-" },     { 'B', "-..." },   { 'C', "-.-." },  { 'D', "-.." },
    { 'E', "." },      { 'F', "..-." },   { 'G', "--." },   { 'H', "...." },
    { 'I', ".." },     { 'J', ".---" },   { 'K', "-.-" },   { 'L', ".-.." },
    { 'M', "--" },     { 'N', "-." },     { 'O', "---" },   { 'P', ".--." },
    { 'Q', "--.-" },   { 'R', ".-." },    { 'S', "..." },   { 'T', "-" },
    { 'U', "..-" },    { 'V', "...-" },   { 'W', ".--" },   { 'X', "-..-" },
    { 'Y', "-.--" },   { 'Z', "--.." },
    { '0', "-----" },  { '1', ".----" },  { '2', "..---" }, { '3', "...--" },
    { '4', "....-" },  { '5', "....." },  { '6', "-...." }, { '7', "--..." },
    { '8', "---.." },  { '9', "----." },
    { '?', "..--.." }, { '/', "-..-." },
  };

  for(const auto& entry : table) {
    uint8_t len = 0;
    uint8_t symbol = testMorseSymbol(entry.code, &len);
    BOOST_CHECK_EQUAL(MorseClient::decode(symbol, len), entry.c);
  }
}

BOOST_AUTO_TEST_CASE(Morse_DecodeExplicitVectors) {
  BOOST_TEST_MESSAGE("--- Test Morse decoding against explicit bit patterns ---");

  // E = dot
  BOOST_CHECK_EQUAL(MorseClient::decode(0b00000000, 1), 'E');
  // T = dash
  BOOST_CHECK_EQUAL(MorseClient::decode(0b00000001, 1), 'T');
  // A = dot dash
  BOOST_CHECK_EQUAL(MorseClient::decode(0b00000010, 2), 'A');
  // 5 = five dots (symbol value zero, only the guard bit distinguishes it from E)
  BOOST_CHECK_EQUAL(MorseClient::decode(0b00000000, 5), '5');
  // K = dash dot dash
  BOOST_CHECK_EQUAL(MorseClient::decode(0b00000101, 3), 'K');
  // 0 = five dashes
  BOOST_CHECK_EQUAL(MorseClient::decode(0b00011111, 5), '0');
}

BOOST_AUTO_TEST_SUITE_END()
