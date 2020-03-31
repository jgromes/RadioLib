#ifndef _RADIOLIB_SSTV_H
#define _RADIOLIB_SSTV_H

#include "../../TypeDef.h"
#include "../PhysicalLayer/PhysicalLayer.h"

// the following implementation is based on information from
// http://www.barberdsp.com/downloads/Dayton%20Paper.pdf

// VIS codes
#define SSTV_SCOTTIE_1                                60
#define SSTV_SCOTTIE_2                                56
#define SSTV_SCOTTIE_DX                               76
#define SSTV_MARTIN_1                                 44
#define SSTV_MARTIN_2                                 40
#define SSTV_WRASSE_SC2_180                           55
#define SSTV_PASOKON_P3                               113
#define SSTV_PASOKON_P5                               114
#define SSTV_PASOKON_P7                               115

// SSTV tones in Hz
#define SSTV_TONE_LEADER                              1900
#define SSTV_TONE_BREAK                               1200
#define SSTV_TONE_VIS_1                               1100
#define SSTV_TONE_VIS_0                               1300
#define SSTV_TONE_BRIGHTNESS_MIN                      1500
#define SSTV_TONE_BRIGHTNESS_MAX                      2300

// calibration header timing in us
#define SSTV_HEADER_LEADER_LENGTH                     300000
#define SSTV_HEADER_BREAK_LENGTH                      10000
#define SSTV_HEADER_BIT_LENGTH                        30000

// structure to save data about tone
struct tone_t {
  enum {
    GENERIC = 0,
    SCAN_GREEN,
    SCAN_BLUE,
    SCAN_RED
  } type;
  uint32_t len;
  uint16_t freq;
};

// structure to save data about SSTV mode
struct SSTVMode_t {
  uint8_t visCode;
  uint16_t width;
  uint16_t height;
  uint16_t scanPixelLen;
  uint8_t numTones;
  tone_t tones[8];
};

// all currently supported SSTV modes
extern const SSTVMode_t Scottie1;
extern const SSTVMode_t Scottie2;
extern const SSTVMode_t ScottieDX;
extern const SSTVMode_t Martin1;
extern const SSTVMode_t Martin2;
extern const SSTVMode_t Wrasse;
extern const SSTVMode_t PasokonP3;
extern const SSTVMode_t PasokonP5;
extern const SSTVMode_t PasokonP7;

class SSTVClient {
  public:
    /*!
      \brief Default constructor.

      \param phy Pointer to the wireless module providing PhysicalLayer communication.
    */
    SSTVClient(PhysicalLayer* phy);

    // basic methods

    /*!
      \brief Initialization method.

      \param base Base RF frequency to be used in MHz. In USB modulation, this corresponds to "0 Hz tone".

      \param mode SSTV mode to be used. Currently supported modes are Scottie1, Scottie2, ScottieDX, Martin1, Martin2, Wrasse, PasokonP3, PasokonP5 and PasokonP7.
    */
    int16_t begin(float base, SSTVMode_t mode, float correction = 1.0);

    /*!
      \brief Sends out tone at 1900 Hz.
    */
    void idle();

    /*!
      \brief Sends synchronization header for the SSTV mode set in begin method.
    */
    void sendHeader();

    /*!
      \brief Sends single picture line in the currently configured SSTV mode.

      \param imgLine Image line to send, in 24-bit RGB. It is up to the user to ensure that imgLine has enough pixels to send it in the current SSTV mode.
    */
    void sendLine(uint32_t* imgLine);

    /*!
      \brief Get picture height of the currently configured SSTV mode.

      \returns Picture height of the currently configured SSTV mode in pixels.
    */
    uint16_t getPictureHeight();

#ifndef RADIOLIB_GODMODE
  private:
#endif
    PhysicalLayer* _phy;

    uint32_t _base;
    SSTVMode_t _mode;
    bool _firstLine;

    void tone(float freq, uint32_t len = 0);
};

#endif
