#include "SSDV.h"

#include <string.h>

#if !RADIOLIB_EXCLUDE_SSDV

#include "../../utils/CRC.h"
#include "../../utils/ReedSolomon.h"

/*
  Encoder and decoder derived from ssdv by Philip Heron <phil@sanslogic.co.uk>
  https://github.com/fsphil/ssdv, Copyright 2011-2016, GPL-3.0

  The encoder parses the JPEG, re-quantises the DCT coefficients to one of the standard
  tables and re-encodes them with the standard Huffman tables, so only scan data is transmitted.
  The first MCU of each packet is byte-aligned and DC-absolute, so the decoder can resume after lost packets.
*/

// maximum size of the DQT and DHT tables, plus room for marker data
#define RADIOLIB_SSDV_TBL_LEN                                   (546)
#define RADIOLIB_SSDV_HBUFF_LEN                                 (16)

// JPEG markers
#define RADIOLIB_SSDV_JPEG_TEM                                  (0xFF01)
#define RADIOLIB_SSDV_JPEG_SOF0                                 (0xFFC0)
#define RADIOLIB_SSDV_JPEG_SOF2                                 (0xFFC2)
#define RADIOLIB_SSDV_JPEG_DHT                                  (0xFFC4)
#define RADIOLIB_SSDV_JPEG_RST0                                 (0xFFD0)
#define RADIOLIB_SSDV_JPEG_RST7                                 (0xFFD7)
#define RADIOLIB_SSDV_JPEG_SOI                                  (0xFFD8)
#define RADIOLIB_SSDV_JPEG_EOI                                  (0xFFD9)
#define RADIOLIB_SSDV_JPEG_SOS                                  (0xFFDA)
#define RADIOLIB_SSDV_JPEG_DQT                                  (0xFFDB)
#define RADIOLIB_SSDV_JPEG_DRI                                  (0xFFDD)
#define RADIOLIB_SSDV_JPEG_APP0                                 (0xFFE0)
#define RADIOLIB_SSDV_JPEG_COM                                  (0xFFFE)

// codec results
enum {
  SSDV_CODEC_OK = 0,
  SSDV_CODEC_FEED_ME,
  SSDV_CODEC_HAVE_PACKET,
  SSDV_CODEC_BUFFER_FULL,
  SSDV_CODEC_EOI,
  SSDV_CODEC_ERROR,
};

// codec states
enum {
  SSDV_STATE_MARKER = 0,
  SSDV_STATE_MARKER_LEN,
  SSDV_STATE_MARKER_DATA,
  SSDV_STATE_HUFF,
  SSDV_STATE_INT,
  SSDV_STATE_EOI,
};

struct SSDVState_t {
  // packet configuration
  uint8_t type;
  uint16_t payloadLen;
  uint16_t crcDataLen;

  // image information
  uint16_t width;
  uint16_t height;
  uint32_t callsign;
  uint8_t imageId;
  uint16_t packetId;
  uint8_t mcuMode;
  uint16_t mcuId;
  uint16_t mcuCount;
  uint8_t quality;
  uint16_t packetMcuId;
  uint8_t packetMcuOffset;

  // input bytes and bits
  const uint8_t* inp;
  size_t inLen;
  size_t inSkip;
  uint32_t workBits;
  uint8_t workLen;

  // output bytes and bits
  uint8_t* out;
  uint8_t* outp;
  size_t outLen;
  bool outStuff;
  uint32_t outBits;
  uint8_t outBitsLen;

  // JPEG state machine
  uint8_t state;
  bool encoding;
  uint16_t marker;
  uint16_t markerLen;
  uint8_t* markerData;
  uint16_t markerDataLen;
  bool greyscale;
  uint8_t component;      // 0 = Y, 1 = Cb, 2 = Cr
  uint8_t ycParts;        // number of Y blocks per MCU
  uint8_t mcuPart;        // 0 - 3 = Y, then Cb, Cr
  uint8_t acPart;         // 0 = DC, 1 - 63 = AC
  int32_t dc[3];
  int32_t adc[3];
  uint8_t acRle;
  uint8_t acAccRle;
  uint16_t dri;
  uint32_t resetMcu;
  uint32_t nextResetMcu;
  uint8_t needBits;

  // source (input) and destination (output) Huffman and quantisation tables
  uint8_t srcTbls[RADIOLIB_SSDV_TBL_LEN + RADIOLIB_SSDV_HBUFF_LEN];
  uint8_t* srcDht[2][2];
  uint8_t* srcDqt[2];
  uint16_t srcTblLen;
  uint8_t dstTbls[RADIOLIB_SSDV_TBL_LEN];
  uint8_t* dstDht[2][2];
  uint8_t* dstDqt[2];
  uint16_t dstTblLen;
};

// JFIF APP0 and SOS header data
static const uint8_t ssdvApp0[14] = { 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x01, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00 };
static const uint8_t ssdvSos[10] = { 0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11, 0x00, 0x3F, 0x00 };

// quantisation table scaling factors for quality levels 0 - 7
static const uint16_t ssdvDqtScales[8] = { 5000, 357, 172, 116, 100, 58, 28, 0 };

// standard luminance and chrominance quantisation tables
static const uint8_t ssdvDqt0[65] = {
  0x00,0x10,0x0C,0x0C,0x0E,0x0C,0x0A,0x10,0x0E,0x0E,0x0E,0x12,0x12,0x10,0x14,0x18,
  0x28,0x1A,0x18,0x16,0x16,0x18,0x32,0x24,0x26,0x1E,0x28,0x3A,0x34,0x3E,0x3C,0x3A,
  0x34,0x38,0x38,0x40,0x48,0x5C,0x4E,0x40,0x44,0x58,0x46,0x38,0x38,0x50,0x6E,0x52,
  0x58,0x60,0x62,0x68,0x68,0x68,0x3E,0x4E,0x72,0x7A,0x70,0x64,0x78,0x5C,0x66,0x68,
  0x64,
};

static const uint8_t ssdvDqt1[65] = {
  0x01,0x12,0x12,0x12,0x16,0x16,0x16,0x30,0x1A,0x1A,0x30,0x64,0x42,0x38,0x42,0x64,
  0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
  0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
  0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,
  0x64,
};

// standard Huffman tables (DC/AC, luminance/chrominance)
static const uint8_t ssdvDht00[29] = {
  0x00,0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,
};

static const uint8_t ssdvDht01[29] = {
  0x01,0x00,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,
};

static const uint8_t ssdvDht10[179] = {
  0x10,0x00,0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,0x01,
  0x7D,0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,
  0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,
  0xF0,0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,
  0x28,0x29,0x2A,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,
  0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,
  0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,
  0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,
  0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,
  0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,
  0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,
  0xF8,0xF9,0xFA,
};

static const uint8_t ssdvDht11[179] = {
  0x11,0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,0x02,
  0x77,0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,
  0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,0x52,
  0xF0,0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,
  0x26,0x27,0x28,0x29,0x2A,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,
  0x48,0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,
  0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x82,0x83,0x84,0x85,0x86,
  0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,
  0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,
  0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,
  0xDA,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,
  0xF8,0xF9,0xFA,
};

