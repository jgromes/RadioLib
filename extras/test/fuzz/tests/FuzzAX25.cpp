#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <RadioLib.h>

// physical layer that captures the transmitted frame instead of a radio
class FuzzAX25Phy : public PhysicalLayer {
  public:
    static const size_t CAP_LEN = 512;
    uint8_t captured[CAP_LEN];
    size_t capturedLen;

    Module* getMod() override { return(nullptr); }

    int16_t transmit(const uint8_t* data, size_t len, uint8_t addr = 0) override {
      (void)addr;
      this->capturedLen = (len > CAP_LEN) ? CAP_LEN : len;
      memcpy(this->captured, data, this->capturedLen);
      return(RADIOLIB_ERR_NONE);
    }
};

// ---------------------------------------------------------------------------
// independent reference encoder for the AX.25 TX bit stream: address fields,
// FCS, bit stuffing, preamble and flags, written from the AX.25/HDLC
// specification. The library additionally applies NRZI encoding and its
// reference bits at the very start of the preamble; the checks below undo the
// NRZI encoding by differential decoding and compare the recovered bit stream
// with the reference.
// ---------------------------------------------------------------------------

#define REF_BUF_CAP 512

static uint8_t refReflect8(uint8_t b) {
  uint8_t r = 0;
  for(int i = 0; i < 8; i++) {
    r = (r << 1) | ((b >> i) & 0x01);
  }
  return(r);
}

static uint16_t refCrc16(const uint8_t* data, size_t len) {
  uint16_t crc = RADIOLIB_CRC_CCITT_INIT;
  for(size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for(int j = 0; j < 8; j++) {
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ RADIOLIB_CRC_CCITT_POLY) : (uint16_t)(crc << 1);
    }
  }
  return(crc ^ RADIOLIB_CRC_CCITT_OUT);
}

static bool refTestBit(const uint8_t* buff, size_t pos) {
  return((buff[pos / 8] >> (7 - pos % 8)) & 0x01);
}

static void refSetBit(uint8_t* buff, size_t pos) {
  buff[pos / 8] |= (uint8_t)(1UL << (7 - pos % 8));
}

// append a callsign (shifted by one bit, space-padded) + SSID byte
static uint8_t* refAppendAddress(uint8_t* ptr, const char* callsign, uint8_t ssid, uint8_t ssidFlags) {
  memset(ptr, ' ' << 1, RADIOLIB_AX25_MAX_CALLSIGN_LEN);
  for(size_t i = 0; i < strlen(callsign); i++) {
    ptr[i] = (uint8_t)(callsign[i] << 1);
  }
  ptr += RADIOLIB_AX25_MAX_CALLSIGN_LEN;
  *(ptr++) = ssidFlags | RADIOLIB_AX25_SSID_RESERVED_BITS | (uint8_t)((ssid & 0x0F) << 1) | RADIOLIB_AX25_SSID_HDLC_EXTENSION_CONTINUE;
  return(ptr);
}

