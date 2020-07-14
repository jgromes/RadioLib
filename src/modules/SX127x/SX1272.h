#if !defined(_RADIOLIB_SX1272_H)
#define _RADIOLIB_SX1272_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_SX127X)

#include "../../Module.h"
#include "SX127x.h"

// SX1272 specific register map
#define SX1272_REG_AGC_REF                            0x43
#define SX1272_REG_AGC_THRESH_1                       0x44
#define SX1272_REG_AGC_THRESH_2                       0x45
#define SX1272_REG_AGC_THRESH_3                       0x46
#define SX1272_REG_PLL_HOP                            0x4B
#define SX1272_REG_TCXO                               0x58
#define SX1272_REG_PA_DAC                             0x5A
#define SX1272_REG_PLL                                0x5C
#define SX1272_REG_PLL_LOW_PN                         0x5E
#define SX1272_REG_FORMER_TEMP                        0x6C
#define SX1272_REG_BIT_RATE_FRAC                      0x70

// SX1272 LoRa modem settings
// SX1272_REG_FRF_MSB + REG_FRF_MID + REG_FRF_LSB
#define SX1272_FRF_MSB                                0xE4        //  7     0     carrier frequency setting: f_RF = (F(XOSC) * FRF)/2^19
#define SX1272_FRF_MID                                0xC0        //  7     0         where F(XOSC) = 32 MHz
#define SX1272_FRF_LSB                                0x00        //  7     0               FRF = 3 byte value of FRF registers

// SX1272_REG_MODEM_CONFIG_1
#define SX1272_BW_125_00_KHZ                          0b00000000  //  7     6     bandwidth:  125 kHz
#define SX1272_BW_250_00_KHZ                          0b01000000  //  7     6                 250 kHz
#define SX1272_BW_500_00_KHZ                          0b10000000  //  7     6                 500 kHz
#define SX1272_CR_4_5                                 0b00001000  //  5     3     error coding rate:  4/5
#define SX1272_CR_4_6                                 0b00010000  //  5     3                         4/6
#define SX1272_CR_4_7                                 0b00011000  //  5     3                         4/7
#define SX1272_CR_4_8                                 0b00100000  //  5     3                         4/8
#define SX1272_HEADER_EXPL_MODE                       0b00000000  //  2     2     explicit header mode
#define SX1272_HEADER_IMPL_MODE                       0b00000100  //  2     2     implicit header mode
#define SX1272_RX_CRC_MODE_OFF                        0b00000000  //  1     1     CRC disabled
#define SX1272_RX_CRC_MODE_ON                         0b00000010  //  1     1     CRC enabled
#define SX1272_LOW_DATA_RATE_OPT_OFF                  0b00000000  //  0     0     low data rate optimization disabled
#define SX1272_LOW_DATA_RATE_OPT_ON                   0b00000001  //  0     0     low data rate optimization enabled, mandatory for SF 11 and 12 with BW 125 kHz

// SX1272_REG_MODEM_CONFIG_2
#define SX1272_AGC_AUTO_OFF                           0b00000000  //  2     2     LNA gain set by REG_LNA
#define SX1272_AGC_AUTO_ON                            0b00000100  //  2     2     LNA gain set by internal AGC loop

// SX127X_REG_VERSION
#define SX1272_CHIP_VERSION                           0x22

// SX1272 FSK modem settings
// SX127X_REG_OP_MODE
#define SX1272_NO_SHAPING                             0b00000000  //  4     3     data shaping: no shaping (default)
#define SX1272_FSK_GAUSSIAN_1_0                       0b00001000  //  4     3                   FSK modulation Gaussian filter, BT = 1.0
#define SX1272_FSK_GAUSSIAN_0_5                       0b00010000  //  4     3                   FSK modulation Gaussian filter, BT = 0.5
#define SX1272_FSK_GAUSSIAN_0_3                       0b00011000  //  4     3                   FSK modulation Gaussian filter, BT = 0.3
#define SX1272_OOK_FILTER_BR                          0b00001000  //  4     3                   OOK modulation filter, f_cutoff = BR
#define SX1272_OOK_FILTER_2BR                         0b00010000  //  4     3                   OOK modulation filter, f_cutoff = 2*BR

// SX127X_REG_PA_RAMP
#define SX1272_LOW_PN_TX_PLL_OFF                      0b00010000  //  4     4     use standard PLL in transmit mode (default)
#define SX1272_LOW_PN_TX_PLL_ON                       0b00000000  //  4     4     use lower phase noise PLL in transmit mode

