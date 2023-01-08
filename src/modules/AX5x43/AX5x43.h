#if !defined(_RADIOLIB_AX5X43_H)
#define _RADIOLIB_AX504X_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_AX5X43)

#include "../../Module.h"
#include "../../protocols/PhysicalLayer/PhysicalLayer.h"

// AX5x43 physical layer properties
#define RADIOLIB_AX5X43_MAX_PACKET_LENGTH                       (256)
#define RADIOLIB_AX5X43_CRYSTAL_FREQ                            (26.0)
#define RADIOLIB_AX5X43_DIV_EXPONENT                            (24)
#define RADIOLIB_AX5X43_FREQUENCY_STEP_SIZE                     (((float)(RADIOLIB_AX5X43_CRYSTAL_FREQ * 1000000.0))/((float)(1UL << RADIOLIB_AX5X43_DIV_EXPONENT)))

// AX5x43 register map
#define RADIOLIB_AX5X43_REG_REVISION                            (0x0000)
#define RADIOLIB_AX5X43_REG_SCRATCH                             (0x0001)
#define RADIOLIB_AX5X43_REG_PWR_MODE                            (0x0002)
#define RADIOLIB_AX5X43_REG_POW_STAT                            (0x0003)
#define RADIOLIB_AX5X43_REG_POW_STICKY_STAT                     (0x0004)
#define RADIOLIB_AX5X43_REG_POW_IRQ_MASK                        (0x0005)
#define RADIOLIB_AX5X43_REG_IRQ_MASK_1                          (0x0006)
#define RADIOLIB_AX5X43_REG_IRQ_MASK_0                          (0x0007)
#define RADIOLIB_AX5X43_REG_RADIO_EVENT_MASK_1                  (0x0008)
#define RADIOLIB_AX5X43_REG_RADIO_EVENT_MASK_0                  (0x0009)
#define RADIOLIB_AX5X43_REG_IRQ_INVERSION_1                     (0x000A)
#define RADIOLIB_AX5X43_REG_IRQ_INVERSION_0                     (0x000B)
#define RADIOLIB_AX5X43_REG_IRQ_REQUEST_1                       (0x000C)
#define RADIOLIB_AX5X43_REG_IRQ_REQUEST_0                       (0x000D)
#define RADIOLIB_AX5X43_REG_RADIO_EVENT_REQ_1                   (0x000E)
#define RADIOLIB_AX5X43_REG_RADIO_EVENT_REQ_0                   (0x000F)
#define RADIOLIB_AX5X43_REG_MODULATION                          (0x0010)
#define RADIOLIB_AX5X43_REG_ENCODING                            (0x0011)
#define RADIOLIB_AX5X43_REG_FRAMING                             (0x0012)
#define RADIOLIB_AX5X43_REG_CRC_INIT_3                          (0x0014)
#define RADIOLIB_AX5X43_REG_CRC_INIT_2                          (0x0015)
#define RADIOLIB_AX5X43_REG_CRC_INIT_1                          (0x0016)
#define RADIOLIB_AX5X43_REG_CRC_INIT_0                          (0x0017)
#define RADIOLIB_AX5X43_REG_FEC                                 (0x0018)
#define RADIOLIB_AX5X43_REG_FEC_SYNC                            (0x0019)
#define RADIOLIB_AX5X43_REG_FEC_STATUS                          (0x001A)
#define RADIOLIB_AX5X43_REG_RADIO_STATE                         (0x001C)
#define RADIOLIB_AX5X43_REG_XTAL_STATUS                         (0x001D)
#define RADIOLIB_AX5X43_REG_PIN_STATE                           (0x0020)
#define RADIOLIB_AX5X43_REG_PIN_FUNC_SYSCLK                     (0x0021)
#define RADIOLIB_AX5X43_REG_PIN_FUNC_DCLK                       (0x0022)
#define RADIOLIB_AX5X43_REG_PIN_FUNC_DATA                       (0x0023)
#define RADIOLIB_AX5X43_REG_PIN_FUNC_IRQ                        (0x0024)
#define RADIOLIB_AX5X43_REG_PIN_FUNC_ANTSEL                     (0x0025)
#define RADIOLIB_AX5X43_REG_PIN_FUNC_PWRAMP                     (0x0026)
#define RADIOLIB_AX5X43_REG_PWRAMP                              (0x0027)
#define RADIOLIB_AX5X43_REG_FIFO_STAT                           (0x0028)
#define RADIOLIB_AX5X43_REG_FIFO_DATA                           (0x0029)
#define RADIOLIB_AX5X43_REG_FIFO_COUNT_1                        (0x002A)
#define RADIOLIB_AX5X43_REG_FIFO_COUNT_0                        (0x002B)
#define RADIOLIB_AX5X43_REG_FIFO_FREE_1                         (0x002C)
#define RADIOLIB_AX5X43_REG_FIFO_FREE_0                         (0x002D)
#define RADIOLIB_AX5X43_REG_FIFO_THRESH_1                       (0x002E)
#define RADIOLIB_AX5X43_REG_FIFO_THRESH_0                       (0x002F)
#define RADIOLIB_AX5X43_REG_PLL_LOOP                            (0x0030)
#define RADIOLIB_AX5X43_REG_PLL_CPI                             (0x0031)
#define RADIOLIB_AX5X43_REG_PLL_VCO_DIV                         (0x0032)
#define RADIOLIB_AX5X43_REG_PLL_RANGING_A                       (0x0033)
#define RADIOLIB_AX5X43_REG_FREQ_A_3                            (0x0034)
#define RADIOLIB_AX5X43_REG_FREQ_A_2                            (0x0035)
#define RADIOLIB_AX5X43_REG_FREQ_A_1                            (0x0036)
#define RADIOLIB_AX5X43_REG_FREQ_A_0                            (0x0037)
#define RADIOLIB_AX5X43_REG_PLL_LOOP_BOOST                      (0x0038)
#define RADIOLIB_AX5X43_REG_PLL_CPI_BOOST                       (0x0039)
#define RADIOLIB_AX5X43_REG_PLL_RANGING_B                       (0x003B)
#define RADIOLIB_AX5X43_REG_FREQ_B_3                            (0x003C)
#define RADIOLIB_AX5X43_REG_FREQ_B_2                            (0x003D)
#define RADIOLIB_AX5X43_REG_FREQ_B_1                            (0x003E)
#define RADIOLIB_AX5X43_REG_FREQ_B_0                            (0x003F)
#define RADIOLIB_AX5X43_REG_RSSI                                (0x0040)
#define RADIOLIB_AX5X43_REG_BGND_RSSI                           (0x0041)
#define RADIOLIB_AX5X43_REG_DIVERSITY                           (0x0042)
#define RADIOLIB_AX5X43_REG_AGC_COUNTER                         (0x0043)
#define RADIOLIB_AX5X43_REG_TRK_DATARATE_2                      (0x0045)
#define RADIOLIB_AX5X43_REG_TRK_DATARATE_1                      (0x0046)
#define RADIOLIB_AX5X43_REG_TRK_DATARATE_0                      (0x0047)
#define RADIOLIB_AX5X43_REG_TRK_AMPL_1                          (0x0048)
#define RADIOLIB_AX5X43_REG_TRK_AMPL_0                          (0x0049)
#define RADIOLIB_AX5X43_REG_TRK_PHASE_1                         (0x004A)
#define RADIOLIB_AX5X43_REG_TRK_PHASE_0                         (0x004B)
#define RADIOLIB_AX5X43_REG_TRK_RF_FREQ_2                       (0x004D)
#define RADIOLIB_AX5X43_REG_TRK_RF_FREQ_1                       (0x004E)
#define RADIOLIB_AX5X43_REG_TRK_RF_FREQ_0                       (0x004F)
#define RADIOLIB_AX5X43_REG_TRK_FREQ_2                          (0x0050)
#define RADIOLIB_AX5X43_REG_TRK_FREQ_1                          (0x0051)
#define RADIOLIB_AX5X43_REG_TRK_FSK_DEMOD_1                     (0x0052)
#define RADIOLIB_AX5X43_REG_TRK_FSK_DEMOD_0                     (0x0053)
#define RADIOLIB_AX5X43_REG_TIMER_2                             (0x0059)
#define RADIOLIB_AX5X43_REG_TIMER_1                             (0x005A)
#define RADIOLIB_AX5X43_REG_TIMER_0                             (0x005B)
#define RADIOLIB_AX5X43_REG_WAKEUP_TIMER_1                      (0x0068)
#define RADIOLIB_AX5X43_REG_WAKEUP_TIMER_0                      (0x0069)
#define RADIOLIB_AX5X43_REG_WAKEUP_1                            (0x006A)
#define RADIOLIB_AX5X43_REG_WAKEUP_0                            (0x006B)
#define RADIOLIB_AX5X43_REG_WAKEUP_FREQ_1                       (0x006C)
#define RADIOLIB_AX5X43_REG_WAKEUP_FREQ_0                       (0x006D)
#define RADIOLIB_AX5X43_REG_WAKEUP_XO_EARLY                     (0x006E)
#define RADIOLIB_AX5X43_REG_IF_FREQ_1                           (0x0100)
#define RADIOLIB_AX5X43_REG_IF_FREQ_0                           (0x0101)
#define RADIOLIB_AX5X43_REG_DECIMATION                          (0x0102)
#define RADIOLIB_AX5X43_REG_RX_DATA_RATE_2                      (0x0103)
#define RADIOLIB_AX5X43_REG_RX_DATA_RATE_1                      (0x0104)
#define RADIOLIB_AX5X43_REG_RX_DATA_RATE_0                      (0x0105)
#define RADIOLIB_AX5X43_REG_MAX_DR_OFFSET_2                     (0x0106)
#define RADIOLIB_AX5X43_REG_MAX_DR_OFFSET_1                     (0x0107)
#define RADIOLIB_AX5X43_REG_MAX_DR_OFFSET_0                     (0x0108)
#define RADIOLIB_AX5X43_REG_MAX_RF_OFFSET_2                     (0x0109)
#define RADIOLIB_AX5X43_REG_MAX_RF_OFFSET_1                     (0x010A)
#define RADIOLIB_AX5X43_REG_MAX_RF_OFFSET_0                     (0x010B)
#define RADIOLIB_AX5X43_REG_FSK_DMAX_1                          (0x010C)
#define RADIOLIB_AX5X43_REG_FSK_DMAX_0                          (0x010D)
#define RADIOLIB_AX5X43_REG_FSK_DMIN_1                          (0x010E)
#define RADIOLIB_AX5X43_REG_FSK_DMIN_0                          (0x010F)
#define RADIOLIB_AX5X43_REG_AFSK_SPACE_1                        (0x0110)
#define RADIOLIB_AX5X43_REG_AFSK_SPACE_0                        (0x0111)
#define RADIOLIB_AX5X43_REG_AFSK_MARK_1                         (0x0112)
#define RADIOLIB_AX5X43_REG_AFSK_MARK_0                         (0x0113)
#define RADIOLIB_AX5X43_REG_AFSK_CTRL                           (0x0114)
#define RADIOLIB_AX5X43_REG_AMPL_FILTER                         (0x0115)
#define RADIOLIB_AX5X43_REG_FREQUENCY_LEAK                      (0x0116)
#define RADIOLIB_AX5X43_REG_RX_PARAM_SETS                       (0x0117)
#define RADIOLIB_AX5X43_REG_RX_PARAM_CUR_SET                    (0x0118)
#define RADIOLIB_AX5X43_REG_RX_PARAM_SET_0                      (0x0120)
#define RADIOLIB_AX5X43_REG_RX_PARAM_SET_1                      (0x0130)
#define RADIOLIB_AX5X43_REG_RX_PARAM_SET_2                      (0x0140)
#define RADIOLIB_AX5X43_REG_RX_PARAM_SET_3                      (0x0150)
#define RADIOLIB_AX5X43_REG_RXPAR_AGC_GAIN                      (0x0000)
#define RADIOLIB_AX5X43_REG_RXPAR_AGC_TARGET                    (0x0001)
#define RADIOLIB_AX5X43_REG_RXPAR_AGC_HYST                      (0x0002)
#define RADIOLIB_AX5X43_REG_RXPAR_AGC_MIN_MAX                   (0x0003)
#define RADIOLIB_AX5X43_REG_RXPAR_TIME_GAIN                     (0x0004)
#define RADIOLIB_AX5X43_REG_RXPAR_DR_GAIN                       (0x0005)
#define RADIOLIB_AX5X43_REG_RXPAR_PHASE_GAIN                    (0x0006)
#define RADIOLIB_AX5X43_REG_RXPAR_FREQ_GAIN_A                   (0x0007)
#define RADIOLIB_AX5X43_REG_RXPAR_FREQ_GAIN_B                   (0x0008)
#define RADIOLIB_AX5X43_REG_RXPAR_FREQ_GAIN_C                   (0x0009)
#define RADIOLIB_AX5X43_REG_RXPAR_FREQ_GAIN_D                   (0x000A)
#define RADIOLIB_AX5X43_REG_RXPAR_AMPL_GAIN                     (0x000B)
#define RADIOLIB_AX5X43_REG_RXPAR_FREQ_DEV_1                    (0x000C)
#define RADIOLIB_AX5X43_REG_RXPAR_FREQ_DEV_0                    (0x000D)
#define RADIOLIB_AX5X43_REG_RXPAR_FOUR_FSK                      (0x000E)
#define RADIOLIB_AX5X43_REG_RXPAR_BB_OFFS_RES                   (0x000F)
#define RADIOLIB_AX5X43_REG_MOD_CFG_F                           (0x0160)
#define RADIOLIB_AX5X43_REG_FSK_DEV_2                           (0x0161)
#define RADIOLIB_AX5X43_REG_FSK_DEV_1                           (0x0162)
#define RADIOLIB_AX5X43_REG_FSK_DEV_0                           (0x0163)
#define RADIOLIB_AX5X43_REG_MOD_CFG_A                           (0x0164)
#define RADIOLIB_AX5X43_REG_TX_RATE_2                           (0x0165)
#define RADIOLIB_AX5X43_REG_TX_RATE_1                           (0x0166)
#define RADIOLIB_AX5X43_REG_TX_RATE_0                           (0x0167)
#define RADIOLIB_AX5X43_REG_TX_PWR_COEFF_A_1                    (0x0168)
#define RADIOLIB_AX5X43_REG_TX_PWR_COEFF_A_0                    (0x0169)
#define RADIOLIB_AX5X43_REG_TX_PWR_COEFF_B_1                    (0x016A)
#define RADIOLIB_AX5X43_REG_TX_PWR_COEFF_B_0                    (0x016B)
#define RADIOLIB_AX5X43_REG_TX_PWR_COEFF_C_1                    (0x016C)
#define RADIOLIB_AX5X43_REG_TX_PWR_COEFF_C_0                    (0x016D)
#define RADIOLIB_AX5X43_REG_TX_PWR_COEFF_D_1                    (0x016E)
#define RADIOLIB_AX5X43_REG_TX_PWR_COEFF_D_0                    (0x016F)
#define RADIOLIB_AX5X43_REG_TX_PWR_COEFF_E_1                    (0x0170)
#define RADIOLIB_AX5X43_REG_TX_PWR_COEFF_E_0                    (0x0171)
#define RADIOLIB_AX5X43_REG_PLL_VCO_I                           (0x0180)
#define RADIOLIB_AX5X43_REG_PLL_VCO_IR                          (0x0181)
#define RADIOLIB_AX5X43_REG_PLL_LOCK_DET                        (0x0182)
#define RADIOLIB_AX5X43_REG_PLL_RNG_CLK                         (0x0183)
#define RADIOLIB_AX5X43_REG_XTAL_CAP                            (0x0184)
#define RADIOLIB_AX5X43_REG_BB_TUNE                             (0x0188)
#define RADIOLIB_AX5X43_REG_BB_OFFS_CAP                         (0x0189)
#define RADIOLIB_AX5X43_REG_PKT_ADDR_CFG                        (0x0200)
#define RADIOLIB_AX5X43_REG_PKT_LEN_CFG                         (0x0201)
#define RADIOLIB_AX5X43_REG_PKT_LEN_OFFSET                      (0x0202)
#define RADIOLIB_AX5X43_REG_PKT_MAX_LEN                         (0x0203)
#define RADIOLIB_AX5X43_REG_PKT_ADDR_3                          (0x0204)
#define RADIOLIB_AX5X43_REG_PKT_ADDR_2                          (0x0205)
#define RADIOLIB_AX5X43_REG_PKT_ADDR_1                          (0x0206)
#define RADIOLIB_AX5X43_REG_PKT_ADDR_0                          (0x0207)
#define RADIOLIB_AX5X43_REG_PKT_ADDR_MASK_3                     (0x0208)
#define RADIOLIB_AX5X43_REG_PKT_ADDR_MASK_2                     (0x0209)
#define RADIOLIB_AX5X43_REG_PKT_ADDR_MASK_1                     (0x020A)
#define RADIOLIB_AX5X43_REG_PKT_ADDR_MASK_0                     (0x020B)
#define RADIOLIB_AX5X43_REG_MATCH_0_PAT_3                       (0x0210)
#define RADIOLIB_AX5X43_REG_MATCH_0_PAT_2                       (0x0211)
#define RADIOLIB_AX5X43_REG_MATCH_0_PAT_1                       (0x0212)
#define RADIOLIB_AX5X43_REG_MATCH_0_PAT_0                       (0x0213)
#define RADIOLIB_AX5X43_REG_MATCH_0_LEN                         (0x0214)
#define RADIOLIB_AX5X43_REG_MATCH_0_MIN                         (0x0215)
#define RADIOLIB_AX5X43_REG_MATCH_0_MAX                         (0x0216)
#define RADIOLIB_AX5X43_REG_MATCH_1_PAT_1                       (0x0218)
#define RADIOLIB_AX5X43_REG_MATCH_1_PAT_0                       (0x0219)
#define RADIOLIB_AX5X43_REG_MATCH_1_LEN                         (0x021C)
#define RADIOLIB_AX5X43_REG_MATCH_1_MIN                         (0x021D)
#define RADIOLIB_AX5X43_REG_MATCH_1_MAX                         (0x021E)
#define RADIOLIB_AX5X43_REG_TMG_TX_BOOST                        (0x0220)
#define RADIOLIB_AX5X43_REG_TMG_TX_SETTLE                       (0x0221)
#define RADIOLIB_AX5X43_REG_TMG_RX_BOOST                        (0x0223)
#define RADIOLIB_AX5X43_REG_TMG_RX_SETTLE                       (0x0224)
#define RADIOLIB_AX5X43_REG_TMG_RX_OFFS_ACQ                     (0x0225)
#define RADIOLIB_AX5X43_REG_TMG_RX_COARSE_ACQ                   (0x0226)
#define RADIOLIB_AX5X43_REG_TMG_RX_AGC                          (0x0227)
#define RADIOLIB_AX5X43_REG_TMG_RX_RSSI                         (0x0228)
#define RADIOLIB_AX5X43_REG_TMG_RX_PREAMBLE_1                   (0x0229)
#define RADIOLIB_AX5X43_REG_TMG_RX_PREAMBLE_2                   (0x022A)
#define RADIOLIB_AX5X43_REG_TMG_RX_PREAMBLE_3                   (0x022B)
#define RADIOLIB_AX5X43_REG_RSSI_REFERENCE                      (0x022C)
#define RADIOLIB_AX5X43_REG_RSSI_ABS_THR                        (0x022D)
#define RADIOLIB_AX5X43_REG_BGND_RSSI_GAIN                      (0x022E)
#define RADIOLIB_AX5X43_REG_BGND_RSSI_THR                       (0x022F)
#define RADIOLIB_AX5X43_REG_PKT_CHUNK_SIZE                      (0x0230)
#define RADIOLIB_AX5X43_REG_PKT_MISC_FLAGS                      (0x0231)
#define RADIOLIB_AX5X43_REG_PKT_STORE_FLAGS                     (0x0232)
#define RADIOLIB_AX5X43_REG_PKT_ACCEPT_FLAGS                    (0x0233)
#define RADIOLIB_AX5X43_REG_GP_ADC_CTRL                         (0x0300)
#define RADIOLIB_AX5X43_REG_GP_ADC_PERIOD                       (0x0301)
#define RADIOLIB_AX5X43_REG_GP_ADC_13_VALUE_1                   (0x0308)
#define RADIOLIB_AX5X43_REG_GP_ADC_13_VALUE_0                   (0x0309)
#define RADIOLIB_AX5X43_REG_LP_OSC_CONFIG                       (0x0310)
#define RADIOLIB_AX5X43_REG_LP_OSC_STATUS                       (0x0311)
#define RADIOLIB_AX5X43_REG_LP_OSC_FILTER_1                     (0x0312)
#define RADIOLIB_AX5X43_REG_LP_OSC_FILTER_0                     (0x0313)
#define RADIOLIB_AX5X43_REG_LP_OSC_REF_1                        (0x0314)
#define RADIOLIB_AX5X43_REG_LP_OSC_REF_0                        (0x0315)
#define RADIOLIB_AX5X43_REG_LP_OSC_FREQ_1                       (0x0316)
#define RADIOLIB_AX5X43_REG_LP_OSC_FREQ_0                       (0x0317)
#define RADIOLIB_AX5X43_REG_LP_OSC_PER_1                        (0x0318)
#define RADIOLIB_AX5X43_REG_LP_OSC_PER_0                        (0x0319)
#define RADIOLIB_AX5X43_REG_DAC_VALUE_1                         (0x0330)
#define RADIOLIB_AX5X43_REG_DAC_VALUE_0                         (0x0331)
#define RADIOLIB_AX5X43_REG_DAC_CONFIG                          (0x0332)

