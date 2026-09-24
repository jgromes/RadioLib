#if !defined(RADIOLIB_SSDV_H)
#define RADIOLIB_SSDV_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SSDV

#include "../PhysicalLayer/PhysicalLayer.h"

/*
  SSDV (Slow Scan Digital Video) packet layout, see https://ukhas.org.uk/doku.php?id=guides:ssdv

  Byte     Normal (FEC)       No-FEC            Content
  0        0x55               0x55              sync
  1        0x66               0x67              packet type
  2-5                                           callsign, base-40 encoded, MSB first
  6                                             image ID
  7-8                                           packet ID, MSB first
  9                                             image width / 16
  10                                            image height / 16
  11                                            [7:6] 0, [5:3] quality XOR 4, [2] EOI, [1:0] MCU mode
  12                                            offset of first MCU in payload (0xFF = none)
  13-14                                         index of first MCU, MSB first (0xFFFF = none)
  15-...   205 B payload      237 B payload     re-encoded JPEG scan data
  ...      CRC-32             CRC-32            over bytes 1 to end of payload
  224-255  RS(255,223) parity -                 over bytes 1-223
*/

// packet sizes in bytes
#define RADIOLIB_SSDV_PACKET_LEN                                (256)
#define RADIOLIB_SSDV_HEADER_LEN                                (15)
#define RADIOLIB_SSDV_CRC_LEN                                   (4)
#define RADIOLIB_SSDV_RS_LEN                                    (32)
#define RADIOLIB_SSDV_PAYLOAD_LEN_FEC                           (RADIOLIB_SSDV_PACKET_LEN - RADIOLIB_SSDV_HEADER_LEN - RADIOLIB_SSDV_CRC_LEN - RADIOLIB_SSDV_RS_LEN)
#define RADIOLIB_SSDV_PAYLOAD_LEN_NOFEC                         (RADIOLIB_SSDV_PACKET_LEN - RADIOLIB_SSDV_HEADER_LEN - RADIOLIB_SSDV_CRC_LEN)

// packet fields
#define RADIOLIB_SSDV_SYNC_WORD                                 (0x55)
#define RADIOLIB_SSDV_TYPE_OFFSET                               (0x66)
#define RADIOLIB_SSDV_TYPE_NORMAL                               (0x00)
#define RADIOLIB_SSDV_TYPE_NOFEC                                (0x01)
#define RADIOLIB_SSDV_TYPE_INVALID                              (0xFF)
#define RADIOLIB_SSDV_MCU_NONE                                  (0xFFFF)

// maximum callsign length in characters
#define RADIOLIB_SSDV_CALLSIGN_MAX_LEN                          (6)

// maximum quality level
#define RADIOLIB_SSDV_QUALITY_MAX                               (7)

/*!
  \brief Returned by SSDVClient::feedPacket when the last packet of an image was decoded.
*/
#define RADIOLIB_SSDV_EOI                                       (1)

/*!
  \struct SSDVPacketInfo_t
  \brief Header fields of an SSDV packet, filled by SSDVClient::parseHeader.
*/
struct SSDVPacketInfo_t {
  /*! \brief Packet type, RADIOLIB_SSDV_TYPE_NORMAL or RADIOLIB_SSDV_TYPE_NOFEC. */
  uint8_t type;

  /*! \brief Base-40 encoded callsign. */
  uint32_t callsignCode;

  /*! \brief Decoded, null-terminated callsign. */
  char callsign[RADIOLIB_SSDV_CALLSIGN_MAX_LEN + 1];

  /*! \brief Image ID. */
  uint8_t imageId;

  /*! \brief Packet ID, starting from 0. */
  uint16_t packetId;

  /*! \brief Image width in pixels. */
  uint16_t width;

  /*! \brief Image height in pixels. */
  uint16_t height;

  /*! \brief Whether this is the last packet of the image. */
  bool eoi;

  /*! \brief Quality level, 0 - 7. */
  uint8_t quality;

  /*! \brief Chroma subsampling mode: 0 = 2x2, 1 = 1x2, 2 = 2x1, 3 = 1x1. */
  uint8_t mcuMode;

  /*! \brief Offset of the first MCU in the payload. */
  uint8_t mcuOffset;

  /*! \brief Index of the first MCU in this packet, RADIOLIB_SSDV_MCU_NONE if no MCU starts here. */
  uint16_t mcuId;

  /*! \brief Total number of MCU blocks in the image. */
  uint16_t mcuCount;
};

// internal encoder/decoder state, defined in SSDV.cpp
struct SSDVState_t;

/*!
  \class SSDVClient
  \brief Client for SSDV image transmission and reception.
  Encodes a baseline JPEG into 256-byte SSDV packets and reassembles received packets into a JPEG.
  The JPEG must be baseline DCT, greyscale or YCbCr, with dimensions that are multiples of 16 (max. 4080 x 4080).
  Encoder and decoder are derived from ssdv by Philip Heron, https://github.com/fsphil/ssdv (GPL-3.0).
*/
class SSDVClient {
  public:
    /*!
      \brief Default constructor.
      \param phy Pointer to the wireless module providing PhysicalLayer communication.
      \param fec Set to true for Normal mode (205-byte payload + Reed-Solomon FEC, e.g. for RTTY/FSK),
      or false for No-FEC mode (237-byte payload, e.g. for LoRa which has its own FEC).
    */
    explicit SSDVClient(PhysicalLayer* phy, bool fec = true);