// SX127X_REG_SYNC_CONFIG
#define SX1272_FIFO_FILL_CONDITION_SYNC_ADDRESS       0b00000000  //  3     3     FIFO will be filled when sync address interrupt occurs (default)
#define SX1272_FIFO_FILL_CONDITION_ALWAYS             0b00001000  //  3     3     FIFO will be filled as long as this bit is set

// SX1272_REG_AGC_REF
#define SX1272_AGC_REFERENCE_LEVEL                    0x13        //  5     0     floor reference for AGC thresholds: AgcRef = -174 + 10*log(2*RxBw) + 8 + AGC_REFERENCE_LEVEL [dBm]

// SX1272_REG_AGC_THRESH_1
#define SX1272_AGC_STEP_1                             0x0E        //  4     0     1st AGC threshold

// SX1272_REG_AGC_THRESH_2
#define SX1272_AGC_STEP_2                             0x50        //  7     4     2nd AGC threshold
#define SX1272_AGC_STEP_3                             0x0B        //  4     0     3rd AGC threshold

// SX1272_REG_AGC_THRESH_3
#define SX1272_AGC_STEP_4                             0xD0        //  7     4     4th AGC threshold
#define SX1272_AGC_STEP_5                             0x0B        //  4     0     5th AGC threshold

// SX1272_REG_PLL_LOW_PN
#define SX1272_PLL_LOW_PN_BANDWIDTH_75_KHZ            0b00000000  //  7     6     low phase noise PLL bandwidth: 75 kHz
#define SX1272_PLL_LOW_PN_BANDWIDTH_150_KHZ           0b01000000  //  7     6                                    150 kHz
#define SX1272_PLL_LOW_PN_BANDWIDTH_225_KHZ           0b10000000  //  7     6                                    225 kHz
#define SX1272_PLL_LOW_PN_BANDWIDTH_300_KHZ           0b11000000  //  7     6                                    300 kHz (default)

/*!
  \class SX1272

  \brief Derived class for %SX1272 modules. Also used as base class for SX1273.
  Both modules use the same basic hardware and only differ in parameter ranges.
*/
class SX1272: public SX127x {
  public:

    // constructor

    /*!
      \brief Default constructor. Called from Arduino sketch when creating new LoRa instance.

      \param mod Instance of Module that will be used to communicate with the %LoRa chip.
    */
    SX1272(Module* mod);

    // basic methods

    /*!
      \brief %LoRa modem initialization method. Must be called at least once from Arduino sketch to initialize the module.

      \param freq Carrier frequency in MHz. Allowed values range from 860.0 MHz to 1020.0 MHz.

      \param bw %LoRa link bandwidth in kHz. Allowed values are 125, 250 and 500 kHz.

      \param sf %LoRa link spreading factor. Allowed values range from 6 to 12.

      \param cr %LoRa link coding rate denominator. Allowed values range from 5 to 8.

      \param syncWord %LoRa sync word. Can be used to distinguish different networks. Note that value 0x34 is reserved for LoRaWAN networks.

      \param currentLimit Trim value for OCP (over current protection) in mA. Can be set to multiplies of 5 in range 45 to 120 mA and to multiples of 10 in range 120 to 240 mA.
      Set to 0 to disable OCP (not recommended).

      \param preambleLength Length of %LoRa transmission preamble in symbols. The actual preamble length is 4.25 symbols longer than the set number.
      Allowed values range from 6 to 65535.

      \param gain Gain of receiver LNA (low-noise amplifier). Can be set to any integer in range 1 to 6 where 1 is the highest gain.
      Set to 0 to enable automatic gain control (recommended).

      \returns \ref status_codes
    */
    int16_t begin(float freq = 915.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = SX127X_SYNC_WORD, int8_t power = 10, uint16_t preambleLength = 8, uint8_t gain = 0);

    /*!
      \brief FSK modem initialization method. Must be called at least once from Arduino sketch to initialize the module.

      \param freq Carrier frequency in MHz. Allowed values range from 860.0 MHz to 1020.0 MHz.

      \param br Bit rate of the FSK transmission in kbps (kilobits per second). Allowed values range from 1.2 to 300.0 kbps.

      \param freqDev Frequency deviation of the FSK transmission in kHz. Allowed values range from 0.6 to 200.0 kHz.
      Note that the allowed range changes based on bit rate setting, so that the condition FreqDev + BitRate/2 <= 250 kHz is always met.

      \param rxBw Receiver bandwidth in kHz. Allowed values are 2.6, 3.1, 3.9, 5.2, 6.3, 7.8, 10.4, 12.5, 15.6, 20.8, 25, 31.3, 41.7, 50, 62.5, 83.3, 100, 125, 166.7, 200 and 250 kHz.

      \param power Transmission output power in dBm. Allowed values range from 2 to 17 dBm.

      \param preambleLength Length of FSK preamble in bits.

      \param enableOOK Use OOK modulation instead of FSK.

      \returns \ref status_codes
    */
    int16_t beginFSK(float freq = 915.0, float br = 48.0, float rxBw = 125.0, float freqDev = 50.0, int8_t power = 10, uint16_t preambleLength = 16, bool enableOOK = false);

