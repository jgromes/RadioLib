#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <RadioLib.h>
#include "utils/ReedSolomon.h"

static uint32_t fuzzGetU32(const uint8_t* p) {
  return((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
}

// reference convolutional encoder using the published LR-FHSS generator
// polynomials: rate 1/3 (0155, 0117, 0127), rate 1/2 (035, 023), octal,
// bit 0 = current input bit
static int fuzzParity(uint32_t x) {
  x ^= x >> 16; x ^= x >> 8; x ^= x >> 4; x ^= x >> 2; x ^= x >> 1;
  return(x & 1);
}

static size_t fuzzConvReference(const uint8_t* in, size_t inBits, uint8_t* out, uint8_t rate) {
  static const uint32_t gens1_3[3] = { 0155, 0117, 0127 };
  static const uint32_t gens1_2[2] = { 035, 023 };
  const uint32_t* gens = (rate == 2) ? gens1_2 : gens1_3;
  uint8_t numGens = (rate == 2) ? 2 : 3;

  uint32_t reg = 0;
  size_t bitCount = 0;
  uint32_t word = 0;
  for(size_t i = 0; i < inBits; i++) {
    uint8_t bit = (in[i / 8] >> (7 - (i % 8))) & 1;
    reg = ((reg << 1) | bit) & 0x7F;
    for(uint8_t g = 0; g < numGens; g++) {
      word = (word << 1) | (uint32_t)fuzzParity(reg & gens[g]);
      bitCount++;
      if(bitCount % 8 == 0) {
        *out++ = (uint8_t)word;
      }
    }
  }
  if(bitCount % 8) {
    *out++ = (uint8_t)(word << (8 - (bitCount % 8)));
  }
  return(bitCount);
}

// convolutional encoder against the independent reference
extern "C" int FuzzConvCode(const uint8_t* data, size_t size) {
  // 1 byte rate selector + payload
  if(size < 2) { return(0); }
  uint8_t rate = (data[0] & 0x01) ? 2 : 3;

  const uint8_t* in = &data[1];
  size_t inBits = (size - 1) * 8;

  uint8_t* out = new uint8_t[(inBits * rate) / 8 + 4];
  uint8_t* ref = new uint8_t[(inBits * rate) / 8 + 4];

  RadioLibConvCode conv;
  conv.begin(rate);
  size_t outBits = 0;
  assert(conv.encode(in, inBits, out, &outBits) == RADIOLIB_ERR_NONE);
  assert(outBits == inBits * rate);

  size_t refBits = fuzzConvReference(in, inBits, ref, rate);
  assert(refBits == outBits);
  // compare complete bytes only, the last partial byte may be padded differently
  assert(memcmp(out, ref, outBits / 8) == 0);

  delete[] out;
  delete[] ref;
  return(0);
}

// GF(2^5) syndromes over the POCSAG primitive polynomial x^5 + x^2 + 1,
// oracle for POCSAG BCH(31,21) code words produced by the encoder
static int fuzzGfExp[64], fuzzGfLog[64];
static bool fuzzGfReady = false;

static void fuzzGfInit() {
  int x = 1;
  for(int i = 0; i < 5; i++) { fuzzGfExp[i] = x; fuzzGfLog[x] = i; x <<= 1; }
  for(int i = 5; i < 31; i++) {
    x = fuzzGfExp[i - 1] << 1;
    if(x & 0x20) { x ^= 0x25; }
    fuzzGfExp[i] = x; fuzzGfLog[x] = i;
  }
  for(int i = 31; i < 64; i++) { fuzzGfExp[i] = fuzzGfExp[i - 31]; }
  fuzzGfReady = true;
}

static int fuzzGfMul(int a, int b) {
  if(a == 0 || b == 0) { return(0); }
  return(fuzzGfExp[fuzzGfLog[a] + fuzzGfLog[b]]);
}

static int fuzzSyndrome(uint32_t cw31, int i) {
  int a = fuzzGfExp[i];
  int s = 0;
  for(int b = 30; b >= 0; b--) {
    s = fuzzGfMul(s, a);
    if((cw31 >> b) & 1) { s ^= 1; }
  }
  return(s);
}

// every POCSAG code word produced by the encoder must have vanishing
// syndromes S1..S4, balanced parity and unchanged data bits
extern "C" int FuzzBCH(const uint8_t* data, size_t size) {
  if(size < 4) { return(0); }
  if(!fuzzGfReady) { fuzzGfInit(); }

  static RadioLibBCH bch;
  static bool bchStarted = false;
  if(!bchStarted) {
    bch.begin(RADIOLIB_PAGER_BCH_N, RADIOLIB_PAGER_BCH_K, RADIOLIB_PAGER_BCH_PRIMITIVE_POLY);
    bchStarted = true;
  }

  // consume the input in 4-byte data words
  for(size_t off = 0; off + 4 <= size; off += 4) {
    uint32_t dataword = fuzzGetU32(&data[off]);
    uint32_t codeword = bch.encode(dataword);

    assert((codeword & 0xFFFFF800) == (dataword & 0xFFFFF800));
    uint32_t cw31 = (codeword >> 1) & 0x7FFFFFFF;
    for(int i = 1; i <= 4; i++) {
      assert(fuzzSyndrome(cw31, i) == 0);
    }
    uint32_t ones = 0, v = cw31;
    while(v) { ones += v & 1; v >>= 1; }
    assert((codeword & 1) == (ones & 1));
  }
  return(0);
}

// Reed-Solomon(223,255): corrupt the code word arbitrarily, then decode.
// If decoding claims success, the data must match the original message.
// Note: the code word itself is NDATA + NROOTS = 255 bytes (RADIOLIB_RS_BLOCK is 256).
extern "C" int FuzzReedSolomon(const uint8_t* data, size_t size) {
  // 1 byte control + at least 1 corruption byte
  if(size < 2) { return(0); }

  const size_t cwLen = RADIOLIB_RS_NDATA + RADIOLIB_RS_NROOTS;
  uint8_t block[cwLen];

  // build the data section from fuzzer input (or a fixed pattern if input is short)
  size_t avail = size - 1;
  for(size_t i = 0; i < RADIOLIB_RS_NDATA; i++) {
    block[i] = (i < avail) ? data[1 + i] : (uint8_t)(i * 7 + 3);
  }

  // generate parity
  uint8_t parity[RADIOLIB_RS_NROOTS];
  rlb_encode_rs_8(block, parity, 0);
  memcpy(&block[RADIOLIB_RS_NDATA], parity, RADIOLIB_RS_NROOTS);

  // corrupt up to 16 distinct bytes (the correction capability of RS(223,255))
  // so that a successful decode is mathematically guaranteed to be exact
  size_t numCorrupt = 0;
  uint8_t clean[cwLen];
  memcpy(clean, block, cwLen);
  bool corrupted[cwLen] = { false };
  for(size_t i = 0; i < avail && numCorrupt < RADIOLIB_RS_NROOTS / 2; i += 2) {
    size_t pos = data[1 + i] % cwLen;
    if((i + 1 < avail) && (data[1 + i + 1] == 0)) { continue; }
    if(!corrupted[pos]) {
      block[pos] ^= (uint8_t)(0x5A + i);
      corrupted[pos] = true;
      numCorrupt++;
    }
  }

  // decode without erasure information
  int res = rlb_decode_rs_8(block, NULL, 0, 0);
  if(res >= 0) {
    // decoder claims correction succeeded - result must equal the clean block
    assert(memcmp(block, clean, cwLen) == 0);
  }
  return(0);
}
