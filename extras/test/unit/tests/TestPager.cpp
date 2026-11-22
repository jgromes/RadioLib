// boost test header
#include <boost/test/unit_test.hpp>

#include <cstring>
#include <vector>

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

// ---------------------------------------------------------------------------
// POCSAG reception: readData() decoding and address filtering.
// the frames below are built with an independent encoder (ETSI EN 300 064
// framing: 21-bit address + function bits, BCH parity, 20-bit symbol blocks)

// a physical layer exposing a canned POCSAG code word stream through the
// direct receive buffer, exactly like a radio in direct mode would
class TestBatchPhy: public PhysicalLayer {
  public:
    Module* getMod() override { return(nullptr); }

    void load(const uint32_t* words, size_t numWords) {
      memset(buffer, 0, sizeof(buffer));
      bufferWritePos = 0;
      bufferReadPos = 0;
      bufferBitPos = 0;
      directSyncWordLen = 0;
      gotSync = true;
      for(size_t i = 0; i < numWords; i++) {
        for(int bit = 31; bit >= 0; bit--) {
          updateDirectBuffer((uint8_t)((words[i] >> bit) & 0x01));
        }
      }
    }
};

// BCH(31,21)-protected POCSAG code word builders, mirroring the frame
// layout defined by the POCSAG standard
static RadioLibBCH testBch;

static uint32_t testAddressWord(uint32_t addr, uint8_t func) {
  uint32_t cw = ((addr >> 3) << RADIOLIB_PAGER_ADDRESS_POS) | ((uint32_t)func << RADIOLIB_PAGER_FUNC_BITS_POS);
  return(testBch.encode(cw));
}

static uint32_t testBcdWord(const char* msg, size_t num) {
  TestNullPhy nullPhy;
  PagerClient ref(&nullPhy);
  uint32_t cw = RADIOLIB_PAGER_MESSAGE_CODE_WORD << (RADIOLIB_PAGER_CODE_WORD_LEN - 1);
  for(size_t i = 0; i < num; i++) {
    uint8_t sym = ref.encodeBCD(msg[i]);
    sym = (uint8_t)(rlb_reflect(sym, 8) >> 4);
    cw |= (uint32_t)sym << (27 - 4 * (int)i);
  }
  return(testBch.encode(cw));
}

// pack ASCII symbols into BCH-protected message code words: the 7-bit symbol
// values pack continuously downwards from bit 30 of the 20-bit message area
// and symbols overflowing it continue at the top of the next message code word
static size_t testAsciiMessage(const char* msg, uint32_t* out, size_t maxWords) {
  size_t words = 0;
  uint32_t cw = 0x80000000UL;
  int pos = 30;
  for(const char* p = msg; *p != '\0'; p++) {
    uint8_t v = rlb_reflect((uint8_t)(2 * (uint8_t)*p), 8);
    for(int b = 6; b >= 0; b--) {
      if(pos < RADIOLIB_PAGER_MESSAGE_END_POS) {
        out[words++] = testBch.encode(cw);
        cw = 0x80000000UL;
        pos = 30;
      }
      if((v >> b) & 1) {
        cw |= (1UL << pos);
      } else {
        cw &= ~(1UL << pos);
      }
      pos--;
    }
  }
  if(pos != 30) {
    out[words++] = testBch.encode(cw);
  }
  (void)maxWords;
  return(words);
}

