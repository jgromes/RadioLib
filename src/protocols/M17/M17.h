#if !defined(_RADIOLIB_M17_H)
#define _RADIOLIB_M17_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_M17

#include "../PhysicalLayer/PhysicalLayer.h"
#include "../FSK4/FSK4.h"

// basic M17 properties
#define RADIOLIB_M17_SHIFT_HZ                                   (1600)
#define RADIOLIB_M17_RATE_BAUD                                  (4800)

// preamble
#define RADIOLIB_M17_PRE_LEN_BYTES                              (192/4)
#define RADIOLIB_M17_PRE_PATTERN_LSF                            (0x77)
#define RADIOLIB_M17_PRE_PATTERN_BERT                           (0xDD)

// end-of-transmission
#define RADIOLIB_M17_EOT_LEN_BYTES                              (192/4)
#define RADIOLIB_M17_EOT_PATTERN                                (0x555D)

// sync-burst
#define RADIOLIB_M17_SYNC_BURST_LSF                             (0x55F7)
#define RADIOLIB_M17_SYNC_BURST_BERT                            (0xDF55)
#define RADIOLIB_M17_SYNC_BURST_STREAM                          (0xFF5D)
#define RADIOLIB_M17_SYNC_BURST_PACKET                          (0x75FF)

// link setup frame (LFS) bit fields                                                MSB   LSB   DESCRIPTION
#define RADIOLIB_M17_LSF_MODE_PACKET                            (0x00UL << 0)   //  0     0     LSF packet/stream indicator: packet
#define RADIOLIB_M17_LSF_MODE_STREAM                            (0x01UL << 0)   //  0     0                                  stream
#define RADIOLIB_M17_LSF_DATA_TYPE_DATA                         (0x01UL << 1)   //  2     1     data type: data
#define RADIOLIB_M17_LSF_DATA_TYPE_VOICE                        (0x02UL << 1)   //  2     1                voice
#define RADIOLIB_M17_LSF_DATA_TYPE_VOICE_DATA                   (0x03UL << 1)   //  2     1                data + voice
#define RADIOLIB_M17_LSF_ENC_NONE                               (0x00UL << 3)   //  4     3     encryption: none
#define RADIOLIB_M17_LSF_ENC_SCRAMBLER                          (0x01UL << 3)   //  4     3                 scrambler
#define RADIOLIB_M17_LSF_ENC_AES                                (0x02UL << 3)   //  4     3                 AES
#define RADIOLIB_M17_LSF_ENC_OTHER                              (0x03UL << 3)   //  4     3                 other
#define RADIOLIB_M17_LSF_AES_LEN_128                            (0x00UL << 5)   //  6     5     encryption key length: 128-bit
#define RADIOLIB_M17_LSF_AES_LEN_192                            (0x01UL << 5)   //  6     5                            192-bit
#define RADIOLIB_M17_LSF_AES_LEN_256                            (0x02UL << 5)   //  6     5                            256-bit
#define RADIOLIB_M17_LSF_SCRAMLER_LEN_8                         (0x00UL << 5)   //  6     5     scrambler length: 8-bit
#define RADIOLIB_M17_LSF_SCRAMLER_LEN_16                        (0x01UL << 5)   //  6     5                       16-bit
#define RADIOLIB_M17_LSF_SCRAMLER_LEN_24                        (0x02UL << 5)   //  6     5                       24-bit

// maximum length of LSF frame before puncturing
#define RADIOLIB_M17_LSF_MAXLEN_BYTES_ENCODED                   (368/8)

#define RADIOLIB_M17_ADDR_LEN                                   (6)

#define RADIOLIB_M17_RANDOMIZER_LEN                             (46)

static const uint8_t m17_randomizer[RADIOLIB_M17_RANDOMIZER_LEN] = {
  0xD6, 0xB5, 0xE2, 0x30, 0x82, 0xFF, 0x84, 0x62,
  0xBA, 0x4E, 0x96, 0x90, 0xD8, 0x98, 0xDD, 0x5D,
  0x0C, 0xC8, 0x52, 0x43, 0x91, 0x1D, 0xF8, 0x6E,
  0x68, 0x2F, 0x35, 0xDA, 0x14, 0xEA, 0xCD, 0x76,
  0x19, 0x8D, 0xD5, 0x80, 0xD1, 0x33, 0x87, 0x13,
  0x57, 0x18, 0x2D, 0x29, 0x78, 0xC3
};

/*!
  \class M17Client
  \brief Client for M17 transmissions.
*/
class M17Client: public FSK4Client {
  public:
    /*!
      \brief Constructor for 4-FSK mode.
      \param phy Pointer to the wireless module providing PhysicalLayer communication.
    */
    explicit M17Client(PhysicalLayer* phy);

    /*!
      \brief Initialization method.
      \param base Base (space) frequency to be used in MHz.
      \returns \ref status_codes
    */
    int16_t begin(float base, char* addr);

    int16_t transmit(uint8_t* data, size_t len, char* dst);

#if !RADIOLIB_GODMODE
  private:
#endif
    PhysicalLayer* phyLayer;
    uint8_t src[RADIOLIB_M17_ADDR_LEN] = { 0 };
    uint8_t randIndex = 0;

    int16_t encodeAddr(char* in, uint8_t* out);
    size_t encodeLsf(char* dst, uint16_t type, uint8_t* out, uint8_t* meta = NULL, size_t metaLen = 0);
    void randomize(uint8_t* buff, size_t len);
};

#endif

#endif