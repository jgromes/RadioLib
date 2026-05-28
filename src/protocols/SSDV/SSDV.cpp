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
 * Reception implementation notes
 * ───────────────────────────────
 * receiveLoRa() / receive() call PhysicalLayer::receive() to collect raw
 * bytes from the radio.  The sync byte (0x55) is prepended as needed for
 * LoRa packets.
 *
 * isValidPacket() delegates directly to ssdv_dec_is_packet(), which:
 *   1. Tests the CRC-32 of the payload.
 *   2. If the CRC fails for a Normal-mode packet, runs the RS(255,223)
 *      decoder to correct up to 16 byte errors, then re-checks the CRC.
 *   3. Copies the corrected packet back to the caller's buffer if valid.
 *
 * JPEG reconstruction (decoder) notes
 * ─────────────────────────────────────
 * The ssdv decoder state (ssdv_t) is heap-allocated in beginDecoder() and
 * freed in endDecoder() / destructor.  The JPEG output buffer is
 * caller-supplied and NOT owned by this class.
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
    imageId(0xFF),         // rolls to 0x00 on the first auto-inc setImage()
    decoder(nullptr),
    decoderInitialized(false),
    jpegReady(false),
    userJpegBuf(nullptr),
    userJpegBufLen(0)
{
  memset(callsign, 0, sizeof(callsign));
}

SSDVClient::~SSDVClient() {
  this->freeBuffer();
  this->endDecoder();
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

int16_t SSDVClient::receive(uint8_t* packet) {
  if(packet == nullptr) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }
  // Receive a full 256-byte packet directly from the radio.
  // PhysicalLayer::receive() blocks until a packet arrives or times out.
  return(phy->receive(packet, SSDV_PKT_SIZE));
}

int16_t SSDVClient::receiveLoRa(uint8_t* packet) {
  if(packet == nullptr) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }

  // LoRa payloads are limited to 255 bytes.  The transmitter omits the 0x55
  // sync byte (byte 0), so the radio payload is SSDV bytes [1 … 255].
  // We receive into packet[1] and then fill packet[0] = 0x55 ourselves so
  // that the downstream validation and decoder APIs see a full 256-byte packet.
  int16_t state = phy->readData(packet + 1, SSDV_PKT_SIZE - 1);
  if(state != RADIOLIB_ERR_NONE) {
    return(state);
  }

  packet[0] = 0x55;
  return(RADIOLIB_ERR_NONE);
}

int16_t SSDVClient::isValidPacket(uint8_t* packet, int* errors) {
  if(packet == nullptr) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }

  // ssdv_dec_is_packet() works on an internal copy of the packet.
  // If validation succeeds (possibly after RS correction), the corrected
  // packet is written back to 'packet' in place.
  // Returns 0 on success, -1 on failure.
  int errs = 0;
  int r = ssdv_dec_is_packet(packet, &errs);

  if(errors != nullptr) {
    *errors = errs;
  }

  if(r != 0) {
    return(RADIOLIB_ERR_SSDV_INVALID_PACKET);
  }

  return(RADIOLIB_ERR_NONE);
}

/*static*/
void SSDVClient::parseHeader(uint8_t* packet, ssdv_packet_info_t* info) {
  // Delegate directly to the ssdv C library.
  // The packet must have been validated by isValidPacket() first.
  ssdv_dec_header(info, packet);
}