// ------------------------------------------------------------------------------------------------
// helpers
// ------------------------------------------------------------------------------------------------

static uint32_t ssdvCrc32(const uint8_t* data, size_t len) {
  RadioLibCRCInstance.size = 32;
  RadioLibCRCInstance.poly = 0x04C11DB7;
  RadioLibCRCInstance.init = 0xFFFFFFFF;
  RadioLibCRCInstance.out = 0xFFFFFFFF;
  RadioLibCRCInstance.refIn = true;
  RadioLibCRCInstance.refOut = true;
  return(RadioLibCRCInstance.checksum(data, len));
}

static uint32_t ssdvGetU32(const uint8_t* p) {
  return(((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3]);
}

static void ssdvSetU32(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x >> 24);
  p[1] = (uint8_t)(x >> 16);
  p[2] = (uint8_t)(x >> 8);
  p[3] = (uint8_t)x;
}

static uint16_t ssdvPayloadLen(uint8_t type) {
  return(type == RADIOLIB_SSDV_TYPE_NORMAL ? RADIOLIB_SSDV_PAYLOAD_LEN_FEC : RADIOLIB_SSDV_PAYLOAD_LEN_NOFEC);
}

// check CRC of a packet with given type, bytes 1 to end of payload
static bool ssdvCheckCrc(const uint8_t* pkt, uint8_t type) {
  uint16_t len = RADIOLIB_SSDV_HEADER_LEN + ssdvPayloadLen(type) - 1;
  return(ssdvCrc32(&pkt[1], len) == ssdvGetU32(&pkt[1 + len]));
}

// base-40 callsign encoding: 0 = '-', 1 - 10 = '0' - '9', 14 - 39 = 'A' - 'Z'
static uint32_t ssdvEncodeCallsign(const char* callsign) {
  size_t len = strlen(callsign);
  if(len > RADIOLIB_SSDV_CALLSIGN_MAX_LEN) {
    len = RADIOLIB_SSDV_CALLSIGN_MAX_LEN;
  }

  // encode backwards, first character ends up least significant
  uint32_t x = 0;
  while(len--) {
    char c = callsign[len];
    x *= 40;
    if(c >= 'A' && c <= 'Z') {
      x += c - 'A' + 14;
    } else if(c >= 'a' && c <= 'z') {
      x += c - 'a' + 14;
    } else if(c >= '0' && c <= '9') {
      x += c - '0' + 1;
    }
  }
  return(x);
}

static void ssdvDecodeCallsign(char* callsign, uint32_t code) {
  char* c = callsign;
  if(code <= 0xF423FFFF) {
    for(; code; c++, code /= 40) {
      uint8_t s = code % 40;
      if(s == 0) {
        *c = '-';
      } else if(s < 11) {
        *c = '0' + s - 1;
      } else if(s < 14) {
        *c = '-';
      } else {
        *c = 'A' + s - 14;
      }
    }
  }
  *c = '\0';
}

// integer division with rounding
static int32_t ssdvDivRound(int32_t i, int32_t div) {
  i = i * 2 / div;
  if(i & 1) {
    i += (i > 0 ? 1 : -1);
  }
  return(i / 2);
}

// ------------------------------------------------------------------------------------------------
// tables
// ------------------------------------------------------------------------------------------------

static void ssdvScaleDqt(uint8_t* dst, const uint8_t* table, uint8_t quality) {
  // table ID, then 64 scaled coefficients
  *dst++ = *table++;
  uint16_t scale = ssdvDqtScales[quality > RADIOLIB_SSDV_QUALITY_MAX ? RADIOLIB_SSDV_QUALITY_MAX : quality];
  for(uint8_t i = 0; i < 64; i++) {
    uint32_t t = ((uint32_t)*table++ * scale + 50) / 100;
    *dst++ = (t == 0) ? 1 : (t > 255 ? 255 : (uint8_t)t);
  }
}

// append a table to a table buffer, scaling it if it is a quantisation table
static uint8_t* ssdvAddTbl(uint8_t* tbls, uint16_t* tblLen, size_t cap, const uint8_t* src, size_t n, bool dqt, uint8_t quality) {
  if(*tblLen + n > cap) {
    return(NULL);
  }
  uint8_t* r = &tbls[*tblLen];
  if(dqt) {
    ssdvScaleDqt(r, src, quality);
  } else {
    memcpy(r, src, n);
  }
  *tblLen += n;
  return(r);
}

#define ssdvSrcTbl(S, SRC, N, DQT, Q) ssdvAddTbl((S)->srcTbls, &(S)->srcTblLen, sizeof((S)->srcTbls), SRC, N, DQT, Q)
#define ssdvDstTbl(S, SRC, N, DQT, Q) ssdvAddTbl((S)->dstTbls, &(S)->dstTblLen, sizeof((S)->dstTbls), SRC, N, DQT, Q)

static void ssdvLoadDstDht(SSDVState_t* s) {
  s->dstDht[0][0] = ssdvDstTbl(s, ssdvDht00, sizeof(ssdvDht00), false, 0);
  s->dstDht[0][1] = ssdvDstTbl(s, ssdvDht01, sizeof(ssdvDht01), false, 0);
  s->dstDht[1][0] = ssdvDstTbl(s, ssdvDht10, sizeof(ssdvDht10), false, 0);
  s->dstDht[1][1] = ssdvDstTbl(s, ssdvDht11, sizeof(ssdvDht11), false, 0);
}

// current quantisation factors and conversion between source and destination tables
static uint8_t ssdvSrcQ(SSDVState_t* s) {
  return(s->srcDqt[s->component ? 1 : 0][1 + s->acPart]);
}

static uint8_t ssdvDstQ(SSDVState_t* s) {
  return(s->dstDqt[s->component ? 1 : 0][1 + s->acPart]);
}

static int32_t ssdvAdjA(SSDVState_t* s, int32_t i) {
  return(ssdvSrcQ(s) == ssdvDstQ(s) ? i : ssdvDivRound(i, ssdvDstQ(s)));
}

static int32_t ssdvAdjU(SSDVState_t* s, int32_t i) {
  return(ssdvSrcQ(s) == ssdvDstQ(s) ? i : i * ssdvSrcQ(s));
}

static int32_t ssdvAdjB(SSDVState_t* s, int32_t i) {
  return(ssdvSrcQ(s) == ssdvDstQ(s) ? i : ssdvDivRound(i * ssdvSrcQ(s), ssdvDstQ(s)));
}

// ------------------------------------------------------------------------------------------------
// Huffman and bit I/O
// ------------------------------------------------------------------------------------------------

static uint8_t ssdvDhtLookup(SSDVState_t* s, uint8_t* symbol, uint8_t* width) {
  const uint8_t* dht = s->srcDht[s->acPart ? 1 : 0][s->component ? 1 : 0];
  const uint8_t* ss = &dht[17];
  uint16_t code = 0;
  for(uint8_t cw = 1; cw <= 16; cw++) {
    if(cw > s->workLen) {
      return(SSDV_CODEC_FEED_ME);
    }
    for(uint8_t n = dht[cw]; n > 0; n--, ss++, code++) {
      if((s->workBits >> (s->workLen - cw)) == code) {
        *symbol = *ss;
        *width = cw;
        return(SSDV_CODEC_OK);
      }
    }
    code <<= 1;
  }
  return(SSDV_CODEC_ERROR);
}

