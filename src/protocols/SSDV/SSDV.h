/*
 * SSDVClient.h — SSDV (Slow Scan Digital Video) transmit client
 *
 * Encodes a JPEG image into the SSDV packet format defined at
 *   https://ukhas.org.uk/doku.php?id=guides:ssdv
 * and transmits each 256-byte packet via any radio.
 *
 * Depends on:
 *   • fsphil/ssdv  — ssdv.h / ssdv.c  (packet building + JPEG re-encoding)
 *   • rs8.h / rs8.c (Phil Karn KA9Q Reed-Solomon codec)
 *
 * SSDV packet layout (256 bytes, FEC / Normal mode):
 *   Byte   0    : 0x55  sync
 *   Byte   1    : 0x00  type (Normal = FEC on)
 *   Bytes  2–5  : callsign encoded base-40 (big-endian uint32_t)
 *   Byte   6    : image ID  (0–255)
 *   Bytes  7–8  : packet ID (big-endian uint16_t)
 *   Byte   9    : width  in MCU blocks  (pixels/16 – 1, max 255 → 4080 px)
 *   Byte  10    : height in MCU blocks
 *   Byte  11    : flags  [7]=EOI [6:4]=quality(0–7) [3:2]=mcu_mode [1:0]=0
 *   Bytes 12–13   : MCU index (big-endian uint16_t)
 *   Byte  14    : MCU offset within packet
 *   Bytes 15–219  : payload (205 bytes of re-encoded JPEG scan data)
 *   Bytes 220–223 : CRC-32  (big-endian, covers bytes 1–219)
 *   Bytes 224–255 : Reed-Solomon parity (32 bytes, covers bytes 1–219 + CRC)
 *
 * No-FEC mode (type 0x01):
 *   Bytes 15–251  : payload (237 bytes)
 *   Bytes 252–255 : CRC-32
 *   (no RS bytes)
 *
 * JPEG image requirements (enforced by the ssdv encoder):
 *   • Width and height must be multiples of 16 (up to 4080 × 4080)
 *   • Baseline DCT (not progressive)
 *   • Greyscale or YCbCr colour space (not RGB / CMYK)
 *   • Total MCU count ≤ 65535
 */

#ifndef RADIOLIB_SSDV_CLIENT_H
#define RADIOLIB_SSDV_CLIENT_H

#include <stdint.h>
#include <stddef.h>
#include <RadioLib.h>

// ── SSDV + Reed-Solomon C headers ─────────────────────────────────────────
extern "C" {
#include "ssdv_coding.h"   // fsphil/ssdv — encoder/decoder API + ssdv_packet_info_t
#include "rs8.h"           // Phil Karn KA9Q RS(255,223) codec
}

/*!
    \brief Returned by feedPacket() when the last (EOI) SSDV packet of an
          image has been successfully processed.  Call getJpeg() to
          retrieve the reconstructed JPEG.
  */
#define RADIOLIB_SSDV_EOI                         (1)

// ──────────────────────────────────────────────────────────────────────────

