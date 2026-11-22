#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <RadioLib.h>

static uint32_t fuzzGetU32(const uint8_t* p) {
  return((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
}

// independent bitwise reference implementation of the catalogue CRC definition,
// used as an oracle against RadioLibCRC
static uint32_t fuzzCrcReference(size_t size, uint32_t poly, uint32_t init, uint32_t out,
                                 bool refIn, bool refOut, const uint8_t* buff, size_t len) {
  uint32_t mask = (size == 32) ? 0xFFFFFFFF : ((uint32_t)1 << size) - 1;
  uint32_t msb = (uint32_t)1 << (size - 1);
  uint32_t crc = init & mask;
  for(size_t i = 0; i < len; i++) {
    uint8_t b = buff[i];
    if(refIn) {
      uint8_t r = 0;
      for(int k = 0; k < 8; k++) { r = (r << 1) | ((b >> k) & 1); }
      b = r;
    }
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
  crc ^= out & mask;
  if(refOut) {
    uint32_t r = 0;
    for(uint32_t k = 0; k < size; k++) { r = (r << 1) | ((crc >> k) & 1); }
    crc = r;
  }
  return(crc & mask);
}

// feed arbitrary CRC configurations and data, assert against the reference
extern "C" int FuzzCRC(const uint8_t* data, size_t size) {
  // 13 bytes configuration + at least 1 data byte
  if(size < 14) { return(0); }

  static const size_t sizes[] = { 8, 16, 24, 32 };
  size_t sizeBits = sizes[data[0] % 4];
  uint32_t mask = (sizeBits == 32) ? 0xFFFFFFFF : ((uint32_t)1 << sizeBits) - 1;

  RadioLibCRC crc;
  crc.size = (uint8_t)sizeBits;
  crc.poly = fuzzGetU32(&data[1]) & mask;
  crc.init = fuzzGetU32(&data[5]) & mask;
  crc.out = fuzzGetU32(&data[9]) & mask;
  crc.refIn = data[0] & 0x10;
  crc.refOut = data[0] & 0x20;
  bool useRef = data[0] & 0x40;

  const uint8_t* payload = &data[13];
  size_t len = size - 13;
  uint32_t res = crc.checksum(payload, len);

  if(useRef) {
    uint32_t ref = fuzzCrcReference(sizeBits, crc.poly, crc.init, crc.out, crc.refIn, crc.refOut, payload, len);
    assert(res == ref);
  }
  return(0);
}

// scramble/descramble round trip: descrambling must recover the original data
extern "C" int FuzzScrambler(const uint8_t* data, size_t size) {
  // 9 bytes header: 4 poly + 4 init + 1 control, rest is payload
  if(size < 10) { return(0); }

  uint32_t poly = fuzzGetU32(&data[0]);
  uint32_t init = fuzzGetU32(&data[4]);

  size_t len = size - 9;
  uint8_t* orig = new uint8_t[len];
  uint8_t* buf = new uint8_t[len];
  memcpy(orig, &data[9], len);

  // scramble, then descramble - original data must be restored
  memcpy(buf, orig, len);
  rlb_scrambler(buf, len, poly, init, true);
  rlb_scrambler(buf, len, poly, init, false);
  assert(memcmp(buf, orig, len) == 0);

  // a zero polynomial is a no-op
  if(poly != 0) {
    memset(buf, 0x5A, len);
    rlb_scrambler(buf, len, 0, init, true);
    for(size_t i = 0; i < len; i++) {
      assert(buf[i] == 0x5A);
    }
  }

  delete[] orig;
  delete[] buf;
  return(0);
}