static uint8_t ssdvDhtLookupSymbol(SSDVState_t* s, uint8_t symbol, uint16_t* bits, uint8_t* width) {
  const uint8_t* dht = s->dstDht[s->acPart ? 1 : 0][s->component ? 1 : 0];
  const uint8_t* ss = &dht[17];
  uint16_t code = 0;
  for(uint8_t cw = 1; cw <= 16; cw++) {
    for(uint8_t n = dht[cw]; n > 0; n--, ss++, code++) {
      if(*ss == symbol) {
        *bits = code;
        *width = cw;
        return(SSDV_CODEC_OK);
      }
    }
    code <<= 1;
  }
  return(SSDV_CODEC_ERROR);
}

static uint8_t ssdvOutBits(SSDVState_t* s, uint16_t bits, uint8_t len) {
  if(len) {
    s->outBits <<= len;
    s->outBits |= bits & (((uint32_t)1 << len) - 1);
    s->outBitsLen += len;
  }

  while(s->outBitsLen >= 8 && s->outLen > 0) {
    uint8_t b = s->outBits >> (s->outBitsLen - 8);
    *(s->outp++) = b;
    s->outBitsLen -= 8;
    s->outLen--;

    // JPEG byte stuffing: 0xFF is followed by 0x00
    if(s->outStuff && b == 0xFF) {
      s->outBits &= ((uint32_t)1 << s->outBitsLen) - 1;
      s->outBitsLen += 8;
    }
  }

  return(s->outLen ? SSDV_CODEC_OK : SSDV_CODEC_BUFFER_FULL);
}

// pad to byte boundary with 1-bits
static void ssdvOutSync(SSDVState_t* s) {
  uint8_t b = s->outBitsLen % 8;
  if(b) {
    ssdvOutBits(s, 0xFF, 8 - b);
  }
}

static void ssdvOutInt(SSDVState_t* s, uint8_t rle, int32_t value) {
  // value in JPEG magnitude/bits representation
  int32_t bits = value;
  uint8_t width = 0;
  for(int32_t v = (value < 0) ? -value : value; v; v >>= 1) {
    width++;
  }
  if(bits < 0) {
    bits = -bits ^ (((int32_t)1 << width) - 1);
  }

  uint16_t huffBits = 0;
  uint8_t huffLen = 0;
  if(ssdvDhtLookupSymbol(s, (rle << 4) | (width & 0x0F), &huffBits, &huffLen) != SSDV_CODEC_OK) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: no Huffman code for %ld (rle %d)", (long)value, rle);
  }
  ssdvOutBits(s, huffBits, huffLen);
  if(width) {
    ssdvOutBits(s, (uint16_t)bits, width);
  }
}

// write empty DC and AC for the current component
static void ssdvOutEmptyBlock(SSDVState_t* s) {
  s->component = (s->mcuPart < s->ycParts) ? 0 : s->mcuPart - s->ycParts + 1;
  s->acPart = 0;
  ssdvOutInt(s, 0, 0);
  s->acPart = 1;
  ssdvOutInt(s, 0, 0);
}

static bool ssdvIsAbsoluteDc(SSDVState_t* s) {
  return(s->resetMcu == s->mcuId && (s->mcuPart == 0 || s->mcuPart >= s->ycParts));
}

static void ssdvDropWorkBits(SSDVState_t* s, uint8_t n) {
  s->workLen -= n;
  s->workBits &= ((uint32_t)1 << s->workLen) - 1;
}

// ------------------------------------------------------------------------------------------------
// scan data transcoder, shared by encoder and decoder
// ------------------------------------------------------------------------------------------------

static uint8_t ssdvProcess(SSDVState_t* s) {
  if(s->state == SSDV_STATE_HUFF) {
    if(s->mcuPart == 0 && s->acPart == 0 && s->nextResetMcu > s->resetMcu) {
      s->resetMcu = s->nextResetMcu;
    }

    uint8_t symbol, width;
    uint8_t r = ssdvDhtLookup(s, &symbol, &width);
    if(r != SSDV_CODEC_OK) {
      return(r);
    }

    if(s->acPart == 0) {
      // DC
      if(symbol == 0x00) {
        // no change from previous block
        if(ssdvIsAbsoluteDc(s)) {
          if(s->encoding) {
            ssdvOutInt(s, 0, s->adc[s->component]);
          } else {
            ssdvOutInt(s, 0, -s->dc[s->component]);
            s->dc[s->component] = 0;
          }
        } else {
          ssdvOutInt(s, 0, 0);
        }
        s->acPart++;
      } else {
        s->state = SSDV_STATE_INT;
        s->needBits = symbol;
      }

    } else {
      // AC
      s->acRle = 0;
      if(symbol == 0x00) {
        // end of block
        ssdvOutInt(s, 0, 0);
        s->acPart = 64;
      } else if(symbol == 0xF0) {
        // 16 zeros
        ssdvOutInt(s, 15, 0);
        s->acPart += 16;
      } else {
        s->state = SSDV_STATE_INT;
        s->acRle = symbol >> 4;
        s->acPart += s->acRle;
        s->needBits = symbol & 0x0F;
      }
    }

    ssdvDropWorkBits(s, width);

  } else if(s->state == SSDV_STATE_INT) {
    if(s->workLen < s->needBits) {
      return(SSDV_CODEC_FEED_ME);
    }

    // decode JPEG integer
    int32_t i = s->workBits >> (s->workLen - s->needBits);
    int32_t m = ((int32_t)1 << s->needBits) - 1;
    if(i <= (m >> 1)) {
      i = -(i ^ m);
    }

    if(s->acPart == 0) {
      // DC
      if(ssdvIsAbsoluteDc(s)) {
        if(s->encoding) {
          s->dc[s->component] += ssdvAdjU(s, i);
          s->adc[s->component] = ssdvAdjA(s, s->dc[s->component]);
          ssdvOutInt(s, 0, s->adc[s->component]);
        } else {
          ssdvOutInt(s, 0, i - s->dc[s->component]);
          s->dc[s->component] = i;
        }
      } else if(s->encoding) {
        s->dc[s->component] += ssdvAdjU(s, i);
        i = ssdvAdjA(s, s->dc[s->component]);
        ssdvOutInt(s, 0, i - s->adc[s->component]);
        s->adc[s->component] = i;
      } else {
        s->dc[s->component] += ssdvAdjU(s, i);
        ssdvOutInt(s, 0, i);
      }

    } else {
      // AC
      if((i = ssdvAdjB(s, i)) != 0) {
        s->acAccRle += s->acRle;
        while(s->acAccRle >= 16) {
          ssdvOutInt(s, 15, 0);
          s->acAccRle -= 16;
        }
        ssdvOutInt(s, s->acAccRle, i);
        s->acAccRle = 0;
      } else if(s->acPart >= 63) {
        // coefficient was quantised to zero, end the block
        ssdvOutInt(s, 0, 0);
        s->acAccRle = 0;
      } else {
        s->acAccRle += s->acRle + 1;
      }
    }

    s->acPart++;
    s->state = SSDV_STATE_HUFF;
    ssdvDropWorkBits(s, s->needBits);
  }

  if(s->acPart >= 64) {
    s->mcuPart++;

    // greyscale images are sent as colour, pad with empty chroma blocks
    if(s->greyscale && s->mcuPart == s->ycParts) {
      for(; s->mcuPart < s->ycParts + 2; s->mcuPart++) {
        ssdvOutEmptyBlock(s);
      }
    }

    // end of MCU
    if(s->mcuPart == s->ycParts + 2) {
      s->mcuPart = 0;
      s->mcuId++;

      if(s->mcuId >= s->mcuCount) {
        ssdvOutSync(s);
        return(SSDV_CODEC_EOI);
      }

      // encoder: first MCU of each packet is byte-aligned and recorded in the header
      if(s->encoding && s->packetMcuId == RADIOLIB_SSDV_MCU_NONE) {
        ssdvOutSync(s);
        s->nextResetMcu = s->mcuId;
        s->packetMcuId = s->mcuId;
        s->packetMcuOffset = s->payloadLen - s->outLen + ((s->outBitsLen + 7) / 8);
      }

      // restart marker follows
      if(s->dri > 0 && s->mcuId % s->dri == 0) {
        s->state = SSDV_STATE_MARKER;
        return(SSDV_CODEC_FEED_ME);
      }
    }

    s->component = (s->mcuPart < s->ycParts) ? 0 : s->mcuPart - s->ycParts + 1;
    s->acPart = 0;
    s->acAccRle = 0;
  }

  return(s->outLen ? SSDV_CODEC_OK : SSDV_CODEC_BUFFER_FULL);
}