// command to access registers via long addresses
#define RADIOLIB_AX5X43_SPI_LONG_ADDR_CMD                       (0x70)

// FIFO chunk headers
#define RADIOLIB_AX5X43_FIFO_CHUNK_HDR_NOP                      (0x00)
#define RADIOLIB_AX5X43_FIFO_CHUNK_HDR_RSSI                     (0x31)
#define RADIOLIB_AX5X43_FIFO_CHUNK_HDR_TXCTRL                   (0x3C)
#define RADIOLIB_AX5X43_FIFO_CHUNK_HDR_FREQOFFS                 (0x52)
#define RADIOLIB_AX5X43_FIFO_CHUNK_HDR_ANTRSSI2                 (0x55)
#define RADIOLIB_AX5X43_FIFO_CHUNK_HDR_REPEATDATA               (0x62)
#define RADIOLIB_AX5X43_FIFO_CHUNK_HDR_TIMER                    (0x70)
#define RADIOLIB_AX5X43_FIFO_CHUNK_HDR_RFFREQOFFS               (0x73)
#define RADIOLIB_AX5X43_FIFO_CHUNK_HDR_DATARATE                 (0x74)
#define RADIOLIB_AX5X43_FIFO_CHUNK_HDR_ANTRSSI3                 (0x75)
#define RADIOLIB_AX5X43_FIFO_CHUNK_HDR_DATA                     (0xE1)
#define RADIOLIB_AX5X43_FIFO_CHUNK_HDR_TXPWR                    (0xFD)

