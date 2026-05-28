/*
 * SSDV image client for RadioLib
 *
 * Transmission implementation notes
 * ───────────────────────────────────
 * Encoding is performed in two passes over the JPEG data:
 *
 *   Pass 1 (count)  — run the ssdv encoder to determine the exact number of
 *                     output packets without allocating storage.
 *   Allocation      — malloc(packetCount × 256) bytes.
 *   Pass 2 (store)  — run the ssdv encoder a second time, copying each
 *                     completed packet into the flat buffer.
 *
 * Two passes are necessary because the SSDV encoder re-quantises and
 * re-huffman-codes the JPEG, so the output size cannot be predicted from
 * the input length alone.
 *
 * Memory: 256 × N bytes on the heap.  For a typical 320×240 image at quality
 * level 4 this is ~30–60 packets → ~8–15 KB.  Fits comfortably in the 320 KB
 * SRAM of an ESP32.
 *
 * Thread safety: none.  Call from a single task / context.
 */

#include "SSDV.h"

#include <stdlib.h>   // malloc, free
#include <string.h>   // memcpy, memset, strlen, strncpy

SSDVClient::SSDVClient(PhysicalLayer* phy, bool fec)
  : phy(phy),
    fec(fec),
    initialized(false),
    packetBuf(nullptr),
    packetCount(0),
    currentPacket(0),
    imageId(0xFF)       // will roll over to 0x00 on first setImage() auto-inc
{
  memset(callsign, 0, sizeof(callsign));
}

SSDVClient::~SSDVClient() {
  this->freeBuffer();
}

void SSDVClient::freeBuffer() {
  if(this->packetBuf != nullptr) {
    free(this->packetBuf);
    this->packetBuf = nullptr;
  }
  this->packetCount   = 0;
  this->currentPacket = 0;
}

/*
 * Core encoding coroutine, used by both the count pass (packetBuf == nullptr)
 * and the store pass (packetBuf != nullptr).
 *
 * The ssdv encoder API is a coroutine driven by ssdv_enc_get_packet():
 *
 *   SSDV_FEED_ME    — encoder needs more JPEG input; call ssdv_enc_feed().
 *   SSDV_HAVE_PACKET — one complete packet written to tempPkt; tally/store it.
 *   SSDV_EOI         — final packet written (EOI flag set); encoding complete.
 *   SSDV_ERROR       — unrecoverable error.
 *
 * We feed the entire JPEG in one shot the first time FEED_ME is returned.
 * A second FEED_ME after the data is exhausted means the JPEG is truncated.
 */
int32_t SSDVClient::runEncoder(uint8_t type, const uint8_t* jpegData,
                               size_t jpegLen, uint8_t quality) {
  ssdv_t  s;
  uint8_t tempPkt[SSDV_PKT_SIZE];

  char r = ssdv_enc_init(&s, type, this->callsign, this->imageId, (int8_t)quality);
  if(r != SSDV_OK) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ssdv_enc_init(): %d", (uint8_t)r);
    return(RADIOLIB_ERR_SSDV_ENCODE_FAILED);
  }

  ssdv_enc_set_buffer(&s, tempPkt);

  bool    fed   = false;
  bool    done  = false;
  int32_t count = 0;

  while(!done) {
    r = ssdv_enc_get_packet(&s);

    switch(r) {

      case SSDV_OK:
        break;

      // ── Encoder needs more input ─────────────────────────────────────
      case SSDV_FEED_ME:
        if(!fed) {
          ssdv_enc_feed(&s, const_cast<uint8_t*>(jpegData), jpegLen);
          fed = true;
        } else {
          // Second FEED_ME — JPEG has no EOI marker (truncated).
          return(RADIOLIB_ERR_SSDV_JPEG_TRUNCATED);
        }
        break;

      // ── A complete non-final packet is ready ─────────────────────────
      case SSDV_HAVE_PACKET:
        if(this->packetBuf != nullptr) {
          if((uint16_t)count >= this->packetCount) {
            return(RADIOLIB_ERR_SSDV_INTERNAL_MISMATCH);
          }
          memcpy(this->packetBuf + (size_t)count * SSDV_PKT_SIZE,
                 tempPkt,
                 SSDV_PKT_SIZE);
        }
        count++;
        ssdv_enc_set_buffer(&s, tempPkt);
        break;

      // ── Final packet (EOI flag set in header) ────────────────────────
      case SSDV_EOI:
        if(this->packetBuf != nullptr) {
          if((uint16_t)count >= this->packetCount) {
            return(RADIOLIB_ERR_SSDV_INTERNAL_MISMATCH);
          }
          memcpy(this->packetBuf + (size_t)count * SSDV_PKT_SIZE,
                 tempPkt,
                 SSDV_PKT_SIZE);
        }
        count++;
        done = true;
        break;

      case SSDV_ERROR:
      default:
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ssdv_enc_get_packet(): %d", (uint8_t)r);
        return(RADIOLIB_ERR_SSDV_ENCODE_FAILED);
    }
  }

  return count;  // positive number of packets
}

