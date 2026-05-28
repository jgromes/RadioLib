/*
 * SSDVClient.cpp — SSDV image transmit client for RadioLib
 *
 * See SSDVClient.h for the full API and protocol documentation.
 *
 * Implementation notes
 * ────────────────────
 * Encoding is performed in two passes over the JPEG data:
 *
 *  Pass 1 (count)  — run the ssdv encoder to determine the exact number of
 *          output packets without allocating storage.
 *  Allocation    — malloc(packetCount × 256) bytes.
 *  Pass 2 (store)  — run the ssdv encoder a second time, copying each
 *          completed packet into the flat buffer.
 *
 * Two passes are necessary because the SSDV encoder re-quantises and
 * re-huffman-codes the JPEG, so the output size cannot be predicted from the
 * input length alone.
 *
 * Memory: 256 × N bytes on the heap.  For a typical 320×240 image at quality
 * level 4 this is ≈ 30–60 packets → ≈ 8–15 KB.  Easily fits in the 320 KB
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
  this->packetCount  = 0;
  this->currentPacket = 0;
}

/*!
 * Core encoding routine used by both passes.
 *
 * When packetBuf == nullptr (count pass) we just tally completed packets.
 * When packetBuf != nullptr (store pass) we copy each packet into the buffer.
 *
 * The ssdv encoder API works as a coroutine:
 *
 *   ssdv_enc_set_buffer(&s, buf)   — point encoder at a 256-byte output buffer
 *   ssdv_enc_get_packet(&s) returns one of:
 *   SSDV_FEED_ME   — encoder needs more input; call ssdv_enc_feed()
 *   SSDV_HAVE_PACKET — one complete packet written to buf; reset buf pointer
 *   SSDV_EOI     — final packet written (contains EOI flag in header);
 *             encoding complete
 *   SSDV_ERROR   — unrecoverable encoding error
 *
 * We feed all JPEG bytes in one shot the first time FEED_ME is returned.
 * For a well-formed JPEG the encoder will encounter the JPEG EOI marker
 * (0xFF 0xD9) before exhausting the input and return SSDV_EOI, so FEED_ME
 * should occur exactly once. A second FEED_ME indicates a truncated file.
 */