// RADIOLIB_AX5X43_FIFO_CHUNK_HDR_DATA                                            MSB   LSB   DESCRIPTION
#define RADIOLIB_AX5X43_FIFO_TX_DATA_UNENC                      (0x01 << 5)   //  5     5     disable framing and encoder
#define RADIOLIB_AX5X43_FIFO_TX_DATA_RAW                        (0x01 << 4)   //  4     4     disable only framing
#define RADIOLIB_AX5X43_FIFO_TX_DATA_NOCRC                      (0x01 << 3)   //  3     3     disable CRC generation
#define RADIOLIB_AX5X43_FIFO_RX_DATA_ABORT                      (0x01 << 6)   //  6     6     packet has been aborted
#define RADIOLIB_AX5X43_FIFO_RX_DATA_SIZEFAIL                   (0x01 << 5)   //  5     5     packet size checks failed
#define RADIOLIB_AX5X43_FIFO_RX_DATA_ADDRFAIL                   (0x01 << 4)   //  4     4     packet address checks failed
#define RADIOLIB_AX5X43_FIFO_RX_DATA_CRCFAIL                    (0x01 << 3)   //  3     3     packet CRC checks failed
#define RADIOLIB_AX5X43_FIFO_DATA_RESIDUE                       (0x01 << 2)   //  2     2     non-octet transmission
#define RADIOLIB_AX5X43_FIFO_DATA_PKTEND                        (0x01 << 1)   //  1     1     mark packet end if packet length is larger than FIFO
#define RADIOLIB_AX5X43_FIFO_DATA_PKTSTART                      (0x01 << 0)   //  0     0     mark packet start if packet length is larger than FIFO

// RADIOLIB_AX5X43_REG_REVISION
#define RADIOLIB_AX5X43_SILICONREV                              (0x51 << 0)   //  7     0     silicon revision number

// RADIOLIB_AX5X43_REG_PWR_MODE
#define RADIOLIB_AX5X43_PWR_MODE_RESET_SET                      (0x01 << 7)   //  7     7     reset the chip
#define RADIOLIB_AX5X43_PWR_MODE_RESET_CLEAR                    (0x00 << 7)   //  7     7     clear reset condition
#define RADIOLIB_AX5X43_PWR_MODE_XO_ENABLED                     (0x01 << 6)   //  6     6     crystal oscillator: enabled (default)
#define RADIOLIB_AX5X43_PWR_MODE_XO_DISABLED                    (0x00 << 6)   //  6     6                         disabled
#define RADIOLIB_AX5X43_PWR_MODE_REFERENCE_ENABLED              (0x01 << 5)   //  5     5     internal reference circuitry: enabled (default)
#define RADIOLIB_AX5X43_PWR_MODE_REFERENCE_DISABLED             (0x00 << 5)   //  5     5                                   disabled
#define RADIOLIB_AX5X43_PWR_MODE_DEEP_SLEEP_WAKEUP              (0x01 << 4)   //  4     4     wakeup from deep sleep
#define RADIOLIB_AX5X43_PWR_MODE_POWER_DOWN                     (0x00 << 0)   //  3     0     power mode: power down (default)
#define RADIOLIB_AX5X43_PWR_MODE_DEEP_SLEEP                     (0x01 << 0)   //  3     0                 deep sleep
#define RADIOLIB_AX5X43_PWR_MODE_STANDBY                        (0x05 << 0)   //  3     0                 standby, but no FIFO
#define RADIOLIB_AX5X43_PWR_MODE_FIFO_ON                        (0x07 << 0)   //  3     0                 FIFO-on standby
#define RADIOLIB_AX5X43_PWR_MODE_SYNTH_RX                       (0x08 << 0)   //  3     0                 synthesizer running, Rx
#define RADIOLIB_AX5X43_PWR_MODE_FULL_RX                        (0x09 << 0)   //  3     0                 full Rx
#define RADIOLIB_AX5X43_PWR_MODE_WOR_RX                         (0x0B << 0)   //  3     0                 wake-on-radio Rx
#define RADIOLIB_AX5X43_PWR_MODE_SYNTH_TX                       (0x0C << 0)   //  3     0                 synthesizer running, Tx
#define RADIOLIB_AX5X43_PWR_MODE_FULL_TX                        (0x0D << 0)   //  3     0                 full Tx

// RADIOLIB_AX5X43_REG_POW_STAT, RADIOLIB_AX5X43_REG_POW_STICKY_STAT, RADIOLIB_AX5X43_REG_POW_IRQ_MASK
#define RADIOLIB_AX5X43_POW_STAT_SSUM                           (0x01 << 7)   //  7     7     summary status ready
#define RADIOLIB_AX5X43_POW_STAT_SREF                           (0x01 << 6)   //  6     6     reference ready
#define RADIOLIB_AX5X43_POW_STAT_SVREF                          (0x01 << 5)   //  5     5     reference voltage regulator ready
#define RADIOLIB_AX5X43_POW_STAT_SVANA                          (0x01 << 4)   //  4     4     analogue domain voltage regulator ready
#define RADIOLIB_AX5X43_POW_STAT_SVMODEM                        (0x01 << 3)   //  3     3     modem domain voltage regulator ready
#define RADIOLIB_AX5X43_POW_STAT_SBEVANA                        (0x01 << 2)   //  2     2     analogue domain brownout not detected
#define RADIOLIB_AX5X43_POW_STAT_SBEVMODEM                      (0x01 << 1)   //  1     1     modem domain brownout not detected
#define RADIOLIB_AX5X43_POW_STAT_SVIO                           (0x01 << 0)   //  0     0     IO voltage large enough