int16_t SSDVClient::transmitPacket(uint8_t offset, uint16_t* packetId) {
  if(!this->initialized) {
    return(RADIOLIB_ERR_SSDV_NOT_INITIALIZED);
  }
  if(this->packetBuf == nullptr || this->packetCount == 0) {
    return(RADIOLIB_ERR_SSDV_NO_PACKET_BUFFER);
  }
  if(this->currentPacket >= this->packetCount) {
    return(RADIOLIB_ERR_SSDV_ALL_SENT);
  }

  uint8_t* pkt     = this->packetBuf + (size_t)this->currentPacket * SSDV_PKT_SIZE;
  uint16_t txLen   = (uint16_t)(SSDV_PKT_SIZE - offset);
  uint8_t* txStart = pkt + offset;

  if(packetId != nullptr) {
    *packetId = this->currentPacket;
  }

  int16_t state = this->phy->transmit(txStart, txLen);
  RADIOLIB_ASSERT(state);

  this->currentPacket++;
  return((int16_t)this->currentPacket);
}

int16_t SSDVClient::begin(const char* callsign) {
  if(callsign == nullptr) {
    return(RADIOLIB_ERR_SSDV_CALLSIGN_INVALID);
  }

  size_t len = strlen(callsign);
  if(len == 0 || len > SSDV_MAX_CALLSIGN) {
    return(RADIOLIB_ERR_SSDV_CALLSIGN_TOO_LONG);
  }

  // Validate characters: SSDV base-40 charset is A–Z, 0–9, space, -, /, .
  // We disallow space in a callsign as it is an unusual/invisible character.
  for(size_t i = 0; i < len; i++) {
    char c = callsign[i];
    bool ok = ((c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') ||
               c == '-' || c == '/' || c == '.');
    if(!ok) {
      return(RADIOLIB_ERR_SSDV_CALLSIGN_INVALID);
    }
  }

  strncpy(this->callsign, callsign, SSDV_MAX_CALLSIGN);
  this->callsign[SSDV_MAX_CALLSIGN] = '\0';

  this->initialized = true;
  return(RADIOLIB_ERR_NONE);
}

int16_t SSDVClient::setImage(const uint8_t* jpegData, size_t jpegLen,
                             uint8_t quality, int16_t imageId) {
  if(!this->initialized) {
    return(RADIOLIB_ERR_SSDV_NOT_INITIALIZED);
  }
  if(jpegData == nullptr || jpegLen == 0) {
    return(RADIOLIB_ERR_SSDV_NO_IMAGE);
  }
  if(quality > 7) {
    return(RADIOLIB_ERR_SSDV_INVALID_QUALITY);
  }

  if(imageId < 0) {
    this->imageId = (uint8_t)(this->imageId + 1u);
  } else {
    this->imageId = (uint8_t)((uint16_t)imageId & 0xFFu);
  }

  uint8_t type = this->fec ? SSDV_TYPE_NORMAL : SSDV_TYPE_NOFEC;

  this->freeBuffer();

  // ── Pass 1: count packets ───────────────────────────────────────────────
  int32_t countResult = this->runEncoder(type, jpegData, jpegLen, quality);
  if(countResult <= 0) {
    return((int16_t)countResult);
  }
  this->packetCount = (uint16_t)countResult;

  // ── Allocate flat packet buffer ─────────────────────────────────────────
  size_t bufSize = (size_t)this->packetCount * SSDV_PKT_SIZE;
  this->packetBuf = reinterpret_cast<uint8_t*>(malloc(bufSize));
  if(this->packetBuf == nullptr) {
    this->packetCount = 0;
    return(RADIOLIB_ERR_SSDV_ALLOC_FAILED);
  }

  // ── Pass 2: encode and store ────────────────────────────────────────────
  int32_t storeResult = this->runEncoder(type, jpegData, jpegLen, quality);
  if(storeResult <= 0) {
    this->freeBuffer();
    return((int16_t)storeResult);
  }

  // Sanity-check: both passes must yield the same count.
  if((uint16_t)storeResult != this->packetCount) {
    this->freeBuffer();
    return(RADIOLIB_ERR_SSDV_INTERNAL_MISMATCH);
  }

  this->currentPacket = 0;
  return(RADIOLIB_ERR_NONE);
}

uint16_t SSDVClient::numPackets() {
  return(this->packetCount);
}

bool SSDVClient::isDone() {
  return(this->packetBuf == nullptr || this->currentPacket >= this->packetCount);
}

uint16_t SSDVClient::currentPacketIndex() {
  return(this->currentPacket);
}

int16_t SSDVClient::transmit(uint16_t* packetId) {
  return(this->transmitPacket(0, packetId));
}

int16_t SSDVClient::transmitLoRa(uint16_t* packetId) {
  // offset = 1 → skip the 0x55 sync byte; 255 bytes sent.
  return(this->transmitPacket(1, packetId));
}

void SSDVClient::clearImage() {
  this->freeBuffer();
}
