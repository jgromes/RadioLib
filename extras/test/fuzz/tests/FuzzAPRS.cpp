#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include <RadioLib.h>

// physical layer that captures the transmitted frame instead of a radio
class FuzzAPRSSendPhy : public PhysicalLayer {
  public:
    static const size_t CAP_LEN = 1024;
    uint8_t captured[CAP_LEN];
    size_t capturedLen;

    Module* getMod() override { return(nullptr); }

    int16_t transmit(const uint8_t* data, size_t len, uint8_t addr = 0) override {
      (void)addr;
      this->capturedLen = (len > CAP_LEN) ? CAP_LEN : len;
      memcpy(this->captured, data, this->capturedLen);
      return(RADIOLIB_ERR_NONE);
    }

    // succeed the direct-mode configuration calls, so that
    // AX25Client::begin()/startDirect() does not abort with ERR_UNSUPPORTED
    int16_t setEncoding(uint8_t encoding) override { (void)encoding; return(RADIOLIB_ERR_NONE); }
    int16_t setDataShaping(uint8_t sh) override { (void)sh; return(RADIOLIB_ERR_NONE); }
    int16_t standby() override { return(RADIOLIB_ERR_NONE); }
    int16_t standby(uint8_t mode) override { (void)mode; return(RADIOLIB_ERR_NONE); }
};

// ---------------------------------------------------------------------------
// input helpers
// ---------------------------------------------------------------------------

// mapping from arbitrary bytes to printable ASCII, so that the APRS info
// field builders can be exercised with realistic character data
static char printableByte(uint8_t in) {
  if((in >= ' ') && (in <= '~')) {
    return((char)in);
  } else if(in < ' ') {
    return((char)(in + ' '));
  } else {
    return((char)((in >> 2) + ' '));
  }
}

// next fuzzing byte at cursor *off (wrapping)
static uint8_t nextByte(const uint8_t* data, size_t size, size_t* off) {
  uint8_t b = data[*off % size];
  (*off)++;
  return(b);
}