/*!
   \class SSDVClient
   \brief SSDV image transmitter and receiver.
class SSDVClient {
public:

  /*!
     \brief Construct an SSDVClient.
    
     \param phy  Pointer to an initialised RadioLib PhysicalLayer object
                 (SX1278, SX1262, CC1101, …).
     \param fec  true  → Normal mode: 205-byte payload + 32-byte RS FEC.
                         Use for RTTY, FSK — modes without built-in FEC.
                 false → No-FEC mode: 237-byte payload, no RS codes.
                         Use for LoRa, which provides its own FEC.
   */
  explicit SSDVClient(PhysicalLayer* phy, bool fec = true);

  /*!
     \brief Destructor.  Frees internally allocated buffers (packet buffer
            and decoder state).
   */
  ~SSDVClient();

  /*!
     \brief Initialise the client with a station callsign.
    
     Must be called before setImage() / transmit() / transmitLoRa().
     Not required for receive-only use.
     Can be called again at any time to change the callsign.
    
     \param callsign  Up to 6 characters: A–Z, 0–9, '-', '/', '.'.
     \returns \ref status_codes
   */
  int16_t begin(const char* callsign);

  /*!
     \brief Load and encode a JPEG image ready for transmission.
    
     The entire JPEG is re-encoded into SSDV packets in one call and
     stored in a heap-allocated buffer.  Any previously loaded image is
     discarded.
    
     \param jpegData  Pointer to a complete JPEG file in memory.
     \param jpegLen   Length of the JPEG data in bytes.
     \param quality   SSDV quality level 0–7.
                      Quality mapping: 0→13, 1→18, 2→29, 3→43, 4→50,
                      5→71, 6→86, 7→100 (approx. JPEG quality).
                      Level 4 (quality 50) is a good default.
     \param imageId   Image identifier embedded in every packet header
                      (0–255).  Pass -1 (default) to auto-increment on each
                      call, wrapping around after 255.
     \returns \ref status_codes
   */
  int16_t setImage(const uint8_t* jpegData, size_t jpegLen,
                   uint8_t quality = 4, int16_t imageId = -1);

  /*!
     \brief Return the total number of SSDV packets for the current image.
     \returns Packet count, or 0 if no image has been loaded.
   */
  uint16_t numPackets();

  /*!
     \brief Return whether all packets for the current image have been sent.
     \returns true if done or no image is loaded; false if packets remain.
   */
  bool isDone();

  /*!
     \brief Return the index of the next packet to be transmitted (0-based).
     \returns Index in [0, numPackets()); equals numPackets() when done.
   */
  uint16_t currentPacketIndex();

  /*!
     \brief Transmit the next SSDV packet (all 256 bytes including sync).
    
     Use for RTTY / FSK links.  For LoRa, use transmitLoRa() instead.
    
     \param packetId  Optional: if non-NULL, the 0-based index of the
                      transmitted packet is stored here.
     \returns Number of packets transmitted so far (>0) on success,
              or a negative \ref status_codes on error.
   */
  int16_t transmit(uint16_t* packetId = NULL);

  /*!
     \brief Transmit the next packet omitting the leading 0x55 sync byte.
    
     Sends bytes [1 … 255] of the SSDV packet (255 bytes total), fitting
     within the 255-byte LoRa payload limit.
    
     \param packetId  Optional output pointer for the 0-based packet index.
     \returns Number of packets transmitted so far (>0) on success,
              or a negative \ref status_codes on error.
   */
  int16_t transmitLoRa(uint16_t* packetId = NULL);

  /*!
     \brief Discard the current image and free the packet buffer.
    
     After this call numPackets() returns 0 and isDone() returns true.
   */
  void clearImage();

private:
  SSDVClient(const SSDVClient&);
  SSDVClient& operator=(const SSDVClient&);

  PhysicalLayer* phy;      // Underlying radio object.
  bool           fec;      // true = Normal (RS-FEC); false = No-FEC.

  bool     initialized;
  char     callsign[SSDV_MAX_CALLSIGN + 1]; // NULL-terminated callsign.
  uint8_t* packetBuf;               // packetCount × SSDV_PKT_SIZE.
  uint16_t packetCount;             // total encoded packets for current image.
  uint16_t currentPacket;           // index of next packet to transmit.
  uint8_t  imageId;                 // auto-incrementing image counter.

  ssdv_t*  decoder;                 // Heap-allocated ssdv_t decoder state.
  bool     decoderInitialized;      // beginDecoder() called successfully.
  bool     jpegReady;               // feedPacket() returned RADIOLIB_SSDV_EOI.
  uint8_t* userJpegBuf;             // Caller-supplied JPEG output buffer.
  size_t   userJpegBufLen;          // Size of userJpegBuf.

  /*!
     \brief Free the transmit packet buffer and reset counters.
   */
  void freeBuffer();

  /*!
     \brief Run the ssdv encoder over \p jpegData.
    
     Pass 1 (count): packetBuf == nullptr → count packets without storing.
     Pass 2 (store): packetBuf != nullptr → fill the pre-allocated buffer.
    
     \param type      SSDV_TYPE_NORMAL or SSDV_TYPE_NOFEC.
     \param jpegData  Source JPEG bytes (read-only).
     \param jpegLen   Source JPEG length in bytes.
     \param quality   SSDV quality level 0–7.
     \returns Number of packets encoded (≥ 1) on success, or a negative
              \ref status_codes on failure.
   */
  int32_t runEncoder(uint8_t type, const uint8_t* jpegData,
                     size_t jpegLen, uint8_t quality);

  /*!
     \brief Internal transmit helper.
     \param offset    Byte offset to start transmitting from (0 = full packet;
                      1 = skip sync byte for LoRa).
     \param packetId  Optional output pointer for the packet index.
  */
  int16_t transmitPacket(uint8_t offset, uint16_t* packetId);
};

#endif // RADIOLIB_SSDV_CLIENT_H
