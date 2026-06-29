#if !defined(_RADIOLIB_SX1233_H)
#define _RADIOLIB_SX1233_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SX1231

#include "../../Module.h"
#include "../RF69/RF69.h"
#include "SX1231.h"

// RADIOLIB_SX1233 specific register map
#define RADIOLIB_SX1233_REG_TEST_PLL                            0x5F

// RADIOLIB_SX1233_REG_TEST_PLL
#define RADIOLIB_SX1233_PLL_BW_HIGH_BIT_RATE                    0x0C
#define RADIOLIB_SX1233_PLL_BW_LOW_BIT_RATE                     0x08

/*!
  \class SX1233
  \brief Control class for %SX1233 module. Overrides some methods from SX1231/RF69 due to different register values.
*/
class SX1233: public SX1231  {
  public:
    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    SX1233(Module* mod); // cppcheck-suppress noExplicitConstructor

    /*!
      \brief Initialization method for FSK modem.
      \param config Initialization configuration.
      \details This method initializes the FSK modem with the specified configuration.
      Supports designated initializers when using C++14 or above.
      \returns \ref status_codes
    */
    int16_t begin(const ConfigFSK_t& config) override;

    /*!
      \brief Sets bit rate. Allowed values range from 500 bps to 300 kbps.
      SX1233 also allows 500 kbps and 600 kbps operation.
      NOTE: For 500 kbps rate, the receiver frequency should be offset by 50 kHz from the transmitter.
            For 600 kbps rate, the receiver frequency should be offset by 40 kHz from the transmitter.
      \param br Bit rate to be set in bps.
      \returns \ref status_codes
    */
    int16_t setBitRate(uint32_t br) /*override*/;

#if !RADIOLIB_GODMODE
  private:
#endif

};

#endif

#endif