int16_t SSDVClient::beginDecoder(uint8_t* jpegBuf, size_t jpegBufLen) {
  if(jpegBuf == nullptr || jpegBufLen == 0) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }

  // Free any previously allocated decoder state.
  endDecoder();

  // Heap-allocate the ssdv_t decoder state.
  // ssdv_t is a large struct (~2 KB) containing Huffman and quantisation
  // table arrays; heap allocation avoids blowing the stack.
  decoder = reinterpret_cast<ssdv_t*>(malloc(sizeof(ssdv_t)));
  if(decoder == nullptr) {
    return(RADIOLIB_ERR_SSDV_ALLOC_FAILED);
  }

  // Initialise the decoder (sets up Huffman tables, resets all state).
  char r = ssdv_dec_init(decoder);
  if(r != SSDV_OK) {
    free(decoder);
    decoder = nullptr;
    return(RADIOLIB_ERR_SSDV_DECODE_FAILED);
  }

  // Point the decoder at the caller-supplied JPEG output buffer.
  // ssdv_dec_set_buffer() stores buffer/length and resets the write pointer.
  ssdv_dec_set_buffer(decoder, jpegBuf, jpegBufLen);

  userJpegBuf    = jpegBuf;
  userJpegBufLen = jpegBufLen;
  decoderInitialized = true;
  jpegReady      = false;

  return(RADIOLIB_ERR_NONE);
}

int16_t SSDVClient::feedPacket(const uint8_t* packet) {
  if(!decoderInitialized || decoder == nullptr) {
    return(RADIOLIB_ERR_SSDV_DECODER_NOT_INITIALIZED);
  }
  if(packet == nullptr) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }

  // ssdv_dec_feed() takes a non-const pointer because the C API predates
  // const-correctness; it does not modify the packet data.
  char r = ssdv_dec_feed(decoder, const_cast<uint8_t*>(packet));

  switch(r) {

    // ── SSDV_OK (0): EOI found — image is complete ──────────────────────
    case SSDV_OK:
      jpegReady = true;
      return(RADIOLIB_SSDV_EOI);

    // ── SSDV_FEED_ME (1): packet processed, more packets needed ─────────
    case SSDV_FEED_ME:
      return(RADIOLIB_ERR_NONE);

    // ── SSDV_ERROR (0x7F) or any unexpected value ────────────────────────
    default:
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ssdv_dec_feed() error: %d", (uint8_t)r);
      return(RADIOLIB_ERR_SSDV_DECODE_FAILED);
  }
}

int16_t SSDVClient::getJpeg(uint8_t** jpegPtr, size_t* jpegLen) {
  if(!decoderInitialized || decoder == nullptr) {
    return(RADIOLIB_ERR_SSDV_DECODER_NOT_INITIALIZED);
  }
  if(jpegPtr == nullptr || jpegLen == nullptr) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }
  if(!jpegReady) {
    // feedPacket() has not yet returned RADIOLIB_SSDV_EOI.
    return(RADIOLIB_ERR_SSDV_NO_JPEG);
  }

  // ssdv_dec_get_jpeg() finalises the JPEG stream: it appends any missing
  // MCUs (filled with solid grey), synchronises the bit stream, and writes
  // the EOI marker.  It then sets *jpegPtr and *jpegLen to describe the
  // output in the user-supplied buffer.
  char r = ssdv_dec_get_jpeg(decoder, jpegPtr, jpegLen);
  if(r != SSDV_OK) {
    return(RADIOLIB_ERR_SSDV_DECODE_FAILED);
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t SSDVClient::resetDecoder() {
  if(!decoderInitialized || decoder == nullptr) {
    return(RADIOLIB_ERR_SSDV_DECODER_NOT_INITIALIZED);
  }

  // Re-initialise decoder state tables without reallocating.
  char r = ssdv_dec_init(decoder);
  if(r != SSDV_OK) {
    return(RADIOLIB_ERR_SSDV_DECODE_FAILED);
  }

  // Reset the write pointer to the start of the user-supplied buffer.
  ssdv_dec_set_buffer(decoder, userJpegBuf, userJpegBufLen);

  jpegReady = false;
  return(RADIOLIB_ERR_NONE);
}

void SSDVClient::endDecoder() {
  if(decoder != nullptr) {
    free(decoder);
    decoder = nullptr;
  }
  decoderInitialized = false;
  jpegReady          = false;
  userJpegBuf        = nullptr;
  userJpegBufLen     = 0;
}
