#if !defined(_RADIOLIB_SX1262_H)
#define _RADIOLIB_SX1262_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_SX126X)

#include "../../Module.h"
#include "SX126x.h"

//RADIOLIB_SX126X_CMD_SET_PA_CONFIG
#define RADIOLIB_SX126X_PA_CONFIG_SX1262                       0x00

/*!
  \class SX1262

  \brief Derived class for %SX1262 modules.
*/
class SX1262: public SX126x {
  public:
    /*!
      \brief Default constructor.

      \param mod Instance of Module that will be used to communicate with the radio.
    */
    SX1262(Module* mod);

    // basic methods

    /*!
      \brief Initialization method for LoRa modem.

      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.

      \param bw LoRa bandwidth in kHz. Defaults to 125.0 kHz.

      \param sf LoRa spreading factor. Defaults to 9.

      \param cr LoRa coding rate denominator. Defaults to 7 (coding rate 4/7).

      \param syncWord 2-byte LoRa sync word. Defaults to RADIOLIB_SX126X_SYNC_WORD_PRIVATE (0x12).

      \param power Output power in dBm. Defaults to 10 dBm.

      \param preambleLength LoRa preamble length in symbols. Defaults to 8 symbols.

      \param tcxoVoltage TCXO reference voltage to be set on DIO3. Defaults to 1.6 V, set to 0 to skip.

      \returns \ref status_codes
    */
    int16_t begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = RADIOLIB_SX126X_SYNC_WORD_PRIVATE, int8_t power = 10, uint16_t preambleLength = 8, float tcxoVoltage = 1.6, bool useRegulatorLDO = false);

    /*!
      \brief Initialization method for FSK modem.

      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.

      \param br FSK bit rate in kbps. Defaults to 4.8 kbps.

      \param freqDev Frequency deviation from carrier frequency in kHz. Defaults to 5.0 kHz.

      \param rxBw Receiver bandwidth in kHz. Defaults to 156.2 kHz.

      \param power Output power in dBm. Defaults to 10 dBm.

      \parma preambleLength FSK preamble length in bits. Defaults to 16 bits.

      \param tcxoVoltage TCXO reference voltage to be set on DIO3. Defaults to 1.6 V, set to 0 to skip.

      \param useRegulatorLDO Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.

      \returns \ref status_codes
    */
    int16_t beginFSK(float freq = 434.0, float br = 4.8, float freqDev = 0.0, float rxBw = 156.2, int8_t power = 10, uint16_t preambleLength = 16, float tcxoVoltage = 1.6, bool useRegulatorLDO = false);

    // configuration methods

    /*!
      \brief Sets carrier frequency. Allowed values are in range from 150.0 to 960.0 MHz.

      \param freq Carrier frequency to be set in MHz.

      \param calibrate Run image calibration.

      \returns \ref status_codes
    */
    int16_t setFrequency(float freq, bool calibrate = true);

    /*!
      \brief Sets output power. Allowed values are in range from -17 to 22 dBm.

      \param power Output power to be set in dBm.

      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power);

#if !defined(RADIOLIB_GODMODE)
  private:
#endif

};

#endif

#endif
