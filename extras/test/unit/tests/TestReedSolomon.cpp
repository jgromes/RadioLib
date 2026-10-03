// boost test header
#include <boost/test/unit_test.hpp>

// Reed-Solomon header
#include "utils/ReedSolomon.h"

// Test data: 223 bytes of known data (used for encoding/decoding tests)
// Using a simple pattern for verifiable results
static const uint8_t test_data[RADIOLIB_RS_NDATA] = {
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
  0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
  0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
  0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
  0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
  0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
  0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
  0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
  0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
  0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60,
  0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
  0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
  0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
  0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80,
  0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
  0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90,
  0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
  0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0,
  0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8,
  0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0,
  0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8,
  0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0,
  0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8,
  0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
  0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
  0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf
};

// Reference parity symbols computed from the rs8 implementation
// These are the expected parity symbols when encoding test_data with pad=0
static const uint8_t reference_parity[RADIOLIB_RS_NROOTS] = {
  0xdf, 0x8f, 0xf3, 0x42, 0x00, 0xb1, 0xb6, 0xe8,
  0xb0, 0x4f, 0x72, 0x81, 0x55, 0x39, 0xdf, 0x99,
  0x81, 0x96, 0x5e, 0xee, 0xf1, 0xc8, 0x06, 0x64,
  0xe5, 0x6c, 0xad, 0x3d, 0x62, 0x6b, 0xad, 0xf0
};

BOOST_AUTO_TEST_SUITE(suite_ReedSolomon)

BOOST_AUTO_TEST_CASE(ReedSolomon_Encoding) {
  BOOST_TEST_MESSAGE("--- Test ReedSolomon encoding ---");
  
  // Test with pad=0
  uint8_t parity[RADIOLIB_RS_NROOTS];
  rlb_encode_rs_8(test_data, parity, 0);
  BOOST_CHECK(memcmp(parity, reference_parity, RADIOLIB_RS_NROOTS) == 0);
}

BOOST_AUTO_TEST_CASE(ReedSolomon_Decoding_NoErrors) {
  BOOST_TEST_MESSAGE("--- Test ReedSolomon decoding with no errors ---");
  
  // create reference codeword
  uint8_t codeword[RADIOLIB_RS_BLOCK];
  memcpy(codeword, test_data, RADIOLIB_RS_NDATA);
  memcpy(&codeword[RADIOLIB_RS_NDATA], reference_parity, RADIOLIB_RS_NROOTS);
  
  // decode with no erasures
  int num_errors = rlb_decode_rs_8(codeword, NULL, 0, 0);
  BOOST_CHECK_EQUAL(num_errors, 0);
  
  // verify data is unchanged (no errors to correct)
  BOOST_CHECK(memcmp(codeword, test_data, RADIOLIB_RS_NDATA) == 0);
  BOOST_CHECK(memcmp(&codeword[RADIOLIB_RS_NDATA], reference_parity, RADIOLIB_RS_NROOTS) == 0);
}

BOOST_AUTO_TEST_CASE(ReedSolomon_Decoding_SingleError) {
  BOOST_TEST_MESSAGE("--- Test ReedSolomon decoding with single error ---");
  
  // create reference codeword
  uint8_t codeword[RADIOLIB_RS_BLOCK];
  memcpy(codeword, test_data, RADIOLIB_RS_NDATA);
  memcpy(&codeword[RADIOLIB_RS_NDATA], reference_parity, RADIOLIB_RS_NROOTS);
  
  // Introduce a single-byte error at position 10
  int error_pos = 10;
  uint8_t original_val = codeword[RADIOLIB_RS_NDATA+ error_pos];
  uint8_t error_val = 0xFF;
  codeword[RADIOLIB_RS_NDATA+ error_pos] = error_val;
  
  // Attempt to correct the error
  int num_errors = rlb_decode_rs_8(codeword, NULL, 0, 0);
  BOOST_CHECK_EQUAL(num_errors, 1);
  
  // Verify the error was corrected
  BOOST_CHECK_EQUAL(codeword[RADIOLIB_RS_NDATA+ error_pos], original_val);
}

