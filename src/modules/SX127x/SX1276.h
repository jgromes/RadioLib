#if !defined(_RADIOLIB_SX1276_H)
#define _RADIOLIB_SX1276_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SX127X

#include "SX1278.h"

/*!
  \class SX1276
  \brief Derived class for %SX1276 modules. Overrides some methods from SX1278 due to different parameter ranges.
*/
class SX1276: public SX1278 {
  public:

    // constructor

    /*!
      \brief Default constructor. Called from Arduino sketch when creating new LoRa instance.
      \param mod Instance of Module that will be used to communicate with the LoRa chip.
    */
    SX1276(Module* mod); // cppcheck-suppress noExplicitConstructor

    // basic methods

    /*!
      \brief Initialization method for LoRa modem.
      \details This method initializes the LoRa modem with the specified configuration.
      Supports designated initializers when using C++14 or above.
      \param config Initialization configuration.
      \returns \ref status_codes
    */
    int16_t begin(const ConfigLoRa_t& config) override;

    /*!
      \brief Initialization method for FSK modem.
      \param config Initialization configuration.
      \details This method initializes the FSK modem with the specified configuration.
      Supports designated initializers when using C++14 or above.
      \returns \ref status_codes
    */
    int16_t beginFSK(const ConfigFSK_t& config) override;

    // configuration methods

    /*!
      \brief Sets carrier frequency. Allowed values range from 137 MHz to 175 MHz, 395 to 525 MHz 
      (datasheet minimum is 410 MHz, hardware works lower) and 862 to 1020 MHz.
      \param freq Carrier frequency to be set in Hz.
      \returns \ref status_codes
    */
    int16_t setFrequency(uint32_t freq) /*override*/;
    
    /*!
      \brief Set modem for the radio to use. Will perform full reset and reconfigure the radio
      using its default parameters.
      \param modem Modem type to set - FSK or LoRa.
      \returns \ref status_codes
    */
    int16_t setModem(ModemType_t modem) override;

#if !RADIOLIB_GODMODE
  private:
#endif

};

/*!
  \class RFM95
  \brief Only exists as alias for SX1276, since there seems to be no difference between %RFM95 and %SX1276 modules.
*/
RADIOLIB_TYPE_ALIAS(SX1276, RFM95)

/*!
  \class RFM96
  \brief Only exists as alias for SX1276, since there seems to be no difference between %RFM96 and %SX1276 modules.
*/
RADIOLIB_TYPE_ALIAS(SX1276, RFM96)

#endif

#endif
