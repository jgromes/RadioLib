#if !defined(_RADIOLIB_HELLSCHREIBER_H)
#define _RADIOLIB_HELLSCHREIBER_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_HELLSCHREIBER

#include "../PhysicalLayer/PhysicalLayer.h"
#include "../AFSK/AFSK.h"
#include "../Print/Print.h"

#define RADIOLIB_HELL_FONT_WIDTH                                7
#define RADIOLIB_HELL_FONT_HEIGHT                               7

/*!
  \class HellClient
  \brief Client for Hellschreiber transmissions.
*/
class HellClient: public RadioLibPrint {
  public:
    /*!
      \brief Constructor for 2-FSK mode.
      \param phy Pointer to the wireless module providing PhysicalLayer communication.
    */
    explicit HellClient(PhysicalLayer* phy);

    #if !RADIOLIB_EXCLUDE_AFSK
    /*!
      \brief Constructor for AFSK mode.
      \param audio Pointer to the AFSK instance providing audio.
    */
    explicit HellClient(AFSKClient* audio);
    #endif

    // basic methods

    /*!
      \brief Initialization method.
      \param base Base RF frequency to be used in MHz (in 2-FSK mode), or the tone frequency in Hz (in AFSK mode).
      \param rate Baud rate to be used during transmission. Defaults to 122.5 ("Feld Hell")
    */
    int16_t begin(float base, float rate = 122.5);

    /*!
      \brief Method to "print" a buffer of pixels, this is exposed to allow users to send custom characters.
      \param buff Buffer of pixels to send, in a 7x7 pixel array.
      \returns Always returns the number of printed glyphs (1).
    */
    size_t printGlyph(const uint8_t* buff);

    /*!
      \brief Invert text color.
      \param inv Whether to enable color inversion (white text on black background), or not (black text on white background)
    */
    void setInversion(bool inv);

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
    uint32_t pixelDuration = 0;
    bool invert = false;

    int16_t transmitDirect(uint32_t freq = 0, uint32_t freqHz = 0);
    int16_t standby();
};

#endif

#endif