    /*!
      \brief Destructor, releases the packet buffer and decoder state.
    */
    ~SSDVClient();

    /*!
      \brief Initialization method.
      \param callsign Station callsign, up to 6 characters: A-Z, a-z, 0-9 and '-'.
      \returns \ref status_codes
    */
    int16_t begin(const char* callsign);

    // transmit methods

    /*!
      \brief Encode a JPEG image into SSDV packets. Any previously loaded image is discarded.
      The packets are stored on heap (numPackets() * 256 bytes), so the JPEG may be released afterwards.
      \param jpegData Complete JPEG file.
      \param jpegLen Length of the JPEG in bytes.
      \param quality Quality level 0 - 7, corresponding to JPEG quality 13, 18, 29, 43, 50, 71, 86 and 100. Defaults to 4.
      \param imageId Image ID 0 - 255, or -1 to increment the ID of the previous image. Defaults to -1.
      \returns \ref status_codes
    */
    int16_t setImage(const uint8_t* jpegData, size_t jpegLen, uint8_t quality = 4, int16_t imageId = -1);

    /*!
      \brief Get the number of packets of the current image.
      \returns Number of packets, 0 if no image is loaded.
    */
    uint16_t numPackets();

    /*!
      \brief Check whether all packets of the current image have been sent.
      \returns True if all packets were sent or no image is loaded, false otherwise.
    */
    bool isDone();

    /*!
      \brief Get the index of the next packet to be transmitted.
      \returns Packet index, equal to numPackets() when done.
    */
    uint16_t currentPacketIndex();

    /*!
      \brief Transmit the next packet, all 256 bytes including sync word. Intended for RTTY/FSK.
      \param packetId If not NULL, will be set to the index of the transmitted packet.
      \returns Number of packets transmitted so far, or a negative \ref status_codes on error.
    */
    int16_t transmit(uint16_t* packetId = NULL);

    /*!
      \brief Transmit the next packet without the sync word (255 bytes), to fit the LoRa payload limit.
      \param packetId If not NULL, will be set to the index of the transmitted packet.
      \returns Number of packets transmitted so far, or a negative \ref status_codes on error.
    */
    int16_t transmitLoRa(uint16_t* packetId = NULL);

    /*!
      \brief Discard the current image and release the packet buffer.
    */
    void clearImage();

    // receive methods

    /*!
      \brief Read a received packet from the radio.
      The packet is not validated, call isValidPacket() on it.
      \param packet Buffer of at least 256 bytes.
      \returns \ref status_codes
    */
    int16_t read(uint8_t* packet);

    /*!
      \brief Validate a packet by its CRC. If that fails, Reed-Solomon correction is attempted
      (up to 16 symbol errors) and on success the corrected packet is written back.
      \param packet 256-byte packet.
      \param errors If not NULL, will be set to the number of corrected symbol errors.
      \returns \ref status_codes
    */
    static int16_t isValidPacket(uint8_t* packet, int16_t* errors = NULL);

    /*!
      \brief Parse the header of a validated packet.
      \param packet 256-byte packet.
      \param info Structure to fill.
    */
    static void parseHeader(const uint8_t* packet, SSDVPacketInfo_t* info);

    /*!
      \brief Initialize the JPEG decoder.
      A safe output buffer size is (number of packets) * (payload length) + 2048 bytes, e.g. about 16 kB for 320 x 240 at quality 4.
      \param jpegBuf Output buffer for the reconstructed JPEG.
      \param jpegBufLen Size of the output buffer in bytes.
      \returns \ref status_codes
    */
    int16_t beginDecoder(uint8_t* jpegBuf, size_t jpegBufLen);

    /*!
      \brief Feed a validated packet to the decoder. Packets must arrive in ascending order;
      missing packets are replaced by blank MCU blocks.
      \param packet 256-byte packet.
      \returns RADIOLIB_SSDV_EOI when the image is complete, otherwise \ref status_codes
    */
    int16_t feedPacket(const uint8_t* packet);

    /*!
      \brief Get the reconstructed JPEG after feedPacket() returned RADIOLIB_SSDV_EOI.
      \param jpegPtr Will be set to the start of the JPEG in the output buffer.
      \param jpegLen Will be set to the length of the JPEG in bytes.
      \returns \ref status_codes
    */
    int16_t getJpeg(uint8_t** jpegPtr, size_t* jpegLen);

    /*!
      \brief Reset the decoder to receive a new image into the same output buffer.
      \returns \ref status_codes
    */
    int16_t resetDecoder();

    /*!
      \brief Release the decoder state. The output buffer is owned by the caller and is not released.
    */
    void endDecoder();

#if !RADIOLIB_GODMODE
  private:
#endif
    PhysicalLayer* phyLayer;
    bool fec;
    bool initialized;
    char callsign[RADIOLIB_SSDV_CALLSIGN_MAX_LEN + 1];
    uint8_t imageId;

    // transmitter
    uint8_t* packetBuf;
    uint16_t packetCount;
    uint16_t packetIndex;

    // receiver
    SSDVState_t* decoder;
    uint8_t* jpegBuf;
    size_t jpegBufLen;
    size_t jpegLen;
    bool jpegReady;

    SSDVClient(const SSDVClient&);
    SSDVClient& operator=(const SSDVClient&);

    int32_t encode(const uint8_t* jpegData, size_t jpegLen, uint8_t quality, uint8_t* dst, uint16_t maxPackets);
    int16_t transmitPacket(uint8_t offset, uint16_t* packetId);
};

#endif

#endif