// RADIOLIB_AX5X43_REG_IRQ_MASK_0, RADIOLIB_AX5X43_REG_IRQ_MASK_1
// RADIOLIB_AX5X43_REG_IRQ_INVERSION_0, RADIOLIB_AX5X43_REG_IRQ_INVERSION_1
// RADIOLIB_AX5X43_REG_IRQ_REQUEST_0, RADIOLIB_AX5X43_REG_IRQ_REQUEST_1
#define RADIOLIB_AX5X43_IRQ_PLL_RNG_DONE                        (0x001 << 12) // 12    21     IRQ: PLL autoranging done
#define RADIOLIB_AX5X43_IRQ_GPADC                               (0x001 << 11) // 11    11          general-purpose ADC
#define RADIOLIB_AX5X43_IRQ_LPOSC                               (0x001 << 10) // 10    10          low-power oscillator
#define RADIOLIB_AX5X43_IRQ_WAKEUPTIMER                         (0x001 << 9)  //  9     9          wakeup timer
#define RADIOLIB_AX5X43_IRQ_XTAL_READY                          (0x001 << 8)  //  8     8          crystal oscillator ready
#define RADIOLIB_AX5X43_IRQ_POWER                               (0x001 << 7)  //  7     7          power
#define RADIOLIB_AX5X43_IRQ_RADIO_CTRL                          (0x001 << 6)  //  6     6          radio controller
#define RADIOLIB_AX5X43_IRQ_PLL_UNLOCK                          (0x001 << 5)  //  5     5          PLL lock lost
#define RADIOLIB_AX5X43_IRQ_FIFO_ERROR                          (0x001 << 4)  //  4     4          FIFO error
#define RADIOLIB_AX5X43_IRQ_FIFO_THR_FREE                       (0x001 << 3)  //  3     3          FIFO free level over threshold
#define RADIOLIB_AX5X43_IRQ_FIFO_THR_CNT                        (0x001 << 2)  //  2     2          FIFO count level over threshold
#define RADIOLIB_AX5X43_IRQ_FIFO_NOT_FULL                       (0x001 << 1)  //  1     1          FIFO not full
#define RADIOLIB_AX5X43_IRQ_FIFO_NOT_EMPTY                      (0x001 << 0)  //  0     0          FIFO not empty

// RADIOLIB_AX5X43_REG_RADIO_EVENT_MASK_0, RADIOLIB_AX5X43_REG_RADIO_EVENT_MASK_1
// RADIOLIB_AX5X43_REG_RADIO_EVENT_REQ_0, RADIOLIB_AX5X43_REG_RADIO_EVENT_REQ_1
#define RADIOLIB_AX5X43_EVENT_FRAME_CLK                         (0x01 << 4)   //  4     4     radio event: frame clock
#define RADIOLIB_AX5X43_EVENT_RX_PARAM_SET_CHG                  (0x01 << 3)   //  3     3                  receive parameter set changed
#define RADIOLIB_AX5X43_EVENT_RADIO_STATE_CHG                   (0x01 << 2)   //  2     2                  radio state changed
#define RADIOLIB_AX5X43_EVENT_PLL_SETTLED                       (0x01 << 1)   //  1     1                  PLL settled
#define RADIOLIB_AX5X43_EVENT_RX_TX_DONE                        (0x01 << 0)   //  0     0                  receive or transmit done

// RADIOLIB_AX5X43_REG_MODULATION
#define RADIOLIB_AX5X43_RX_HALFSPEED                            (0x01 << 4)   //  4     4     halve receive bitrate
#define RADIOLIB_AX5X43_MODULATION_FM                           (0x0B << 0)   //  3     0     modulation type: FM
#define RADIOLIB_AX5X43_MODULATION_AFSK                         (0x0A << 0)   //  3     0                      AFSK
#define RADIOLIB_AX5X43_MODULATION_4FSK                         (0x09 << 0)   //  3     0                      4-FSK
#define RADIOLIB_AX5X43_MODULATION_FSK                          (0x08 << 0)   //  3     0                      FSK
#define RADIOLIB_AX5X43_MODULATION_MSK                          (0x07 << 0)   //  3     0                      MSK
#define RADIOLIB_AX5X43_MODULATION_OQSK                         (0x06 << 0)   //  3     0                      OQSK
#define RADIOLIB_AX5X43_MODULATION_PSK                          (0x04 << 0)   //  3     0                      PSK
#define RADIOLIB_AX5X43_MODULATION_ASK_COH                      (0x01 << 0)   //  3     0                      ASK coherent
#define RADIOLIB_AX5X43_MODULATION_ASK                          (0x00 << 0)   //  3     0                      ASK

// RADIOLIB_AX5X43_REG_ENCODING
#define RADIOLIB_AX5X43_ENC_4FSK_SYNC_DISABLED                  (0x01 << 4)   //  4     4     dibit sync in 4-FSK mode: disabled
#define RADIOLIB_AX5X43_ENC_4FSK_SYNC_ENABLED                   (0x00 << 4)   //  4     4                               enabled (default)
#define RADIOLIB_AX5X43_ENC_MANCH_DISABLED                      (0x00 << 3)   //  3     3     Manchester encoding: disabled (default)
#define RADIOLIB_AX5X43_ENC_MANCH_ENABLED                       (0x01 << 3)   //  3     3                          enabled
#define RADIOLIB_AX5X43_ENC_SCRAM_DISABLED                      (0x00 << 2)   //  2     2     scrambler: disabled (default)
#define RADIOLIB_AX5X43_ENC_SCRAM_ENABLED                       (0x01 << 2)   //  2     2                enabled
#define RADIOLIB_AX5X43_ENC_DIFF_DISABLED                       (0x00 << 1)   //  1     1     differential encoding: disabled
#define RADIOLIB_AX5X43_ENC_DIFF_ENABLED                        (0x01 << 1)   //  1     1                            enabled (default)
#define RADIOLIB_AX5X43_ENC_INV_DISABLED                        (0x00 << 0)   //  0     0     data inversion: disabled (default)
#define RADIOLIB_AX5X43_ENC_INV_ENABLED                         (0x01 << 0)   //  0     0                     enabled

// RADIOLIB_AX5X43_REG_FRAMING
#define RADIOLIB_AX5X43_FRM_RX                                  (0x01 << 7)   //  7     7     packet start detected
#define RADIOLIB_AX5X43_FRM_CRC_MODE_CRC_32                     (0x06 << 4)   //  6     4     CRC mode: CRC-32
#define RADIOLIB_AX5X43_FRM_CRC_MODE_DNP_16                     (0x03 << 4)   //  6     4               DNP (16-bit)
#define RADIOLIB_AX5X43_FRM_CRC_MODE_CRC_16                     (0x02 << 4)   //  6     4               CRC-16
#define RADIOLIB_AX5X43_FRM_CRC_MODE_CCITT_16                   (0x01 << 4)   //  6     4               CCITT (16-bit)
#define RADIOLIB_AX5X43_FRM_CRC_MODE_DISABLED                   (0x00 << 4)   //  6     4               disabled (default)
#define RADIOLIB_AX5X43_FRM_MODE_W_MBUS_4_TO_6                  (0x05 << 1)   //  3     1     framing mode: Wireless M-bus (4-to-6 encoding)
#define RADIOLIB_AX5X43_FRM_MODE_W_MBUS                         (0x04 << 1)   //  3     1                   Wireless M-bus
#define RADIOLIB_AX5X43_FRM_MODE_RAW_PATTERN                    (0x03 << 1)   //  3     1                   raw, pattern matching
#define RADIOLIB_AX5X43_FRM_MODE_HDLC                           (0x02 << 1)   //  3     1                   HDLC
#define RADIOLIB_AX5X43_FRM_MODE_RAW_SOFT                       (0x01 << 1)   //  3     1                   raw, soft bits
#define RADIOLIB_AX5X43_FRM_MODE_RAW                            (0x00 << 1)   //  3     1                   raw (default)
#define RADIOLIB_AX5X43_FRM_ABORT                               (0x01 << 0)   //  0     0     abort current HDLC packet or pattern matching

// RADIOLIB_AX5X43_REG_FEC
#define RADIOLIB_AX5X43_FEC_SHORT_MEM_DISABLED                  (0x00 << 7)   //  7     7     backtrack memory shortening: disabled (default)
#define RADIOLIB_AX5X43_FEC_SHORT_MEM_ENABLED                   (0x01 << 7)   //  7     7                                  enabled
#define RADIOLIB_AX5X43_FEC_RESET_VITERBI                       (0x01 << 6)   //  6     6     reset Viterbit decoder
#define RADIOLIB_AX5X43_FEC_INV_INTER_SYNC_DISABLED             (0x00 << 5)   //  5     5     inverted interleaver synchronization: disabled (default)
#define RADIOLIB_AX5X43_FEC_INV_INTER_SYNC_ENABLED              (0x01 << 5)   //  5     5                                           enabled
#define RADIOLIB_AX5X43_FEC_NONINV_INTER_SYNC_DISABLED          (0x00 << 4)   //  4     4     non-inverted interleaver synchronization: disabled (default)
#define RADIOLIB_AX5X43_FEC_NONINV_INTER_SYNC_ENABLED           (0x01 << 4)   //  4     4                                               enabled
#define RADIOLIB_AX5X43_FEC_INPUT_SHIFT                         (0x00 << 1)   //  3     1     soft Rx data attenuation
#define RADIOLIB_AX5X43_FEC_DISABLED                            (0x00 << 0)   //  0     0     convolutional FEC: disabled (default)
#define RADIOLIB_AX5X43_FEC_ENABLED                             (0x01 << 0)   //  0     0                        enabled

