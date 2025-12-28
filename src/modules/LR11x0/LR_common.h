#if !defined(RADIOLIB_LR_COMMON_H)
#define RADIOLIB_LR_COMMON_H

#include "../../Module.h"

#define RADIOLIB_LRXXXX_CMD_NOP                                 (0x0000)
#define RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN                  (256)

// RADIOLIB_LR11X0_CMD_GET_STATUS
// RADIOLIB_LR2021_CMD_GET_STATUS                                                   MSB   LSB   DESCRIPTION
#define RADIOLIB_LRXXXX_STAT_1_CMD_FAIL                         (0x00UL << 1)   //  3     1     command status: last command could not be executed
#define RADIOLIB_LRXXXX_STAT_1_CMD_PERR                         (0x01UL << 1)   //  3     1                     processing error
#define RADIOLIB_LRXXXX_STAT_1_CMD_OK                           (0x02UL << 1)   //  3     1                     successfully processed
#define RADIOLIB_LRXXXX_STAT_1_CMD_DAT                          (0x03UL << 1)   //  3     1                     successfully processed, data is being transmitted

// RADIOLIB_LR11X0_CMD_LR_FHSS_BUILD_FRAME
// RADIOLIB_LR2021_CMD_LR_FHSS_BUILD_FRAME
#define RADIOLIB_LRXXXX_LR_FHSS_CR_5_6                          (0x00UL << 0)   //  7     0     LR FHSS coding rate: 5/6
#define RADIOLIB_LRXXXX_LR_FHSS_CR_2_3                          (0x01UL << 0)   //  7     0                          2/3
#define RADIOLIB_LRXXXX_LR_FHSS_CR_1_2                          (0x02UL << 0)   //  7     0                          1/2
#define RADIOLIB_LRXXXX_LR_FHSS_CR_1_3                          (0x03UL << 0)   //  7     0                          1/3
#define RADIOLIB_LRXXXX_LR_FHSS_MOD_TYPE_GMSK                   (0x00UL << 0)   //  7     0     LR FHSS modulation: GMSK
#define RADIOLIB_LRXXXX_LR_FHSS_GRID_STEP_FCC                   (0x00UL << 0)   //  7     0     LR FHSS step size: 25.390625 kHz (FCC)
#define RADIOLIB_LRXXXX_LR_FHSS_GRID_STEP_NON_FCC               (0x01UL << 0)   //  7     0                        3.90625 kHz (non-FCC)
#define RADIOLIB_LRXXXX_LR_FHSS_HOPPING_DISABLED                (0x00UL << 0)   //  7     0     LR FHSS hopping: disabled
#define RADIOLIB_LRXXXX_LR_FHSS_HOPPING_ENABLED                 (0x01UL << 0)   //  7     0                      enabled
#define RADIOLIB_LRXXXX_LR_FHSS_HOPPING_TEST_NO_HOP             (0x02UL << 0)   //  7     0                      test mode (packet encoded, no hopping)
#define RADIOLIB_LRXXXX_LR_FHSS_HOPPING_TEST_PA_RAMP            (0x03UL << 0)   //  7     0                      test mode (PA ramp up, no hopping)
#define RADIOLIB_LRXXXX_LR_FHSS_BW_39_06                        (0x00UL << 0)   //  7     0     LR FHSS bandwidth: 39.06 kHz
#define RADIOLIB_LRXXXX_LR_FHSS_BW_85_94                        (0x01UL << 0)   //  7     0                        85.94 kHz
#define RADIOLIB_LRXXXX_LR_FHSS_BW_136_72                       (0x02UL << 0)   //  7     0                        136.72 kHz
#define RADIOLIB_LRXXXX_LR_FHSS_BW_183_59                       (0x03UL << 0)   //  7     0                        183.59 kHz
#define RADIOLIB_LRXXXX_LR_FHSS_BW_335_94                       (0x04UL << 0)   //  7     0                        335.94 kHz
#define RADIOLIB_LRXXXX_LR_FHSS_BW_386_72                       (0x05UL << 0)   //  7     0                        386.72 kHz
#define RADIOLIB_LRXXXX_LR_FHSS_BW_722_66                       (0x06UL << 0)   //  7     0                        722.66 kHz
#define RADIOLIB_LRXXXX_LR_FHSS_BW_773_44                       (0x07UL << 0)   //  7     0                        773.44 kHz
#define RADIOLIB_LRXXXX_LR_FHSS_BW_1523_4                       (0x08UL << 0)   //  7     0                        1523.4 kHz
#define RADIOLIB_LRXXXX_LR_FHSS_BW_1574_2                       (0x09UL << 0)   //  7     0                        1574.2 kHz
#define RADIOLIB_LRXXXX_LR_FHSS_HEADER_BITS                     (114)           //  7     0     LR FHSS packet bit widths: header
#define RADIOLIB_LRXXXX_LR_FHSS_FRAG_BITS                       (48)            //  7     0                                payload fragment
#define RADIOLIB_LRXXXX_LR_FHSS_BLOCK_PREAMBLE_BITS             (2)             //  7     0                                block preamble
#define RADIOLIB_LRXXXX_LR_FHSS_BLOCK_BITS                      (RADIOLIB_LRXXXX_LR_FHSS_FRAG_BITS + RADIOLIB_LRXXXX_LR_FHSS_BLOCK_PREAMBLE_BITS)

