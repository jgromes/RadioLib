#if !defined(_RADIOLIB_PAGER_H) && !defined(RADIOLIB_EXCLUDE_PAGER)
#define _RADIOLIB_PAGER_H

#include "../../TypeDef.h"
#include "../PhysicalLayer/PhysicalLayer.h"

// frequency shift in Hz
#define RADIOLIB_PAGER_FREQ_SHIFT_HZ                            (4500)

// supported encoding schemes
#define RADIOLIB_PAGER_ASCII                                    (0)
#define RADIOLIB_PAGER_BCD                                      (1)

// preamble length in 32-bit code words
#define RADIOLIB_PAGER_PREAMBLE_LENGTH                          (18)

// protocol-specified code words
#define RADIOLIB_PAGER_PREAMBLE_CODE_WORD                       (0xAAAAAAAA)
#define RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD                     (0x7CD215D8)
#define RADIOLIB_PAGER_IDLE_CODE_WORD                           (0x7A89C197)

// code word type identification flags
#define RADIOLIB_PAGER_ADDRESS_CODE_WORD                        (0UL)
#define RADIOLIB_PAGER_MESSAGE_CODE_WORD                        (1UL)

// length of code word in bits
#define RADIOLIB_PAGER_CODE_WORD_LEN                            (32)

// number of message bits in a single code block
#define RADIOLIB_PAGER_ADDRESS_POS                              (13)
#define RADIOLIB_PAGER_FUNC_BITS_POS                            (11)
#define RADIOLIB_PAGER_MESSAGE_BITS_LENGTH                      (20)
#define RADIOLIB_PAGER_MESSAGE_END_POS                          (11)

// number of code words in a batch
#define RADIOLIB_PAGER_BATCH_LEN                                (16)

// mask for address bits in a single code word
#define RADIOLIB_PAGER_ADDRESS_BITS_MASK                        (0x7FFFE000UL)

// mask for function bits in a single code word
#define RADIOLIB_PAGER_FUNCTION_BITS_MASK                       (0x00001800UL)

// mask for BCH bits in a single code word
#define RADIOLIB_PAGER_BCH_BITS_MASK                            (0x000007FFUL)

// message type functional bits
#define RADIOLIB_PAGER_FUNC_BITS_NUMERIC                        (0b00UL << RADIOLIB_PAGER_FUNC_BITS_POS)
#define RADIOLIB_PAGER_FUNC_BITS_TONE                           (0b01UL << RADIOLIB_PAGER_FUNC_BITS_POS)
#define RADIOLIB_PAGER_FUNC_BITS_ALPHA                          (0b11UL << RADIOLIB_PAGER_FUNC_BITS_POS)

// the maximum allowed address (2^22 - 1)
#define RADIOLIB_PAGER_ADDRESS_MAX                              (2097151)

// BCH(31, 21) code constants
#define RADIOLIB_PAGER_BCH_M                                    (5)
#define RADIOLIB_PAGER_BCH_N                                    (31)
#define RADIOLIB_PAGER_BCH_K                                    (21)
#define RADIOLIB_PAGER_BCH_D                                    (5)

 // BCH(31, 21) primitive polynomial x^5 + x^2 + 1
#define RADIOLIB_PAGER_BCH_PRIMITIVE_POLY                       (0x25)

/*!
  \class PagerClient

  \brief Client for Pager communication.
*/
class PagerClient {
  public:
    /*!
      \brief Default constructor.

      \param phy Pointer to the wireless module providing PhysicalLayer communication.
    */
    explicit PagerClient(PhysicalLayer* phy);

    // basic methods

    /*!
      \brief Initialization method.

      \param base Base (center) frequency to be used in MHz.

      \param speed Bit rate to use in bps. Common POCSAG decoders can receive 512, 1200 and 2400 bps.

      \returns \ref status_codes
    */
    int16_t begin(float base, uint16_t speed, uint16_t shift = RADIOLIB_PAGER_FREQ_SHIFT_HZ);

    /*!
      \brief Method to send a tone-only alert to a destination pager.

      \param addr Address of the destination pager. Allowed values are 0 to 2097151 - values above 2000000 are reserved.

      \returns \ref status_codes
    */
    int16_t sendTone(uint32_t addr);