// RADIOLIB_AX5X43_REG_RADIO_STATE
#define RADIOLIB_AX5X43_RADIO_STATE_RX                          (0x0F << 0)   //  3     0     radio state: Rx
#define RADIOLIB_AX5X43_RADIO_STATE_RX_PREAMBLE_3               (0x0E << 0)   //  3     0                  Rx preamble 3
#define RADIOLIB_AX5X43_RADIO_STATE_RX_PREAMBLE_2               (0x0D << 0)   //  3     0                  Rx preamble 2
#define RADIOLIB_AX5X43_RADIO_STATE_RX_PREAMBLE_1               (0x0C << 0)   //  3     0                  Rx preamble 1
#define RADIOLIB_AX5X43_RADIO_STATE_RX_ANT_SEL                  (0x09 << 0)   //  3     0                  Rx antenna selection
#define RADIOLIB_AX5X43_RADIO_STATE_RX_PLL                      (0x08 << 0)   //  3     0                  Rx PLL
#define RADIOLIB_AX5X43_RADIO_STATE_TX_TAIL                     (0x07 << 0)   //  3     0                  Tx tail
#define RADIOLIB_AX5X43_RADIO_STATE_TX                          (0x06 << 0)   //  3     0                  Tx
#define RADIOLIB_AX5X43_RADIO_STATE_TX_PLL                      (0x04 << 0)   //  3     0                  Tx PLL
#define RADIOLIB_AX5X43_RADIO_STATE_POWER_DOWN                  (0x01 << 0)   //  3     0                  power down
#define RADIOLIB_AX5X43_RADIO_STATE_IDLE                        (0x00 << 0)   //  3     0                  idle

// RADIOLIB_AX5X43_REG_XTAL_STATUS
#define RADIOLIB_AX5X43_XTAL_RUNNING                            (0x01 << 0)   //  0     0     crystal is running and stable

// RADIOLIB_AX5X43_REG_PIN_STATE
#define RADIOLIB_AX5X43_PIN_STATE_PWRAMP                        (0x01 << 5)   //  5     5     signal level on: PWRAMP
#define RADIOLIB_AX5X43_PIN_STATE_ANTSEL                        (0x01 << 4)   //  4     4                      ANTSEL
#define RADIOLIB_AX5X43_PIN_STATE_IRQ                           (0x01 << 3)   //  3     3                      IRQ
#define RADIOLIB_AX5X43_PIN_STATE_DATA                          (0x01 << 2)   //  2     2                      DATA
#define RADIOLIB_AX5X43_PIN_STATE_DCLK                          (0x01 << 1)   //  1     1                      DCLK
#define RADIOLIB_AX5X43_PIN_STATE_SYSCLK                        (0x01 << 0)   //  0     0                      SYSCLK

// RADIOLIB_AX5X43_REG_PIN_FUNC_SYSCLK
#define RADIOLIB_AX5X43_PIN_SYSCLK_PU_DISABLED                  (0x00 << 7)   //  7     7     SYSCLK pull-up: disabled (default)
#define RADIOLIB_AX5X43_PIN_SYSCLK_PU_ENABLED                   (0x01 << 7)   //  7     7                     enabled
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_TEST                     (0x1F << 0)   //  4     0     SYSCLK: test output
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_LPOSC                    (0x0F << 0)   //  4     0             low-power oscillator
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_XTAL_1024                (0x0E << 0)   //  4     0             f_XTAL/1024
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_XTAL_512                 (0x0D << 0)   //  4     0             f_XTAL/512
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_XTAL_256                 (0x0C << 0)   //  4     0             f_XTAL/256
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_XTAL_128                 (0x0B << 0)   //  4     0             f_XTAL/128
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_XTAL_64                  (0x0A << 0)   //  4     0             f_XTAL/64
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_XTAL_32                  (0x09 << 0)   //  4     0             f_XTAL/32
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_XTAL_16                  (0x08 << 0)   //  4     0             f_XTAL/16 (default)
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_XTAL_8                   (0x07 << 0)   //  4     0             f_XTAL/8
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_XTAL_4                   (0x06 << 0)   //  4     0             f_XTAL/4
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_XTAL_2                   (0x05 << 0)   //  4     0             f_XTAL/2
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_XTAL                     (0x04 << 0)   //  4     0             f_XTAL
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_XTAL_INV                 (0x03 << 0)   //  4     0             inverted f_XTAL
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_HI_Z                     (0x02 << 0)   //  4     0             high-Z
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_HIGH                     (0x01 << 0)   //  4     0             high
#define RADIOLIB_AX5X43_PIN_SYSCLK_OUT_LOW                      (0x00 << 0)   //  4     0             low

// RADIOLIB_AX5X43_REG_PIN_FUNC_DCLK
#define RADIOLIB_AX5X43_PIN_DCLK_PU_DISABLED                    (0x00 << 7)   //  7     7     DCLK pull-up: disabled (default)
#define RADIOLIB_AX5X43_PIN_DCLK_PU_ENABLED                     (0x01 << 7)   //  7     7                   enabled
#define RADIOLIB_AX5X43_PIN_DCLK_INV_DISABLED                   (0x00 << 6)   //  6     6     DCLK inversion: disabled (default)
#define RADIOLIB_AX5X43_PIN_DCLK_INV_ENABLED                    (0x01 << 6)   //  6     6                     enabled
#define RADIOLIB_AX5X43_PIN_DCLK_OUT_TEST                       (0x07 << 0)   //  3     0     DCLK: test output
#define RADIOLIB_AX5X43_PIN_DCLK_OUT_CLOCK_FRAMING              (0x05 << 0)   //  3     0           modem clock for framing data
#define RADIOLIB_AX5X43_PIN_DCLK_OUT_CLOCK_MODEM                (0x04 << 0)   //  3     0           modem clock for modem data (default)
#define RADIOLIB_AX5X43_PIN_DCLK_IN_CLOCK_FRAMING               (0x03 << 0)   //  3     0           modem clock for framing data input
#define RADIOLIB_AX5X43_PIN_DCLK_OUT_HI_Z                       (0x02 << 0)   //  3     0           high-Z
#define RADIOLIB_AX5X43_PIN_DCLK_OUT_HIGH                       (0x01 << 0)   //  3     0           high
#define RADIOLIB_AX5X43_PIN_DCLK_OUT_LOW                        (0x00 << 0)   //  3     0           low

// RADIOLIB_AX5X43_REG_PIN_FUNC_DATA
#define RADIOLIB_AX5X43_PIN_DATA_PU_DISABLED                    (0x00 << 7)   //  7     7     DATA pull-up: disabled (default)
#define RADIOLIB_AX5X43_PIN_DATA_PU_ENABLED                     (0x01 << 7)   //  7     7                   enabled
#define RADIOLIB_AX5X43_PIN_DATA_INV_DISABLED                   (0x00 << 6)   //  6     6     DATA inversion: disabled (default)
#define RADIOLIB_AX5X43_PIN_DATA_INV_ENABLED                    (0x01 << 6)   //  6     6                     enabled
#define RADIOLIB_AX5X43_PIN_DATA_OUT_TEST                       (0x0F << 0)   //  3     0     DATA: test output
#define RADIOLIB_AX5X43_PIN_DATA_OUT_MODEM                      (0x07 << 0)   //  3     0           modem data output (default)
#define RADIOLIB_AX5X43_PIN_DATA_IN_OUT_MODEM_ASYNC             (0x05 << 0)   //  3     0           async modem data input/output
#define RADIOLIB_AX5X43_PIN_DATA_IN_OUT_MODEM                   (0x04 << 0)   //  3     0           modem data input/output
#define RADIOLIB_AX5X43_PIN_DATA_IN_OUT_FRAMING                 (0x03 << 0)   //  3     0           framing data input/output
#define RADIOLIB_AX5X43_PIN_DATA_OUT_HI_Z                       (0x02 << 0)   //  3     0           high-Z
#define RADIOLIB_AX5X43_PIN_DATA_OUT_HIGH                       (0x01 << 0)   //  3     0           high
#define RADIOLIB_AX5X43_PIN_DATA_OUT_LOW                        (0x00 << 0)   //  3     0           low

// RADIOLIB_AX5X43_REG_PIN_FUNC_IRQ
#define RADIOLIB_AX5X43_PIN_IRQ_PU_DISABLED                     (0x00 << 7)   //  7     7     IRQ pull-up: disabled (default)
#define RADIOLIB_AX5X43_PIN_IRQ_PU_ENABLED                      (0x01 << 7)   //  7     7                  enabled
#define RADIOLIB_AX5X43_PIN_IRQ_INV_DISABLED                    (0x00 << 6)   //  6     6     IRQ inversion: disabled (default)
#define RADIOLIB_AX5X43_PIN_IRQ_INV_ENABLED                     (0x01 << 6)   //  6     6                    enabled
#define RADIOLIB_AX5X43_PIN_IRQ_OUT_TEST                        (0x07 << 0)   //  2     0     IRQ: test output
#define RADIOLIB_AX5X43_PIN_IRQ_OUT_INTERRUPT                   (0x03 << 0)   //  2     0          interrupt request output (default)
#define RADIOLIB_AX5X43_PIN_IRQ_OUT_HI_Z                        (0x02 << 0)   //  2     0          high-Z
#define RADIOLIB_AX5X43_PIN_IRQ_OUT_HIGH                        (0x01 << 0)   //  2     0          high
#define RADIOLIB_AX5X43_PIN_IRQ_OUT_LOW                         (0x00 << 0)   //  2     0          low

