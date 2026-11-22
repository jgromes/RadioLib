// boost test header
#include <boost/test/unit_test.hpp>

// the crypto header
#include "utils/Cryptography.h"

// mock HAL
#include "ModuleFixture.hpp"

#include <string.h>

// test message, key and vectors
// from https://www.rfc-editor.org/rfc/rfc4493.html#section-4

static const unsigned char msg[64] = {
  0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
  0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
  0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
  0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
  0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
  0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
  0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
  0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
};

static unsigned char key[16] = {
  0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
  0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

static const unsigned char testVectEx1[] = {
  0xbb, 0x1d, 0x69, 0x29, 0xe9, 0x59, 0x37, 0x28,
  0x7f, 0xa3, 0x7d, 0x12, 0x9b, 0x75, 0x67, 0x46
};

static const unsigned char testVectEx2[] = {
  0x07, 0x0a, 0x16, 0xb4, 0x6b, 0x4d, 0x41, 0x44,
  0xf7, 0x9b, 0xdd, 0x9d, 0xd0, 0x4a, 0x28, 0x7c
};

static const unsigned char testVectEx3[] = {
  0xdf, 0xa6, 0x67, 0x47, 0xde, 0x9a, 0xe6, 0x30,
  0x30, 0xca, 0x32, 0x61, 0x14, 0x97, 0xc8, 0x27
};

static const unsigned char testVectEx4[] = {
  0x51, 0xf0, 0xbe, 0xbf, 0x7e, 0x3b, 0x9d, 0x92,
  0xfc, 0x49, 0x74, 0x17, 0x79, 0x36, 0x3c, 0xfe
};

static RadioLibSoftwareAES128 RadioLibAES128Instance;

BOOST_FIXTURE_TEST_SUITE(suite_Crypto, ModuleFixture)

BOOST_FIXTURE_TEST_CASE(Crypto_CMAC, ModuleFixture) {
  BOOST_TEST_MESSAGE("--- Test Crypto::CMAC ---");
  uint8_t cmac[RADIOLIB_AES128_BLOCK_SIZE];
  size_t testLen;

  BOOST_TEST_MESSAGE("--- Test Crypto::CMAC RFC 4493 chapter 4 Example 1: len = 0 ---");
  testLen = 0;
  hal->aes128 = &RadioLibAES128Instance;
  hal->aes128->init(key);
  hal->aes128->generateCMAC(msg, testLen, cmac);
  BOOST_TEST(memcmp(cmac, testVectEx1, RADIOLIB_AES128_BLOCK_SIZE) == 0);

  BOOST_TEST_MESSAGE("--- Test Crypto::CMAC RFC 4493 chapter 4 Example 2: len = 16 ---");
  testLen = 16;
  hal->aes128->init(key);
  hal->aes128->generateCMAC(msg, testLen, cmac);
  BOOST_TEST(memcmp(cmac, testVectEx2, RADIOLIB_AES128_BLOCK_SIZE) == 0);


  BOOST_TEST_MESSAGE("--- Test Crypto::CMAC RFC 4493 chapter 4 Example 2: len = 40 ---");
  testLen = 40;
  hal->aes128->init(key);
  hal->aes128->generateCMAC(msg, testLen, cmac);
  BOOST_TEST(memcmp(cmac, testVectEx3, RADIOLIB_AES128_BLOCK_SIZE) == 0);

  BOOST_TEST_MESSAGE("--- Test Crypto::CMAC RFC 4493 chapter 4 Example 2: len = 64 ---");
  testLen = 64;
  hal->aes128->init(key);
  hal->aes128->generateCMAC(msg, testLen, cmac);
  BOOST_TEST(memcmp(cmac, testVectEx4, RADIOLIB_AES128_BLOCK_SIZE) == 0);
}

BOOST_AUTO_TEST_SUITE_END()

// FIPS-197 Appendix C AES-128 ECB test vectors
static unsigned char ecbKey1[16] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

static const unsigned char ecbPlain1[16] = {
  0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};

static const unsigned char ecbCipher1[16] = {
  0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
  0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a
};

static const unsigned char ecbPlain2[16] = {
  0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
  0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
};

static const unsigned char ecbCipher2[16] = {
  0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
  0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97
};

BOOST_AUTO_TEST_SUITE(suite_CryptoECB)

BOOST_AUTO_TEST_CASE(CryptoECB_FIPS197) {
  BOOST_TEST_MESSAGE("--- Test AES-128 ECB against FIPS-197 Appendix C vectors ---");
  RadioLibSoftwareAES128 aes;
  uint8_t out[16];

  // FIPS-197 C.1: key 000102...0f
  BOOST_TEST_MESSAGE("--- Encrypt, FIPS-197 C.1 ---");
  aes.init(ecbKey1);
  BOOST_CHECK_EQUAL(aes.encryptECB(ecbPlain1, RADIOLIB_AES128_BLOCK_SIZE, out), (size_t)RADIOLIB_AES128_BLOCK_SIZE);
  BOOST_CHECK(memcmp(out, ecbCipher1, RADIOLIB_AES128_BLOCK_SIZE) == 0);

  BOOST_TEST_MESSAGE("--- Decrypt, FIPS-197 C.1 ---");
  aes.init(ecbKey1);
  BOOST_CHECK_EQUAL(aes.decryptECB(ecbCipher1, RADIOLIB_AES128_BLOCK_SIZE, out), (size_t)RADIOLIB_AES128_BLOCK_SIZE);
  BOOST_CHECK(memcmp(out, ecbPlain1, RADIOLIB_AES128_BLOCK_SIZE) == 0);

  // FIPS-197 C.2 first block: same key as the RFC 4493 CMAC vector
  BOOST_TEST_MESSAGE("--- Encrypt, FIPS-197 C.2 ---");
  aes.init(key);
  BOOST_CHECK_EQUAL(aes.encryptECB(ecbPlain2, RADIOLIB_AES128_BLOCK_SIZE, out), (size_t)RADIOLIB_AES128_BLOCK_SIZE);
  BOOST_CHECK(memcmp(out, ecbCipher2, RADIOLIB_AES128_BLOCK_SIZE) == 0);

  // multi-block encrypt/decrypt round trip
  BOOST_TEST_MESSAGE("--- Multi-block round trip ---");
  uint8_t plain[64], enc[64], dec[64];
  for(size_t i = 0; i < sizeof(plain); i++) {
    plain[i] = (uint8_t)i;
  }
  aes.init(key);
  BOOST_CHECK_EQUAL(aes.encryptECB(plain, sizeof(plain), enc), sizeof(plain));
  aes.init(key);
  BOOST_CHECK_EQUAL(aes.decryptECB(enc, sizeof(enc), dec), sizeof(dec));
  BOOST_CHECK(memcmp(dec, plain, sizeof(plain)) == 0);
}

BOOST_AUTO_TEST_CASE(CryptoECB_StreamingCMAC) {
  BOOST_TEST_MESSAGE("--- Test streaming CMAC against RFC 4493 vectors and one-shot ---");
  RadioLibSoftwareAES128 aes;
  uint8_t mac[RADIOLIB_AES128_BLOCK_SIZE];
  RadioLibCmacState st;

  // empty message streamed directly: RFC 4493 example 1
  aes.init(key);
  aes.initCMAC(&st);
  aes.finishCMAC(&st, mac);
  BOOST_CHECK(memcmp(mac, testVectEx1, RADIOLIB_AES128_BLOCK_SIZE) == 0);

  // 40-byte message in 24 + 16 chunks: RFC 4493 example 3
  aes.init(key);
  aes.initCMAC(&st);
  aes.updateCMAC(&st, msg, 24);
  aes.updateCMAC(&st, &msg[24], 16);
  aes.finishCMAC(&st, mac);
  BOOST_CHECK(memcmp(mac, testVectEx3, RADIOLIB_AES128_BLOCK_SIZE) == 0);

  // 64-byte message in one update: RFC 4493 example 4
  aes.init(key);
  aes.initCMAC(&st);
  aes.updateCMAC(&st, msg, 64);
  aes.finishCMAC(&st, mac);
  BOOST_CHECK(memcmp(mac, testVectEx4, RADIOLIB_AES128_BLOCK_SIZE) == 0);

  // streaming in arbitrary chunks must match the one-shot generateCMAC
  const size_t lengths[] = { 1, 15, 16, 17, 31, 40, 48, 64 };
  for(size_t len : lengths) {
    uint8_t oneShot[RADIOLIB_AES128_BLOCK_SIZE];
    aes.init(key);
    aes.generateCMAC(msg, len, oneShot);

    aes.init(key);
    aes.initCMAC(&st);
    for(size_t off = 0; off < len; off += 7) {
      aes.updateCMAC(&st, &msg[off], (len - off < 7) ? len - off : 7);
    }
    aes.finishCMAC(&st, mac);
    BOOST_CHECK(memcmp(mac, oneShot, RADIOLIB_AES128_BLOCK_SIZE) == 0);
  }
}

BOOST_AUTO_TEST_SUITE_END()