int32_t SSDVClient::runEncoder(uint8_t type, const uint8_t* jpegData, size_t jpegLen, uint8_t quality) {
  ssdv_t  s;
  uint8_t tempPkt[SSDV_PKT_SIZE];   // 256 bytes on the stack — encoder writes here

  // Initialise the ssdv encoder.
  // ssdv_enc_init returns SSDV_OK (0) on success.
  char r = ssdv_enc_init(&s, type, this->callsign, this->imageId, (int8_t)quality);
  if(r != SSDV_OK) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ssdv_enc_init(): %d", (uint8_t)r);
    return(RADIOLIB_ERR_SSDV_ENCODE_FAILED);
  }

  // Point the encoder at our temporary output buffer.
  ssdv_enc_set_buffer(&s, tempPkt);

  bool  fed   = false;  // Have we called ssdv_enc_feed() yet?
  bool  done  = false;
  int32_t count = 0;    // Packets produced so far

  while(!done) {
    r = ssdv_enc_get_packet(&s);

    switch(r) {

      case SSDV_OK:
        break;

      // ── Encoder needs more input ───────────────────────────────────
      case SSDV_FEED_ME:
        if(!fed) {
          // Feed the entire JPEG in one shot.
          // ssdv_enc_feed() just sets s->inp / s->in_len;
          // actual processing happens inside ssdv_enc_get_packet().
          // The cast away of const is safe: the encoder only reads.
          ssdv_enc_feed(&s, const_cast<uint8_t*>(jpegData), jpegLen);
          fed = true;
        } else {
          // Second FEED_ME after all data was consumed.
          // The JPEG is truncated — no EOI marker found.
          return(RADIOLIB_ERR_SSDV_JPEG_TRUNCATED);
        }
        break;

      // ── A complete packet is ready in tempPkt ──────────────────────
      case SSDV_HAVE_PACKET:
        if(this->packetBuf != nullptr) {
          // Store pass: copy packet into the flat buffer.
          if((uint16_t)count >= this->packetCount) {
            // More packets than counted — should never happen.
            return(RADIOLIB_ERR_SSDV_INTERNAL_MISMATCH);
          }
          memcpy(this->packetBuf + (size_t)count * SSDV_PKT_SIZE,
               tempPkt,
               SSDV_PKT_SIZE);
        }
        count++;
        // Reset the output buffer pointer for the next packet.
        ssdv_enc_set_buffer(&s, tempPkt);
        break;

      // ── Final packet (EOI flag set in header) ─────────────────────
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

      // ── Unrecoverable encoder error ────────────────────────────────
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

  // Pointer to the start of the current packet in the flat buffer.
  uint8_t* pkt   = this->packetBuf + (size_t)this->currentPacket * SSDV_PKT_SIZE;
  uint16_t txLen   = (uint16_t)(SSDV_PKT_SIZE - offset);
  uint8_t* txStart = pkt + offset;

  // Report the packet index before transmitting.
  if(packetId != nullptr) {
    *packetId = this->currentPacket;
  }

  // Transmit the packet
  int16_t state = this->phy->transmit(txStart, txLen);
  RADIOLIB_ASSERT(state);

  this->currentPacket++;
  // Return the running count of packets sent (positive → success).
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
  this->callsign[SSDV_MAX_CALLSIGN] = '\0';   // guarantee NUL termination

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

  // ── Assign image ID ────────────────────────────────────────────────────
  if(imageId < 0) {
    // Auto-increment, wrapping 255 → 0.
    this->imageId = (uint8_t)(this->imageId + 1u);
  } else {
    this->imageId = (uint8_t)((uint16_t)imageId & 0xFFu);
  }

  // ── Determine packet type ──────────────────────────────────────────────
  uint8_t type = this->fec ? SSDV_TYPE_NORMAL : SSDV_TYPE_NOFEC;

  // Discard any previously loaded image.
  this->freeBuffer();

  // ── Pass 1: count packets ──────────────────────────────────────────────
  // packetBuf is nullptr → runEncoder() counts without storing.
  int32_t countResult = this->runEncoder(type, jpegData, jpegLen, quality);
  if(countResult <= 0) {
    // Negative value is an error code from runEncoder.
    return((int16_t)countResult);
  }

  this->packetCount = (uint16_t)countResult;

  // ── Allocate flat packet buffer ───────────────────────────────────────
  // Total bytes = packetCount × 256.  Max possible: 65536 × 256 = 16 MB.
  // In practice HAB images are << 1000 packets.
  size_t bufSize = (size_t)this->packetCount * SSDV_PKT_SIZE;
  this->packetBuf = reinterpret_cast<uint8_t *>(malloc(bufSize));
  if(this->packetBuf == nullptr) {
    this->packetCount = 0;
    return(RADIOLIB_ERR_SSDV_ALLOC_FAILED);
  }

  // ── Pass 2: encode and store ───────────────────────────────────────────
  // packetBuf is non-nullptr → runEncoder() fills the buffer.
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
  // True if nothing is loaded or every packet has been sent.
  return(this->packetBuf == nullptr || this->currentPacket >= this->packetCount);
}

uint16_t SSDVClient::currentPacketIndex() {
  return(this->currentPacket);
}

int16_t SSDVClient::transmit(uint16_t* packetId) {
  // offset = 0 → send all 256 bytes including the 0x55 sync byte.
  return(this->transmitPacket(0, packetId));
}

int16_t SSDVClient::transmitLoRa(uint16_t* packetId) {
  // offset = 1 → skip the 0x55 sync byte; sends 255 bytes which fits
  // within the 255-byte LoRa payload limit.
  // The sync byte is only useful for RTTY stream decoders; LoRa has its
  // own frame synchronisation and does not need it.
  return(this->transmitPacket(1, packetId));
}

void SSDVClient::clearImage() {
  this->freeBuffer();
}
