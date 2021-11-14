#if !defined(_RADIOLIB_RADIOLIB_MORSE_H) && !defined(RADIOLIB_EXCLUDE_MORSE)
#define _RADIOLIB_RADIOLIB_MORSE_H

#include "../../TypeDef.h"
#include "../PhysicalLayer/PhysicalLayer.h"
#include "../AFSK/AFSK.h"

#define RADIOLIB_MORSE_DOT                                      0b0
#define RADIOLIB_MORSE_DASH                                     0b1
#define RADIOLIB_MORSE_GUARDBIT                                 0b1
#define RADIOLIB_MORSE_UNSUPORTED                               0xFF

// Morse character table: - using codes defined in ITU-R M.1677-1
//                        - Morse code representation is saved LSb first, using additional bit as guard
//                        - position in array corresponds ASCII code minus RADIOLIB_MORSE_ASCII_OFFSET
//                        - ASCII characters marked RADIOLIB_MORSE_UNSUPORTED do not have ITU-R M.1677-1 equivalent
static const uint8_t MorseTable[] RADIOLIB_NONVOLATILE = {
    0b00,                         // space
    0b110101,                     // ! (unsupported)
    0b1010010,                    // "
    RADIOLIB_MORSE_UNSUPORTED,    // # (unsupported)
    RADIOLIB_MORSE_UNSUPORTED,    // $ (unsupported)
    RADIOLIB_MORSE_UNSUPORTED,    // % (unsupported)
    RADIOLIB_MORSE_UNSUPORTED,    // & (unsupported)
    0b1011110,                    // '
    0b101101,                     // (
    0b1101101,                    // )
    RADIOLIB_MORSE_UNSUPORTED,    // * (unsupported)
    0b101010,                     // +
    0b1110011,                    // ,
    0b1100001,                    // -
    0b1101010,                    // .
    0b101001,                     // /
    0b111111,                     // 0
    0b111110,                     // 1
    0b111100,                     // 2
    0b111000,                     // 3
    0b110000,                     // 4
    0b100000,                     // 5
    0b100001,                     // 6
    0b100011,                     // 7
    0b100111,                     // 8
    0b101111,                     // 9
    0b1000111,                    // :
    RADIOLIB_MORSE_UNSUPORTED,    // ; (unsupported)
    RADIOLIB_MORSE_UNSUPORTED,    // < (unsupported)
    0b110001,                     // =
    RADIOLIB_MORSE_UNSUPORTED,    // > (unsupported)
    0b1001100,                    // ?
    0b1010110,                    // @
    0b110,                        // A
    0b10001,                      // B
    0b10101,                      // C
    0b1001,                       // D
    0b10,                         // E
    0b10100,                      // F
    0b1011,                       // G
    0b10000,                      // H
    0b100,                        // I
    0b11110,                      // J
    0b1101,                       // K
    0b10010,                      // L
    0b111,                        // M
    0b101,                        // N
    0b1111,                       // O
    0b10110,                      // P
    0b11011,                      // Q
    0b1010,                       // R
    0b1000,                       // S
    0b11,                         // T
    0b1100,                       // U
    0b11000,                      // V
    0b1110,                       // W
    0b11001,                      // X
    0b11101,                      // Y
    0b10011,                      // Z
    RADIOLIB_MORSE_UNSUPORTED,    // [ (unsupported)
    RADIOLIB_MORSE_UNSUPORTED,    // \ (unsupported)
    RADIOLIB_MORSE_UNSUPORTED,    // ] (unsupported)
    0b1101000,                    // ^ (unsupported, used as alias for end of work)
    0b110101                      // _ (unsupported, used as alias for starting signal)
};

/*!
  \class MorseClient

  \brief Client for Morse Code communication. The public interface is the same as Arduino Serial.
*/
class MorseClient {
  public:
    /*!
      \brief Constructor for 2-FSK mode.

      \param phy Pointer to the wireless module providing PhysicalLayer communication.
    */
    explicit MorseClient(PhysicalLayer* phy);

    #if !defined(RADIOLIB_EXCLUDE_AFSK)
    /*!
      \brief Constructor for AFSK mode.

      \param audio Pointer to the AFSK instance providing audio.
    */
    explicit MorseClient(AFSKClient* audio);
    #endif

    // basic methods

    /*!
      \brief Initialization method.

      \param base Base RF frequency to be used in MHz (in 2-FSK mode), or the tone frequency in Hz (in AFSK mode)

      \param speed Coding speed in words per minute.

      \returns \ref status_codes
    */
    int16_t begin(float base, uint8_t speed = 20);

    /*!
      \brief Send start signal.

      \returns Number of bytes sent (always 0).
    */
    size_t startSignal();

    size_t write(const char* str);
    size_t write(uint8_t* buff, size_t len);
    size_t write(uint8_t b);

    size_t print(__FlashStringHelper*);
    size_t print(const String &);
    size_t print(const char[]);
    size_t print(char);
    size_t print(unsigned char, int = DEC);
    size_t print(int, int = DEC);
    size_t print(unsigned int, int = DEC);
    size_t print(long, int = DEC);
    size_t print(unsigned long, int = DEC);
    size_t print(double, int = 2);

    size_t println(void);
    size_t println(__FlashStringHelper*);
    size_t println(const String &);
    size_t println(const char[]);
    size_t println(char);
    size_t println(unsigned char, int = DEC);
    size_t println(int, int = DEC);
    size_t println(unsigned int, int = DEC);
    size_t println(long, int = DEC);
    size_t println(unsigned long, int = DEC);
    size_t println(double, int = 2);

#if !defined(RADIOLIB_GODMODE)
  private:
#endif
    PhysicalLayer* _phy;
    #if !defined(RADIOLIB_EXCLUDE_AFSK)
    AFSKClient* _audio;
    #endif

    uint32_t _base = 0, _baseHz = 0;
    uint16_t _dotLength = 0;

    size_t printNumber(unsigned long, uint8_t);
    size_t printFloat(double, uint8_t);

    int16_t transmitDirect(uint32_t freq = 0, uint32_t freqHz = 0);
    int16_t standby();
};

#endif
