/*
   SSDV (Slow Scan Digital Video) transmit/receive client
  
   Encodes a JPEG image into the SSDV packet format defined at
     https://ukhas.org.uk/doku.php?id=guides:ssdv
   and transmits or receives each 256-byte packet via any radio.
  
   Depends on:
     • fsphil/ssdv  — ssdv_coding.h / ssdv_coding.c  (encoder/decoder API)
     • rs8.h / rs8.c (Phil Karn KA9Q Reed-Solomon codec)
  
   SSDV packet layout (256 bytes, FEC / Normal mode):
     Byte   0    : 0x55  sync
     Byte   1    : type
                    0x66 = Normal (FEC on)   (SSDV_TYPE_NORMAL)
                    0x67 = No-FEC            (SSDV_TYPE_NOFEC)
     Bytes  2–5  : callsign, base-40 encoded (big-endian uint32_t)
     Byte   6    : image ID  (0–255)
     Bytes  7–8  : packet ID (big-endian uint16_t)
     Byte   9    : width  in MCU blocks  (pixels/16; max 255 → 4080 px)
     Byte  10    : height in MCU blocks
     Byte  11    : flags  bits [6:4] = quality XOR 4; [3:2] = mcu_mode;
                          bit  [2]   = EOI; bits [1:0] = 0
     Byte  12    : MCU offset within first MCU of this packet
     Bytes 13–14 : MCU index (big-endian uint16_t); 0xFFFF = no new MCU
   Normal mode:
     Bytes 15–219  : payload (205 bytes of re-encoded JPEG scan data)
     Bytes 220–223 : CRC-32  (big-endian, covers bytes 1–219)
     Bytes 224–255 : Reed-Solomon parity (32 bytes, covers bytes 1–219 + CRC)
   No-FEC mode:
     Bytes 15–251  : payload (237 bytes)
     Bytes 252–255 : CRC-32
     (no RS bytes)
  
   Transmission notes:
     LoRa payloads are limited to 255 bytes; the 0x55 sync byte is omitted.
     Use transmitLoRa() when operating over LoRa.
     Use transmit() for FSK/RTTY links that carry all 256 bytes.
  
   JPEG image requirements (enforced by the ssdv encoder):
     • Width and height must be multiples of 16 (up to 4080 × 4080)
     • Baseline DCT (not progressive)
     • Greyscale or YCbCr colour space (not RGB / CMYK)
     • Total MCU count ≤ 65535
 */

#ifndef RADIOLIB_SSDV_CLIENT_H
#define RADIOLIB_SSDV_CLIENT_H

#include <RadioLib.h>

// SSDV + Reed-Solomon C headers
extern "C" {
#include "ssdv_coding.h"
}

// signals a decoded JPEG
#define RADIOLIB_SSDV_EOI                         (1)

/*!
   \class SSDVClient
   \brief SSDV image transmitter and receiver.
*/
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

  /*!
     \brief Get the SSDV packet from the radio.
     The returned buffer is NOT validated — call isValidPacket() to check.
     \param packet buffer of at least 256 bytes.
     \returns \ref status_codes
   */
  int16_t read(uint8_t* packet);

  /*!
     \brief Validate an SSDV packet and optionally correct bit errors.
     For Normal-mode (FEC) packets, the Reed-Solomon decoder is run first
     if the CRC fails; up to 16 byte errors in the 224-byte codeword can
     be corrected.  If RS correction succeeds, the corrected bytes are
     written back in place.
     For No-FEC packets, only the CRC-32 is checked (no correction).
     \param packet  256-byte SSDV packet.
     \param errors  Optional: number of RS symbol errors corrected.
                    0 = no errors; >0 = corrected; -1 = uncorrectable
     \returns \ref status_codes
   */
  static int16_t isValidPacket(uint8_t* packet, int* errors = nullptr);

  /*!
     \brief Parse the header fields of a validated 256-byte SSDV packet.
     \param packet  Validated 256-byte SSDV packet (read-only).
     \param info    Pointer to an \c ssdv_packet_info_t struct to fill.
    
     \par Fields populated in \p info:
       - \c type           Packet type: SSDV_TYPE_NORMAL or SSDV_TYPE_NOFEC.
       - \c callsign       Encoded callsign (uint32_t, base-40).
       - \c callsign_s     Decoded callsign as a NUL-terminated C string.
       - \c image_id       Image identifier (0–255).
       - \c packet_id      Packet sequence number (0-based).
       - \c width          Image width in pixels (multiple of 16).
       - \c height         Image height in pixels (multiple of 16).
       - \c eoi            1 if this is the last packet of the image; 0 otherwise.
       - \c quality        SSDV quality level (0–7).
       - \c mcu_mode       Chroma subsampling mode (0=4:2:0 2x2, 1=2x1,
                            2=1x2, 3=4:4:4 1x1).
       - \c mcu_offset     Byte offset of the first MCU within the packet payload.
       - \c mcu_id         MCU block index for the first MCU in this packet.
       - \c mcu_count      Total number of MCU blocks in the image.
   */
  static void parseHeader(uint8_t* packet, ssdv_packet_info_t* info);

  /*!
     \brief Initialise the JPEG decoder with an output buffer.
     The output buffer must be large enough to hold the reconstructed JPEG.
     The reconstructed JPEG is slightly larger than the raw SSDV payload
     because JPEG headers (~1 KB) are added.  A safe upper bound:
       jpegBufLen = numExpectedPackets × SSDV_PKT_SIZE_PAYLOAD + 2048
     Where SSDV_PKT_SIZE_PAYLOAD is 205 (FEC mode) or 237 (no-FEC mode).
    
     Practical sizing examples at SSDV quality level 4:
       • 160 × 120 image → ~20 packets → ~6 KB buffer
       • 320 × 240 image → ~60 packets → ~16 KB buffer
       • 640 × 480 image → ~250 packets → ~55 KB buffer
    
     \param jpegBuf     output buffer for the JPEG.
     \param jpegBufLen  Size of \p jpegBuf in bytes.
     \returns \ref status_codes.
   */
  int16_t beginDecoder(uint8_t* jpegBuf, size_t jpegBufLen);

  /*!
     \brief Feed one validated SSDV packet to the JPEG decoder.
    
     Packets must be fed in order (ascending packet_id).  The decoder
     can bridge gaps caused by missing packets by interpolating missing
     MCU blocks; however, image quality degrades with each missing packet.
    
     \param packet validated 256-byte SSDV packet.
     \returns \ref status_codes
   */
  int16_t feedPacket(const uint8_t* packet);

  /*!
     \brief Retrieve the reconstructed JPEG after image reception is complete.
     \param jpegPtr  Set to the start of the reconstructed JPEG data.
     \param jpegLen  Set to the length of the JPEG data in bytes.
     \returns \ref status_codes.
   */
  int16_t getJpeg(uint8_t** jpegPtr, size_t* jpegLen);

  /*!
     \brief Reset the decoder state to receive a new image.
     \returns \ref status_codes.
   */
  int16_t resetDecoder();

  /*!
     \brief Release decoder resources and free the internal ssdv_t state.
     This does not release the user's JPEG buffer
   */
  void endDecoder();

private:
  SSDVClient(const SSDVClient&);
  SSDVClient& operator=(const SSDVClient&);

  PhysicalLayer* phy;      // radio
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
