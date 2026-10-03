#if !defined(_RADIOLIB_LLCC68_H)
#define _RADIOLIB_LLCC68_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SX126X

#include "../../Module.h"
#include "../SX126x/SX1262.h"
#include "../SX126x/SX1261.h"

//RADIOLIB_SX126X_REG_VERSION_STRING
#define RADIOLIB_LLCC68_CHIP_TYPE                               "LLCC68"

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
    LLCC68(Module* mod); // cppcheck-suppress noExplicitConstructor

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
    
    /*!
      \brief Initialization method for BPSK modem.
      \param config Initialization configuration.
      \details This method initializes the BPSK modem with the specified configuration.
      Supports designated initializers when using C++14 or above.
      \returns \ref status_codes
    */
    int16_t beginBPSK(const ConfigBPSK_t& config) override;

    /*!
      \brief Initialization method for LR-FHSS modem.
      \param config Initialization configuration.
      \details This method initializes the LR-FHSS modem with the specified configuration.
      Supports designated initializers when using C++14 or above.
      \returns \ref status_codes
    */
    int16_t beginLRFHSS(const ConfigLRFHSS_t& config) override;

    // configuration methods

    /*!
      \brief Sets LoRa bandwidth. Allowed values are 125, 250 and 500 kHz.
      \param bw LoRa bandwidth to be set in Hz.
      \returns \ref status_codes
    */
    int16_t setBandwidth(uint32_t bw) override;

    /*!
      \brief Sets LoRa spreading factor. Allowed values range from 5 to 11, depending on currently set spreading factor.
      \param sf LoRa spreading factor to be set.
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
      \param modem Modem type to set - FSK, LoRa or LR-FHSS.
      \returns \ref status_codes
    */
    int16_t setModem(ModemType_t modem) override;

#if !RADIOLIB_GODMODE
  private:
#endif

};

#endif

#endif
