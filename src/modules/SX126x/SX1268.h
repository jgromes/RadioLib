#if !defined(_RADIOLIB_SX1268_H)
#define _RADIOLIB_SX1268_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SX126X

#include "../../Module.h"
#include "SX126x.h"

//RADIOLIB_SX126X_CMD_SET_PA_CONFIG
#define RADIOLIB_SX126X_PA_CONFIG_SX1268                        0x00

//RADIOLIB_SX126X_REG_VERSION_STRING
#define RADIOLIB_SX1268_CHIP_TYPE                               "SX1268"

/*!
  \class SX1268
  \brief Derived class for %SX1268 modules.
*/
class SX1268: public SX126x {
  public:
    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    SX1268(Module* mod); // cppcheck-suppress noExplicitConstructor

    // basic methods

    /*!
      \brief Initialization method for LoRa modem.
      \details This method initializes the LoRa modem with the specified configuration.
      Supports designated initializers when using C++14 or above.
      \param config Initialization configuration.
      \returns \ref status_codes
    */
    int16_t begin(const ConfigLoRa_t& config);

    /*!
      \brief Initialization method for FSK modem.
      \param config Initialization configuration.
      \details This method initializes the FSK modem with the specified configuration.
      Supports designated initializers when using C++14 or above.
      \returns \ref status_codes
    */
    int16_t beginFSK(const ConfigFSK_t& config);

    /*!
      \brief Initialization method for BPSK modem.
      \param config Initialization configuration.
      \details This method initializes the BPSK modem with the specified configuration.
      Supports designated initializers when using C++14 or above.
      \returns \ref status_codes
    */
    int16_t beginBPSK(const ConfigBPSK_t& config);

    /*!
      \brief Initialization method for LR-FHSS modem.
      \param config Initialization configuration.
      \details This method initializes the LR-FHSS modem with the specified configuration.
      Supports designated initializers when using C++14 or above.
      \returns \ref status_codes
    */
    int16_t beginLRFHSS(const ConfigLRFHSS_t& config);

    // configuration methods

    /*!
      \brief Sets carrier frequency. Allowed values are in range from 410 to 810 MHz.
      Will automatically perform image calibration if the frequency changes by
      more than RADIOLIB_SX126X_CAL_IMG_FREQ_TRIG MHz.
      \param freq Carrier frequency to be set in Hz.
      \returns \ref status_codes
    */
    int16_t setFrequency(uint32_t freq) /*override*/;

    /*!
      \brief Sets carrier frequency. Allowed values are in range from 150.0 to 960.0 MHz.
      Will automatically perform image calibration if the frequency changes by
      more than RADIOLIB_SX126X_CAL_IMG_FREQ_TRIG_MHZ.
      \param freq Carrier frequency to be set in MHz.
      \param skipCalibration Skip automated image calibration.
      \returns \ref status_codes
    */
    int16_t setFrequency(uint32_t freq, bool skipCalibration);

    /*!
      \brief Sets output power. Allowed values are in range from -9 to 22 dBm.
      \param power Output power to be set in dBm.
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power) override;

    /*!
      \brief Sets output power. Allowed values are in range from -9 to 22 dBm.
      \param power Output power to be set in dBm.
      \param optimize Whether to use power-optimized PA configuration (true) or datasheet default (false).
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power, bool optimize);

    /*!
      \brief Check if output power is configurable.
      \param power Output power in dBm.
      \param clipped Clipped output power value to what is possible within the module's range.
      \returns \ref status_codes
    */
    int16_t checkOutputPower(int8_t power, int8_t* clipped) override;
    
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
