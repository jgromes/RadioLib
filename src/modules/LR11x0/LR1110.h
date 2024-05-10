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
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param bw LoRa bandwidth in kHz. Defaults to 125.0 kHz.
      \param sf LoRa spreading factor. Defaults to 9.
      \param cr LoRa coding rate denominator. Defaults to 7 (coding rate 4/7).
      \param syncWord 1-byte LoRa sync word. Defaults to RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE (0x12).
      \param power Output power in dBm. Defaults to 10 dBm.
      \param preambleLength LoRa preamble length in symbols. Defaults to 8 symbols.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set LR11x0::XTAL to true.
      \returns \ref status_codes
    */
    int16_t begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE, int8_t power = 10, uint16_t preambleLength = 8, float tcxoVoltage = 1.6);

    /*!
      \brief Initialization method for FSK modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param br FSK bit rate in kbps. Defaults to 4.8 kbps.
      \param freqDev Frequency deviation from carrier frequency in kHz. Defaults to 5.0 kHz.
      \param rxBw Receiver bandwidth in kHz. Defaults to 156.2 kHz.
      \param power Output power in dBm. Defaults to 10 dBm.
      \param preambleLength FSK preamble length in bits. Defaults to 16 bits.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set LR11x0::XTAL to true.
      \returns \ref status_codes
    */
    int16_t beginGFSK(float freq = 434.0, float br = 4.8, float freqDev = 5.0, float rxBw = 156.2, int8_t power = 10, uint16_t preambleLength = 16, float tcxoVoltage = 1.6);
    
    /*!
      \brief Initialization method for LR-FHSS modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param bw LR-FHSS bandwidth, one of RADIOLIB_LR11X0_LR_FHSS_BW_* values. Defaults to 722.66 kHz.
      \param cr LR-FHSS coding rate, one of RADIOLIB_LR11X0_LR_FHSS_CR_* values. Defaults to 2/3 coding rate.
      \param power Output power in dBm. Defaults to 10 dBm.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set LR11x0::XTAL to true.
      \returns \ref status_codes
    */
    int16_t beginLRFHSS(float freq = 434.0, uint8_t bw = RADIOLIB_LR11X0_LR_FHSS_BW_722_66, uint8_t cr = RADIOLIB_LR11X0_LR_FHSS_CR_2_3, int8_t power = 10, float tcxoVoltage = 1.6);

    // configuration methods

    /*!
      \brief Sets carrier frequency. Allowed values are in range from 150.0 to 960.0 MHz.
      Will also perform calibrations.
      \param freq Carrier frequency to be set in MHz.
      \returns \ref status_codes
    */
    int16_t setFrequency(float freq) override;

    /*!
      \brief Sets carrier frequency. Allowed values are in range from 150.0 to 960.0 MHz.
      \param freq Carrier frequency to be set in MHz.
      \param calibrate Run image calibration.
      \param band Half bandwidth for image calibration. For example,
      if carrier is 434 MHz and band is set to 4 MHz, then the image will be calibrate
      for band 430 - 438 MHz. Unused if calibrate is set to false, defaults to 4 MHz
      \returns \ref status_codes
    */
    int16_t setFrequency(float freq, bool calibrate, float band = 4);

#if !RADIOLIB_GODMODE
  private:
#endif

};

#endif

#endif
