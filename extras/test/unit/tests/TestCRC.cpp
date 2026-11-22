// boost test header
#include <boost/test/unit_test.hpp>

// CRC and Utils headers
#include "utils/CRC.h"
#include "utils/Utils.h"

#include <string.h>

// standard check field ("check" parameter of the CRC catalogue, "123456789")
static const uint8_t crcCheckVec[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
static const size_t crcCheckVecLen = 9;

// independent bitwise reference implementation of the catalogue CRC definition,
// used to cross-check RadioLibCRC for arbitrary parameter combinations
static uint32_t crcReference(size_t size, uint32_t poly, uint32_t init, uint32_t out,
                             bool refIn, bool refOut, const uint8_t* buff, size_t len) {
  uint32_t mask = (size == 32) ? 0xFFFFFFFF : ((uint32_t)1 << size) - 1;
  uint32_t msb = (uint32_t)1 << (size - 1);
  uint32_t crc = init;
  for(size_t i = 0; i < len; i++) {
    uint8_t b = refIn ? (uint8_t)rlb_reflect(buff[i], 8) : buff[i];
    crc ^= ((uint32_t)b) << (size - 8);
    for(int k = 0; k < 8; k++) {
      if(crc & msb) {
        crc = (crc << 1) ^ poly;
      } else {
        crc <<= 1;
      }
      crc &= mask;
    }
  }
  crc ^= out;
  if(refOut) {
    crc = rlb_reflect(crc, size);
  }
  return(crc & mask);
}

BOOST_AUTO_TEST_SUITE(suite_CRC)

BOOST_AUTO_TEST_CASE(CRC_CatalogCheckFields) {
  BOOST_TEST_MESSAGE("--- Test CRC against catalogue check fields for \"123456789\" ---");

  // CRC-8 (poly 0x07, init 0x00, no reflection) -> 0xF4
  {
    RadioLibCRC crc;
    crc.size = 8; crc.poly = 0x07; crc.init = 0x00; crc.out = 0x00;
    BOOST_CHECK_EQUAL(crc.checksum(crcCheckVec, crcCheckVecLen), (uint32_t)0xF4);
  }

  // CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no reflection) -> 0x29B1
  {
    RadioLibCRC crc;
    crc.size = 16; crc.poly = 0x1021; crc.init = 0xFFFF; crc.out = 0x0000;
    BOOST_CHECK_EQUAL(crc.checksum(crcCheckVec, crcCheckVecLen), (uint32_t)0x29B1);
  }

  // CRC-16/ARC (poly 0x8005, init 0x0000, reflected in/out) -> 0xBB3D
  {
    RadioLibCRC crc;
    crc.size = 16; crc.poly = 0x8005; crc.init = 0x0000; crc.out = 0x0000;
    crc.refIn = true; crc.refOut = true;
    BOOST_CHECK_EQUAL(crc.checksum(crcCheckVec, crcCheckVecLen), (uint32_t)0xBB3D);
  }

  // CRC-16/GENIBUS (poly 0x1021, init 0xFFFF, out 0xFFFF) -> 0xD64E
  {
    RadioLibCRC crc;
    crc.size = 16; crc.poly = 0x1021; crc.init = 0xFFFF; crc.out = 0xFFFF;
    BOOST_CHECK_EQUAL(crc.checksum(crcCheckVec, crcCheckVecLen), (uint32_t)0xD64E);
  }

  // CRC-32 (poly 0x04C11DB7, init 0xFFFFFFFF, reflected, out 0xFFFFFFFF) -> 0xCBF43926
  {
    RadioLibCRC crc;
    crc.size = 32; crc.poly = 0x04C11DB7; crc.init = 0xFFFFFFFF; crc.out = 0xFFFFFFFF;
    crc.refIn = true; crc.refOut = true;
    BOOST_CHECK_EQUAL(crc.checksum(crcCheckVec, crcCheckVecLen), (uint32_t)0xCBF43926);
  }

  // empty buffer returns the initial value
  {
    RadioLibCRC crc;
    crc.size = 16; crc.poly = 0x1021; crc.init = 0xFFFF; crc.out = 0x0000;
    BOOST_CHECK_EQUAL(crc.checksum(NULL, 0), (uint32_t)0xFFFF);
  }
}

BOOST_AUTO_TEST_CASE(CRC_MatchesReference) {
  BOOST_TEST_MESSAGE("--- Test CRC against independent bitwise reference ---");

  uint8_t data[37];
  for(size_t i = 0; i < sizeof(data); i++) {
    data[i] = (uint8_t)(i * 37 + 11);
  }

  struct { size_t size; uint32_t poly; uint32_t init; uint32_t out; bool refIn; bool refOut; } configs[] = {
    // CRC-8
    { 8,  0x07,     0x00000000, 0x00000000, false, false },
    // CRC-8/DARC
    { 8,  0x9B,     0x000000FF, 0x00000000, true,  false },
    // CRC-16/CCITT-FALSE
    { 16, 0x1021,   0x0000FFFF, 0x00000000, false, false },
    // CRC-16/ARC
    { 16, 0x8005,   0x00000000, 0x00000000, true,  true },
    // CRC-16/CCITT-TRUE (KERMIT)
    { 16, 0x1021,   0x0000FFFF, 0x00000000, true,  true },
    // CRC-24/OPENPGP
    { 24, 0x864CFB, 0x00B704CE, 0x00000000, false, false },
    // CRC-32
    { 32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true,  true },
    // CRC-32/BZIP2
    { 32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, false, false },
  };

  for(const auto& cfg : configs) {
    RadioLibCRC crc;
    crc.size = cfg.size; crc.poly = cfg.poly; crc.init = cfg.init; crc.out = cfg.out;
    crc.refIn = cfg.refIn; crc.refOut = cfg.refOut;
    uint32_t ref = crcReference(cfg.size, cfg.poly, cfg.init, cfg.out, cfg.refIn, cfg.refOut, data, sizeof(data));
    BOOST_CHECK_EQUAL(crc.checksum(data, sizeof(data)), ref);
  }
}

BOOST_AUTO_TEST_SUITE_END()
