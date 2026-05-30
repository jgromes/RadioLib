#if !defined(_RADIOLIB_MORSE_H) && !RADIOLIB_EXCLUDE_MORSE
#define _RADIOLIB_MORSE_H

#include "../../TypeDef.h"
#include "../PhysicalLayer/PhysicalLayer.h"
#include "../AFSK/AFSK.h"
#include "../Print/Print.h"

#define RADIOLIB_MORSE_DOT                                      0b0
#define RADIOLIB_MORSE_DASH                                     0b1
#define RADIOLIB_MORSE_GUARDBIT                                 0b1
#define RADIOLIB_MORSE_UNSUPPORTED                              0xFF
#define RADIOLIB_MORSE_ASCII_OFFSET                             32
#define RADIOLIB_MORSE_INTER_SYMBOL                             0x00
#define RADIOLIB_MORSE_CHAR_COMPLETE                            0x01
#define RADIOLIB_MORSE_WORD_COMPLETE                            0x02
#define RADIOLIB_MORSE_UNKNOWN_SYMBOL                           '*'

/*!
  \class MorseClient
  \brief Client for Morse Code communication. The public interface is the same as Arduino Serial.
*/
class MorseClient: public RadioLibPrint {
  public:
    /*!
      \brief Constructor for 2-FSK mode.
      \param phy Pointer to the wireless module providing PhysicalLayer communication.
    */
    explicit MorseClient(PhysicalLayer* phy);

    #if !RADIOLIB_EXCLUDE_AFSK
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

    /*!
      \brief Decode Morse symbol to ASCII.
      \param symbol Morse code symbol, represented as outlined in MorseTable.
      \param len Symbol length (number of dots and dashes).
      \returns ASCII character matching the symbol, or 0xFF if no match is found.
    */
    static char decode(uint8_t symbol, uint8_t len);

    /*!
      \brief Read Morse tone on input pin.
      \param symbol Pointer to the symbol buffer.
      \param len Pointer to the length counter.
      \param low Low threshold for decision limit (dot length, pause length etc.), defaults to 0.75.
      \param high High threshold for decision limit (dot length, pause length etc.), defaults to 1.25.
      \returns 0 if not enough symbols were decoded, 1 if inter-character space was detected,
      2 if inter-word space was detected.
    */
    #if !RADIOLIB_EXCLUDE_AFSK
    int read(uint8_t* symbol, uint8_t* len, float low = 0.75f, float high = 1.25f);
    #endif

    /*!
      \brief Write one byte. Implementation of interface of the RadioLibPrint/Print class.
      \param b Byte to write.
      \returns 1 if the byte was written, 0 otherwise.
    */
    size_t write(uint8_t b) override;

#if !RADIOLIB_GODMODE
  private:
#endif
    PhysicalLayer* phyLayer;
    #if !RADIOLIB_EXCLUDE_AFSK
    AFSKClient* audioClient;
    #endif

    uint32_t baseFreq = 0, baseFreqHz = 0;
    float basePeriod = 0.0f;
    uint32_t dotLength = 0;
    uint32_t dashLength = 0;
    uint32_t letterSpace = 0;
    uint16_t wordSpace = 0;

    // variables to keep decoding state
    uint32_t signalCounter = 0;
    RadioLibTime_t signalStart = 0;
    uint32_t pauseCounter = 0;
    RadioLibTime_t pauseStart = 0;

    int16_t transmitDirect(uint32_t freq = 0, uint32_t freqHz = 0);
    int16_t standby();
};

#endif