// RADIOLIB_LR11X0_CMD_SET_TCXO_MODE
// RADIOLIB_LR2021_CMD_SET_TCXO_MODE
#define RADIOLIB_LRXXXX_TCXO_VOLTAGE_1_6                        (0x00UL << 0)   //  2     0     TCXO supply voltage: 1.6V
#define RADIOLIB_LRXXXX_TCXO_VOLTAGE_1_7                        (0x01UL << 0)   //  2     0                          1.7V
#define RADIOLIB_LRXXXX_TCXO_VOLTAGE_1_8                        (0x02UL << 0)   //  2     0                          1.8V
#define RADIOLIB_LRXXXX_TCXO_VOLTAGE_2_2                        (0x03UL << 0)   //  2     0                          2.2V
#define RADIOLIB_LRXXXX_TCXO_VOLTAGE_2_4                        (0x04UL << 0)   //  2     0                          2.4V
#define RADIOLIB_LRXXXX_TCXO_VOLTAGE_2_7                        (0x05UL << 0)   //  2     0                          2.7V
#define RADIOLIB_LRXXXX_TCXO_VOLTAGE_3_0                        (0x06UL << 0)   //  2     0                          3.0V
#define RADIOLIB_LRXXXX_TCXO_VOLTAGE_3_3                        (0x07UL << 0)   //  2     0                          3.3V

class LRxxxx {
  public:
    LRxxxx(Module* mod);

    /*!
      \brief Reset method. Will reset the chip to the default state using RST pin.
      \returns \ref status_codes
    */
    int16_t reset();

    /*!
      \brief Whether the module has an XTAL (true) or TCXO (false). Defaults to false.
    */
    bool XTAL;

  protected:
    Module* mod;

    // cached LoRa parameters
    uint8_t bandwidth = 0, spreadingFactor = 0, codingRate = 0, ldrOptimize = 0, crcTypeLoRa = 0, headerType = 0;
    uint16_t preambleLengthLoRa = 0;
    float bandwidthKhz = 0;
    bool ldroAuto = true;
    size_t implicitLen = 0;
    bool invertIQEnabled = false;

    // a lot of SPI commands have the same structure and arguments on both LR11xx as well as LR2021
    // the only difference is the 16-bit command code - however, having everything in this base class
    // will actually increase the binary size, because of the extra method calls that are needed
    // for that reason, only the methods that are 100% the same are kept here
    int16_t getStatus(uint8_t* stat1, uint8_t* stat2, uint32_t* irq);
    int16_t lrFhssBuildFrame(uint16_t cmd, uint8_t hdrCount, uint8_t cr, uint8_t grid, uint8_t hop, uint8_t bw, uint16_t hopSeq, int8_t devOffset, const uint8_t* payload, size_t len);

    // several commands just send unsigned 32-bit number
    int16_t setU32(uint16_t cmd, uint32_t u32);

    int16_t writeCommon(uint16_t cmd, uint32_t addrOffset, const uint32_t* data, size_t len, bool nonvolatile);
    int16_t SPIcommand(uint16_t cmd, bool write, uint8_t* data, size_t len, const uint8_t* out = NULL, size_t outLen = 0);

    static int16_t SPIparseStatus(uint8_t in);
    static int16_t SPIcheckStatus(Module* mod);

  private:
};

#endif
