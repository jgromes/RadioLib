// boost test header
#include <boost/test/unit_test.hpp>

// Utils header
#include "utils/Utils.h"

#include <string.h>
#include <functional>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

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

// ---------------------------------------------------------------------------
// rlb_hexdump

// capture everything written to stdout while run() executes
static std::string captureStdout(const std::function<void()>& run) {
  char path[] = "/tmp/opencode/radiolib-hexdump-XXXXXX";
  int fd = mkstemp(path);
  BOOST_REQUIRE(fd >= 0);
  close(fd);

  fflush(stdout);
  int saved = dup(fileno(stdout));
  BOOST_REQUIRE(saved >= 0);
  BOOST_REQUIRE(freopen(path, "w", stdout) != nullptr);
  run();
  fflush(stdout);
  BOOST_REQUIRE(dup2(saved, fileno(stdout)) >= 0);
  close(saved);

  std::string out;
  char buf[256];
  FILE* f = fopen(path, "rb");
  BOOST_REQUIRE(f != nullptr);
  size_t n;
  while((n = fread(buf, 1, sizeof(buf), f)) > 0) {
    out.append(buf, n);
  }
  fclose(f);
  remove(path);
  return(out);
}

#if RADIOLIB_DEBUG
// independent reference for one rlb_hexdump output line (width 1):
// 8-hex-digit offset, 16 two-digit bytes, then a 16-column ASCII sidebar
static std::string hexdumpLine(uint32_t addr, const uint8_t* data, size_t len) {
  char tmp[16];
  std::string line;
  snprintf(tmp, sizeof(tmp), "%08" PRIx32 ": ", addr);
  line += tmp;
  for(size_t j = 0; j < len; j++) {
    snprintf(tmp, sizeof(tmp), "%02x ", data[j]);
    line += tmp;
  }
  for(size_t j = len; j < 16; j++) {
    line += "   ";
  }
  for(size_t j = 0; j < len; j++) {
    char c = ((data[j] < ' ') || (data[j] > '~')) ? '.' : (char)data[j];
    line += c;
  }
  // short lines are space-padded in the ASCII column
  while(line.size() < (len == 16 ? 74 : 76)) {
    line += ' ';
  }
  line += RADIOLIB_LINE_FEED;
  return(line);
}
#endif

BOOST_AUTO_TEST_CASE(Utils_Hexdump) {
  BOOST_TEST_MESSAGE("--- Test rlb_hexdump output format ---");

  // deterministic test data, mixed printable/non-printable
  uint8_t data[33];
  for(size_t i = 0; i < sizeof(data); i++) {
    data[i] = (uint8_t)(i * 13 + 7);
  }

#if RADIOLIB_DEBUG
  // an empty dump prints nothing
  BOOST_CHECK_EQUAL(captureStdout([&]() { rlb_hexdump(NULL, data, 0); }), std::string(""));

  // 33 bytes -> three lines, second line offset by 16, last line partial
  {
    std::string expected = hexdumpLine(0x00000000, &data[0], 16)
                         + hexdumpLine(0x00000010, &data[16], 16)
                         + hexdumpLine(0x00000020, &data[32], 1);
    BOOST_CHECK_EQUAL(captureStdout([&]() { rlb_hexdump(NULL, data, sizeof(data)); }), expected);
  }

  // the offset argument shifts the printed address and wraps around
  {
    const uint8_t one = 0x41;
    std::string expected = hexdumpLine(0xFFFFFFF0, &one, 1);
    BOOST_CHECK_EQUAL(captureStdout([&]() { rlb_hexdump(NULL, &one, 1, 0xFFFFFFF0); }), expected);
  }

  // ASCII sidebar: printable range 0x20..0x7E shown verbatim, everything else as '.'
  {
    const uint8_t ascii[] = { 0x00, 0x1F, 0x20, 'A', '~', 0x7F, 0xFF, '%' };
    std::string expected = hexdumpLine(0, ascii, sizeof(ascii));
    BOOST_CHECK_EQUAL(captureStdout([&]() { rlb_hexdump(NULL, ascii, sizeof(ascii)); }), expected);
    // the sidebar must contain the printable characters in order,
    // 0x20 is printable (rendered as space), 0x00/0x1F/0x7F/0xFF become '.'
    BOOST_CHECK_EQUAL(expected.substr(58, sizeof(ascii)), std::string(".. A~..%"));
  }

  // '%' in the data must not be interpreted as a printf format string
  {
    uint8_t percents[40];
    memset(percents, '%', sizeof(percents));
    std::string expected = hexdumpLine(0, percents, 16) + hexdumpLine(16, percents + 16, 16)
                         + hexdumpLine(32, percents + 32, 8);
    BOOST_CHECK_EQUAL(captureStdout([&]() { rlb_hexdump(NULL, percents, sizeof(percents)); }), expected);
  }

  // a level string is printed before the dump, prefixed by the debug timestamp
  {
    const uint8_t one = 0x41;
    std::string out = captureStdout([&]() { rlb_hexdump("DBG: ", &one, 1); });
    std::string body = hexdumpLine(0, &one, 1);
    body.erase(body.size() - strlen(RADIOLIB_LINE_FEED));
    BOOST_CHECK_EQUAL(out[0], '[');
    BOOST_CHECK(out.find(std::string("] DBG: ") + body + RADIOLIB_LINE_FEED) != std::string::npos);
  }
#else
  // without debug builds, rlb_hexdump is a no-op and must stay silent
  BOOST_CHECK_EQUAL(captureStdout([&]() { rlb_hexdump(NULL, data, sizeof(data)); }), std::string(""));
  BOOST_CHECK_EQUAL(captureStdout([&]() { rlb_hexdump("DBG: ", data, sizeof(data), 0x10); }), std::string(""));
#endif
}

BOOST_AUTO_TEST_SUITE_END()