    /*!
      \brief Arduino String transmit method.

      \param str Address of Arduino string that will be transmitted.

      \param addr Address of the destination pager. Allowed values are 0 to 2097151 - values above 2000000 are reserved.

      \param encoding Encoding to be used (BCD or ASCII). Defaults to RADIOLIB_PAGER_BCD.

      \returns \ref status_codes
    */
    int16_t transmit(String& str, uint32_t addr, uint8_t encoding = RADIOLIB_PAGER_BCD);

    /*!
      \brief C-string transmit method.

      \param str C-string that will be transmitted.

      \param addr Address of the destination pager. Allowed values are 0 to 2097151 - values above 2000000 are reserved.

      \param encoding Encoding to be used (BCD or ASCII). Defaults to RADIOLIB_PAGER_BCD.

      \returns \ref status_codes
    */
    int16_t transmit(const char* str, uint32_t addr, uint8_t encoding = RADIOLIB_PAGER_BCD);

    /*!
      \brief Binary transmit method. Will transmit arbitrary binary data.

      \param data Binary data that will be transmitted.

      \param len Length of binary data to transmit (in bytes).

      \param addr Address of the destination pager. Allowed values are 0 to 2097151 - values above 2000000 are reserved.

      \param encoding Encoding to be used (BCD or ASCII). Defaults to RADIOLIB_PAGER_BCD.

      \returns \ref status_codes
    */
    int16_t transmit(uint8_t* data, size_t len, uint32_t addr, uint8_t encoding = RADIOLIB_PAGER_BCD);

    /*!
      \brief Start reception of POCSAG packets.

      \param pin Pin to receive digital data on (e.g., DIO2 for SX127x).

      \param addr Address of this "pager". Allowed values are 0 to 2097151 - values above 2000000 are reserved.

      \param mask Address filter mask - set individual bits to enable or disable match on that bit of the address.¨Set to 0xFFFFF (all bits checked) by default.

      \returns \ref status_codes
    */
    int16_t startReceive(RADIOLIB_PIN_TYPE pin, uint32_t addr, uint32_t mask = 0xFFFFF);

    /*!
      \brief Get the number of POCSAG batches available in buffer. Limited by the size of direct mode buffer!

      \returns Number of available batches.
    */
    size_t available();

    /*!
      \brief Reads data that was received after calling startReceive method.

      \param str Address of Arduino String to save the received data.

      \param len Expected number of characters in the message. When set to 0, the message length will be retreived automatically.
      When more bytes than received are requested, only the number of bytes requested will be returned.

      \param addr Pointer to variable holding the address of the received pager message. Set to NULL to not retrieve address.

      \returns \ref status_codes
    */
    int16_t readData(String& str, size_t len = 0, uint32_t* addr = NULL);

    /*!
      \brief Reads data that was received after calling startReceive method.

      \param data Pointer to array to save the received message.

      \param len Pointer to variable holding the number of bytes that will be read. When set to 0, the packet length will be retreived automatically.
      When more bytes than received are requested, only the number of bytes requested will be returned.
      Upon completion, the number of bytes received will be written to this variable.

      \param addr Pointer to variable holding the address of the received pager message. Set to NULL to not retrieve address.

      \returns \ref status_codes
    */
    int16_t readData(uint8_t* data, size_t* len, uint32_t* addr = NULL);

#if !defined(RADIOLIB_GODMODE)
  private:
#endif
    PhysicalLayer* _phy;

    float _base;
    float _speed;
    uint32_t _baseRaw;
    uint16_t _shift;
    uint16_t _shiftHz;
    uint16_t _bitDuration;
    uint32_t _readBatchPos;
    uint32_t _filterAddr;
    uint32_t _filterMask;

    // BCH encoder
    int32_t _bchAlphaTo[RADIOLIB_PAGER_BCH_N + 1];
    int32_t _bchIndexOf[RADIOLIB_PAGER_BCH_N + 1];
    int32_t _bchG[RADIOLIB_PAGER_BCH_N - RADIOLIB_PAGER_BCH_K + 1];

    void write(uint32_t* data, size_t len);
    void write(uint32_t codeWord);
    uint32_t read();

    uint8_t encodeBCD(char c);
    char decodeBCD(uint8_t b);

    void encoderInit();
    uint32_t encodeBCH(uint32_t data);
};

#endif