BOOST_AUTO_TEST_CASE(Pager_ReadDataNumeric) {
  BOOST_TEST_MESSAGE("--- Test POCSAG BCD message reception ---");
  testBch.begin(RADIOLIB_PAGER_BCH_N, RADIOLIB_PAGER_BCH_K, RADIOLIB_PAGER_BCH_PRIMITIVE_POLY);

  TestBatchPhy phy;
  PagerClient pager(&phy);
  // the injected code words are the true (non-inverted) words
  pager.inv = true;
  pager.filterNumAddresses = 0;
  pager.filterAddr = 0x1B3F01;
  pager.filterMask = 0x1FFFFF;

  // two preamble words, idle padding, sync, a stray message word, idle,
  // then the address word at batch frame 1 and the 10-symbol BCD message
  const uint32_t words[] = {
    RADIOLIB_PAGER_PREAMBLE_CODE_WORD, RADIOLIB_PAGER_PREAMBLE_CODE_WORD,
    RADIOLIB_PAGER_IDLE_CODE_WORD, RADIOLIB_PAGER_IDLE_CODE_WORD,
    RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD,
    0x80000000UL,
    RADIOLIB_PAGER_IDLE_CODE_WORD,
    testAddressWord(0x1B3F01, RADIOLIB_PAGER_FUNC_BITS_NUMERIC),
    testBcdWord("12345", 5),
    testBcdWord("67890", 5),
    RADIOLIB_PAGER_IDLE_CODE_WORD,
  };
  phy.load(words, sizeof(words) / sizeof(words[0]));

  uint8_t data[64];
  size_t len = sizeof(data);
  uint32_t rxAddr = 0;
  BOOST_CHECK_EQUAL(pager.readData(data, &len, &rxAddr), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(len, (size_t)10);
  BOOST_CHECK_EQUAL_COLLECTIONS(data, data + len, "1234567890", "1234567890" + 10);
  BOOST_CHECK_EQUAL(rxAddr, (uint32_t)0x1B3F01);
}

BOOST_AUTO_TEST_CASE(Pager_ReadDataAlpha) {
  BOOST_TEST_MESSAGE("--- Test POCSAG ASCII message reception ---");
  testBch.begin(RADIOLIB_PAGER_BCH_N, RADIOLIB_PAGER_BCH_K, RADIOLIB_PAGER_BCH_PRIMITIVE_POLY);

  TestBatchPhy phy;
  PagerClient pager(&phy);
  pager.inv = true;
  pager.filterNumAddresses = 0;
  pager.filterAddr = 0x1B3F00;
  pager.filterMask = 0x1FFFFF;

  // address word directly after the sync word -> batch frame 0
  // "SOS" = 3 symbols: the third one overflows into the next message code
  // word, whose remaining symbol slots decode as zero padding
  uint32_t msgWords[4];
  size_t msgLen = testAsciiMessage("SOS", msgWords, 4);
  std::vector<uint32_t> words;
  words.push_back(RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD);
  words.push_back(testAddressWord(0x1B3F00, RADIOLIB_PAGER_FUNC_BITS_ALPHA));
  words.insert(words.end(), msgWords, msgWords + msgLen);
  words.push_back(RADIOLIB_PAGER_IDLE_CODE_WORD);
  phy.load(words.data(), words.size());

  uint8_t data[64];
  size_t len = sizeof(data);
  uint32_t rxAddr = 0;
  BOOST_CHECK_EQUAL(pager.readData(data, &len, &rxAddr), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(len, (size_t)5);
  BOOST_CHECK_EQUAL_COLLECTIONS(data, data + 3, "SOS", "SOS" + 3);
  BOOST_CHECK_EQUAL(data[3], (uint8_t)0);
  BOOST_CHECK_EQUAL(data[4], (uint8_t)0);
  BOOST_CHECK_EQUAL(rxAddr, (uint32_t)0x1B3F00);
}

BOOST_AUTO_TEST_CASE(Pager_AddressFilter) {
  BOOST_TEST_MESSAGE("--- Test POCSAG address filtering ---");
  testBch.begin(RADIOLIB_PAGER_BCH_N, RADIOLIB_PAGER_BCH_K, RADIOLIB_PAGER_BCH_PRIMITIVE_POLY);

  // the same "SOS" frame as in the Alpha test
  uint32_t sosWords[4];
  size_t sosLen = testAsciiMessage("SOS", sosWords, 4);
  std::vector<uint32_t> words;
  words.push_back(RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD);
  words.push_back(testAddressWord(0x1B3F00, RADIOLIB_PAGER_FUNC_BITS_ALPHA));
  words.insert(words.end(), sosWords, sosWords + sosLen);
  words.push_back(RADIOLIB_PAGER_IDLE_CODE_WORD);

  TestBatchPhy phy;
  PagerClient pager(&phy);
  pager.inv = true;

  uint8_t data[64];
  size_t len;
  uint32_t rxAddr;

  // a partial mask only compares the unmasked address bits
  pager.filterNumAddresses = 0;
  pager.filterAddr = 0x000000;
  pager.filterMask = 0x000007;
  phy.load(words.data(), words.size());
  len = sizeof(data);
  BOOST_CHECK_EQUAL(pager.readData(data, &len, &rxAddr), RADIOLIB_ERR_NONE);

  // a non-matching address must be reported
  pager.filterAddr = 0x1B3F02;
  pager.filterMask = 0x1FFFFF;
  phy.load(words.data(), words.size());
  len = sizeof(data);
  BOOST_CHECK_EQUAL(pager.readData(data, &len, &rxAddr), RADIOLIB_ERR_ADDRESS_NOT_FOUND);

  // multiple addresses: the received address is the second entry
  uint32_t addrs[] = { 0x000001, 0x1B3F00 };
  uint32_t masks[] = { 0x1FFFFF, 0x1FFFFF };
  pager.filterAddresses = addrs;
  pager.filterMasks = masks;
  pager.filterNumAddresses = 2;
  phy.load(words.data(), words.size());
  len = sizeof(data);
  BOOST_CHECK_EQUAL(pager.readData(data, &len, &rxAddr), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(rxAddr, (uint32_t)0x1B3F00);

  // multi-address filtering without the arrays must never match
  pager.filterAddresses = nullptr;
  phy.load(words.data(), words.size());
  len = sizeof(data);
  BOOST_CHECK_EQUAL(pager.readData(data, &len, &rxAddr), RADIOLIB_ERR_ADDRESS_NOT_FOUND);
  pager.filterNumAddresses = 0;

  // a stream without any address code word
  const uint32_t noAddr[] = {
    RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD,
    RADIOLIB_PAGER_IDLE_CODE_WORD,
    0x81234567UL,
    RADIOLIB_PAGER_IDLE_CODE_WORD,
  };
  pager.filterAddr = 0x1B3F00;
  pager.filterMask = 0x1FFFFF;
  phy.load(noAddr, sizeof(noAddr) / sizeof(noAddr[0]));
  len = sizeof(data);
  BOOST_CHECK_EQUAL(pager.readData(data, &len, &rxAddr), RADIOLIB_ERR_ADDRESS_NOT_FOUND);
}

BOOST_AUTO_TEST_SUITE_END()