// ------------------------------------------------------------------------------------------------
// encoder
// ------------------------------------------------------------------------------------------------

static uint8_t ssdvHaveMarker(SSDVState_t* s) {
  switch(s->marker) {
    case RADIOLIB_SSDV_JPEG_SOF0:
    case RADIOLIB_SSDV_JPEG_SOS:
    case RADIOLIB_SSDV_JPEG_DRI:
    case RADIOLIB_SSDV_JPEG_DHT:
    case RADIOLIB_SSDV_JPEG_DQT:
      // copy the marker data before processing
      if(s->markerLen == 0 || s->markerLen > sizeof(s->srcTbls) - s->srcTblLen) {
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: marker %04X too long", s->marker);
        return(SSDV_CODEC_ERROR);
      }
      s->markerData = &s->srcTbls[s->srcTblLen];
      s->markerDataLen = 0;
      s->state = SSDV_STATE_MARKER_DATA;
      break;

    case RADIOLIB_SSDV_JPEG_SOF2:
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: progressive JPEG not supported");
      return(SSDV_CODEC_ERROR);

    case RADIOLIB_SSDV_JPEG_EOI:
      s->state = SSDV_STATE_EOI;
      break;

    default:
      if(s->marker >= RADIOLIB_SSDV_JPEG_RST0 && s->marker <= RADIOLIB_SSDV_JPEG_RST7) {
        // restart: reset DC predictors and bit buffer
        s->dc[0] = s->dc[1] = s->dc[2] = 0;
        s->mcuPart = s->acPart = s->component = 0;
        s->acRle = s->acAccRle = 0;
        s->workBits = s->workLen = 0;
        s->state = SSDV_STATE_HUFF;
      } else {
        // skip other markers
        s->inSkip = s->markerLen;
        s->state = SSDV_STATE_MARKER;
      }
      break;
  }

  return(SSDV_CODEC_OK);
}

static uint8_t ssdvHaveMarkerData(SSDVState_t* s) {
  uint8_t* d = s->markerData;
  size_t l = s->markerLen;

  switch(s->marker) {
    case RADIOLIB_SSDV_JPEG_SOF0: {
      if(l < 6 || d[0] != 8 || (d[5] != 1 && d[5] != 3) || l < (size_t)(6 + 3*d[5])) {
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: must be 8-bit with 1 or 3 components");
        return(SSDV_CODEC_ERROR);
      }

      s->width = ((uint16_t)d[3] << 8) | d[4];
      s->height = ((uint16_t)d[1] << 8) | d[2];
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: %dx%d, %d components", s->width, s->height, d[5]);
      if(s->width > 4080 || s->height > 4080 || (s->width & 0x0F) || (s->height & 0x0F)) {
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: dimensions must be multiples of 16, max. 4080");
        return(SSDV_CODEC_ERROR);
      }

      for(uint8_t i = 0; i < d[5]; i++) {
        const uint8_t* dq = &d[i*3 + 6];
        if(dq[0] != i + 1) {
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: components out of order");
          return(SSDV_CODEC_ERROR);
        }

        // Y may be subsampled, Cb and Cr must be 1x1
        if(dq[0] == 1) {
          switch(dq[1]) {
            case 0x22: s->mcuMode = 0; s->ycParts = 4; break;
            case 0x12: s->mcuMode = 1; s->ycParts = 2; break;
            case 0x21: s->mcuMode = 2; s->ycParts = 2; break;
            case 0x11: s->mcuMode = 3; s->ycParts = 1; break;
            default:
              RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: unsupported Y sampling %02X", dq[1]);
              return(SSDV_CODEC_ERROR);
          }
        } else if(dq[1] != 0x11) {
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: chroma sampling must be 1x1");
          return(SSDV_CODEC_ERROR);
        }
      }

      // greyscale is converted to 2x1 colour
      if(d[5] == 1) {
        s->greyscale = true;
        s->mcuMode = 2;
        s->ycParts = 2;
      }

      uint32_t mcus = 0;
      switch(s->mcuMode) {
        case 0: mcus = (uint32_t)(s->width >> 4) * (s->height >> 4); break;
        case 1: mcus = (uint32_t)(s->width >> 4) * (s->height >> 3); break;
        case 2: mcus = (uint32_t)(s->width >> 3) * (s->height >> 4); break;
        case 3: mcus = (uint32_t)(s->width >> 3) * (s->height >> 3); break;
      }
      if(mcus > 0xFFFF) {
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: too many MCU blocks (%lu)", (unsigned long)mcus);
        return(SSDV_CODEC_ERROR);
      }
      s->mcuCount = mcus;
    } break;

    case RADIOLIB_SSDV_JPEG_SOS:
      if(d[0] != 1 && d[0] != 3) {
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: scan must have 1 or 3 components");
        return(SSDV_CODEC_ERROR);
      }
      for(uint8_t i = 0; i < d[0]; i++) {
        if(d[i*2 + 1] != i + 1) {
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: scan components out of order");
          return(SSDV_CODEC_ERROR);
        }
      }
      if(!s->srcDqt[0] || (d[0] > 1 && !s->srcDqt[1]) ||
         !s->srcDht[0][0] || !s->srcDht[1][0] || (d[0] > 1 && (!s->srcDht[0][1] || !s->srcDht[1][1]))) {
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: missing DQT or DHT table");
        return(SSDV_CODEC_ERROR);
      }

      // scan data follows
      s->state = SSDV_STATE_HUFF;
      return(SSDV_CODEC_OK);

    case RADIOLIB_SSDV_JPEG_DHT:
      s->srcTblLen += l;
      while(l > 0) {
        switch(d[0]) {
          case 0x00: s->srcDht[0][0] = d; break;
          case 0x01: s->srcDht[0][1] = d; break;
          case 0x10: s->srcDht[1][0] = d; break;
          case 0x11: s->srcDht[1][1] = d; break;
        }

        size_t j = 17;
        for(uint8_t i = 1; (i <= 16) && (i < l); i++) {
          j += d[i];
        }
        if(j > l) {
          return(SSDV_CODEC_ERROR);
        }
        l -= j;
        d += j;
      }
      break;

    case RADIOLIB_SSDV_JPEG_DQT:
      s->srcTblLen += l;
      while(l > 0) {
        // only 8-bit tables are supported
        if(l < 65 || (d[0] >> 4)) {
          return(SSDV_CODEC_ERROR);
        }
        if((d[0] & 0x0F) < 2) {
          s->srcDqt[d[0] & 0x0F] = d;
        }
        l -= 65;
        d += 65;
      }
      break;

    case RADIOLIB_SSDV_JPEG_DRI:
      s->dri = ((uint16_t)d[0] << 8) | d[1];
      break;
  }

  s->state = SSDV_STATE_MARKER;
  return(SSDV_CODEC_OK);
}

