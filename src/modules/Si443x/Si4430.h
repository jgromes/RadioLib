#if !defined(_RADIOLIB_SI4430_H)
#define _RADIOLIB_SI4430_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SI443X

#include "../../Module.h"
#include "Si4432.h"

/*!
  \class Si4430
  \brief Derived class for %Si4430 modules.
*/
class Si4430: public Si4432 {
  public:

    // constructor

    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio chip.
    */
    Si4430(Module* mod); // cppcheck-suppress noExplicitConstructor

    // basic methods

    /*!
      \brief Initialization method for FSK modem.
      \param config Initialization configuration.
      \details This method initializes the FSK modem with the specified configuration.
      Supports designated initializers when using C++14 or above.
      \returns \ref status_codes
    */
    int16_t begin(const ConfigFSK_t& config) override;

    // configuration methods

    /*!
      \brief Sets carrier frequency. Allowed values range from 900 MHz to 960 MHz.
      \param freq Carrier frequency to be set in Hz.
      \returns \ref status_codes
    */
    int16_t setFrequency(uint32_t freq) /*override*/;

    /*!
      \brief Sets output power. Allowed values range from -8 to 13 dBm in 3 dBm steps.
      \param power Output power to be set in dBm.
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power) override;

#if !RADIOLIB_GODMODE
  protected:
#endif

#if !RADIOLIB_GODMODE
  private:
#endif
};

#endif

#endif
