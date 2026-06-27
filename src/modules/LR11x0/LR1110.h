#if !defined(_RADIOLIB_LR1110_H)
#define _RADIOLIB_LR1110_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR11X0

#include "../../Module.h"
#include "LR11x0.h"

/*!
  \class LR1110
  \brief Derived class for %LR1110 modules.
*/
class LR1110: public LR11x0 {
  public:
    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    LR1110(Module* mod); // cppcheck-suppress noExplicitConstructor

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
      \brief Initialization method for GFSK modem.
      \param config Initialization configuration.
      \details This method initializes the GFSK modem with the specified configuration.
      Supports designated initializers when using C++14 or above.
      \returns \ref status_codes
    */
    int16_t beginGFSK(const ConfigFSK_t& config);

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
      \brief Sets carrier frequency. Allowed values are in range from 150 to 960 MHz.
      Will automatically perform image calibration if the frequency changes by
      more than RADIOLIB_LR11X0_CAL_IMG_FREQ_TRIG Hz.
      \param freq Carrier frequency to be set in Hz.
      \returns \ref status_codes
    */
    int16_t setFrequency(uint32_t freq) /*override*/;

    /*!
      \brief Sets carrier frequency. Allowed values are in range from 150.0 to 960.0 MHz.
      Will automatically perform image calibration if the frequency changes by
      more than RADIOLIB_LR11X0_CAL_IMG_FREQ_TRIG Hz.
      \param freq Carrier frequency to be set in Hz.
      \param skipCalibration Skip automated image calibration.
      \param band Half bandwidth for image calibration. For example,
      if carrier is 434 MHz and band is set to 4 MHz, then the image will be calibrate
      for band 430 - 438 MHz. Unused if calibrate is set to false, defaults to 4 MHz
      \returns \ref status_codes
    */
    int16_t setFrequency(uint32_t freq, bool skipCalibration, uint32_t band = RADIOLIB_UNIT_MEGA(4));
    
    /*!
      \brief Sets output power. Allowed values are in range from -9 to 22 dBm (high-power PA) or -17 to 14 dBm (low-power PA).
      \param power Output power to be set in dBm, output PA is determined automatically preferring the low-power PA.
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power) override;

    /*!
      \brief Sets output power. Allowed values are in range from -9 to 22 dBm (high-power PA) or -17 to 14 dBm (low-power PA).
      \param power Output power to be set in dBm.
      \param forceHighPower Force using the high-power PA. If set to false, PA will be determined automatically
      based on configured output power, preferring the low-power PA. If set to true, only high-power PA will be used.
      \param rampTimeUs PA power ramping time in microseconds. Provided value is rounded up to the
      nearest discrete ramp time supported by the PA. Defaults to 48 us.
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power, bool forceHighPower, uint32_t rampTimeUs = 48);

    /*!
      \brief Check if output power is configurable.
      This method is needed for compatibility with PhysicalLayer::checkOutputPower.
      \param power Output power in dBm, PA will be determined automatically.
      \param clipped Clipped output power value to what is possible within the module's range.
      \returns \ref status_codes
    */
    int16_t checkOutputPower(int8_t power, int8_t* clipped) override;

    /*!
      \brief Check if output power is configurable.
      \param power Output power in dBm.
      \param clipped Clipped output power value to what is possible within the module's range.
      \param forceHighPower Force using the high-power PA. If set to false, PA will be determined automatically
      based on configured output power, preferring the low-power PA. If set to true, only high-power PA will be used.
      \returns \ref status_codes
    */
    int16_t checkOutputPower(int8_t power, int8_t* clipped, bool forceHighPower);

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