static void ssdvSetPacketConf(SSDVState_t* s) {
  s->payloadLen = ssdvPayloadLen(s->type);
  s->crcDataLen = RADIOLIB_SSDV_HEADER_LEN + s->payloadLen - 1;
}

static void ssdvEncInit(SSDVState_t* s, uint8_t type, const char* callsign, uint8_t imageId, uint8_t quality) {
  memset(s, 0, sizeof(SSDVState_t));
  s->encoding = true;
  s->type = type;
  s->callsign = ssdvEncodeCallsign(callsign);
  s->imageId = imageId;
  s->quality = quality;
  ssdvSetPacketConf(s);

  s->dstDqt[0] = ssdvDstTbl(s, ssdvDqt0, sizeof(ssdvDqt0), true, quality);
  s->dstDqt[1] = ssdvDstTbl(s, ssdvDqt1, sizeof(ssdvDqt1), true, quality);
  ssdvLoadDstDht(s);
}

static void ssdvEncSetBuffer(SSDVState_t* s, uint8_t* buff) {
  memset(buff, 0, RADIOLIB_SSDV_PACKET_LEN);
  s->out = buff;
  s->outp = buff + RADIOLIB_SSDV_HEADER_LEN;
  s->outLen = s->payloadLen;

  // flush bits left over from the previous packet
  ssdvOutBits(s, 0, 0);
}

static void ssdvEncFinishPacket(SSDVState_t* s, bool eoi) {
  uint16_t mcuId = s->packetMcuId;
  uint8_t mcuOffset = s->packetMcuOffset;

  if(mcuOffset != 0xFF && mcuOffset >= s->payloadLen) {
    // first MCU starts in the next packet
    mcuId = RADIOLIB_SSDV_MCU_NONE;
    mcuOffset = 0xFF;
    s->packetMcuOffset -= s->payloadLen;
  } else {
    s->packetMcuId = RADIOLIB_SSDV_MCU_NONE;
    s->packetMcuOffset = 0xFF;
  }

  // header
  uint8_t* p = s->out;
  p[0] = RADIOLIB_SSDV_SYNC_WORD;
  p[1] = RADIOLIB_SSDV_TYPE_OFFSET + s->type;
  ssdvSetU32(&p[2], s->callsign);
  p[6] = s->imageId;
  p[7] = s->packetId >> 8;
  p[8] = s->packetId & 0xFF;
  p[9] = s->width >> 4;
  p[10] = s->height >> 4;
  p[11] = (((s->quality ^ 4) & 0x07) << 3) | ((eoi ? 1 : 0) << 2) | (s->mcuMode & 0x03);
  p[12] = mcuOffset;
  p[13] = mcuId >> 8;
  p[14] = mcuId & 0xFF;

  // fill unused payload with whitening noise
  for(uint8_t l = 0; s->outLen > 0; s->outLen--) {
    *(s->outp++) = (l = l * 245 + 45);
  }

  // CRC and Reed-Solomon parity
  uint16_t i = 1 + s->crcDataLen;
  ssdvSetU32(&p[i], ssdvCrc32(&p[1], s->crcDataLen));
  if(s->type == RADIOLIB_SSDV_TYPE_NORMAL) {
    rlb_encode_rs_8(&p[1], &p[i + RADIOLIB_SSDV_CRC_LEN], 0);
  }

  s->packetId++;
}

// produce the next packet, feeding input as needed
static uint8_t ssdvEncGetPacket(SSDVState_t* s) {
  if(s->state == SSDV_STATE_EOI) {
    return(SSDV_CODEC_EOI);
  }

  if(s->outLen == 0) {
    ssdvEncSetBuffer(s, s->out);
  }

  uint8_t r;
  while(s->inLen) {
    uint8_t b = *(s->inp++);
    s->inLen--;

    if(s->inSkip) {
      s->inSkip--;
      continue;
    }

    switch(s->state) {
      case SSDV_STATE_MARKER:
        s->marker = (s->marker << 8) | b;
        if(s->marker == RADIOLIB_SSDV_JPEG_TEM || (s->marker >= RADIOLIB_SSDV_JPEG_RST0 && s->marker <= RADIOLIB_SSDV_JPEG_EOI)) {
          // marker without data
          s->markerLen = 0;
          if((r = ssdvHaveMarker(s)) != SSDV_CODEC_OK) {
            return(r);
          }
        } else if(s->marker >= RADIOLIB_SSDV_JPEG_SOF0 && s->marker <= RADIOLIB_SSDV_JPEG_COM) {
          // 16-bit length follows
          s->markerLen = 0;
          s->state = SSDV_STATE_MARKER_LEN;
          s->needBits = 16;
        }
        break;

      case SSDV_STATE_MARKER_LEN:
        s->markerLen = (s->markerLen << 8) | b;
        if((s->needBits -= 8) == 0) {
          if(s->markerLen < 2) {
            return(SSDV_CODEC_ERROR);
          }
          s->markerLen -= 2;
          if((r = ssdvHaveMarker(s)) != SSDV_CODEC_OK) {
            return(r);
          }
        }
        break;

      case SSDV_STATE_MARKER_DATA:
        s->markerData[s->markerDataLen++] = b;
        if(s->markerDataLen == s->markerLen) {
          if((r = ssdvHaveMarkerData(s)) != SSDV_CODEC_OK) {
            return(r);
          }
        }
        break;

      case SSDV_STATE_HUFF:
      case SSDV_STATE_INT:
        // skip the 0x00 stuffing byte after 0xFF
        if(b == 0xFF) {
          s->inSkip++;
        }

        s->workBits = (s->workBits << 8) | b;
        s->workLen += 8;

        while((r = ssdvProcess(s)) == SSDV_CODEC_OK);

        if(r == SSDV_CODEC_BUFFER_FULL || r == SSDV_CODEC_EOI) {
          ssdvEncFinishPacket(s, r == SSDV_CODEC_EOI);
          if(r == SSDV_CODEC_EOI) {
            s->state = SSDV_STATE_EOI;
            return(SSDV_CODEC_EOI);
          }
          return(SSDV_CODEC_HAVE_PACKET);
        } else if(r != SSDV_CODEC_FEED_ME) {
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: invalid scan data");
          return(SSDV_CODEC_ERROR);
        }
        break;

      default:
        break;
    }
  }

  return(SSDV_CODEC_FEED_ME);
}