BOOST_AUTO_TEST_CASE(ReedSolomon_Decoding_Erasure) {
  BOOST_TEST_MESSAGE("--- Test ReedSolomon decoding with erasure ---");
  
  // create a codeword with a known erasure
  uint8_t codeword[RADIOLIB_RS_BLOCK];
  memcpy(codeword, test_data, RADIOLIB_RS_NDATA);
  memcpy(&codeword[RADIOLIB_RS_NDATA], reference_parity, RADIOLIB_RS_NROOTS);
  
  // mark position 5 as an erasure (known bad position)
  int erasure_pos = 5;
  codeword[RADIOLIB_RS_NDATA+ erasure_pos] = 0x00; // Corrupt the data
  
  int erasurePositions[RADIOLIB_RS_NROOTS];
  erasurePositions[0] = RADIOLIB_RS_NDATA+ erasure_pos;
  
  // attempt to correct with erasure
  int num_errors = rlb_decode_rs_8(codeword, erasurePositions, 1, 0);
  BOOST_CHECK_EQUAL(num_errors, 1);
}

BOOST_AUTO_TEST_CASE(ReedSolomon_Decoding_TooManyErrors) {
  BOOST_TEST_MESSAGE("--- Test ReedSolomon decoding with too many errors ---");
    
  // create a codeword with too many errors (17 errors, more than 32/2=16)
  uint8_t codeword[RADIOLIB_RS_BLOCK];
  memcpy(codeword, test_data, RADIOLIB_RS_NDATA);
  memcpy(&codeword[RADIOLIB_RS_NDATA], reference_parity, RADIOLIB_RS_NROOTS);
  
  // introduce 17 errors
  for(int i = 0; i < 17; i++) {
    codeword[RADIOLIB_RS_NDATA+ i] ^= 0x01;
  }
  
  // attempt to correct - should fail
  int num_errors = rlb_decode_rs_8(codeword, NULL, 0, 0);
  BOOST_CHECK_EQUAL(num_errors, -1);
}

BOOST_AUTO_TEST_CASE(ReedSolomon_PadNonZero) {
  BOOST_TEST_MESSAGE("--- Test ReedSolomon with pad>0 ---");
  
  // test with pad=10 (10 pad symbols at the end)
  int pad = 10;
  uint8_t parity[RADIOLIB_RS_NROOTS];
  rlb_encode_rs_8(test_data, parity, pad);
  
  // decoding should work with pad=10
  uint8_t codeword[RADIOLIB_RS_BLOCK];
  memcpy(codeword, test_data, RADIOLIB_RS_NDATA - pad);
  memcpy(&codeword[RADIOLIB_RS_NDATA - pad], parity, RADIOLIB_RS_NROOTS);
  int num_errors = rlb_decode_rs_8(codeword, NULL, 0, pad);

  // should succeed with pad adjustment
  BOOST_CHECK(num_errors >= 0);
}

BOOST_AUTO_TEST_CASE(ReedSolomon_EncodeDecodeRoundtrip) {
  BOOST_TEST_MESSAGE("--- Test ReedSolomon encode/decode roundtrip ---");
  
  // create a data buffer
  uint8_t codeword[RADIOLIB_RS_BLOCK];
  memcpy(codeword, test_data, RADIOLIB_RS_NDATA);
  
  // encode
  rlb_encode_rs_8(codeword, &codeword[RADIOLIB_RS_NDATA], 0);
  
  // introduce a single error in the middle of the codeword
  int error_idx = RADIOLIB_RS_NDATA+ RADIOLIB_RS_NROOTS / 2;
  codeword[error_idx] ^= 0xFF;
  
  // decode
  int num_errors = rlb_decode_rs_8(codeword, NULL, 0, 0);
  BOOST_CHECK_EQUAL(num_errors, 1);
  
  // Verify the data was restored
  BOOST_CHECK(memcmp(codeword, test_data, RADIOLIB_RS_NDATA) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