// RADIOLIB_AX5X43_REG_PIN_FUNC_ANTSEL
#define RADIOLIB_AX5X43_PIN_ANTSEL_PU_DISABLED                  (0x00 << 7)   //  7     7     ANTSEL pull-up: disabled (default)
#define RADIOLIB_AX5X43_PIN_ANTSEL_PU_ENABLED                   (0x01 << 7)   //  7     7                     enabled
#define RADIOLIB_AX5X43_PIN_ANTSEL_INV_DISABLED                 (0x00 << 6)   //  6     6     ANTSEL inversion: disabled (default)
#define RADIOLIB_AX5X43_PIN_ANTSEL_INV_ENABLED                  (0x01 << 6)   //  6     6                       enabled
#define RADIOLIB_AX5X43_PIN_ANTSEL_OUT_TEST                     (0x07 << 0)   //  2     0     ANTSEL: test output
#define RADIOLIB_AX5X43_PIN_ANTSEL_OUT_DIV                      (0x06 << 0)   //  2     0             diversity antenna select output (default)
#define RADIOLIB_AX5X43_PIN_ANTSEL_OUT_DAC                      (0x05 << 0)   //  2     0             DAC output
#define RADIOLIB_AX5X43_PIN_ANTSEL_OUT_EXT_TCXO_EN              (0x04 << 0)   //  2     0             external TCXO enable output
#define RADIOLIB_AX5X43_PIN_ANTSEL_OUT_BB_TUNE_CLK              (0x03 << 0)   //  2     0             baseband tune clock output
#define RADIOLIB_AX5X43_PIN_ANTSEL_OUT_HI_Z                     (0x02 << 0)   //  2     0             high-Z
#define RADIOLIB_AX5X43_PIN_ANTSEL_OUT_HIGH                     (0x01 << 0)   //  2     0             high
#define RADIOLIB_AX5X43_PIN_ANTSEL_OUT_LOW                      (0x00 << 0)   //  2     0             low

// RADIOLIB_AX5X43_REG_PIN_FUNC_PWRAMP
#define RADIOLIB_AX5X43_PIN_PWRAMP_PU_DISABLED                  (0x00 << 7)   //  7     7     PWRAMP pull-up: disabled (default)
#define RADIOLIB_AX5X43_PIN_PWRAMP_PU_ENABLED                   (0x01 << 7)   //  7     7                     enabled
#define RADIOLIB_AX5X43_PIN_PWRAMP_INV_DISABLED                 (0x00 << 6)   //  6     6     PWRAMP inversion: disabled (default)
#define RADIOLIB_AX5X43_PIN_PWRAMP_INV_ENABLED                  (0x01 << 6)   //  6     6                       enabled
#define RADIOLIB_AX5X43_PIN_PWRAMP_OUT_TEST                     (0x0F << 0)   //  3     0     PWRAMP: test output
#define RADIOLIB_AX5X43_PIN_PWRAMP_OUT_EXT_TCXO_EN              (0x07 << 0)   //  3     0             external TCXO enable output
#define RADIOLIB_AX5X43_PIN_PWRAMP_OUT_PA_CTRL                  (0x06 << 0)   //  3     0             power amplifier control output (default)
#define RADIOLIB_AX5X43_PIN_PWRAMP_OUT_DAC                      (0x05 << 0)   //  3     0             DAC output
#define RADIOLIB_AX5X43_PIN_PWRAMP_OUT_DIBIT_SYNC               (0x04 << 0)   //  3     0             4-FSK dibit synchronization output
#define RADIOLIB_AX5X43_PIN_PWRAMP_IN_DIBIT_SYNC                (0x03 << 0)   //  3     0             4-FSK dibit synchronization input
#define RADIOLIB_AX5X43_PIN_PWRAMP_OUT_HI_Z                     (0x02 << 0)   //  2     0             high-Z
#define RADIOLIB_AX5X43_PIN_PWRAMP_OUT_HIGH                     (0x01 << 0)   //  2     0             high
#define RADIOLIB_AX5X43_PIN_PWRAMP_OUT_LOW                      (0x00 << 0)   //  2     0             low

// RADIOLIB_AX5X43_REG_FIFO_STAT
#define RADIOLIB_AX5X43_FIFO_AUTO_COMMIT_ENABLED                (0x01 << 7)   //  7     7     FIFO auto commit on every write: enabled
#define RADIOLIB_AX5X43_FIFO_AUTO_COMMIT_DISABLED               (0x00 << 7)   //  7     7                                      disabled (default)
#define RADIOLIB_AX5X43_FIFO_CMD_ROLLBACK                       (0x05 << 0)   //  5     0     FIFO command: rollback
#define RADIOLIB_AX5X43_FIFO_CMD_COMMIT                         (0x04 << 0)   //  5     0                   commit
#define RADIOLIB_AX5X43_FIFO_CMD_CLEAR_ALL                      (0x03 << 0)   //  5     0                   clear data and flags
#define RADIOLIB_AX5X43_FIFO_CMD_CLEAR_FLAGS                    (0x02 << 0)   //  5     0                   clear flags
#define RADIOLIB_AX5X43_FIFO_CMD_ASK                            (0x01 << 0)   //  5     0                   ASK coherent
#define RADIOLIB_AX5X43_FIFO_CMD_NOP                            (0x00 << 0)   //  5     0                   no operation
#define RADIOLIB_AX5X43_FIFO_FREE_OVER_THR                      (0x01 << 5)   //  5     5     FIFO free level is over threshold
#define RADIOLIB_AX5X43_FIFO_CNT_OVER_THR                       (0x01 << 4)   //  4     4     FIFO count level is over threshold
#define RADIOLIB_AX5X43_FIFO_OVER                               (0x01 << 3)   //  3     3     FIFO overflow occurred
#define RADIOLIB_AX5X43_FIFO_UNDER                              (0x01 << 2)   //  2     2     FIFO underrun occurred
#define RADIOLIB_AX5X43_FIFO_FULL                               (0x01 << 1)   //  1     1     FIFO is full
#define RADIOLIB_AX5X43_FIFO_EMPTY                              (0x00 << 1)   //  1     1     FIFO is empty

// RADIOLIB_AX5X43_REG_PLL_LOOP, RADIOLIB_AX5X43_REG_PLL_LOOP_BOOST
#define RADIOLIB_AX5X43_PLL_FREQSEL_A                           (0x00 << 7)   //  7     7     frequency selection: FREQA (default)
#define RADIOLIB_AX5X43_PLL_FREQSEL_B                           (0x01 << 7)   //  7     7                          FREQB
#define RADIOLIB_AX5X43_PLL_DIRECT_ENABLED                      (0x01 << 3)   //  3     3     bypass external filter: enabled (default)
#define RADIOLIB_AX5X43_PLL_DIRECT_DISABLED                     (0x00 << 3)   //  3     3                             disabled
#define RADIOLIB_AX5X43_PLL_FLT_ENABLED                         (0x01 << 2)   //  2     2     external filter: enabled
#define RADIOLIB_AX5X43_PLL_FLT_DISABLED                        (0x00 << 2)   //  2     2                      disabled (default)
#define RADIOLIB_AX5X43_PLL_FLT_INT_5                           (0x03 << 0)   //  1     0     loop filter: internal x5 (default for PLLBOOST)
#define RADIOLIB_AX5X43_PLL_FLT_INT_2                           (0x02 << 0)   //  1     0                  internal x2
#define RADIOLIB_AX5X43_PLL_FLT_INT_1                           (0x01 << 0)   //  1     0                  internal x1 (default for PLL)
#define RADIOLIB_AX5X43_PLL_FLT_EXT                             (0x00 << 0)   //  1     0                  external

// RADIOLIB_AX5X43_REG_PLL_VCO_DIV
#define RADIOLIB_AX5X43_VCO_VCO2_INT                            (0x03 << 5)   //  5     4     VCO selection: internal VCO2 with external inductor
#define RADIOLIB_AX5X43_VCO_EXT                                 (0x02 << 5)   //  5     4                    external VCO
#define RADIOLIB_AX5X43_VCO_VCO1_INT                            (0x00 << 5)   //  5     4                    internal VCO1 (default)
#define RADIOLIB_AX5X43_RFDIV_ENABLED                           (0x01 << 2)   //  2     2     RF divide by 2: enabled
#define RADIOLIB_AX5X43_RFDIV_DISABLED                          (0x00 << 2)   //  2     2                     disabled (default)

// RADIOLIB_AX5X43_REG_PLL_RANGING_A, RADIOLIB_AX5X43_REG_PLL_RANGING_B
#define RADIOLIB_AX5X43_PLL_STICKY_LOCK                         (0x01 << 7)   //  7     7     PLL remained locked since last time this was read
#define RADIOLIB_AX5X43_PLL_LOCK                                (0x01 << 6)   //  6     6     PLL locked
#define RADIOLIB_AX5X43_PLL_RNG_ERR                             (0x01 << 5)   //  5     5     PLL ranging error
#define RADIOLIB_AX5X43_PLL_RNG_START                           (0x01 << 4)   //  4     4     start PLL ranging
#define RADIOLIB_AX5X43_PLL_VCO_RNG_DEFAULT                     (0x08 << 0)   //  3     0     default VCO range (8 if unknown)

// RADIOLIB_AX5X43_REG_DIVERSITY
#define RADIOLIB_AX5X43_DIVERSITY_ENABLED                       (0x01 << 0)   //  0     0     antenna diversity logic: enabled
#define RADIOLIB_AX5X43_DIVERSITY_DISABLED                      (0x00 << 0)   //  0     0                              disabled (default)

// RADIOLIB_AX5X43_REG_MOD_CFG_F
#define RADIOLIB_AX5X43_FREQ_SHAPE_BT_0_5                       (0x03 << 0)   //  1     0     shaping mode: gaussian BT = 0.5
#define RADIOLIB_AX5X43_FREQ_SHAPE_BT_0_3                       (0x02 << 0)   //  1     0                   gaussian BT = 0.3
#define RADIOLIB_AX5X43_FREQ_SHAPE_EXT                          (0x00 << 0)   //  1     0                   external loop (default)