// ------------------------------------------------------------------------------------------------
// decoder
// ------------------------------------------------------------------------------------------------

static void ssdvWriteMarker(SSDVState_t* s, uint16_t id, uint16_t len, const uint8_t* data) {
  ssdvOutBits(s, id, 16);
  if(len > 0) {
    ssdvOutBits(s, len + 2, 16);
    while(len--) {
      ssdvOutBits(s, *(data++), 8);
    }
  }
}

static void ssdvOutHeaders(SSDVState_t* s) {
  static const uint8_t sampling[4] = { 0x22, 0x12, 0x21, 0x11 };
  const uint8_t sof0[15] = {
    8, (uint8_t)(s->height >> 8), (uint8_t)s->height, (uint8_t)(s->width >> 8), (uint8_t)s->width, 3,
    1, sampling[s->mcuMode & 0x03], 0x00,
    2, 0x11, 0x01,
    3, 0x11, 0x01,
  };

  ssdvWriteMarker(s, RADIOLIB_SSDV_JPEG_SOI, 0, NULL);
  ssdvWriteMarker(s, RADIOLIB_SSDV_JPEG_APP0, sizeof(ssdvApp0), ssdvApp0);
  ssdvWriteMarker(s, RADIOLIB_SSDV_JPEG_DQT, 65, s->dstDqt[0]);
  ssdvWriteMarker(s, RADIOLIB_SSDV_JPEG_DQT, 65, s->dstDqt[1]);
  ssdvWriteMarker(s, RADIOLIB_SSDV_JPEG_SOF0, sizeof(sof0), sof0);
  ssdvWriteMarker(s, RADIOLIB_SSDV_JPEG_DHT, sizeof(ssdvDht00), ssdvDht00);
  ssdvWriteMarker(s, RADIOLIB_SSDV_JPEG_DHT, sizeof(ssdvDht10), ssdvDht10);
  ssdvWriteMarker(s, RADIOLIB_SSDV_JPEG_DHT, sizeof(ssdvDht01), ssdvDht01);
  ssdvWriteMarker(s, RADIOLIB_SSDV_JPEG_DHT, sizeof(ssdvDht11), ssdvDht11);
  ssdvWriteMarker(s, RADIOLIB_SSDV_JPEG_SOS, sizeof(ssdvSos), ssdvSos);
}

// finish the current MCU and insert empty MCUs up to nextMcu
static void ssdvFillGap(SSDVState_t* s, uint16_t nextMcu) {
  if(s->mcuPart > 0 || s->acPart > 0) {
    if(s->acPart > 0) {
      ssdvOutInt(s, 0, 0);
      s->mcuPart++;
    }
    for(; s->mcuPart < s->ycParts + 2; s->mcuPart++) {
      ssdvOutEmptyBlock(s);
    }
    s->mcuId++;
  }

  for(; s->mcuId < nextMcu; s->mcuId++) {
    for(s->mcuPart = 0; s->mcuPart < s->ycParts + 2; s->mcuPart++) {
      ssdvOutEmptyBlock(s);
    }
  }
}

static void ssdvDecInit(SSDVState_t* s, uint8_t* buff, size_t len) {
  memset(s, 0, sizeof(SSDVState_t));

  // packets contain scan data only
  s->state = SSDV_STATE_HUFF;
  s->encoding = false;

  s->srcDht[0][0] = ssdvSrcTbl(s, ssdvDht00, sizeof(ssdvDht00), false, 0);
  s->srcDht[0][1] = ssdvSrcTbl(s, ssdvDht01, sizeof(ssdvDht01), false, 0);
  s->srcDht[1][0] = ssdvSrcTbl(s, ssdvDht10, sizeof(ssdvDht10), false, 0);
  s->srcDht[1][1] = ssdvSrcTbl(s, ssdvDht11, sizeof(ssdvDht11), false, 0);
  ssdvLoadDstDht(s);

  s->out = buff;
  s->outp = buff;
  s->outLen = len;
}

static uint8_t ssdvDecFeed(SSDVState_t* s, const uint8_t* packet) {
  SSDVPacketInfo_t info;
  SSDVClient::parseHeader(packet, &info);
  s->packetMcuOffset = info.mcuOffset;
  s->packetMcuId = info.mcuId;
  if(s->packetMcuId != RADIOLIB_SSDV_MCU_NONE) {
    s->nextResetMcu = s->packetMcuId;
  }

  // first packet: configure the decoder and write the JPEG headers
  if(s->packetId == 0 && s->outp == s->out) {
    s->type = info.type;
    s->callsign = info.callsignCode;
    s->imageId = info.imageId;
    s->width = info.width;
    s->height = info.height;
    s->mcuCount = info.mcuCount;
    s->quality = info.quality;
    s->mcuMode = info.mcuMode;
    s->ycParts = (s->mcuMode == 0) ? 4 : (s->mcuMode == 3 ? 1 : 2);
    ssdvSetPacketConf(s);

    s->srcDqt[0] = ssdvSrcTbl(s, ssdvDqt0, sizeof(ssdvDqt0), true, s->quality);
    s->srcDqt[1] = ssdvSrcTbl(s, ssdvDqt1, sizeof(ssdvDqt1), true, s->quality);
    s->dstDqt[0] = ssdvDstTbl(s, ssdvDqt0, sizeof(ssdvDqt0), true, s->quality);
    s->dstDqt[1] = ssdvDstTbl(s, ssdvDqt1, sizeof(ssdvDqt1), true, s->quality);

    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: %s image %d, %dx%d, Q%d, %d MCUs",
      info.callsign, s->imageId, s->width, s->height, s->quality, s->mcuCount);

    ssdvOutHeaders(s);
    s->outStuff = true;
  }

  uint16_t i = 0;
  if(info.packetId != s->packetId) {
    if(info.packetId < s->packetId) {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: packet %d out of order, dropped", info.packetId);
      return(SSDV_CODEC_FEED_ME);
    }

    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: packets %d - %d lost", s->packetId, info.packetId - 1);

    // nothing to resynchronise on
    if(s->packetMcuId == RADIOLIB_SSDV_MCU_NONE) {
      return(SSDV_CODEC_FEED_ME);
    }

    // blank out the missing MCUs and continue from the first MCU in this packet
    ssdvFillGap(s, s->packetMcuId);
    i = s->packetMcuOffset;
    s->state = SSDV_STATE_HUFF;
    s->component = 0;
    s->mcuPart = 0;
    s->acPart = 0;
    s->acAccRle = 0;
    s->packetId = info.packetId;
  }

  for(; i < s->payloadLen; i++) {
    if(i == s->packetMcuOffset) {
      // first MCU is byte-aligned, drop leftover bits
      s->workBits = s->workLen = 0;
      if(s->mcuId != s->packetMcuId) {
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: unexpected MCU ID in packet %d", info.packetId);
        return(SSDV_CODEC_FEED_ME);
      }
    }

    s->workBits = (s->workBits << 8) | packet[RADIOLIB_SSDV_HEADER_LEN + i];
    s->workLen += 8;

    uint8_t r;
    while((r = ssdvProcess(s)) == SSDV_CODEC_OK);
    if(r == SSDV_CODEC_EOI) {
      return(SSDV_CODEC_EOI);
    } else if(r == SSDV_CODEC_BUFFER_FULL) {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: JPEG output buffer full");
      return(SSDV_CODEC_ERROR);
    } else if(r != SSDV_CODEC_FEED_ME) {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: invalid scan data in packet %d", info.packetId);
      return(SSDV_CODEC_ERROR);
    }
  }

  s->packetId++;
  return(SSDV_CODEC_FEED_ME);
}

