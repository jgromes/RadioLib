#if !defined(_RADIOLIB_SX1231_H)
#define _RADIOLIB_SX1231_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_SX1231)

#include "../../Module.h"
#include "../RF69/RF69.h"

#define SX1231_CHIP_REVISION_2_A                      0x21
#define SX1231_CHIP_REVISION_2_B                      0x22
#define SX1231_CHIP_REVISION_2_C                      0x23

//SX1231 specific register map
#define SX1231_REG_TEST_OOK                           0x6E

//SX1231_REG_TEST_OOK
#define SX1231_OOK_DELTA_THRESHOLD                    0x0C

/*!
  \class SX1231

  \brief Control class for %SX1231 module. Overrides some methods from RF69 due to different register values.
*/
class SX1231: public RF69  {
  public:
    /*!
      \brief Default constructor.

      \param mod Instance of Module that will be used to communicate with the radio.
    */
    SX1231(Module* mod);

    /*!
      \brief Initialization method.

      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.

      \param br Bit rate to be used in kbps. Defaults to 48.0 kbps.

      \param rxBw Receiver bandwidth in kHz. Defaults to 125.0 kHz.

      \param freqDev Frequency deviation from carrier frequency in kHz Defaults to 50.0 kHz.

      \param power Output power in dBm. Defaults to 10 dBm.

      \param preambleLen Preamble Length in bits. Defaults to 16 bits.

      \returns \ref status_codes
    */
    int16_t begin(float freq = 434.0, float br = 48.0, float rxBw = 125.0, float freqDev = 50.0, int8_t power = 10, uint8_t preambleLen = 16);

#ifndef RADIOLIB_GODMODE
  private:
#endif
    uint8_t _chipRevision = 0;
};

#endif

#endif
