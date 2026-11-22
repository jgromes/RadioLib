// boost test header
#include <boost/test/unit_test.hpp>

// AX.25 header
#include "protocols/AX25/AX25.h"

#include <string.h>

BOOST_AUTO_TEST_SUITE(suite_AX25Frame)

BOOST_AUTO_TEST_CASE(AX25Frame_CallsignHandling) {
  BOOST_TEST_MESSAGE("--- Test AX25Frame callsign copying and truncation ---");

  // normal callsigns are copied verbatim
  AX25Frame frame("N0CALL", 0, "W99999", 1, RADIOLIB_AX25_CONTROL_UNNUMBERED_FRAME);
  BOOST_CHECK_EQUAL(strcmp(frame.destCallsign, "N0CALL"), 0);
  BOOST_CHECK_EQUAL(strcmp(frame.srcCallsign, "W99999"), 0);
  BOOST_CHECK_EQUAL(frame.destSSID, 0);
  BOOST_CHECK_EQUAL(frame.srcSSID, 1);

  // callsigns longer than RADIOLIB_AX25_MAX_CALLSIGN_LEN are truncated
  AX25Frame longFrame("ABCDEFGHIX", 0, "S", 1, RADIOLIB_AX25_CONTROL_UNNUMBERED_FRAME);
  BOOST_CHECK_EQUAL(strcmp(longFrame.destCallsign, "ABCDEF"), 0);
  BOOST_CHECK_EQUAL(strcmp(longFrame.srcCallsign, "S"), 0);
  BOOST_CHECK_EQUAL(strlen(longFrame.destCallsign), (size_t)RADIOLIB_AX25_MAX_CALLSIGN_LEN);
}

BOOST_AUTO_TEST_CASE(AX25Frame_SetRepeaters) {
  BOOST_TEST_MESSAGE("--- Test AX25Frame repeater configuration ---");

  char rep1[] = "W999";
  char rep2[] = "N0CALL";
  char* repeaters[] = { rep1, rep2 };
  const uint8_t ssids[] = { 1, 2 };

  AX25Frame frame("N0CALL", 0, "W99999", 1, RADIOLIB_AX25_CONTROL_UNNUMBERED_FRAME);

  // too few / too many repeaters are rejected (AX.25 allows up to 8)
  BOOST_CHECK(frame.setRepeaters(repeaters, ssids, 0) != RADIOLIB_ERR_NONE);
  BOOST_CHECK(frame.setRepeaters(repeaters, ssids, 9) != RADIOLIB_ERR_NONE);

  // null configuration is rejected
  BOOST_CHECK(frame.setRepeaters(NULL, ssids, 2) != RADIOLIB_ERR_NONE);
  BOOST_CHECK(frame.setRepeaters(repeaters, NULL, 2) != RADIOLIB_ERR_NONE);

  // an over-long repeater callsign is rejected
  char bad[] = "TOOLONG7";
  char* badRepeaters[] = { bad };
  BOOST_CHECK(frame.setRepeaters(badRepeaters, ssids, 1) != RADIOLIB_ERR_NONE);

  // valid configuration is stored
  BOOST_CHECK_EQUAL(frame.setRepeaters(repeaters, ssids, 2), RADIOLIB_ERR_NONE);
  BOOST_CHECK_EQUAL(frame.numRepeaters, 2);
  BOOST_CHECK_EQUAL(strcmp(frame.repeaterCallsigns[0], "W999"), 0);
  BOOST_CHECK_EQUAL(strcmp(frame.repeaterCallsigns[1], "N0CALL"), 0);
  BOOST_CHECK_EQUAL(frame.repeaterSSIDs[0], 1);
  BOOST_CHECK_EQUAL(frame.repeaterSSIDs[1], 2);
}

BOOST_AUTO_TEST_CASE(AX25Frame_Assignment) {
  BOOST_TEST_MESSAGE("--- Test AX25Frame copy and self-assignment ---");

  char rep1[] = "W999";
  char rep2[] = "N0CALL";
  char* repeaters[] = { rep1, rep2 };
  const uint8_t ssids[] = { 1, 2 };

  AX25Frame src("N0CALL", 3, "W99999", 1, RADIOLIB_AX25_CONTROL_UNNUMBERED_FRAME);
  BOOST_CHECK_EQUAL(src.setRepeaters(repeaters, ssids, 2), RADIOLIB_ERR_NONE);
  src.setRecvSequence(4);
  src.setSendSequence(5);

  // copy assignment must replicate callsigns, repeaters and sequence numbers
  AX25Frame dst("SOMETHING", 0, "OTHER", 0, RADIOLIB_AX25_CONTROL_UNNUMBERED_FRAME);
  dst = src;
  BOOST_CHECK_EQUAL(strcmp(dst.destCallsign, "N0CALL"), 0);
  BOOST_CHECK_EQUAL(strcmp(dst.srcCallsign, "W99999"), 0);
  BOOST_CHECK_EQUAL(dst.numRepeaters, 2);
  BOOST_CHECK_EQUAL(strcmp(dst.repeaterCallsigns[0], "W999"), 0);
  BOOST_CHECK_EQUAL(strcmp(dst.repeaterCallsigns[1], "N0CALL"), 0);
  BOOST_CHECK_EQUAL(dst.repeaterSSIDs[1], 2);
  BOOST_CHECK_EQUAL(dst.rcvSeqNumber, 4);
  BOOST_CHECK_EQUAL(dst.sendSeqNumber, 5);

  // self-assignment must be a no-op and must not free then use its own buffers
  dst = dst;
  BOOST_CHECK_EQUAL(dst.numRepeaters, 2);
  BOOST_CHECK_EQUAL(strcmp(dst.destCallsign, "N0CALL"), 0);
  BOOST_CHECK_EQUAL(strcmp(dst.repeaterCallsigns[0], "W999"), 0);
}

BOOST_AUTO_TEST_CASE(AX25Frame_SequenceNumbers) {
  BOOST_TEST_MESSAGE("--- Test AX25Frame sequence number accessors ---");
  AX25Frame frame("N0CALL", 0, "W99999", 1, RADIOLIB_AX25_CONTROL_UNNUMBERED_FRAME);
  frame.setRecvSequence(7);
  frame.setSendSequence(63);
  BOOST_CHECK_EQUAL(frame.rcvSeqNumber, 7);
  BOOST_CHECK_EQUAL(frame.sendSeqNumber, 63);
}

BOOST_AUTO_TEST_SUITE_END()