// RADIOLIB_AX5X43_REG_MOD_CFG_A
#define RADIOLIB_AX5X43_BROWN_GATE_DISABLED                     (0x00 << 7)   //  7     7     disable transmitter on brownout: disabled (default)
#define RADIOLIB_AX5X43_BROWN_GATE_ENABLED                      (0x01 << 7)   //  7     7                                      enabled 
#define RADIOLIB_AX5X43_PLL_GATE_DISABLED                       (0x00 << 6)   //  6     6     disable transmitter on PLL lock lost: disabled (default)
#define RADIOLIB_AX5X43_PLL_GATE_ENABLED                        (0x01 << 6)   //  6     6                                           enabled 
#define RADIOLIB_AX5X43_SLOWRAMP_8_BIT                          (0x03 << 4)   //  5     4     slowramp: 8-bit startup
#define RADIOLIB_AX5X43_SLOWRAMP_4_BIT                          (0x02 << 4)   //  5     4               4-bit startup
#define RADIOLIB_AX5X43_SLOWRAMP_2_BIT                          (0x01 << 4)   //  5     4               2-bit startup
#define RADIOLIB_AX5X43_SLOWRAMP_1_BIT                          (0x00 << 4)   //  5     4               1-bit startup (default)
#define RADIOLIB_AX5X43_AMPLSHAPE_COSINE                        (0x01 << 2)   //  2     2     amplshape: raised cosine (default)
#define RADIOLIB_AX5X43_AMPLSHAPE_UNSHAPED                      (0x00 << 2)   //  2     2                unshaped
#define RADIOLIB_AX5X43_TX_SE                                   (0x02 << 0)   //  1     0     transmitter: single-ended
#define RADIOLIB_AX5X43_TX_DIFF                                 (0x01 << 0)   //  1     0                  differential (default)

// RADIOLIB_AX5X43_REG_PLL_VCO_I
#define RADIOLIB_AX5X43_VCOI_MAN_ENABLED                        (0x01 << 7)   //  7     7     manual VCO bias: enabled
#define RADIOLIB_AX5X43_VCOI_MAN_DISABLED                       (0x00 << 7)   //  7     7                      disabled (default)

// RADIOLIB_AX5X43_REG_PLL_LOCK_DET
#define RADIOLIB_AX5X43_PLL_LOCK_DET_DELAY_MANUAL               (0x01 << 2)   //  2     2     manual lock delay
#define RADIOLIB_AX5X43_PLL_LOCK_DET_DELAY_AUTO                 (0x00 << 2)   //  2     2     automatic lock delay (default)
#define RADIOLIB_AX5X43_PLL_LOCK_DET_DELAY_14_NS                (0x03 << 0)   //  1     0     lock detector delay: 14 ns (default)
#define RADIOLIB_AX5X43_PLL_LOCK_DET_DELAY_12_NS                (0x02 << 0)   //  1     0                          12 ns
#define RADIOLIB_AX5X43_PLL_LOCK_DET_DELAY_9_NS                 (0x01 << 0)   //  1     0                          9 ns
#define RADIOLIB_AX5X43_PLL_LOCK_DET_DELAY_6_NS                 (0x00 << 0)   //  1     0                          6 ns

// RADIOLIB_AX5X43_REG_BB_TUNE
#define RADIOLIB_AX5X43_BB_TUNE_START                           (0x01 << 4)   //  4     4     baseband tune start

// RADIOLIB_AX5X43_REG_PKT_ADDR_CFG
#define RADIOLIB_AX5X43_PKT_MSB_FIRST                           (0x01 << 7)   //  7     7     packet byte order: MSB first
#define RADIOLIB_AX5X43_PKT_LSB_FIRST                           (0x00 << 7)   //  7     7                        LSB first (default)
#define RADIOLIB_AX5X43_PKT_CRC_SKIP_FIRST_ENABLED              (0x01 << 6)   //  6     6     skip first byte in CRC calculation: enabled
#define RADIOLIB_AX5X43_PKT_CRC_SKIP_FIRST_DISABLED             (0x00 << 6)   //  6     6                                         disabled (default)
#define RADIOLIB_AX5X43_PKT_FEC_SYNC_ENABLED                    (0x00 << 5)   //  5     5     FEC sync search: enabled (default)
#define RADIOLIB_AX5X43_PKT_FEC_SYNC_DISABLED                   (0x01 << 5)   //  5     5                      disabled

// RADIOLIB_AX5X43_REG_MATCH_0_LEN, RADIOLIB_AX5X43_REG_MATCH_1_LEN
#define RADIOLIB_AX5X43_MATCH_RAW_ENABLED                       (0x01 << 7)   //  7     7     pattern match on raw bits: enabled
#define RADIOLIB_AX5X43_MATCH_RAW_DISABLED                      (0x00 << 7)   //  7     7                                disabled (default)

// RADIOLIB_AX5X43_REG_PKT_CHUNK_SIZE
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_240                      (0x0D << 0)   //  3     0     maximum packet chunk size: 240 bytes
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_224                      (0x0C << 0)   //  3     0                                224 bytes
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_192                      (0x0B << 0)   //  3     0                                192 bytes
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_160                      (0x0A << 0)   //  3     0                                160 bytes
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_128                      (0x09 << 0)   //  3     0                                128 bytes
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_96                       (0x08 << 0)   //  3     0                                96 bytes
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_64                       (0x07 << 0)   //  3     0                                64 bytes
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_32                       (0x06 << 0)   //  3     0                                32 bytes
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_16                       (0x05 << 0)   //  3     0                                16 bytes
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_8                        (0x04 << 0)   //  3     0                                8 bytes
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_4                        (0x03 << 0)   //  3     0                                4 bytes
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_2                        (0x02 << 0)   //  3     0                                2 bytes
#define RADIOLIB_AX5X43_PKT_CHUNK_SIZE_1                        (0x01 << 0)   //  3     0                                1 byte

// RADIOLIB_AX5X43_REG_PKT_MISC_FLAGS
#define RADIOLIB_AX5X43_WOR_MULTI_PACKET_ENABLED                (0x01 << 4)   //  4     4     continue Rx after wake-on-radio event: enabled
#define RADIOLIB_AX5X43_WOR_MULTI_PACKET_DISABLED               (0x00 << 4)   //  4     4                                            disabled (default)
#define RADIOLIB_AX5X43_AGC_SETTLE_DET_ENABLED                  (0x01 << 3)   //  3     3     terminate AGC when settled: enabled
#define RADIOLIB_AX5X43_AGC_SETTLE_DET_DISABLED                 (0x00 << 3)   //  3     3                                 disabled (default)
#define RADIOLIB_AX5X43_BACKGROUND_RSSI_ENABLED                 (0x01 << 2)   //  2     2     background RSSI calculation: enabled
#define RADIOLIB_AX5X43_BACKGROUND_RSSI_DISABLED                (0x00 << 2)   //  2     2                                  disabled (default)
#define RADIOLIB_AX5X43_AGC_CLK_BIT                             (0x01 << 1)   //  1     1     AGC timeout clock source: bit clock
#define RADIOLIB_AX5X43_AGC_CLK_1_US                            (0x00 << 1)   //  1     1                               1 us (default)
#define RADIOLIB_AX5X43_RSSI_CLK_BIT                            (0x01 << 1)   //  1     1     RSSI timeout clock source: bit clock
#define RADIOLIB_AX5X43_RSSI_CLK_1_US                           (0x00 << 1)   //  1     1                                1 us (default)

// RADIOLIB_AX5X43_REG_PKT_STORE_FLAGS
#define RADIOLIB_AX5X43_STORE_ANT_RSSI_ENABLED                  (0x01 << 6)   //  6     6     store background RSSI at the end of packet: enabled
#define RADIOLIB_AX5X43_STORE_ANT_RSSI_DISABLED                 (0x00 << 6)   //  6     6                                                 disabled (default)
#define RADIOLIB_AX5X43_STORE_CRC_ENABLED                       (0x01 << 5)   //  5     5     store CRC at the end of packet: enabled
#define RADIOLIB_AX5X43_STORE_CRC_DISABLED                      (0x00 << 5)   //  5     5                                     disabled (default)
#define RADIOLIB_AX5X43_STORE_RSSI_ENABLED                      (0x01 << 4)   //  4     4     store RSSI at the end of packet: enabled
#define RADIOLIB_AX5X43_STORE_RSSI_DISABLED                     (0x00 << 4)   //  4     4                                      disabled (default)
#define RADIOLIB_AX5X43_STORE_DATA_RATE_ENABLED                 (0x01 << 3)   //  3     3     store data rate at the end of packet: enabled
#define RADIOLIB_AX5X43_STORE_DATA_RATE_DISABLED                (0x00 << 3)   //  3     3                                           disabled (default)
#define RADIOLIB_AX5X43_STORE_RF_OFFS_ENABLED                   (0x01 << 2)   //  2     2     store RF offset at the end of packet: enabled
#define RADIOLIB_AX5X43_STORE_RF_OFFS_DISABLED                  (0x00 << 2)   //  2     2                                           disabled (default)
#define RADIOLIB_AX5X43_STORE_FREQ_OFFS_ENABLED                 (0x01 << 1)   //  1     1     store frequency offset at the end of packet: enabled
#define RADIOLIB_AX5X43_STORE_FREQ_OFFS_DISABLED                (0x00 << 1)   //  1     1                                                  disabled (default)
#define RADIOLIB_AX5X43_STORE_TIMER_ENABLED                     (0x01 << 0)   //  0     0     store timer when delimiter is detected: enabled
#define RADIOLIB_AX5X43_STORE_TIMER_DISABLED                    (0x00 << 0)   //  0     0                                             disabled (default)