    /*!
      \brief Reset method. Will reset the chip to the default state using RST pin.
    */
    void reset() override;

    // configuration methods

    /*!
      \brief Sets carrier frequency. Allowed values range from 860.0 MHz to 1020.0 MHz.

      \param freq Carrier frequency to be set in MHz.

      \returns \ref status_codes
    */
    int16_t setFrequency(float freq);

    /*!
      \brief Sets %LoRa link bandwidth. Allowed values are 125, 250 and 500 kHz. Only available in %LoRa mode.

      \param bw %LoRa link bandwidth to be set in kHz.

      \returns \ref status_codes
    */
    int16_t setBandwidth(float bw);

    /*!
      \brief Sets %LoRa link spreading factor. Allowed values range from 6 to 12. Only available in %LoRa mode.

      \param sf %LoRa link spreading factor to be set.

      \returns \ref status_codes
    */
    int16_t setSpreadingFactor(uint8_t sf);

    /*!
      \brief Sets %LoRa link coding rate denominator. Allowed values range from 5 to 8. Only available in %LoRa mode.

      \param cr %LoRa link coding rate denominator to be set.

      \returns \ref status_codes
    */
    int16_t setCodingRate(uint8_t cr);

    /*!
      \brief Sets transmission output power. Allowed values range from 2 to 17 dBm.

      \param power Transmission output power in dBm.

      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power);

    /*!
      \brief Sets gain of receiver LNA (low-noise amplifier). Can be set to any integer in range 1 to 6 where 1 is the highest gain.
      Set to 0 to enable automatic gain control (recommended). Only available in %LoRa mode.

      \param gain Gain of receiver LNA (low-noise amplifier) to be set.

      \returns \ref status_codes
    */
    int16_t setGain(uint8_t gain);

    /*!
      \brief Sets Gaussian filter bandwidth-time product that will be used for data shaping. Only available in FSK mode with FSK modulation.
      Allowed values are RADIOLIB_SHAPING_0_3, RADIOLIB_SHAPING_0_5 or RADIOLIB_SHAPING_1_0. Set to RADIOLIB_SHAPING_NONE to disable data shaping.

      \param sh Gaussian shaping bandwidth-time product that will be used for data shaping

      \returns \ref status_codes
    */
    int16_t setDataShaping(uint8_t sh) override;

    /*!
      \brief Sets filter cutoff frequency that will be used for data shaping.
      Allowed values are 1 for frequency equal to bit rate and 2 for frequency equal to 2x bit rate. Set to 0 to disable data shaping.
      Only available in FSK mode with OOK modulation.

      \param sh Cutoff frequency that will be used for data shaping

      \returns \ref status_codes
    */
    int16_t setDataShapingOOK(uint8_t sh);

    /*!
      \brief Gets recorded signal strength indicator of the latest received packet for LoRa modem, or current RSSI level for FSK modem.

      \returns Last packet RSSI for LoRa modem, or current RSSI level for FSK modem.
    */
    float getRSSI();

    /*!
      \brief Enables/disables CRC check of received packets.

      \param enableCRC Enable (true) or disable (false) CRC.

      \returns \ref status_codes
    */
    int16_t setCRC(bool enableCRC);

    /*!
      \brief Forces LoRa low data rate optimization. Only available in LoRa mode. After calling this method, LDRO will always be set to
      the provided value, regardless of symbol length. To re-enable automatic LDRO configuration, call SX1278::autoLDRO()

      \param enable Force LDRO to be always enabled (true) or disabled (false).

      \returns \ref status_codes
    */
    int16_t forceLDRO(bool enable);

    /*!
      \brief Re-enables automatic LDRO configuration. Only available in LoRa mode. After calling this method, LDRO will be enabled automatically
      when symbol length exceeds 16 ms.

      \returns \ref status_codes
    */
    int16_t autoLDRO();

#ifndef RADIOLIB_GODMODE
  protected:
#endif
    int16_t setBandwidthRaw(uint8_t newBandwidth);
    int16_t setSpreadingFactorRaw(uint8_t newSpreadingFactor);
    int16_t setCodingRateRaw(uint8_t newCodingRate);

    int16_t configFSK();

#ifndef RADIOLIB_GODMODE
  private:
#endif
    bool _ldroAuto = true;
    bool _ldroEnabled = false;

};

#endif

#endif
