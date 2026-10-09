#if !defined(_RADIOLIB_SX1277_H)
#define _RADIOLIB_SX1277_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SX127X

#include "SX1278.h"

/*!
  \class SX1277
  \brief Derived class for %SX1277 modules. Overrides some methods from SX1278 due to different parameter ranges.
*/
class SX1277: public SX1278 {
  public:

    // constructor

    /*!
      \brief Default constructor. Called from Arduino sketch when creating new LoRa instance.
      \param mod Instance of Module that will be used to communicate with the LoRa chip.
    */
    SX1277(Module* mod); // cppcheck-suppress noExplicitConstructor

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
      \param freq Carrier frequency to be set in MHz.
      \returns \ref status_codes
    */
    int16_t setFrequency(uint32_t freq) override;

    /*!
      \brief Sets LoRa link spreading factor. Allowed values range from 6 to 9. Only available in LoRa mode.
      \param sf LoRa link spreading factor to be set.
      \returns \ref status_codes
    */
    int16_t setSpreadingFactor(uint8_t sf) override;
    
    /*!
      \brief Set data rate.
      \param dr Data rate struct.
      \param modem The modem corresponding to the requested datarate (FSK or LoRa). 
      Defaults to currently active modem if not supplied.
      \returns \ref status_codes
    */
    int16_t setDataRate(DataRate_t dr, ModemType_t modem = RADIOLIB_MODEM_NONE) override;
    
    /*!
      \brief Check the data rate can be configured by this module.
      \param dr Data rate struct.
      \param modem The modem corresponding to the requested datarate (FSK or LoRa). 
      Defaults to currently active modem if not supplied.
      \returns \ref status_codes
    */
    int16_t checkDataRate(DataRate_t dr, ModemType_t modem = RADIOLIB_MODEM_NONE) override;
    
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
  \class RFM97
  \brief Only exists as alias for SX1277, since there seems to be no difference between %RFM97 and %SX1277 modules.
*/
RADIOLIB_TYPE_ALIAS(SX1277, RFM97)

#endif

#endif