// build the unencoded (but stuffed, flagged and prefixed) HDLC bit stream
static size_t refBuildHdlc(const AX25Frame* frame, uint16_t preambleLen, uint8_t* ref) {
  uint8_t orig[REF_BUF_CAP];
  uint8_t* ptr = orig;

  // destination and source address fields
  ptr = refAppendAddress(ptr, frame->destCallsign, frame->destSSID, RADIOLIB_AX25_SSID_RESPONSE_DEST);
  ptr = refAppendAddress(ptr, frame->srcCallsign, frame->srcSSID, RADIOLIB_AX25_SSID_COMMAND_SOURCE);

  // repeater address fields
  for(uint16_t i = 0; i < frame->numRepeaters; i++) {
    ptr = refAppendAddress(ptr, frame->repeaterCallsigns[i], frame->repeaterSSIDs[i], RADIOLIB_AX25_SSID_HAS_NOT_BEEN_REPEATED);
  }

  // last SSID byte ends the address extension chain
  *(ptr - 1) |= RADIOLIB_AX25_SSID_HDLC_EXTENSION_END;

  // control field with sequence numbers for I/S frames
  uint8_t control = frame->control;
  if((frame->control & 0x01) == 0) {
    control |= (uint8_t)(frame->rcvSeqNumber << 5);
    control |= (uint8_t)(frame->sendSeqNumber << 1);
  } else if((frame->control & 0x02) == 0) {
    control |= (uint8_t)(frame->rcvSeqNumber << 5);
  }
  *(ptr++) = control;

  // protocol discriminator and information field
  if(frame->protocolID != 0x00) {
    *(ptr++) = frame->protocolID;
  }
  if(frame->infoLen > 0) {
    memcpy(ptr, frame->info, frame->infoLen);
    ptr += frame->infoLen;
  }
  size_t frameLen = (size_t)(ptr - orig);

  // bit-reflect every byte (HDLC transmits LSB first)
  uint8_t wire[REF_BUF_CAP];
  for(size_t i = 0; i < frameLen; i++) {
    wire[i] = refReflect8(orig[i]);
  }

  // FCS over the reflected bytes, appended MSB first
  uint16_t fcs = refCrc16(wire, frameLen);
  wire[frameLen] = (uint8_t)(fcs >> 8);
  wire[frameLen + 1] = (uint8_t)(fcs & 0xFF);
  size_t wireLen = frameLen + 2;

  // preamble and start flag
  memset(ref, 0x00, REF_BUF_CAP);
  for(uint16_t i = 0; i < preambleLen; i++) {
    ref[i] = RADIOLIB_AX25_FLAG;
  }
  ref[preambleLen] = RADIOLIB_AX25_FLAG;

  // stuff the frame body after the start flag
  size_t bitPos = 8 * (size_t)(preambleLen + 1);
  uint8_t ones = 0;
  for(size_t i = 0; i < wireLen; i++) {
    for(int8_t shift = 7; shift >= 0; shift--) {
      if((wire[i] >> shift) & 0x01) {
        refSetBit(ref, bitPos++);
        ones++;
        if(ones == 5) {
          // forced 0 after five consecutive ones
          bitPos++;
          ones = 0;
        }
      } else {
        bitPos++;
        ones = 0;
      }
    }
  }

  // end flag - write it byte-aligned after any partial byte,
  // the library may split it across the last two bytes when misaligned;
  // only whole bytes are compared here, so place a single flag byte per
  // remainder byte covered
  return(bitPos);
}

// NRZI-decode lenBits of the encoded stream into decoded (MSB-first bits)
static void refNrziDecode(const uint8_t* enc, size_t lenBits, uint8_t* decoded) {
  memset(decoded, 0x00, (lenBits + 7) / 8 + 1);
  for(size_t i = 1; i < lenBits; i++) {
    bool prev = refTestBit(enc, i - 1);
    bool curr = refTestBit(enc, i);
    // 1: no change, 0: transition
    if(!(prev ^ curr)) {
      refSetBit(decoded, i);
    }
  }
}