// RADIOLIB_AX5X43_REG_PKT_ACCEPT_FLAGS
#define RADIOLIB_AX5X43_ACCEPT_LARGE_PACKETS_ENABLED            (0x01 << 5)   //  5     5     accept packets spanning multiple chunks: enabled
#define RADIOLIB_AX5X43_ACCEPT_LARGE_PACKETS_DISABLED           (0x00 << 5)   //  5     5                                              disabled (default)
#define RADIOLIB_AX5X43_ACCEPT_OVERSIZE_ENABLED                 (0x01 << 4)   //  4     4     accept oversized packets: enabled
#define RADIOLIB_AX5X43_ACCEPT_OVERSIZE_DISABLED                (0x00 << 4)   //  4     4                               disabled (default)
#define RADIOLIB_AX5X43_ACCEPT_ADDR_MISMATCH_ENABLED            (0x01 << 3)   //  3     3     accept packets with mismatched address: enabled
#define RADIOLIB_AX5X43_ACCEPT_ADDR_MISMATCH_DISABLED           (0x00 << 3)   //  3     3                                             disabled (default)
#define RADIOLIB_AX5X43_ACCEPT_CRC_MISMATCH_ENABLED             (0x01 << 2)   //  2     2     accept packets with mismatched CRC: enabled
#define RADIOLIB_AX5X43_ACCEPT_CRC_MISMATCH_DISABLED            (0x00 << 2)   //  2     2                                         disabled (default)
#define RADIOLIB_AX5X43_ACCEPT_ABORTED_ENABLED                  (0x01 << 1)   //  1     1     accept aborted packets: enabled
#define RADIOLIB_AX5X43_ACCEPT_ABORTED_DISABLED                 (0x00 << 1)   //  1     1                             disabled (default)
#define RADIOLIB_AX5X43_ACCEPT_RESIDUE_ENABLED                  (0x01 << 0)   //  0     0     accept packets with residue: enabled
#define RADIOLIB_AX5X43_ACCEPT_RESIDUE_DISABLED                 (0x00 << 0)   //  0     0                                  disabled (default)

// RADIOLIB_AX5X43_REG_GP_ADC_CTRL
#define RADIOLIB_AX5X43_GP_ADC_BUSY                             (0x01 << 7)   //  7     7     conversion on-going
#define RADIOLIB_AX5X43_GP_ADC_START                            (0x01 << 7)   //  7     7     start conversion
#define RADIOLIB_AX5X43_GP_ADC_1_3_ENABLED                      (0x01 << 2)   //  2     2     sample GPADC1-3: enabled
#define RADIOLIB_AX5X43_GP_ADC_1_3_DISABLED                     (0x00 << 2)   //  2     2                      disabled (default)
#define RADIOLIB_AX5X43_GP_ADC_CONT_ENABLED                     (0x01 << 1)   //  1     1     continuous GPADC sampling: enabled
#define RADIOLIB_AX5X43_GP_ADC_CONT_DISABLED                    (0x00 << 1)   //  1     1                                disabled (default)
#define RADIOLIB_AX5X43_GP_ADC_ISOL_ENABLED                     (0x01 << 0)   //  0     0     GPADC channel isolation: enabled
#define RADIOLIB_AX5X43_GP_ADC_ISOL_DISABLED                    (0x00 << 0)   //  0     0                              disabled (default)

// RADIOLIB_AX5X43_REG_LP_OSC_CONFIG
#define RADIOLIB_AX5X43_LP_OSC_INV_ENABLED                      (0x01 << 7)   //  7     7     LPOSC inversion: enabled
#define RADIOLIB_AX5X43_LP_OSC_INV_DISABLED                     (0x00 << 7)   //  7     7                      disabled (default)
#define RADIOLIB_AX5X43_LP_OSC_DOUBLE_ENABLED                   (0x01 << 6)   //  6     6     LPOSC calibration doubling: enabled
#define RADIOLIB_AX5X43_LP_OSC_DOUBLE_DISABLED                  (0x00 << 6)   //  6     6                                 disabled (default)
#define RADIOLIB_AX5X43_LP_OSC_CAL_RISING_ENABLED               (0x01 << 5)   //  5     5     LPOSC calibration rising edge: enabled
#define RADIOLIB_AX5X43_LP_OSC_CAL_RISING_DISABLED              (0x00 << 5)   //  5     5                                    disabled (default)
#define RADIOLIB_AX5X43_LP_OSC_CAL_FALLING_ENABLED              (0x01 << 4)   //  4     4     LPOSC calibration falling edge: enabled
#define RADIOLIB_AX5X43_LP_OSC_CAL_FALLING_DISABLED             (0x00 << 4)   //  4     4                                     disabled (default)
#define RADIOLIB_AX5X43_LP_OSC_IRQ_RISING_ENABLED               (0x01 << 3)   //  3     3     LPOSC interrupt rising edge: enabled
#define RADIOLIB_AX5X43_LP_OSC_IRQ_RISING_DISABLED              (0x00 << 3)   //  3     3                                  disabled (default)
#define RADIOLIB_AX5X43_LP_OSC_IRQ_FALLING_ENABLED              (0x01 << 2)   //  2     2     LPOSC interrupt falling edge: enabled
#define RADIOLIB_AX5X43_LP_OSC_IRQ_FALLING_DISABLED             (0x00 << 2)   //  2     2                                   disabled (default)
#define RADIOLIB_AX5X43_LP_OSC_FREQ_10240_HZ                    (0x01 << 1)   //  1     1     LPOSC frequency: 10.24 kHz
#define RADIOLIB_AX5X43_LP_OSC_FREQ_640_HZ                      (0x00 << 1)   //  1     1                      640 Hz (default)
#define RADIOLIB_AX5X43_LP_OSC_ENABLED                          (0x01 << 0)   //  0     0     low power oscillator: enabled
#define RADIOLIB_AX5X43_LP_OSC_DISABLED                         (0x00 << 0)   //  0     0                           disabled (default)

// RADIOLIB_AX5X43_REG_DAC_CONFIG
#define RADIOLIB_AX5X43_DAC_MODE_PWM                            (0x01 << 7)   //  7     7     DAC mode: PWM
#define RADIOLIB_AX5X43_DAC_MODE_SIGMA_DELTA                    (0x00 << 7)   //  7     7               sigma-delta (default)
#define RADIOLIB_AX5X43_DAC_CLK_DOUBLE_ENABLED                  (0x01 << 6)   //  6     6     DAC clock doubling: enabled
#define RADIOLIB_AX5X43_DAC_CLK_DOUBLE_DISABLED                 (0x00 << 6)   //  6     6                         disabled (default)
#define RADIOLIB_AX5X43_DAC_INP_GP_ADC                          (0x0C << 0)   //  3     0     DAC input: GPADC
#define RADIOLIB_AX5X43_DAC_INP_SAMPLE_ROT_Q                    (0x09 << 0)   //  3     0                sample Q
#define RADIOLIB_AX5X43_DAC_INP_SAMPLE_ROT_I                    (0x08 << 0)   //  3     0                sample I
#define RADIOLIB_AX5X43_DAC_INP_RSSI                            (0x07 << 0)   //  3     0                RSSI
#define RADIOLIB_AX5X43_DAC_INP_RX_SOFT_DATA                    (0x06 << 0)   //  3     0                Rx soft data
#define RADIOLIB_AX5X43_DAC_INP_AFSK_DEMOD                      (0x05 << 0)   //  3     0                AFSK demodulated
#define RADIOLIB_AX5X43_DAC_INP_FSK_DEMOD                       (0x04 << 0)   //  3     0                FSK demodulated
#define RADIOLIB_AX5X43_DAC_INP_TRK_FREQUENCY                   (0x03 << 0)   //  3     0                tracked frequency
#define RADIOLIB_AX5X43_DAC_INP_TRK_RF_FREQUENCY                (0x02 << 0)   //  3     0                tracked RF frequency
#define RADIOLIB_AX5X43_DAC_INP_TRK_AMPLITUDE                   (0x01 << 0)   //  3     0                tracked amplitude
#define RADIOLIB_AX5X43_DAC_INP_DAC_VAL_REG                     (0x00 << 0)   //  3     0                DAC value registers

/*!
  \class AX5x43

  \brief Base class for AX5x43 series. All derived classes for AX5x43 (e.g. AX5243 or AX5043) inherit from this base class.
  This class should not be instantiated directly from Arduino sketch, only from its derived classes.
*/
class AX5x43: public PhysicalLayer {
  public:
    // introduce PhysicalLayer overloads
    using PhysicalLayer::transmit;
    using PhysicalLayer::receive;
    using PhysicalLayer::startTransmit;
    using PhysicalLayer::readData;

    /*!
      \brief Default constructor.

      \param mod Instance of Module that will be used to communicate with the radio.
    */
    AX5x43(Module* module);

    Module* getMod();

    // basic methods

    int16_t begin(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength);
    int16_t reset();
    int16_t standby();

    // configuration methods

    int16_t setFrequency(float freq);
    int16_t setFrequency(float freq, bool forceRanging);
    int16_t setBitRate(float br);
    int16_t setFrequencyDeviation(float freqDev);
    int16_t setPreambleLength(uint16_t preambleLength);

    int16_t transmit(uint8_t* data, size_t len, uint8_t addr = 0);
    int16_t receive(uint8_t* data, size_t len);
    int16_t startTransmit(uint8_t* data, size_t len, uint8_t addr = 0);
    int16_t finishTransmit();
    int16_t readData(uint8_t* data, size_t len);
    int16_t transmitDirect(uint32_t frf = 0);
    int16_t receiveDirect();
    int16_t setDataShaping(uint8_t sh);
    int16_t setEncoding(uint8_t encoding);
    size_t getPacketLength(bool update = true);
    uint8_t randomByte();
    void setDirectAction(void (*func)(void));
    void readBit(RADIOLIB_PIN_TYPE pin);

#ifndef RADIOLIB_GODMODE
  private:
#endif
    float freq = 0.0;
    float bitRate = 0;
    uint16_t preambleLen = 0;

    Module* mod;

    int16_t getChipVersion();
    int16_t config();
    int16_t setMode(uint8_t mode);
    void writeFifoChunk(uint8_t hdr, uint8_t* data, size_t len);

};

#endif

#endif