// ------------------------------------------------------------------------------------------------
// SSDVClient
// ------------------------------------------------------------------------------------------------

SSDVClient::SSDVClient(PhysicalLayer* phy, bool fec)
  : phyLayer(phy), fec(fec), initialized(false), imageId(0xFF),
    packetBuf(NULL), packetCount(0), packetIndex(0),
    decoder(NULL), jpegBuf(NULL), jpegBufLen(0), jpegLen(0), jpegReady(false) {
  memset(this->callsign, 0, sizeof(this->callsign));
}

SSDVClient::~SSDVClient() {
  this->clearImage();
  this->endDecoder();
}

int16_t SSDVClient::begin(const char* callsign) {
  if(callsign == NULL) {
    return(RADIOLIB_ERR_SSDV_CALLSIGN_INVALID);
  }

  size_t len = strlen(callsign);
  if(len == 0 || len > RADIOLIB_SSDV_CALLSIGN_MAX_LEN) {
    return(RADIOLIB_ERR_SSDV_CALLSIGN_TOO_LONG);
  }

  // only characters representable in base-40
  for(size_t i = 0; i < len; i++) {
    char c = callsign[i];
    if(!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) {
      return(RADIOLIB_ERR_SSDV_CALLSIGN_INVALID);
    }
  }

  strcpy(this->callsign, callsign);
  this->initialized = true;
  return(RADIOLIB_ERR_NONE);
}

int32_t SSDVClient::encode(const uint8_t* jpegData, size_t jpegLen, uint8_t quality, uint8_t* dst, uint16_t maxPackets) {
  SSDVState_t* s = new SSDVState_t;
  if(s == NULL) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }

  uint8_t pkt[RADIOLIB_SSDV_PACKET_LEN];
  ssdvEncInit(s, this->fec ? RADIOLIB_SSDV_TYPE_NORMAL : RADIOLIB_SSDV_TYPE_NOFEC, this->callsign, this->imageId, quality);
  ssdvEncSetBuffer(s, pkt);
  s->inp = jpegData;
  s->inLen = jpegLen;

  // the whole JPEG is fed at once, so the encoder asking for more means it is truncated
  int32_t count = 0;
  int32_t state = RADIOLIB_ERR_NONE;
  while(state == RADIOLIB_ERR_NONE) {
    uint8_t r = ssdvEncGetPacket(s);
    if(r == SSDV_CODEC_HAVE_PACKET || r == SSDV_CODEC_EOI) {
      if(dst != NULL) {
        if(count >= maxPackets) {
          state = RADIOLIB_ERR_SSDV_INTERNAL_MISMATCH;
          break;
        }
        memcpy(&dst[(size_t)count * RADIOLIB_SSDV_PACKET_LEN], pkt, RADIOLIB_SSDV_PACKET_LEN);
      }
      if(++count > 0xFFFF) {
        state = RADIOLIB_ERR_SSDV_ENCODE_FAILED;
      } else if(r == SSDV_CODEC_EOI) {
        break;
      }
    } else if(r == SSDV_CODEC_FEED_ME) {
      state = RADIOLIB_ERR_SSDV_JPEG_TRUNCATED;
    } else {
      state = RADIOLIB_ERR_SSDV_ENCODE_FAILED;
    }
  }

  delete s;
  return(state == RADIOLIB_ERR_NONE ? count : state);
}

int16_t SSDVClient::setImage(const uint8_t* jpegData, size_t jpegLen, uint8_t quality, int16_t imageId) {
  if(!this->initialized) {
    return(RADIOLIB_ERR_SSDV_NOT_INITIALIZED);
  }
  if(jpegData == NULL || jpegLen == 0) {
    return(RADIOLIB_ERR_SSDV_NO_IMAGE);
  }
  if(quality > RADIOLIB_SSDV_QUALITY_MAX) {
    return(RADIOLIB_ERR_SSDV_INVALID_QUALITY);
  }

  this->clearImage();
  this->imageId = (imageId < 0) ? (uint8_t)(this->imageId + 1) : (uint8_t)imageId;

  // first pass counts the packets, second pass stores them
  int32_t count = this->encode(jpegData, jpegLen, quality, NULL, 0);
  if(count <= 0) {
    return((int16_t)count);
  }

  this->packetBuf = new uint8_t[(size_t)count * RADIOLIB_SSDV_PACKET_LEN];
  if(this->packetBuf == NULL) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }

  int32_t stored = this->encode(jpegData, jpegLen, quality, this->packetBuf, (uint16_t)count);
  if(stored != count) {
    this->clearImage();
    return((int16_t)(stored < 0 ? stored : RADIOLIB_ERR_SSDV_INTERNAL_MISMATCH));
  }

  this->packetCount = (uint16_t)count;
  return(RADIOLIB_ERR_NONE);
}

uint16_t SSDVClient::numPackets() {
  return(this->packetCount);
}

bool SSDVClient::isDone() {
  return(this->packetIndex >= this->packetCount);
}

uint16_t SSDVClient::currentPacketIndex() {
  return(this->packetIndex);
}

int16_t SSDVClient::transmit(uint16_t* packetId) {
  return(this->transmitPacket(0, packetId));
}

int16_t SSDVClient::transmitLoRa(uint16_t* packetId) {
  return(this->transmitPacket(1, packetId));
}

int16_t SSDVClient::transmitPacket(uint8_t offset, uint16_t* packetId) {
  if(!this->initialized) {
    return(RADIOLIB_ERR_SSDV_NOT_INITIALIZED);
  }
  if(this->packetBuf == NULL) {
    return(RADIOLIB_ERR_SSDV_NO_PACKET_BUFFER);
  }
  if(this->isDone()) {
    return(RADIOLIB_ERR_SSDV_ALL_SENT);
  }

  uint8_t* pkt = &this->packetBuf[(size_t)this->packetIndex * RADIOLIB_SSDV_PACKET_LEN];
  int16_t state = this->phyLayer->transmit(pkt + offset, RADIOLIB_SSDV_PACKET_LEN - offset);
  RADIOLIB_ASSERT(state);

  if(packetId != NULL) {
    *packetId = this->packetIndex;
  }
  this->packetIndex++;
  return((int16_t)this->packetIndex);
}

