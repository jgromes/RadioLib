#if !defined(_RADIOLIB_LLCC68_H)
#define _RADIOLIB_LLCC68_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_SX126X)

#include "../../Module.h"
#include "../SX126x/SX1262.h"

/*!
  \class LLCC68

  \brief Derived class for %LLCC68 modules.
*/
class LLCC68: public SX1262 {
  public:
    /*!
      \brief Default constructor.

      \param mod Instance of Module that will be used to communicate with the radio.
    */
    LLCC68(Module* mod);

    // configuration methods

    /*!
      \brief Sets LoRa bandwidth. Allowed values are 125.0, 250.0 and 500.0 kHz.

      \param bw LoRa bandwidth to be set in kHz.

      \returns \ref status_codes
    */
    int16_t setBandwidth(float bw);

    /*!
      \brief Sets LoRa spreading factor. Allowed values range from 5 to 11, depending on currently set spreading factor.

      \param sf LoRa spreading factor to be set.

      \returns \ref status_codes
    */
    int16_t setSpreadingFactor(uint8_t sf);

#if !defined(RADIOLIB_GODMODE)
  private:
#endif

};

#endif

#endif