// A-Z0-9 callsign-style string of length 1..maxLen
static void makeCallsign(const uint8_t* data, size_t size, size_t* off, char* out, size_t maxLen) {
  static const char pool[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  size_t len = 1 + (nextByte(data, size, off) % maxLen);
  for(size_t i = 0; i < len; i++) {
    out[i] = pool[nextByte(data, size, off) % (sizeof(pool) - 1)];
  }
  out[len] = '\0';
}

// printable ASCII string of length 1..maxLen
static void makeString(const uint8_t* data, size_t size, size_t* off, char* out, size_t maxLen) {
  size_t len = 1 + (nextByte(data, size, off) % maxLen);
  for(size_t i = 0; i < len; i++) {
    out[i] = printableByte(nextByte(data, size, off));
  }
  out[len] = '\0';
}

// finite float from the input, clamped into [-limit, limit]
// (non-finite inputs like NaN/inf would make the Mic-E integer conversions
// undefined behaviour before the encoder could even run)
static float makeFloat(const uint8_t* data, size_t size, size_t* off, float limit) {
  uint32_t bits = 0;
  for(int i = 0; i < 4; i++) {
    bits |= (uint32_t)nextByte(data, size, off) << (8 * i);
  }
  float f;
  memcpy(&f, &bits, sizeof(f));
  if(!isfinite(f)) {
    return(0.0f);
  }
  f = fmodf(f, limit);
  return(f);
}

// ---------------------------------------------------------------------------
// independent reference for the APRS position info field, from the APRS
// spec: <prefix>[timestamp]<lat><table><lon><symbol>[<comment>]
// ---------------------------------------------------------------------------

static char refPosPrefix(bool hasTime, bool hasMsg) {
  if(!hasTime && !hasMsg) { return('!'); }
  if(hasTime && !hasMsg) { return('/'); }
  if(!hasTime && hasMsg) { return('='); }
  return('@');
}

static void refPosInfo(char* out, size_t cap, char table, char symbol,
                       const char* time, const char* lat, const char* lon, const char* msg) {
  snprintf(out, cap, "%c%s%s%c%s%c%s",
           refPosPrefix(time != NULL, msg != NULL),
           time ? time : "", lat, table, lon, symbol, msg ? msg : "");
}

static bool refTestBit(const uint8_t* buff, size_t pos) {
  return((buff[pos / 8] >> (7 - pos % 8)) & 0x01);
}

// NRZI-decode the encoded bit stream: a 1 means "no transition"
static void refNrziDecode(const uint8_t* enc, size_t lenBits, uint8_t* decoded) {
  memset(decoded, 0x00, (lenBits + 7) / 8 + 1);
  for(size_t i = 1; i < lenBits; i++) {
    if(!(refTestBit(enc, i - 1) ^ refTestBit(enc, i))) {
      decoded[i / 8] |= (uint8_t)(1UL << (7 - i % 8));
    }
  }
}

// find the start flag 01111110 in the decoded bit stream
static long refFindFlag(const uint8_t* bits, size_t lenBits) {
  for(size_t i = 0; i + 8 <= lenBits; i++) {
    bool ok = !refTestBit(bits, i);
    for(size_t j = 1; ok && j < 7; j++) {
      ok = refTestBit(bits, i + j);
    }
    if(ok && !refTestBit(bits, i + 7)) {
      return((long)i);
    }
  }
  return(-1);
}

static uint8_t refReflect8(uint8_t b) {
  uint8_t r = 0;
  for(int i = 0; i < 8; i++) {
    r = (r << 1) | ((b >> i) & 0x01);
  }
  return(r);
}

// de-bit-stuff the HDLC frame body starting at bit startBit following the
// start flag - HDLC emits each byte LSB-first, with a stuffed 0 after every
// five consecutive 1s - and bit-reflect the reconstructed bytes back to buffer
// order; returns byte count
static size_t refUnstuff(const uint8_t* bits, size_t startBit, size_t lenBits, uint8_t* out, size_t outCap) {
  size_t outLen = 0;
  size_t pos = startBit;
  uint8_t ones = 0, byte = 0, collected = 0;
  while(pos < lenBits && outLen < outCap) {
    bool b = refTestBit(bits, pos++);
    if(ones == 5 && !b) {
      // the stuffed 0, skip it
      ones = 0;
      continue;
    }
    byte = (uint8_t)((byte << 1) | (b ? 1 : 0));
    ones = b ? (uint8_t)(ones + 1) : 0;
    if(++collected == 8) {
      out[outLen++] = refReflect8(byte);
      byte = 0;
      collected = 0;
    }
  }
  return(outLen);
}

// ---------------------------------------------------------------------------
// fuzz the APRS encoders: sendPosition / sendFrame in LoRa (plain text) and
// AX.25 (HDLC) modes, and sendMicE. The transmitted data is compared against
// independent references; all memory errors are caught by ASan/UBSan.
// ---------------------------------------------------------------------------

extern "C" int FuzzAPRS(const uint8_t* data, size_t size) {
  if(size < 16) { return(0); }

  static FuzzAPRSSendPhy* phyLoRa = nullptr;
  static APRSClient* aprsLoRa = nullptr;
  static FuzzAPRSSendPhy* phyAx25 = nullptr;
  static AX25Client* ax25 = nullptr;
  static APRSClient* aprsAx25 = nullptr;
  if(!aprsLoRa) {
    phyLoRa = new FuzzAPRSSendPhy();
    aprsLoRa = new APRSClient(phyLoRa);
    phyAx25 = new FuzzAPRSSendPhy();
    ax25 = new AX25Client(phyAx25);
    aprsAx25 = new APRSClient(ax25);
  }

  size_t off = 0;

  // APRS symbol must be in the printable symbol range ' ' ... '}'
  char symbol = (char)(' ' + (nextByte(data, size, &off) % ('}' - ' ' + 1)));
  bool altTable = nextByte(data, size, &off) & 0x01;
  char table = altTable ? '\\' : '/';

  char src[8], dest[8];
  makeCallsign(data, size, &off, src, RADIOLIB_AX25_MAX_CALLSIGN_LEN);
  makeCallsign(data, size, &off, dest, RADIOLIB_AX25_MAX_CALLSIGN_LEN);
  uint8_t srcSSID = nextByte(data, size, &off) & 0x0F;
  uint8_t destSSID = nextByte(data, size, &off) & 0x07;

  char lat[16], lon[16], msg[32], timestamp[12];
  makeString(data, size, &off, lat, 12);
  makeString(data, size, &off, lon, 12);
  makeString(data, size, &off, msg, 24);
  makeString(data, size, &off, timestamp, 8);
  bool useMsg = nextByte(data, size, &off) & 0x01;
  bool useTime = nextByte(data, size, &off) & 0x01;

  // -- mode 1: LoRa APRS, sendFrame assembles a plain text frame ----------
  assert(aprsLoRa->begin(symbol, src, srcSSID, altTable) == RADIOLIB_ERR_NONE);

  char refInfo[128];
  refPosInfo(refInfo, sizeof(refInfo), table, symbol,
             useTime ? timestamp : NULL, lat, lon, useMsg ? msg : NULL);
  assert(aprsLoRa->sendPosition(dest, destSSID, lat, lon,
                                useMsg ? msg : NULL, useTime ? timestamp : NULL) == RADIOLIB_ERR_NONE);

  char refWire[256];
  int refLen = snprintf(refWire, sizeof(refWire), "<\xff\x01%s-%d>%s,WIDE%d-%d:%s",
                        src, srcSSID, dest, destSSID, destSSID, refInfo);
  assert(refLen > 0 && (size_t)refLen < sizeof(refWire));
  assert(phyLoRa->capturedLen == (size_t)refLen);
  assert(memcmp(phyLoRa->captured, refWire, (size_t)refLen) == 0);

  // raw sendFrame with arbitrary printable info field
  char rawInfo[32];
  makeString(data, size, &off, rawInfo, 28);
  refLen = snprintf(refWire, sizeof(refWire), "<\xff\x01%s-%d>%s,WIDE%d-%d:%s",
                    src, srcSSID, dest, destSSID, destSSID, rawInfo);
  assert(refLen > 0 && (size_t)refLen < sizeof(refWire));
  assert(aprsLoRa->sendFrame(dest, destSSID, rawInfo) == RADIOLIB_ERR_NONE);
  assert(phyLoRa->capturedLen == (size_t)refLen);
  assert(memcmp(phyLoRa->captured, refWire, (size_t)refLen) == 0);

  // -- mode 2: classical AX.25, check the HDLC frame contents -------------
  // begin() would require radio-specific virtuals, configure the client directly
  memcpy(ax25->sourceCallsign, src, strlen(src));
  ax25->sourceCallsign[strlen(src)] = '\0';
  ax25->sourceSSID = srcSSID;
  ax25->preambleLen = nextByte(data, size, &off) % 4;
  assert(aprsAx25->begin(symbol, NULL, 0, altTable) == RADIOLIB_ERR_NONE);

  // repeaters change the frame layout, account for their address fields;
  // useRepeaters() only stores the pointers, so the arrays must outlive the call
  static char repBuf[4][8];
  static char* repCalls[4];
  static uint8_t repSSIDs[4];
  uint8_t numReps = nextByte(data, size, &off) % 4;
  if(numReps > 0) {
    for(uint8_t i = 0; i < numReps; i++) {
      makeCallsign(data, size, &off, repBuf[i], RADIOLIB_AX25_MAX_CALLSIGN_LEN);
      repCalls[i] = repBuf[i];
      repSSIDs[i] = (uint8_t)(nextByte(data, size, &off) & 0x0F);
    }
    aprsAx25->useRepeaters(repCalls, repSSIDs, numReps);
  } else {
    aprsAx25->dropRepeaters();
  }

  assert(aprsAx25->sendPosition(dest, destSSID, lat, lon,
                                useMsg ? msg : NULL, useTime ? timestamp : NULL) == RADIOLIB_ERR_NONE);

  // NRZI-decode, find the start flag (skipping any preamble flags) and
  // de-stuff the frame body
  size_t encBits = phyAx25->capturedLen * 8;
  static uint8_t decoded[FuzzAPRSSendPhy::CAP_LEN];
  static uint8_t frame[FuzzAPRSSendPhy::CAP_LEN];
  refNrziDecode(phyAx25->captured, encBits, decoded);
  long flag = refFindFlag(decoded, encBits);
  assert(flag >= 0);
  size_t bodyStart = (size_t)flag + 8;
  while(bodyStart + 8 <= encBits) {
    // another 01111110 flag byte? the frame body begins after the last one
    bool isFlag = !refTestBit(decoded, bodyStart);
    for(size_t j = 1; isFlag && j < 7; j++) {
      isFlag = refTestBit(decoded, bodyStart + j);
    }
    isFlag = isFlag && !refTestBit(decoded, bodyStart + 7);
    if(!isFlag) { break; }
    bodyStart += 8;
  }
  // AX.25 frame: dest(7) src(7) [repeater addr(7) each] control PID info FCS(2)
  size_t hdrLen = 14 + (size_t)numReps * 7;
  size_t infoLen = strlen(refInfo);
  size_t expFrameLen = hdrLen + 2 + infoLen + 2;

  // de-stuff exactly the expected frame length, so the walk stops before
  // the end flag - the flag's six consecutive 1s are not bit-stuffed data
  size_t frameLen = refUnstuff(decoded, bodyStart, encBits, frame, expFrameLen);
  assert(frameLen == expFrameLen);
  assert(frame[hdrLen] == RADIOLIB_AX25_CONTROL_UNNUMBERED_FRAME);
  assert(frame[hdrLen + 1] == RADIOLIB_AX25_PID_NO_LAYER_3);
  assert(memcmp(&frame[hdrLen + 2], refInfo, infoLen) == 0);

  // -- Mic-E ----------------------------------------------------------------
  float micElat = makeFloat(data, size, &off, 90.0f);
  float micElon = makeFloat(data, size, &off, 180.0f);
  uint16_t heading = (uint16_t)(nextByte(data, size, &off) % 360);
  uint16_t speed = (uint16_t)(nextByte(data, size, &off) * 3);
  uint8_t micEtype = nextByte(data, size, &off);
  static const uint8_t telemLens[3] = { 0, 2, 5 };
  uint8_t telemLen = telemLens[nextByte(data, size, &off) % 3];
  uint8_t telem[5];
  for(int i = 0; i < 5; i++) {
    telem[i] = nextByte(data, size, &off);
  }
  char grid[8], status[16];
  makeCallsign(data, size, &off, grid, 6);
  makeString(data, size, &off, status, 12);
  bool useGrid = nextByte(data, size, &off) & 0x01;
  bool useStatus = nextByte(data, size, &off) & 0x01;
  bool useAlt = nextByte(data, size, &off) & 0x01;
  // sane altitude range, the encoder adds a 10km offset
  int32_t alt = useAlt ? (int32_t)(nextByte(data, size, &off) * 100) : RADIOLIB_APRS_MIC_E_ALTITUDE_UNUSED;

  // the Mic-E LoRa frame is "<\xff\x01<src>-<id>><6-char Mic-E dest>,WIDE1-1:<info>",
  // the info field starts at a fixed offset, its content can legally contain ':'
  size_t idDigits = (srcSSID >= 10) ? 2 : 1;
  size_t micEInfoOffset = 3 + strlen(src) + 1 + idDigits + 1 + 6 + 9;

  // telemetry and grid/status/altitude are mutually exclusive, and that
  // must be reported instead of encoding anything
  if((telemLen > 0) && (useGrid || useStatus || useAlt)) {
    int16_t state = aprsLoRa->sendMicE(micElat, micElon, heading, speed, micEtype, telem, telemLen,
                                       useGrid ? grid : NULL, useStatus ? status : NULL, alt);
    assert(state == RADIOLIB_ERR_MIC_E_TELEMETRY_STATUS);
  } else if((telemLen == 0) && useGrid && useStatus) {
    // grid + status is legal, verify the info field structure
    assert(aprsLoRa->sendMicE(micElat, micElon, heading, speed, micEtype, NULL, 0,
                              grid, status, alt) == RADIOLIB_ERR_NONE);
    assert(phyLoRa->capturedLen >= micEInfoOffset + 9);
    const char* info = (const char*)phyLoRa->captured + micEInfoOffset;
    size_t infoLen = phyLoRa->capturedLen - micEInfoOffset;
    assert(info[0] == RADIOLIB_APRS_MIC_E_GPS_DATA_CURRENT);
    assert(info[7] == symbol);
    assert(info[8] == table);
    assert(infoLen == 9 + (strlen(grid) + 2) + (1 + strlen(status)) + (useAlt ? 4 : 0));
    assert(info[9 + strlen(grid) + 2] == ' ');
    if(useAlt) {
      assert(info[infoLen - 1] == '}');
    }
  } else {
    assert(aprsLoRa->sendMicE(micElat, micElon, heading, speed, micEtype,
                              telemLen > 0 ? telem : NULL, telemLen,
                              useGrid ? grid : NULL, useStatus ? status : NULL, alt) == RADIOLIB_ERR_NONE);
    assert(phyLoRa->capturedLen >= micEInfoOffset + 9);
    assert(memcmp(phyLoRa->captured, "<\xff\x01", 3) == 0);
    const char* info = (const char*)phyLoRa->captured + micEInfoOffset;
    size_t infoLen = phyLoRa->capturedLen - micEInfoOffset;
    assert(info[0] == RADIOLIB_APRS_MIC_E_GPS_DATA_CURRENT);
    assert(info[7] == symbol);
    assert(info[8] == table);
    if(telemLen > 0) {
      // telemetry flag + 2 hex digits per byte, nothing else
      assert(info[9] == ((telemLen == 2) ? RADIOLIB_APRS_MIC_E_TELEMETRY_LEN_2 : RADIOLIB_APRS_MIC_E_TELEMETRY_LEN_5));
      assert(infoLen == 10 + 2 * (size_t)telemLen);
      for(size_t i = 0; i < telemLen; i++) {
        char expect[3];
        snprintf(expect, sizeof(expect), "%02X", telem[i]);
        assert(info[10 + 2 * i] == expect[0]);
        assert(info[10 + 2 * i + 1] == expect[1]);
      }
    } else {
      // base length + optional grid ("/xxG" included) + status + base-91 altitude
      size_t expectedInfoLen = 9
        + (useGrid ? (strlen(grid) + 2) : 0)
        + (useStatus ? (1 + strlen(status)) : 0)
        + (useAlt ? 4 : 0);
      assert(infoLen == expectedInfoLen);
      if(useAlt) {
        // altitude is base-91 encoded in 3 printable characters followed by '}'
        assert(info[expectedInfoLen - 1] == '}');
        assert(info[expectedInfoLen - 2] >= '!');
        assert(info[expectedInfoLen - 2] <= '{');
      }
    }
  }

  return(0);
}