void SSDVClient::clearImage() {
  delete[] this->packetBuf;
  this->packetBuf = NULL;
  this->packetCount = 0;
  this->packetIndex = 0;
}

int16_t SSDVClient::read(uint8_t* packet) {
  if(packet == NULL) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }

  size_t len = this->phyLayer->getPacketLength();
  if(len == RADIOLIB_SSDV_PACKET_LEN - 1) {
    // LoRa: sync word was not transmitted
    packet[0] = RADIOLIB_SSDV_SYNC_WORD;
    packet++;
  } else if(len != RADIOLIB_SSDV_PACKET_LEN) {
    return(RADIOLIB_ERR_SSDV_INVALID_PACKET);
  }

  return(this->phyLayer->readData(packet, len));
}

int16_t SSDVClient::isValidPacket(uint8_t* packet, int16_t* errors) {
  if(packet == NULL) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }
  if(errors != NULL) {
    *errors = 0;
  }

  // work on a copy, as RS correction is destructive
  uint8_t pkt[RADIOLIB_SSDV_PACKET_LEN];
  memcpy(pkt, packet, RADIOLIB_SSDV_PACKET_LEN);
  pkt[0] = RADIOLIB_SSDV_SYNC_WORD;

  uint8_t type = pkt[1] - RADIOLIB_SSDV_TYPE_OFFSET;
  if((type != RADIOLIB_SSDV_TYPE_NORMAL && type != RADIOLIB_SSDV_TYPE_NOFEC) || !ssdvCheckCrc(pkt, type)) {
    // try to recover a Normal packet, the type byte itself may be corrupted
    type = RADIOLIB_SSDV_TYPE_NORMAL;
    pkt[1] = RADIOLIB_SSDV_TYPE_OFFSET + type;
    int corrected = rlb_decode_rs_8(&pkt[1], NULL, 0, 0);
    if(corrected < 0 || !ssdvCheckCrc(pkt, type)) {
      return(RADIOLIB_ERR_SSDV_INVALID_PACKET);
    }
    if(errors != NULL) {
      *errors = (int16_t)corrected;
    }
  }

  // sanity checks
  SSDVPacketInfo_t info;
  parseHeader(pkt, &info);
  if(info.type != type || info.width == 0 || info.height == 0) {
    return(RADIOLIB_ERR_SSDV_INVALID_PACKET);
  }
  if(info.mcuId != RADIOLIB_SSDV_MCU_NONE && (info.mcuId >= info.mcuCount || info.mcuOffset >= ssdvPayloadLen(type))) {
    return(RADIOLIB_ERR_SSDV_INVALID_PACKET);
  }

  memcpy(packet, pkt, RADIOLIB_SSDV_PACKET_LEN);
  return(RADIOLIB_ERR_NONE);
}

void SSDVClient::parseHeader(const uint8_t* packet, SSDVPacketInfo_t* info) {
  info->type = packet[1] - RADIOLIB_SSDV_TYPE_OFFSET;
  info->callsignCode = ssdvGetU32(&packet[2]);
  ssdvDecodeCallsign(info->callsign, info->callsignCode);
  info->imageId = packet[6];
  info->packetId = ((uint16_t)packet[7] << 8) | packet[8];
  info->width = (uint16_t)packet[9] << 4;
  info->height = (uint16_t)packet[10] << 4;
  info->eoi = (packet[11] >> 2) & 0x01;
  info->quality = ((packet[11] >> 3) & 0x07) ^ 4;
  info->mcuMode = packet[11] & 0x03;
  info->mcuOffset = packet[12];
  info->mcuId = ((uint16_t)packet[13] << 8) | packet[14];

  // MCU is 16x16 px in mode 0, 16x8 or 8x16 px in modes 1 and 2, 8x8 px in mode 3
  info->mcuCount = (uint16_t)packet[9] * packet[10];
  if(info->mcuMode == 1 || info->mcuMode == 2) {
    info->mcuCount *= 2;
  } else if(info->mcuMode == 3) {
    info->mcuCount *= 4;
  }
}

int16_t SSDVClient::beginDecoder(uint8_t* jpegBuf, size_t jpegBufLen) {
  if(jpegBuf == NULL || jpegBufLen == 0) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }

  this->endDecoder();
  this->decoder = new SSDVState_t;
  if(this->decoder == NULL) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }

  this->jpegBuf = jpegBuf;
  this->jpegBufLen = jpegBufLen;
  return(this->resetDecoder());
}

int16_t SSDVClient::feedPacket(const uint8_t* packet) {
  if(this->decoder == NULL) {
    return(RADIOLIB_ERR_SSDV_DECODER_NOT_INITIALIZED);
  }
  if(packet == NULL) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }

  switch(ssdvDecFeed(this->decoder, packet)) {
    case SSDV_CODEC_EOI:
      this->jpegReady = true;
      return(RADIOLIB_SSDV_EOI);
    case SSDV_CODEC_FEED_ME:
      return(RADIOLIB_ERR_NONE);
    default:
      return(RADIOLIB_ERR_SSDV_DECODE_FAILED);
  }
}

int16_t SSDVClient::getJpeg(uint8_t** jpegPtr, size_t* jpegLen) {
  if(this->decoder == NULL) {
    return(RADIOLIB_ERR_SSDV_DECODER_NOT_INITIALIZED);
  }
  if(jpegPtr == NULL || jpegLen == NULL) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }
  if(!this->jpegReady) {
    return(RADIOLIB_ERR_SSDV_NO_JPEG);
  }

  // finalize only once: blank any missing MCUs, then append EOI
  if(this->jpegLen == 0) {
    SSDVState_t* s = this->decoder;
    if(s->mcuId < s->mcuCount) {
      ssdvFillGap(s, s->mcuCount);
    }
    ssdvOutSync(s);
    s->outStuff = false;
    ssdvWriteMarker(s, RADIOLIB_SSDV_JPEG_EOI, 0, NULL);
    if(s->outLen == 0) {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("SSDV: JPEG output buffer full");
      return(RADIOLIB_ERR_SSDV_DECODE_FAILED);
    }
    this->jpegLen = (size_t)(s->outp - s->out);
  }

  *jpegPtr = this->jpegBuf;
  *jpegLen = this->jpegLen;
  return(RADIOLIB_ERR_NONE);
}

int16_t SSDVClient::resetDecoder() {
  if(this->decoder == NULL) {
    return(RADIOLIB_ERR_SSDV_DECODER_NOT_INITIALIZED);
  }

  ssdvDecInit(this->decoder, this->jpegBuf, this->jpegBufLen);
  this->jpegLen = 0;
  this->jpegReady = false;
  return(RADIOLIB_ERR_NONE);
}

void SSDVClient::endDecoder() {
  delete this->decoder;
  this->decoder = NULL;
  this->jpegBuf = NULL;
  this->jpegBufLen = 0;
  this->jpegLen = 0;
  this->jpegReady = false;
}

#endif