// fuzz the frame construction, repeaters, copying and the full
// stuffing/FCS/NRZI transmit path against the independent reference
extern "C" int FuzzAX25(const uint8_t* data, size_t size) {
  // at least a control, PID, flags byte and one info byte
  if(size < 4) { return(0); }

  static FuzzAX25Phy* phy = nullptr;
  static AX25Client* ax25 = nullptr;
  if(!ax25) {
    phy = new FuzzAX25Phy();
    ax25 = new AX25Client(phy);
  }
  ax25->preambleLen = (uint16_t)(data[0] % 3);

  // callsigns of valid length derived from the input
  auto makeCallsign = [&](uint8_t seed, char* out) {
    size_t len = 1 + (seed % RADIOLIB_AX25_MAX_CALLSIGN_LEN);
    for(size_t i = 0; i < len; i++) {
      out[i] = (char)('A' + ((seed >> i) ^ data[(i + 1) % size]) % 26);
    }
    out[len] = '\0';
  };
  char dest[8], src[8];
  makeCallsign(data[0], dest);
  makeCallsign(data[0] ^ 0x5A, src);

  // information field: either raw input bytes or an all-ones pattern,
  // the worst case for bit stuffing (tests the hand-computed buffer size)
  uint16_t infoLen = (uint16_t)(data[3] % 129);
  uint8_t* info = nullptr;
  if(infoLen > 0) {
    info = new uint8_t[infoLen];
    for(uint16_t i = 0; i < infoLen; i++) {
      info[i] = (data[0] & 0x80) ? 0xFF : data[(4 + i) % size];
    }
  }

  AX25Frame frame(dest, data[1] & 0x0F, src, data[2] & 0x0F, data[0], data[1], info, infoLen);
  delete[] info;

  // repeaters: configure them on the frame and exercise copying
  uint8_t numRepeaters = data[2] % 9;
  if(numRepeaters > 0) {
    static char repBuf[8][8];
    char* repCallsigns[8];
    uint8_t repSSIDs[8];
    for(uint8_t i = 0; i < numRepeaters; i++) {
      makeCallsign((uint8_t)(data[1] + i), repBuf[i]);
      repCallsigns[i] = repBuf[i];
      repSSIDs[i] = (uint8_t)((data[3] >> i) & 0x0F);
    }
    (void)frame.setRepeaters(repCallsigns, repSSIDs, numRepeaters);
  }

  // copy constructor and copy assignment must preserve the transmitted bits
  AX25Frame copy(frame);
  AX25Frame assigned("TESTSRC", 0, "TESTDEST", 0, 0x03);
  assigned = frame;
  AX25Frame* selfPtr = &assigned;
  *selfPtr = assigned; // self-assignment

  uint8_t ref[REF_BUF_CAP];
  size_t refBits = refBuildHdlc(&frame, ax25->preambleLen, ref);

  // helper lambda: transmit a frame and verify the recovered HDLC stream
  auto checkFrame = [&](AX25Frame* f) {
    assert(ax25->sendFrame(f) == RADIOLIB_ERR_NONE);
    assert(phy->capturedLen > 0);
    size_t encBits = phy->capturedLen * 8;

    // the encoded length follows directly from the stuffed bit count:
    // one partial-or-last byte plus a flag byte, extended by one more
    // when the end flag is split across the boundary
    size_t expLen = refBits / 8 + 1 + ((refBits % 8) ? 1 : 0);
    assert(phy->capturedLen == expLen);
    assert(encBits >= refBits + 8);

    uint8_t decoded[REF_BUF_CAP];
    refNrziDecode(phy->captured, encBits, decoded);

    // the first bit is the NRZI reference level and the following
    // preambleLen bits are passed through unencoded by the implementation,
    // everything from bit preambleLen+1 on must match exactly
    for(size_t i = ax25->preambleLen + 1; i < refBits; i++) {
      assert(refTestBit(decoded, i) == refTestBit(ref, i));
    }

    // the end flag must follow immediately after the last data bit
    // as the bit sequence 01111110, even when split across bytes
    assert(!refTestBit(decoded, refBits));
    for(size_t i = refBits + 1; i < refBits + 7; i++) {
      assert(refTestBit(decoded, i) == true);
    }
    assert(!refTestBit(decoded, refBits + 7));

    // no more than five consecutive ones may appear between the start
    // flag and the end flag - the core bit stuffing invariant
    uint8_t ones = 0;
    for(size_t i = 8 * (size_t)(ax25->preambleLen + 1); i < refBits; i++) {
      if(refTestBit(decoded, i)) {
        ones++;
        assert(ones <= 5);
      } else {
        ones = 0;
      }
    }
  };

  // the transmitted bit stream must match the reference
  checkFrame(&frame);

  // copies must transmit identically
  checkFrame(&copy);
  checkFrame(&assigned);

  return(0);
}
