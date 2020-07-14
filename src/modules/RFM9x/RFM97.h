#if !defined(_RADIOLIB_RFM97_H)
#define _RADIOLIB_RFM97_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_RFM9X)

#include "../../Module.h"
#include "../SX127x/SX127x.h"
#include "../SX127x/SX1278.h"
#include "RFM95.h"

/*!
  \class RFM97

  \brief Derived class for %RFM97 modules. Overrides some methods from RFM95 due to different parameter ranges.
*/
class RFM97: public RFM95 {
  public:

    // constructor

    /*!
      \brief Default constructor. Called from Arduino sketch when creating new LoRa instance.

      \param mod Instance of Module that will be used to communicate with the %LoRa chip.
    */
    RFM97(Module* mod);

    // configuration methods

    /*!
      \brief Sets %LoRa link spreading factor. Allowed values range from 6 to 9. Only available in %LoRa mode.

      \param sf %LoRa link spreading factor to be set.

      \returns \ref status_codes
    */
    int16_t setSpreadingFactor(uint8_t sf);

#ifndef RADIOLIB_GODMODE
  private:
#endif

};

#endif

#endif
