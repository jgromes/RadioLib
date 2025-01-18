#if !defined(_RADIOLIB_LR11X0_H)
#define _RADIOLIB_LR11X0_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR11X0

#include "../../Module.h"

#include "../../protocols/PhysicalLayer/PhysicalLayer.h"

// LR11X0 physical layer properties
#define RADIOLIB_LR11X0_FREQUENCY_STEP_SIZE                     1.0
#define RADIOLIB_LR11X0_MAX_PACKET_LENGTH                       255
#define RADIOLIB_LR11X0_CRYSTAL_FREQ                            32.0
#define RADIOLIB_LR11X0_DIV_EXPONENT                            25

// LR11X0 SPI commands
#define RADIOLIB_LR11X0_CMD_NOP                                 (0x0000)
#define RADIOLIB_LR11X0_CMD_WRITE_REG_MEM                       (0x0105)
#define RADIOLIB_LR11X0_CMD_READ_REG_MEM                        (0x0106)
#define RADIOLIB_LR11X0_CMD_WRITE_BUFFER                        (0x0109)
#define RADIOLIB_LR11X0_CMD_READ_BUFFER                         (0x010A)
#define RADIOLIB_LR11X0_CMD_CLEAR_RX_BUFFER                     (0x010B)
#define RADIOLIB_LR11X0_CMD_WRITE_REG_MEM_MASK                  (0x010C)
#define RADIOLIB_LR11X0_CMD_GET_STATUS                          (0x0100)
#define RADIOLIB_LR11X0_CMD_GET_VERSION                         (0x0101)
#define RADIOLIB_LR11X0_CMD_GET_ERRORS                          (0x010D)
#define RADIOLIB_LR11X0_CMD_CLEAR_ERRORS                        (0x010E)
#define RADIOLIB_LR11X0_CMD_CALIBRATE                           (0x010F)
#define RADIOLIB_LR11X0_CMD_SET_REG_MODE                        (0x0110)
#define RADIOLIB_LR11X0_CMD_CALIB_IMAGE                         (0x0111)
#define RADIOLIB_LR11X0_CMD_SET_DIO_AS_RF_SWITCH                (0x0112)
#define RADIOLIB_LR11X0_CMD_SET_DIO_IRQ_PARAMS                  (0x0113)
#define RADIOLIB_LR11X0_CMD_CLEAR_IRQ                           (0x0114)
#define RADIOLIB_LR11X0_CMD_CONFIG_LF_CLOCK                     (0x0116)
#define RADIOLIB_LR11X0_CMD_SET_TCXO_MODE                       (0x0117)
#define RADIOLIB_LR11X0_CMD_REBOOT                              (0x0118)
#define RADIOLIB_LR11X0_CMD_GET_VBAT                            (0x0119)
#define RADIOLIB_LR11X0_CMD_GET_TEMP                            (0x011A)
#define RADIOLIB_LR11X0_CMD_SET_SLEEP                           (0x011B)
#define RADIOLIB_LR11X0_CMD_SET_STANDBY                         (0x011C)
#define RADIOLIB_LR11X0_CMD_SET_FS                              (0x011D)
#define RADIOLIB_LR11X0_CMD_GET_RANDOM_NUMBER                   (0x0120)
#define RADIOLIB_LR11X0_CMD_ERASE_INFO_PAGE                     (0x0121)
#define RADIOLIB_LR11X0_CMD_WRITE_INFO_PAGE                     (0x0122)
#define RADIOLIB_LR11X0_CMD_READ_INFO_PAGE                      (0x0123)
#define RADIOLIB_LR11X0_CMD_GET_CHIP_EUI                        (0x0125)
#define RADIOLIB_LR11X0_CMD_GET_SEMTECH_JOIN_EUI                (0x0126)
#define RADIOLIB_LR11X0_CMD_DERIVE_ROOT_KEYS_AND_GET_PIN        (0x0127)
#define RADIOLIB_LR11X0_CMD_ENABLE_SPI_CRC                      (0x0128)
#define RADIOLIB_LR11X0_CMD_DRIVE_DIOS_IN_SLEEP_MODE            (0x012A)
#define RADIOLIB_LR11X0_CMD_RESET_STATS                         (0x0200)
#define RADIOLIB_LR11X0_CMD_GET_STATS                           (0x0201)
#define RADIOLIB_LR11X0_CMD_GET_PACKET_TYPE                     (0x0202)
#define RADIOLIB_LR11X0_CMD_GET_RX_BUFFER_STATUS                (0x0203)
#define RADIOLIB_LR11X0_CMD_GET_PACKET_STATUS                   (0x0204)
#define RADIOLIB_LR11X0_CMD_GET_RSSI_INST                       (0x0205)
#define RADIOLIB_LR11X0_CMD_SET_GFSK_SYNC_WORD                  (0x0206)
#define RADIOLIB_LR11X0_CMD_SET_LORA_PUBLIC_NETWORK             (0x0208)
#define RADIOLIB_LR11X0_CMD_SET_RX                              (0x0209)
#define RADIOLIB_LR11X0_CMD_SET_TX                              (0x020A)
#define RADIOLIB_LR11X0_CMD_SET_RF_FREQUENCY                    (0x020B)
#define RADIOLIB_LR11X0_CMD_AUTO_TX_RX                          (0x020C)
#define RADIOLIB_LR11X0_CMD_SET_CAD_PARAMS                      (0x020D)
#define RADIOLIB_LR11X0_CMD_SET_PACKET_TYPE                     (0x020E)
#define RADIOLIB_LR11X0_CMD_SET_MODULATION_PARAMS               (0x020F)
#define RADIOLIB_LR11X0_CMD_SET_PACKET_PARAMS                   (0x0210)
#define RADIOLIB_LR11X0_CMD_SET_TX_PARAMS                       (0x0211)
#define RADIOLIB_LR11X0_CMD_SET_PACKET_ADRS                     (0x0212)
#define RADIOLIB_LR11X0_CMD_SET_RX_TX_FALLBACK_MODE             (0x0213)
#define RADIOLIB_LR11X0_CMD_SET_RX_DUTY_CYCLE                   (0x0214)
#define RADIOLIB_LR11X0_CMD_SET_PA_CONFIG                       (0x0215)
#define RADIOLIB_LR11X0_CMD_STOP_TIMEOUT_ON_PREAMBLE            (0x0217)
#define RADIOLIB_LR11X0_CMD_SET_CAD                             (0x0218)
#define RADIOLIB_LR11X0_CMD_SET_TX_CW                           (0x0219)
#define RADIOLIB_LR11X0_CMD_SET_TX_INFINITE_PREAMBLE            (0x021A)
#define RADIOLIB_LR11X0_CMD_SET_LORA_SYNCH_TIMEOUT              (0x021B)
#define RADIOLIB_LR11X0_CMD_SET_RANGING_ADDR                    (0x021C)
#define RADIOLIB_LR11X0_CMD_SET_RANGING_REQ_ADDR                (0x021D)
#define RADIOLIB_LR11X0_CMD_GET_RANGING_RESULT                  (0x021E)
#define RADIOLIB_LR11X0_CMD_SET_RANGING_TX_RX_DELAY             (0x021F)
#define RADIOLIB_LR11X0_CMD_SET_GFSK_CRC_PARAMS                 (0x0224)
#define RADIOLIB_LR11X0_CMD_SET_GFSK_WHIT_PARAMS                (0x0225)
#define RADIOLIB_LR11X0_CMD_SET_RX_BOOSTED                      (0x0227)
#define RADIOLIB_LR11X0_CMD_SET_RANGING_PARAMETER               (0x0228)
#define RADIOLIB_LR11X0_CMD_SET_RSSI_CALIBRATION                (0x0229)
#define RADIOLIB_LR11X0_CMD_SET_LORA_SYNC_WORD                  (0x022B)
#define RADIOLIB_LR11X0_CMD_LR_FHSS_BUILD_FRAME                 (0x022C)
#define RADIOLIB_LR11X0_CMD_LR_FHSS_SET_SYNC_WORD               (0x022D)
#define RADIOLIB_LR11X0_CMD_CONFIG_BLE_BEACON                   (0x022E)
#define RADIOLIB_LR11X0_CMD_GET_LORA_RX_HEADER_INFOS            (0x0230)
#define RADIOLIB_LR11X0_CMD_BLE_BEACON_SEND                     (0x0231)
#define RADIOLIB_LR11X0_CMD_WIFI_SCAN                           (0x0300)
#define RADIOLIB_LR11X0_CMD_WIFI_SCAN_TIME_LIMIT                (0x0301)
#define RADIOLIB_LR11X0_CMD_WIFI_COUNTRY_CODE                   (0x0302)
#define RADIOLIB_LR11X0_CMD_WIFI_COUNTRY_CODE_TIME_LIMIT        (0x0303)
#define RADIOLIB_LR11X0_CMD_WIFI_GET_NB_RESULTS                 (0x0305)
#define RADIOLIB_LR11X0_CMD_WIFI_READ_RESULTS                   (0x0306)
#define RADIOLIB_LR11X0_CMD_WIFI_RESET_CUMUL_TIMINGS            (0x0307)
#define RADIOLIB_LR11X0_CMD_WIFI_READ_CUMUL_TIMINGS             (0x0308)
#define RADIOLIB_LR11X0_CMD_WIFI_GET_NB_COUNTRY_CODE_RESULTS    (0x0309)
#define RADIOLIB_LR11X0_CMD_WIFI_READ_COUNTRY_CODE_RESULTS      (0x030A)
#define RADIOLIB_LR11X0_CMD_WIFI_CFG_TIMESTAMP_AP_PHONE         (0x030B)
#define RADIOLIB_LR11X0_CMD_WIFI_READ_VERSION                   (0x0320)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_RSSI                      (0x0222)
#define RADIOLIB_LR11X0_CMD_GNSS_SET_CONSTELLATION_TO_USE       (0x0400)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_CONSTELLATION_TO_USE      (0x0401)
#define RADIOLIB_LR11X0_CMD_GNSS_SET_ALMANAC_UPDATE             (0x0402)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_UPDATE            (0x0403)
#define RADIOLIB_LR11X0_CMD_GNSS_SET_FREQ_SEARCH_SPACE          (0x0404)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_FREQ_SEARCH_SPACE         (0x0405)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_VERSION                   (0x0406)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_SUPPORTED_CONSTELLATIONS  (0x0407)
#define RADIOLIB_LR11X0_CMD_GNSS_SET_MODE                       (0x0408)
#define RADIOLIB_LR11X0_CMD_GNSS_AUTONOMOUS                     (0x0409)
#define RADIOLIB_LR11X0_CMD_GNSS_ASSISTED                       (0x040A)
#define RADIOLIB_LR11X0_CMD_GNSS_SCAN                           (0x040B)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_RESULT_SIZE                (0x040C)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_RESULTS                   (0x040D)
#define RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_FULL_UPDATE            (0x040E)
#define RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_READ_ADDR_SIZE         (0x040F)
#define RADIOLIB_LR11X0_CMD_GNSS_SET_ASSISTANCE_POSITION        (0x0410)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_ASSISTANCE_POSITION       (0x0411)
#define RADIOLIB_LR11X0_CMD_GNSS_PUSH_SOLVER_MSG                (0x0414)
#define RADIOLIB_LR11X0_CMD_GNSS_PUSH_DM_MSG                    (0x0415)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_CONTEXT_STATUS             (0x0416)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_NB_SV_DETECTED             (0x0417)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_SV_DETECTED                (0x0418)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_CONSUMPTION                (0x0419)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_PER_SATELLITE     (0x041A)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_SV_VISIBLE                 (0x041F)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_SV_VISIBLE_DOPPLER         (0x0420)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_LAST_SCAN_MODE_LAUNCHED   (0x0426)
#define RADIOLIB_LR11X0_CMD_GNSS_FETCH_TIME                     (0x0432)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_TIME                      (0x0434)
#define RADIOLIB_LR11X0_CMD_GNSS_RESET_TIME                     (0x0435)
#define RADIOLIB_LR11X0_CMD_GNSS_RESET_POSITION                 (0x0437)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_WEEK_NUMBER_ROLLOWER      (0x0438)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_DEMOD_STATUS              (0x0439)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_CUMUL_TIMING              (0x044A)
#define RADIOLIB_LR11X0_CMD_GNSS_SET_TIME                       (0x044B)
#define RADIOLIB_LR11X0_CMD_GNSS_CONFIG_DELAY_RESET_AP          (0x044D)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_DOPPLER_SOLVER_RES        (0x044F)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_DELAY_RESET_AP            (0x0453)
#define RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_UPDATE_FROM_SAT        (0x0454)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_KEEP_SYNC_STATUS          (0x0456)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_STATUS            (0x0457)
#define RADIOLIB_LR11X0_CMD_GNSS_CONFIG_ALMANAC_UPDATE_PERIOD   (0x0463)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_UPDATE_PERIOD     (0x0464)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_SV_WARM_START              (0x0466)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_SV_SYNC                    (0x0466)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_WARM_START_STATUS         (0x0469)
#define RADIOLIB_LR11X0_CMD_GNSS_WRITE_BIT_MASK_SAT_ACTIVATED   (0x0472)
#define RADIOLIB_LR11X0_CMD_CRYPTO_SET_KEY                      (0x0502)
#define RADIOLIB_LR11X0_CMD_CRYPTO_DERIVE_KEY                   (0x0503)
#define RADIOLIB_LR11X0_CMD_CRYPTO_PROCESS_JOIN_ACCEPT          (0x0504)
#define RADIOLIB_LR11X0_CMD_CRYPTO_COMPUTE_AES_CMAC             (0x0505)
#define RADIOLIB_LR11X0_CMD_CRYPTO_VERIFY_AES_CMAC              (0x0506)
#define RADIOLIB_LR11X0_CMD_CRYPTO_AES_ENCRYPT_01               (0x0507)
#define RADIOLIB_LR11X0_CMD_CRYPTO_AES_ENCRYPT                  (0x0508)
#define RADIOLIB_LR11X0_CMD_CRYPTO_AES_DECRYPT                  (0x0509)
#define RADIOLIB_LR11X0_CMD_CRYPTO_STORE_TO_FLASH               (0x050A)
#define RADIOLIB_LR11X0_CMD_CRYPTO_RESTORE_FROM_FLASH           (0x050B)
#define RADIOLIB_LR11X0_CMD_CRYPTO_SET_PARAM                    (0x050D)
#define RADIOLIB_LR11X0_CMD_CRYPTO_GET_PARAM                    (0x050E)
#define RADIOLIB_LR11X0_CMD_CRYPTO_CHECK_ENCRYPTED_FIRMWARE_IMAGE         (0x050F)
#define RADIOLIB_LR11X0_CMD_CRYPTO_CHECK_ENCRYPTED_FIRMWARE_IMAGE_RESULT  (0x0510)
#define RADIOLIB_LR11X0_CMD_BOOT_ERASE_FLASH                    (0x8000)
#define RADIOLIB_LR11X0_CMD_BOOT_WRITE_FLASH_ENCRYPTED          (0x8003)
#define RADIOLIB_LR11X0_CMD_BOOT_REBOOT                         (0x8005)
#define RADIOLIB_LR11X0_CMD_BOOT_GET_PIN                        (0x800B)
#define RADIOLIB_LR11X0_CMD_BOOT_GET_CHIP_EUI                   (0x800C)
#define RADIOLIB_LR11X0_CMD_BOOT_GET_JOIN_EUI                   (0x800D)

// LR11X0 register map
#define RADIOLIB_LR11X0_REG_SF6_SX127X_COMPAT                   (0x00F20414)
#define RADIOLIB_LR11X0_REG_LORA_HIGH_POWER_FIX                 (0x00F30054)
#define RADIOLIB_LR11X0_REG_LNA_MODE                            (0x00F3008C)
// TODO add fix for br 600/1200 bps

// LR11X0 SPI command variables

// RADIOLIB_LR11X0_CMD_GET_STATUS                                                   MSB   LSB   DESCRIPTION
#define RADIOLIB_LR11X0_STAT_1_CMD_FAIL                         (0x00UL << 1)   //  3     1     command status: last command could not be executed
#define RADIOLIB_LR11X0_STAT_1_CMD_PERR                         (0x01UL << 1)   //  3     1                     processing error
#define RADIOLIB_LR11X0_STAT_1_CMD_OK                           (0x02UL << 1)   //  3     1                     successfully processed
#define RADIOLIB_LR11X0_STAT_1_CMD_DAT                          (0x03UL << 1)   //  3     1                     successfully processed, data is being transmitted
#define RADIOLIB_LR11X0_STAT_1_IRQ_INACTIVE                     (0x00UL << 0)   //  0     0     interrupt status: inactive
#define RADIOLIB_LR11X0_STAT_1_IRQ_ACTIVE                       (0x01UL << 0)   //  0     0                       at least 1 interrupt active
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_CLEARED                  (0x00UL << 4)   //  7     4     reset status: cleared
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_ANALOG                   (0x01UL << 4)   //  7     4                   analog (power on, brown-out)
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_EXTERNAL                 (0x02UL << 4)   //  7     4                   NRESET pin
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_SYSTEM                   (0x03UL << 4)   //  7     4                   system
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_WATCHDOG                 (0x04UL << 4)   //  7     4                   watchdog
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_WAKEUP                   (0x05UL << 4)   //  7     4                   NSS toggling wake-up
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_RTC                      (0x06UL << 4)   //  7     4                   realtime clock
#define RADIOLIB_LR11X0_STAT_2_MODE_SLEEP                       (0x00UL << 1)   //  3     1     chip mode: sleep
#define RADIOLIB_LR11X0_STAT_2_MODE_STBY_RC                     (0x01UL << 1)   //  3     1                standby with RC oscillator
#define RADIOLIB_LR11X0_STAT_2_MODE_STBY_OSC                    (0x02UL << 1)   //  3     1                standby with external oscillator
#define RADIOLIB_LR11X0_STAT_2_MODE_FS                          (0x03UL << 1)   //  3     1                frequency synthesis
#define RADIOLIB_LR11X0_STAT_2_MODE_RX                          (0x04UL << 1)   //  3     1                receive
#define RADIOLIB_LR11X0_STAT_2_MODE_TX                          (0x05UL << 1)   //  3     1                transmit
#define RADIOLIB_LR11X0_STAT_2_MODE_WIFI_GNSS                   (0x06UL << 1)   //  3     1                WiFi or GNSS geolocation
#define RADIOLIB_LR11X0_STAT_2_BOOT                             (0x00UL << 0)   //  0     0     code executed from: bootloader
#define RADIOLIB_LR11X0_STAT_2_FLASH                            (0x01UL << 0)   //  0     0                         flash

// RADIOLIB_LR11X0_CMD_WRITE_REG_MEM
#define RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN                  (256)           //  7     0     maximum length of read/write SPI payload in bytes

// RADIOLIB_LR11X0_CMD_GET_VERSION
#define RADIOLIB_LR11X0_DEVICE_LR1110                           (0x01UL << 0)   //  7     0     HW device: LR1110
#define RADIOLIB_LR11X0_DEVICE_LR1120                           (0x02UL << 0)   //  7     0                LR1120
#define RADIOLIB_LR11X0_DEVICE_LR1121                           (0x03UL << 0)   //  7     0                LR1121
#define RADIOLIB_LR11X0_DEVICE_BOOT                             (0xDFUL << 0)   //  7     0                bootloader mode

// RADIOLIB_LR11X0_CMD_GET_ERRORS
#define RADIOLIB_LR11X0_ERROR_STAT_LF_RC_CALIB_ERR              (0x01UL << 0)   //  15    0     error: low frequency RC not calibrated
#define RADIOLIB_LR11X0_ERROR_STAT_HF_RC_CALIB_ERR              (0x01UL << 1)   //  15    0            high frequency RC not calibrated
#define RADIOLIB_LR11X0_ERROR_STAT_ADC_CALIB_ERR                (0x01UL << 2)   //  15    0            ADC not calibrated
#define RADIOLIB_LR11X0_ERROR_STAT_PLL_CALIB_ERR                (0x01UL << 3)   //  15    0            PLL not calibrated
#define RADIOLIB_LR11X0_ERROR_STAT_IMG_CALIB_ERR                (0x01UL << 4)   //  15    0            image rejection not calibrated
#define RADIOLIB_LR11X0_ERROR_STAT_HF_XOSC_START_ERR            (0x01UL << 5)   //  15    0            high frequency oscillator failed to start
#define RADIOLIB_LR11X0_ERROR_STAT_LF_XOSC_START_ERR            (0x01UL << 6)   //  15    0            low frequency oscillator failed to start
#define RADIOLIB_LR11X0_ERROR_STAT_PLL_LOCK_ERR                 (0x01UL << 7)   //  15    0            PLL failed to lock
#define RADIOLIB_LR11X0_ERROR_STAT_RX_ADC_OFFSET_ERR            (0x01UL << 8)   //  15    0            ADC offset not calibrated

// RADIOLIB_LR11X0_CMD_CALIBRATE
#define RADIOLIB_LR11X0_CALIBRATE_PLL_TX                        (0x01UL << 5)   //  5     5     calibrate: Tx PLL
#define RADIOLIB_LR11X0_CALIBRATE_IMG                           (0x01UL << 4)   //  4     4                image rejection
#define RADIOLIB_LR11X0_CALIBRATE_ADC                           (0x01UL << 3)   //  3     3                A/D converter
#define RADIOLIB_LR11X0_CALIBRATE_PLL                           (0x01UL << 2)   //  2     2                PLL
#define RADIOLIB_LR11X0_CALIBRATE_HF_RC                         (0x01UL << 1)   //  1     1                high frequency RC
#define RADIOLIB_LR11X0_CALIBRATE_LF_RC                         (0x01UL << 0)   //  0     0                low frequency RC
#define RADIOLIB_LR11X0_CALIBRATE_ALL                           (0x3FUL << 0)   //  5     0                everything
#define RADIOLIB_LR11X0_CAL_IMG_FREQ_TRIG_MHZ                   (20.0f)

// RADIOLIB_LR11X0_CMD_SET_REG_MODE
#define RADIOLIB_LR11X0_REG_MODE_LDO                            (0x00UL << 0)   //  0     0     regulator mode: LDO in all modes
#define RADIOLIB_LR11X0_REG_MODE_DC_DC                          (0x01UL << 0)   //  0     0                     DC-DC and LDO

// RADIOLIB_LR11X0_CMD_SET_DIO_AS_RF_SWITCH
#define RADIOLIB_LR11X0_RFSW_DIO5_ENABLED                       (0x01UL << 0)   //  4     0     RF switch: DIO5 enabled
#define RADIOLIB_LR11X0_RFSW_DIO5_DISABLED                      (0x00UL << 0)   //  4     0                DIO5 disabled (default)
#define RADIOLIB_LR11X0_RFSW_DIO6_ENABLED                       (0x01UL << 1)   //  4     0     RF switch: DIO6 enabled
#define RADIOLIB_LR11X0_RFSW_DIO6_DISABLED                      (0x00UL << 1)   //  4     0                DIO6 disabled (default)
#define RADIOLIB_LR11X0_RFSW_DIO7_ENABLED                       (0x01UL << 2)   //  4     0     RF switch: DIO7 enabled
#define RADIOLIB_LR11X0_RFSW_DIO7_DISABLED                      (0x00UL << 2)   //  4     0                DIO7 disabled (default)
#define RADIOLIB_LR11X0_RFSW_DIO8_ENABLED                       (0x01UL << 3)   //  4     0     RF switch: DIO8 enabled
#define RADIOLIB_LR11X0_RFSW_DIO8_DISABLED                      (0x00UL << 3)   //  4     0                DIO8 disabled (default)
#define RADIOLIB_LR11X0_RFSW_DIO10_ENABLED                      (0x01UL << 4)   //  4     0     RF switch: DIO10 enabled
#define RADIOLIB_LR11X0_RFSW_DIO10_DISABLED                     (0x00UL << 4)   //  4     0                DIO10 disabled (default)
#define RADIOLIB_LR11X0_DIOx(X)                                 ((X) | RFSWITCH_PIN_FLAG)
#define RADIOLIB_LR11X0_DIOx_VAL(X)                             ((X) & ~RFSWITCH_PIN_FLAG)
#define RADIOLIB_LR11X0_DIO5                                    (RADIOLIB_LR11X0_DIOx(0))
#define RADIOLIB_LR11X0_DIO6                                    (RADIOLIB_LR11X0_DIOx(1))
#define RADIOLIB_LR11X0_DIO7                                    (RADIOLIB_LR11X0_DIOx(2))
#define RADIOLIB_LR11X0_DIO8                                    (RADIOLIB_LR11X0_DIOx(3))
#define RADIOLIB_LR11X0_DIO10                                   (RADIOLIB_LR11X0_DIOx(4))

// RADIOLIB_LR11X0_CMD_SET_DIO_IRQ_PARAMS
#define RADIOLIB_LR11X0_IRQ_TX_DONE                             (0x01UL << 2)   //  31    0     interrupt: packet transmitted
#define RADIOLIB_LR11X0_IRQ_RX_DONE                             (0x01UL << 3)   //  31    0                packet received
#define RADIOLIB_LR11X0_IRQ_PREAMBLE_DETECTED                   (0x01UL << 4)   //  31    0                preamble detected
#define RADIOLIB_LR11X0_IRQ_SYNC_WORD_HEADER_VALID              (0x01UL << 5)   //  31    0                sync word or LoRa header valid
#define RADIOLIB_LR11X0_IRQ_HEADER_ERR                          (0x01UL << 6)   //  31    0                LoRa header CRC error
#define RADIOLIB_LR11X0_IRQ_CRC_ERR                             (0x01UL << 7)   //  31    0                packet CRC error 
#define RADIOLIB_LR11X0_IRQ_CAD_DONE                            (0x01UL << 8)   //  31    0                CAD completed
#define RADIOLIB_LR11X0_IRQ_CAD_DETECTED                        (0x01UL << 9)   //  31    0                CAD detected
#define RADIOLIB_LR11X0_IRQ_TIMEOUT                             (0x01UL << 10)  //  31    0                Rx or Tx timeout
#define RADIOLIB_LR11X0_IRQ_LR_FHSS_HOP                         (0x01UL << 11)  //  31    0                FHSS hop
#define RADIOLIB_LR11X0_IRQ_GNSS_DONE                           (0x01UL << 19)  //  31    0                GNSS scan finished
#define RADIOLIB_LR11X0_IRQ_WIFI_DONE                           (0x01UL << 20)  //  31    0                WiFi scan finished
#define RADIOLIB_LR11X0_IRQ_LBD                                 (0x01UL << 21)  //  31    0                low battery detected
#define RADIOLIB_LR11X0_IRQ_CMD_ERROR                           (0x01UL << 22)  //  31    0                command error
#define RADIOLIB_LR11X0_IRQ_ERROR                               (0x01UL << 23)  //  31    0                some other error than CMD_ERR
#define RADIOLIB_LR11X0_IRQ_FSK_LEN_ERROR                       (0x01UL << 24)  //  31    0                FSK packet received with length error
#define RADIOLIB_LR11X0_IRQ_FSK_ADDR_ERROR                      (0x01UL << 25)  //  31    0                FSK packet received with address error
#define RADIOLIB_LR11X0_IRQ_LORA_RX_TIMESTAMP                   (0x01UL << 27)  //  31    0                last LoRa symbol was received (timestamp source)
#define RADIOLIB_LR11X0_IRQ_GNSS_ABORT                          (0x01UL << 28)  //  31    0                GNSS scan aborted
#define RADIOLIB_LR11X0_IRQ_ALL                                 (0x1BF80FFCUL)  //  31    0                all interrupts
#define RADIOLIB_LR11X0_IRQ_NONE                                (0x00UL << 0)   //  31    0                no interrupts

// RADIOLIB_LR11X0_CMD_CONFIG_LF_CLOCK
#define RADIOLIB_LR11X0_LF_CLK_RC                               (0x00UL << 0)   //  1     0     32.768 kHz source: RC oscillator
#define RADIOLIB_LR11X0_LF_CLK_XOSC                             (0x01UL << 0)   //  1     0                        crystal oscillator
#define RADIOLIB_LR11X0_LF_CLK_EXT                              (0x02UL << 0)   //  1     0                        external signal on DIO11
#define RADIOLIB_LR11X0_LF_BUSY_RELEASE_DISABLED                (0x00UL << 2)   //  2     2
#define RADIOLIB_LR11X0_LF_BUSY_RELEASE_ENABLED                 (0x01UL << 2)   //  2     2

// RADIOLIB_LR11X0_CMD_SET_TCXO_MODE
#define RADIOLIB_LR11X0_TCXO_VOLTAGE_1_6                        (0x00UL << 0)   //  2     0     TCXO supply voltage: 1.6V
#define RADIOLIB_LR11X0_TCXO_VOLTAGE_1_7                        (0x01UL << 0)   //  2     0                          1.7V
#define RADIOLIB_LR11X0_TCXO_VOLTAGE_1_8                        (0x02UL << 0)   //  2     0                          1.8V
#define RADIOLIB_LR11X0_TCXO_VOLTAGE_2_2                        (0x03UL << 0)   //  2     0                          2.2V
#define RADIOLIB_LR11X0_TCXO_VOLTAGE_2_4                        (0x04UL << 0)   //  2     0                          2.4V
#define RADIOLIB_LR11X0_TCXO_VOLTAGE_2_7                        (0x05UL << 0)   //  2     0                          2.7V
#define RADIOLIB_LR11X0_TCXO_VOLTAGE_3_0                        (0x06UL << 0)   //  2     0                          3.0V
#define RADIOLIB_LR11X0_TCXO_VOLTAGE_3_3                        (0x07UL << 0)   //  2     0                          3.3V

// RADIOLIB_LR11X0_CMD_SET_SLEEP
#define RADIOLIB_LR11X0_SLEEP_RETENTION_DISABLED                (0x00UL << 0)   //  0     0     configuration retention in sleep mode: disabled
#define RADIOLIB_LR11X0_SLEEP_RETENTION_ENABLED                 (0x01UL << 0)   //  0     0                                            enabled
#define RADIOLIB_LR11X0_SLEEP_WAKEUP_DISABLED                   (0x00UL << 0)   //  1     1     automated wakeup: disabled
#define RADIOLIB_LR11X0_SLEEP_WAKEUP_ENABLED                    (0x01UL << 0)   //  1     1                       enabled

// RADIOLIB_LR11X0_CMD_SET_STANDBY
#define RADIOLIB_LR11X0_STANDBY_RC                              (0x00UL << 0)   //  7     0     standby mode: RC oscillator
#define RADIOLIB_LR11X0_STANDBY_XOSC                            (0x00UL << 0)   //  7     0                   XTAL/TCXO oscillator

// RADIOLIB_LR11X0_CMD_ERASE_INFO_PAGE
#define RADIOLIB_LR11X0_INFO_PAGE                               (1)

// RADIOLIB_LR11X0_CMD_GET_CHIP_EUI
#define RADIOLIB_LR11X0_EUI_LEN                                 (8)

// RADIOLIB_LR11X0_CMD_DERIVE_ROOT_KEYS_AND_GET_PIN
#define RADIOLIB_LR11X0_PIN_LEN                                 (4)

// RADIOLIB_LR11X0_CMD_GET_PACKET_STATUS
#define RADIOLIB_LR11X0_RX_STATUS_ADDR_ERR                      (0x01UL << 5)   //  7     0     Rx status: address filtering error
#define RADIOLIB_LR11X0_RX_STATUS_CRC_ERR                       (0x01UL << 4)   //  7     0                CRC error
#define RADIOLIB_LR11X0_RX_STATUS_LEN_ERR                       (0x01UL << 3)   //  7     0                length filtering error
#define RADIOLIB_LR11X0_RX_STATUS_ABORTED                       (0x01UL << 2)   //  7     0                packet reception aborted
#define RADIOLIB_LR11X0_RX_STATUS_PACKET_RECEIVED               (0x01UL << 1)   //  7     0                packet received
#define RADIOLIB_LR11X0_RX_STATUS_PACKET_SENT                   (0x01UL << 0)   //  7     0                packet sent

// RADIOLIB_LR11X0_CMD_SET_GFSK_SYNC_WORD
#define RADIOLIB_LR11X0_GFSK_SYNC_WORD_LEN                      (8)

// RADIOLIB_LR11X0_CMD_SET_LORA_PUBLIC_NETWORK
#define RADIOLIB_LR11X0_LORA_PRIVATE_NETWORK                    (0x00UL << 0)   //  7     0     LoRa sync word: private network
#define RADIOLIB_LR11X0_LORA_PUBLIC_NETWORK                     (0x01UL << 0)   //  7     0                     public network

// RADIOLIB_LR11X0_CMD_SET_RX
#define RADIOLIB_LR11X0_RX_TIMEOUT_NONE                         (0x000000UL)    //  23    0     Rx timeout duration: no timeout (Rx single mode)
#define RADIOLIB_LR11X0_RX_TIMEOUT_INF                          (0xFFFFFFUL)    //  23    0                          infinite (Rx continuous mode)

// RADIOLIB_LR11X0_CMD_SET_TX
#define RADIOLIB_LR11X0_TX_TIMEOUT_NONE                         (0x000000UL)    //  23    0     disable Tx timeout

// RADIOLIB_LR11X0_CMD_AUTO_TX_RX
#define RADIOLIB_LR11X0_AUTO_TX_RX_DISABLED                     (0xFFFFFFUL)    //  23    0     disable auto Tx/Rx mode
#define RADIOLIB_LR11X0_AUTO_TX_RX_SKIP_INT                     (0x000000UL)    //  23    0     skip intermediary mode
#define RADIOLIB_LR11X0_AUTO_INTERMEDIARY_MODE_SLEEP            (0x00UL << 0)   //  1     0     intermediary mode: sleep
#define RADIOLIB_LR11X0_AUTO_INTERMEDIARY_MODE_STBY_RC          (0x01UL << 0)   //  1     0                        standby with RC
#define RADIOLIB_LR11X0_AUTO_INTERMEDIARY_MODE_STBY_XOSC        (0x02UL << 0)   //  1     0                        standby with XOSC
#define RADIOLIB_LR11X0_AUTO_INTERMEDIARY_MODE_FS               (0x03UL << 0)   //  1     0                        frequency synthesis
#define RADIOLIB_LR11X0_AUTO_TX_RX_TIMEOUT_DISABLED             (0x000000UL)    //  23    0     disable timeout of the second mode

// RADIOLIB_LR11X0_CMD_SET_CAD_PARAMS
#define RADIOLIB_LR11X0_CAD_EXIT_MODE_STBY_RC                   (0x00UL << 0)   //  7     0     mode to set after CAD: standby with RC
#define RADIOLIB_LR11X0_CAD_EXIT_MODE_RX                        (0x01UL << 0)   //  7     0                            receive if activity detected
#define RADIOLIB_LR11X0_CAD_EXIT_MODE_LBT                       (0x10UL << 0)   //  7     0                            transmit if no activity detected
#define RADIOLIB_LR11X0_CAD_PARAM_DEFAULT                       (0xFFUL << 0)   //  7     0     used by the CAD methods to specify default parameter value

// RADIOLIB_LR11X0_CMD_SET_PACKET_TYPE
#define RADIOLIB_LR11X0_PACKET_TYPE_NONE                        (0x00UL << 0)   //  2     0     packet type: none
#define RADIOLIB_LR11X0_PACKET_TYPE_GFSK                        (0x01UL << 0)   //  2     0                  (G)FSK
#define RADIOLIB_LR11X0_PACKET_TYPE_LORA                        (0x02UL << 0)   //  2     0                  LoRa
#define RADIOLIB_LR11X0_PACKET_TYPE_SIGFOX                      (0x03UL << 0)   //  2     0                  Sigfox
#define RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS                     (0x04UL << 0)   //  2     0                  GMSK/LR-FHSS
#define RADIOLIB_LR11X0_PACKET_TYPE_RANGING                     (0x05UL << 0)   //  2     0                  ranging
#define RADIOLIB_LR11X0_PACKET_TYPE_BLE                         (0x06UL << 0)   //  2     0                  BLE beacon

// RADIOLIB_LR11X0_CMD_SET_MODULATION_PARAMS
#define RADIOLIB_LR11X0_LORA_BW_62_5                            (0x03UL << 0)   //  7     0     LoRa bandwidth: 62.5 kHz
#define RADIOLIB_LR11X0_LORA_BW_125_0                           (0x04UL << 0)   //  7     0                     125.0 kHz
#define RADIOLIB_LR11X0_LORA_BW_250_0                           (0x05UL << 0)   //  7     0                     250.0 kHz
#define RADIOLIB_LR11X0_LORA_BW_500_0                           (0x06UL << 0)   //  7     0                     500.0 kHz
#define RADIOLIB_LR11X0_LORA_BW_203_125                         (0x0DUL << 0)   //  7     0                     203.0 kHz (2.4GHz only)
#define RADIOLIB_LR11X0_LORA_BW_406_25                          (0x0EUL << 0)   //  7     0                     406.0 kHz (2.4GHz only)
#define RADIOLIB_LR11X0_LORA_BW_812_50                          (0x0FUL << 0)   //  7     0                     812.0 kHz (2.4GHz only)
#define RADIOLIB_LR11X0_LORA_CR_4_5_SHORT                       (0x01UL << 0)   //  7     0     coding rate: 4/5 with short interleaver
#define RADIOLIB_LR11X0_LORA_CR_4_6_SHORT                       (0x02UL << 0)   //  7     0                  4/6 with short interleaver
#define RADIOLIB_LR11X0_LORA_CR_4_7_SHORT                       (0x03UL << 0)   //  7     0                  4/7 with short interleaver
#define RADIOLIB_LR11X0_LORA_CR_4_8_SHORT                       (0x04UL << 0)   //  7     0                  4/8 with short interleaver
#define RADIOLIB_LR11X0_LORA_CR_4_5_LONG                        (0x05UL << 0)   //  7     0                  4/5 with long interleaver
#define RADIOLIB_LR11X0_LORA_CR_4_6_LONG                        (0x06UL << 0)   //  7     0                  4/6 with long interleaver
#define RADIOLIB_LR11X0_LORA_CR_4_8_LONG                        (0x07UL << 0)   //  7     0                  4/8 with long interleaver
#define RADIOLIB_LR11X0_LORA_LDRO_DISABLED                      (0x00UL << 0)   //  7     0     low data rate optimize: disabled
#define RADIOLIB_LR11X0_LORA_LDRO_ENABLED                       (0x01UL << 0)   //  7     0                             enabled
#define RADIOLIB_LR11X0_GFSK_BIT_RATE_DIV_DISABLED              (0x00UL << 31)  //  31    0     divide bit rate value by 256: disabled
#define RADIOLIB_LR11X0_GFSK_BIT_RATE_DIV_ENABLED               (0x01UL << 31)  //  31    0                                   enabled
#define RADIOLIB_LR11X0_GFSK_SHAPING_NONE                       (0x00UL << 0)   //  7     0     shaping filter: none
#define RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_0_3            (0x08UL << 0)   //  7     0                     Gaussian, BT = 0.3
#define RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_0_5            (0x09UL << 0)   //  7     0                     Gaussian, BT = 0.5
#define RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_0_7            (0x0AUL << 0)   //  7     0                     Gaussian, BT = 0.7
#define RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_1_0            (0x0BUL << 0)   //  7     0                     Gaussian, BT = 1.0
#define RADIOLIB_LR11X0_GFSK_SHAPING_RAISED_COSINE_BT_0_7       (0x16UL << 0)   //  7     0                     raised cosine, BT = 0.7
#define RADIOLIB_LR11X0_GFSK_RX_BW_4_8                          (0x1FUL << 0)   //  7     0     GFSK Rx bandwidth: 4.8 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_5_8                          (0x17UL << 0)   //  7     0                        5.8 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_7_3                          (0x0FUL << 0)   //  7     0                        7.3 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_9_7                          (0x1EUL << 0)   //  7     0                        9.7 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_11_7                         (0x16UL << 0)   //  7     0                        11.7 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_14_6                         (0x0EUL << 0)   //  7     0                        14.6 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_19_5                         (0x1DUL << 0)   //  7     0                        19.5 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_23_4                         (0x15UL << 0)   //  7     0                        23.4 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_29_3                         (0x0DUL << 0)   //  7     0                        29.3 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_39_0                         (0x1CUL << 0)   //  7     0                        39.0 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_46_9                         (0x14UL << 0)   //  7     0                        46.9 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_58_6                         (0x0CUL << 0)   //  7     0                        58.6 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_78_2                         (0x1BUL << 0)   //  7     0                        78.2 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_93_8                         (0x13UL << 0)   //  7     0                        93.8 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_117_3                        (0x0BUL << 0)   //  7     0                        117.3 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_156_2                        (0x1AUL << 0)   //  7     0                        156.2 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_187_2                        (0x12UL << 0)   //  7     0                        187.2 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_234_3                        (0x0AUL << 0)   //  7     0                        234.3 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_312_0                        (0x19UL << 0)   //  7     0                        312.0 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_373_6                        (0x11UL << 0)   //  7     0                        373.6 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_467_0                        (0x09UL << 0)   //  7     0                        467.0 kHz
#define RADIOLIB_LR11X0_LR_FHSS_BIT_RATE                        (488.28215f)    //  31    0     LR FHSS bit rate: 488.28215 bps
#define RADIOLIB_LR11X0_LR_FHSS_BIT_RATE_RAW                    (0x8001E848UL)  //  31    0                       488.28215 bps in raw
#define RADIOLIB_LR11X0_LR_FHSS_SHAPING_GAUSSIAN_BT_1_0         (0x0BUL << 0)   //  7     0     shaping filter: Gaussian, BT = 1.0
#define RADIOLIB_LR11X0_SIGFOX_SHAPING_GAUSSIAN_BT_0_7          (0x16UL << 0)   //  7     0     shaping filter: Gaussian, BT = 0.7

// RADIOLIB_LR11X0_CMD_SET_PACKET_PARAMS
#define RADIOLIB_LR11X0_LORA_HEADER_EXPLICIT                    (0x00UL << 0)   //  7     0     LoRa header mode: explicit
#define RADIOLIB_LR11X0_LORA_HEADER_IMPLICIT                    (0x01UL << 0)   //  7     0                       implicit
#define RADIOLIB_LR11X0_LORA_PAYLOAD_LEN_ANY                    (0x00UL << 0)   //  7     0     accept any payload length
#define RADIOLIB_LR11X0_LORA_CRC_ENABLED                        (0x01UL << 0)   //  7     0     CRC: enabled
#define RADIOLIB_LR11X0_LORA_CRC_DISABLED                       (0x00UL << 0)   //  7     0          disabled
#define RADIOLIB_LR11X0_LORA_IQ_STANDARD                        (0x00UL << 0)   //  7     0     IQ setup: standard
#define RADIOLIB_LR11X0_LORA_IQ_INVERTED                        (0x01UL << 0)   //  7     0               inverted
#define RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_DISABLED           (0x00UL << 0)   //  7     0     preamble detector: disabled
#define RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_8_BITS             (0x04UL << 0)   //  7     0                        8 bits
#define RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_16_BITS            (0x05UL << 0)   //  7     0                        16 bits
#define RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_24_BITS            (0x06UL << 0)   //  7     0                        24 bits
#define RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_32_BITS            (0x07UL << 0)   //  7     0                        32 bits
#define RADIOLIB_LR11X0_GFSK_ADDR_FILTER_DISABLED               (0x00UL << 0)   //  7     0     address filtering: disabled
#define RADIOLIB_LR11X0_GFSK_ADDR_FILTER_NODE                   (0x01UL << 0)   //  7     0                        node address
#define RADIOLIB_LR11X0_GFSK_ADDR_FILTER_NODE_BROADCAST         (0x02UL << 0)   //  7     0                        node and broadcast address
#define RADIOLIB_LR11X0_GFSK_PACKET_LENGTH_FIXED                (0x00UL << 0)   //  7     0     packet length: fixed
#define RADIOLIB_LR11X0_GFSK_PACKET_LENGTH_VARIABLE             (0x01UL << 0)   //  7     0                    variable
#define RADIOLIB_LR11X0_GFSK_PACKET_LENGTH_VARIABLE_SX128X      (0x02UL << 0)   //  7     0                    variable, SX128x 9-bit length encoding
#define RADIOLIB_LR11X0_GFSK_PAYLOAD_LEN_ANY                    (0x00UL << 0)   //  7     0     accept any payload length
#define RADIOLIB_LR11X0_GFSK_CRC_DISABLED                       (0x01UL << 0)   //  7     0     CRC: disabled
#define RADIOLIB_LR11X0_GFSK_CRC_1_BYTE                         (0x00UL << 0)   //  7     0          1-byte
#define RADIOLIB_LR11X0_GFSK_CRC_2_BYTE                         (0x02UL << 0)   //  7     0          2-byte
#define RADIOLIB_LR11X0_GFSK_CRC_1_BYTE_INV                     (0x04UL << 0)   //  7     0          1-byte, inverted
#define RADIOLIB_LR11X0_GFSK_CRC_2_BYTE_INV                     (0x06UL << 0)   //  7     0          2-byte, inverted
#define RADIOLIB_LR11X0_GFSK_WHITENING_DISABLED                 (0x00UL << 0)   //  7     0     whitening: disabled
#define RADIOLIB_LR11X0_GFSK_WHITENING_ENABLED                  (0x01UL << 0)   //  7     0                enabled

// RADIOLIB_LR11X0_CMD_SET_TX_PARAMS
#define RADIOLIB_LR11X0_PA_RAMP_48U                             (0x02UL << 0)   //  7     0     PA ramp time: 48 us

// RADIOLIB_LR11X0_CMD_SET_RX_TX_FALLBACK_MODE
#define RADIOLIB_LR11X0_FALLBACK_MODE_STBY_RC                   (0x01UL << 0)   //  1     0     fallback mode after Rx/Tx: standby with RC
#define RADIOLIB_LR11X0_FALLBACK_MODE_STBY_XOSC                 (0x02UL << 0)   //  1     0                                standby with XOSC
#define RADIOLIB_LR11X0_FALLBACK_MODE_FS                        (0x03UL << 0)   //  1     0                                frequency synthesis

// RADIOLIB_LR11X0_CMD_SET_RX_DUTY_CYCLE
#define RADIOLIB_LR11X0_RX_DUTY_CYCLE_MODE_RX                   (0x00UL << 0)   //  0     0     mode in Rx windows: Rx (default)
#define RADIOLIB_LR11X0_RX_DUTY_CYCLE_MODE_CAD                  (0x01UL << 0)   //  0     0                         CAD
#define RADIOLIB_LR11X0_TIMING_STEP                             (1.0f/32768.0f) //  23    0     timing step fo delays

// RADIOLIB_LR11X0_CMD_SET_PA_CONFIG
#define RADIOLIB_LR11X0_PA_SEL_LP                               (0x00UL << 0)   //  7     0     PA select: low power PA
#define RADIOLIB_LR11X0_PA_SEL_HP                               (0x01UL << 0)   //  7     0                high power PA
#define RADIOLIB_LR11X0_PA_SEL_HF                               (0x02UL << 0)   //  7     0                high frequency PA
#define RADIOLIB_LR11X0_PA_SUPPLY_INTERNAL                      (0x00UL << 0)   //  7     0     PA power source: internal
#define RADIOLIB_LR11X0_PA_SUPPLY_VBAT                          (0x01UL << 0)   //  7     0                      VBAT (required for >= 14 dBm)

// RADIOLIB_LR11X0_CMD_STOP_TIMEOUT_ON_PREAMBLE
#define RADIOLIB_LR11X0_STOP_ON_SYNC_HEADER                     (0x00UL << 0)   //  0     0     stop timeout on: sync word or header (default)
#define RADIOLIB_LR11X0_STOP_ON_PREAMBLE                        (0x01UL << 0)   //  0     0                      preamble

// RADIOLIB_LR11X0_CMD_GET_RANGING_RESULT
#define RADIOLIB_LR11X0_RANGING_RESULT_DISTANCE                 (0)             //  7     0     ranging result type: distance
#define RADIOLIB_LR11X0_RANGING_RESULT_RSSI                     (1)             //  7     0                          RSSI

// RADIOLIB_LR11X0_CMD_SET_RX_BOOSTED
#define RADIOLIB_LR11X0_RX_BOOSTED_ENABLED                      (0x01UL << 0)   //  0     0     Rx boosted mode: enabled
#define RADIOLIB_LR11X0_RX_BOOSTED_DISABLED                     (0x00UL << 0)   //  0     0                      disabled

// RADIOLIB_LR11X0_CMD_SET_LORA_SYNC_WORD
#define RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE                  (0x12)
#define RADIOLIB_LR11X0_LORA_SYNC_WORD_PUBLIC                   (0x34)

// RADIOLIB_LR11X0_CMD_LR_FHSS_BUILD_FRAME
#define RADIOLIB_LR11X0_LR_FHSS_CR_5_6                          (0x00UL << 0)   //  7     0     LR FHSS coding rate: 5/6
#define RADIOLIB_LR11X0_LR_FHSS_CR_2_3                          (0x01UL << 0)   //  7     0                          2/3
#define RADIOLIB_LR11X0_LR_FHSS_CR_1_2                          (0x02UL << 0)   //  7     0                          1/2
#define RADIOLIB_LR11X0_LR_FHSS_CR_1_3                          (0x03UL << 0)   //  7     0                          1/3
#define RADIOLIB_LR11X0_LR_FHSS_MOD_TYPE_GMSK                   (0x00UL << 0)   //  7     0     LR FHSS modulation: GMSK
#define RADIOLIB_LR11X0_LR_FHSS_GRID_STEP_FCC                   (0x00UL << 0)   //  7     0     LR FHSS step size: 25.390625 kHz (FCC)
#define RADIOLIB_LR11X0_LR_FHSS_GRID_STEP_NON_FCC               (0x01UL << 0)   //  7     0                        3.90625 kHz (non-FCC)
#define RADIOLIB_LR11X0_LR_FHSS_HOPPING_DISABLED                (0x00UL << 0)   //  7     0     LR FHSS hopping: disabled
#define RADIOLIB_LR11X0_LR_FHSS_HOPPING_ENABLED                 (0x01UL << 0)   //  7     0                      enabled
#define RADIOLIB_LR11X0_LR_FHSS_BW_39_06                        (0x00UL << 0)   //  7     0     LR FHSS bandwidth: 39.06 kHz
#define RADIOLIB_LR11X0_LR_FHSS_BW_85_94                        (0x01UL << 0)   //  7     0                        85.94 kHz
#define RADIOLIB_LR11X0_LR_FHSS_BW_136_72                       (0x02UL << 0)   //  7     0                        136.72 kHz
#define RADIOLIB_LR11X0_LR_FHSS_BW_183_59                       (0x03UL << 0)   //  7     0                        183.59 kHz
#define RADIOLIB_LR11X0_LR_FHSS_BW_335_94                       (0x04UL << 0)   //  7     0                        335.94 kHz
#define RADIOLIB_LR11X0_LR_FHSS_BW_386_72                       (0x05UL << 0)   //  7     0                        386.72 kHz
#define RADIOLIB_LR11X0_LR_FHSS_BW_722_66                       (0x06UL << 0)   //  7     0                        722.66 kHz
#define RADIOLIB_LR11X0_LR_FHSS_BW_773_44                       (0x07UL << 0)   //  7     0                        773.44 kHz
#define RADIOLIB_LR11X0_LR_FHSS_BW_1523_4                       (0x08UL << 0)   //  7     0                        1523.4 kHz
#define RADIOLIB_LR11X0_LR_FHSS_BW_1574_2                       (0x09UL << 0)   //  7     0                        1574.2 kHz
#define RADIOLIB_LR11X0_LR_FHSS_HEADER_BITS                     (114)           //  7     0     LR FHSS packet bit widths: header
#define RADIOLIB_LR11X0_LR_FHSS_FRAG_BITS                       (48)            //  7     0                                payload fragment
#define RADIOLIB_LR11X0_LR_FHSS_BLOCK_PREAMBLE_BITS             (2)             //  7     0                                block preamble
#define RADIOLIB_LR11X0_LR_FHSS_BLOCK_BITS                      (RADIOLIB_LR11X0_LR_FHSS_FRAG_BITS + RADIOLIB_LR11X0_LR_FHSS_BLOCK_PREAMBLE_BITS)

// RADIOLIB_LR11X0_CMD_GET_LORA_RX_HEADER_INFOS
#define RADIOLIB_LR11X0_LAST_HEADER_CRC_ENABLED                 (0x01UL << 4)   //  4     4     last header CRC: enabled
#define RADIOLIB_LR11X0_LAST_HEADER_CRC_DISABLED                (0x00UL << 4)   //  4     4                      disabled

// RADIOLIB_LR11X0_CMD_WIFI_SCAN
#define RADIOLIB_LR11X0_WIFI_SCAN_802_11_B                      (0x01UL << 0)   //  7     0     Wi-Fi type to scan: 802.11b
#define RADIOLIB_LR11X0_WIFI_SCAN_802_11_G                      (0x02UL << 0)   //  7     0                         802.11g
#define RADIOLIB_LR11X0_WIFI_SCAN_802_11_N                      (0x03UL << 0)   //  7     0                         802.11n
#define RADIOLIB_LR11X0_WIFI_SCAN_ALL                           (0x04UL << 0)   //  7     0                         all (802.11b first)
#define RADIOLIB_LR11X0_WIFI_ACQ_MODE_BEACON_ONLY               (0x01UL << 0)   //  7     0     Wi-Fi acquisition mode: beacon only
#define RADIOLIB_LR11X0_WIFI_ACQ_MODE_BEACON_PACKET             (0x02UL << 0)   //  7     0                             beacon and packet
#define RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_TRAFFIC              (0x03UL << 0)   //  7     0                             full traffic
#define RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON               (0x04UL << 0)   //  7     0                             full beacon
#define RADIOLIB_LR11X0_WIFI_ACQ_MODE_SSID_BEACON               (0x05UL << 0)   //  7     0                             SSID beacon
#define RADIOLIB_LR11X0_WIFI_ABORT_ON_TIMEOUT_ENABLED           (0x01UL << 0)   //  7     0     abort scanning on preamble timeout: enabled
#define RADIOLIB_LR11X0_WIFI_ABORT_ON_TIMEOUT_DISABLED          (0x00UL << 0)   //  7     0                                         disabled
#define RADIOLIB_LR11X0_WIFI_MAX_NUM_RESULTS                    (32)            //  7     0     maximum possible number of Wi-Fi scan results
#define RADIOLIB_LR11X0_WIFI_ALL_CHANNELS                       (0x3FFFUL)      //  16    0     scan all channels

// RADIOLIB_LR11X0_CMD_WIFI_READ_RESULTS
#define RADIOLIB_LR11X0_WIFI_RESULT_TYPE_COMPLETE               (0x01UL << 0)   //  7     0     Wi-Fi scan result type: complete
#define RADIOLIB_LR11X0_WIFI_RESULT_TYPE_BASIC                  (0x04UL << 0)   //  7     0                             basic
#define RADIOLIB_LR11X0_WIFI_RESULT_MAX_LEN                     (79)            //  7     0     maximum possible Wi-Fi scan size
#define RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN                     (6)             //  7     0     MAC address length in bytes
#define RADIOLIB_LR11X0_WIFI_RESULT_SSID_LEN                    (32)            //  7     0     SSID length in bytes

// RADIOLIB_LR11X0_CMD_GNSS_SET_CONSTELLATION_TO_USE
#define RADIOLIB_LR11X0_GNSS_CONSTELLATION_GPS                  (0x01UL << 0)   //  7     0     GNSS constellation to use: GPS
#define RADIOLIB_LR11X0_GNSS_CONSTELLATION_BEIDOU               (0x01UL << 1)   //  7     0                                BeiDou

// RADIOLIB_LR11X0_CMD_GNSS_SET_MODE
#define RADIOLIB_LR11X0_GNSS_MODE_SINGLE_SCAN                   (0x00UL << 0)   //  7     0     GNSS scanning mode: single/legacy
#define RADIOLIB_LR11X0_GNSS_MODE_SINGLE_MULTIPLE               (0x03UL << 1)   //  7     0                         multiple/advanced

// RADIOLIB_LR11X0_CMD_GNSS_AUTONOMOUS
#define RADIOLIB_LR11X0_GNSS_RES_PSEUDO_DOPPLER_ENABLED         (0x01UL << 0)   //  0     0     GNSS results in NAV message: pseudo-range (in single scan mode) or Doppler information (in multiple scan mode)
#define RADIOLIB_LR11X0_GNSS_RES_PSEUDO_DOPPLER_DISABLED        (0x00UL << 0)   //  0     0                                  not included
#define RADIOLIB_LR11X0_GNSS_RES_DOPPLER_ENABLED                (0x01UL << 1)   //  1     1                                  Doppler information
#define RADIOLIB_LR11X0_GNSS_RES_DOPPLER_DISABLED               (0x00UL << 1)   //  1     1                                  not included
#define RADIOLIB_LR11X0_GNSS_NB_SV_ALL                          (0x00UL << 0)   //  7     0     include all detected satellites
#define RADIOLIB_LR11X0_GNSS_AUTO_EFFORT_MODE                   (0x00UL << 0)   //  7     0     reserved, always 0

// RADIOLIB_LR11X0_CMD_GNSS_ASSISTED
#define RADIOLIB_LR11X0_GNSS_ASSIST_LOW_POWER                   (0x00UL << 0)   //  7     0     effort mode: low power
#define RADIOLIB_LR11X0_GNSS_ASSIST_BEST_EFFORT                 (0x01UL << 0)   //  7     0                  best effort

// RADIOLIB_LR11X0_CMD_GNSS_GET_CONTEXT_STATUS
#define RADIOLIB_LR11X0_GNSS_CONTEXT_ERR_NONE                   (0x00UL << 0)   //  7     4     error code: none
#define RADIOLIB_LR11X0_GNSS_CONTEXT_ERR_ALMANAC_OLD            (0x01UL << 0)   //  7     4                 almanac too old
#define RADIOLIB_LR11X0_GNSS_CONTEXT_ERR_ALMANAC_CRC            (0x02UL << 0)   //  7     4                 almanac CRC mismatch
#define RADIOLIB_LR11X0_GNSS_CONTEXT_ERR_FLASH                  (0x03UL << 0)   //  7     4                 flash integrity error
#define RADIOLIB_LR11X0_GNSS_CONTEXT_ERR_ALMANAC_UPD            (0x04UL << 0)   //  7     4                 almanac update not allowed
#define RADIOLIB_LR11X0_GNSS_CONTEXT_FREQ_SPACE_250_HZ          (0x00UL << 0)   //  8     7     frequency search space: 250 Hz
#define RADIOLIB_LR11X0_GNSS_CONTEXT_FREQ_SPACE_500_HZ          (0x01UL << 0)   //  8     7                             500 Hz
#define RADIOLIB_LR11X0_GNSS_CONTEXT_FREQ_SPACE_1000_HZ         (0x02UL << 0)   //  8     7                             1000 Hz
#define RADIOLIB_LR11X0_GNSS_CONTEXT_FREQ_SPACE_2000_HZ         (0x03UL << 0)   //  8     7                             2000 Hz

// RADIOLIB_LR11X0_CMD_GNSS_GET_SV_VISIBLE
#define RADIOLIB_LR11X0_SV_CONSTELLATION_GPS                    (0x00UL << 0)   //  7     0     GNSS constellation: GPS
#define RADIOLIB_LR11X0_SV_CONSTELLATION_BEIDOU                 (0x01UL << 0)   //  7     0                         BeiDou

// RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_FULL_UPDATE
#define RADIOLIB_LR11X0_GNSS_ALMANAC_HEADER_ID                  (0x80UL << 0)   //  7     0     starting byte of GNSS almanac header
#define RADIOLIB_LR11X0_GNSS_ALMANAC_BLOCK_SIZE                 (20)

// RADIOLIB_LR11X0_CMD_GNSS_FETCH_TIME
#define RADIOLIB_LR11X0_GNSS_EFFORT_LOW                         (0x00UL << 0)   //  7     0     GNSS effort mode: low sensitivity
#define RADIOLIB_LR11X0_GNSS_EFFORT_MID                         (0x01UL << 0)   //  7     0                        medium sensitivity
#define RADIOLIB_LR11X0_GNSS_FETCH_TIME_OPT_TOW                 (0x00UL << 0)   //  7     0     time fetch options: ToW only, requires WN to demodulated beforehand
#define RADIOLIB_LR11X0_GNSS_FETCH_TIME_OPT_TOW_WN              (0x01UL << 0)   //  7     0                         ToW and WN
#define RADIOLIB_LR11X0_GNSS_FETCH_TIME_OPT_TOW_WN_ROLL         (0x02UL << 0)   //  7     0                         ToW, WN and rollover

// RADIOLIB_LR11X0_CMD_GNSS_READ_DEMOD_STATUS
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_NOT_POSSIBLE          (-21)           //  7     0     GNSS demodulation status: not possible to demodulate
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_SAT_LOST              (-20)           //  7     0                               satellite lost
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_ALMANAC_DEMOD_ERROR   (-19)           //  7     0                               almanac demodulation error
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_TOO_LATE              (-18)           //  7     0                               woke up after preamble (demodulation started too late)
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_20_MS_FAIL            (-17)           //  7     0                               20ms real-time clock failed
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_WAKE_UP_FAIL          (-16)           //  7     0                               wake up sync failed
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_WN_INVALID            (-15)           //  7     0                               week number not validated
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_NO_ACTIVE_SAT         (-14)           //  7     0                               no active satellite selected in satellite list
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_SLEEP_TOO_LONG        (-13)           //  7     0                               sleep time too long
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_TOW_INVALID           (-12)           //  7     0                               wrong time-of-week demodulated
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_PREAMBLE_INVALID      (-11)           //  7     0                               preamble not validated
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_DISABLED              (-10)           //  7     0                               demodulator disabled
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_EXTR_FAILED           (-9)            //  7     0                               demodulator extraction failed
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_NO_BIT_CHANGE         (-8)            //  7     0                               no bit change found during demodulation start
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_NO_BIT_CHANGE_ADV     (-7)            //  7     0                               no bit change found during advanced scan
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_NO_SAT_FOUND          (-6)            //  7     0                               no satellites found
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_SYNC_LOST             (-5)            //  7     0                               word sync lost
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_PARITY_NOT_ENOUGH     (-3)            //  7     0                               parity check fail (not enough)
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_PARITY_TOO_MANY       (-2)            //  7     0                               parity check fail (too many)
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_NO_PARITY             (-1)            //  7     0                               parity check fail (no parity found)
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_WORD_SYNC_NONE        (0)             //  7     0                               word sync search not started
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_WORD_SYNC_POT         (1)             //  7     0                               potential word sync found
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_WORD_SYNC_OK          (2)             //  7     0                               word sync found
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_TOW_FOUND             (3)             //  7     0                               time-of-week found
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_WN_FOUND              (4)             //  7     0                               week number and time-of-week found
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_ALM_FOUND_UNSAVED     (5)             //  7     0                               almanac found but not saved
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_HALF_ALM_SAVED        (6)             //  7     0                               half of almanac found and saved
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_FULL_ALM_SAVED        (7)             //  7     0                               full almanac found and saved
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_WORD_SYNC_FOUND         (0x01UL << 0)   //  7     0     GNSS demodulation info: word synchronization found
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_TOW_FOUND               (0x01UL << 1)   //  7     0                             time-of-week found
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_WN_DEMODED              (0x01UL << 2)   //  7     0                             week number demodulated
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_WN_FOUND                (0x01UL << 3)   //  7     0                             week number found
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_SUBFRAME_1_FOUND        (0x01UL << 4)   //  7     0                             subframe 1 found
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_SUBFRAME_4_FOUND        (0x01UL << 5)   //  7     0                             subframe 4 found
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_SUBFRAME_5_FOUND        (0x01UL << 6)   //  7     0                             subframe 5 found

// RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_STATUS
#define RADIOLIB_LR11X0_GNSS_ALMANAC_STATUS_UP_TO_DATE          (0)             //  7     0     GPS/BeiDou almanac status: all satellites up-to-date
#define RADIOLIB_LR11X0_GNSS_ALMANAC_STATUS_OUTDATED            (1)             //  7     0                                at least one satellite needs update

// RADIOLIB_LR11X0_CMD_GNSS_READ_DOPPLER_SOLVER_RES
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_NONE                    (0)             //  7     0     internal 2D solver error: no error
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_RES_HIGH                (1)             //  7     0                               residue too high
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_NOT_CONVERGED           (2)             //  7     0                               not converged on solution
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_NOT_ENOUGH_SV           (3)             //  7     0                               not enough satellites
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_ILL_MATRIX              (4)             //  7     0                               matrix error (?)
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_TIME                    (5)             //  7     0                               time error
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_ALM_PART_OLD            (6)             //  7     0                               part of almanac too old or not available
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_INCONSISTENT            (7)             //  7     0                               not consistent with history (?)
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_ALM_OLD                 (8)             //  7     0                               all of almanac too old

// RADIOLIB_LR11X0_CMD_CRYPTO_SET_KEY
#define RADIOLIB_LR11X0_CRYPTO_STATUS_SUCCESS                   (0x00UL << 0)   //  7     0     crypto engine status: success
#define RADIOLIB_LR11X0_CRYPTO_STATUS_FAIL_CMAC                 (0x01UL << 0)   //  7     0                           MIC check failed
#define RADIOLIB_LR11X0_CRYPTO_STATUS_INV_KEY_ID                (0x03UL << 0)   //  7     0                           key/parameter source or destination ID error
#define RADIOLIB_LR11X0_CRYPTO_STATUS_BUF_SIZE                  (0x05UL << 0)   //  7     0                           data buffer size invalid
#define RADIOLIB_LR11X0_CRYPTO_STATUS_ERROR                     (0x06UL << 0)   //  7     0                           generic error

// RADIOLIB_LR11X0_CMD_CRYPTO_PROCESS_JOIN_ACCEPT
#define RADIOLIB_LR11X0_CRYPTO_LORAWAN_VERSION_1_0              (0x00UL << 0)   //  7     0     LoRaWAN version: 1.0.x
#define RADIOLIB_LR11X0_CRYPTO_LORAWAN_VERSION_1_1              (0x01UL << 0)   //  7     0                      1.1

// LR11X0 SPI register variables

// RADIOLIB_LR11X0_REG_SF6_SX127X_COMPAT
#define RADIOLIB_LR11X0_SF6_SX126X                              (0x00UL << 18)  //  18    18    SF6 mode: SX126x series
#define RADIOLIB_LR11X0_SF6_SX127X                              (0x01UL << 18)  //  18    18              SX127x series

// RADIOLIB_LR11X0_REG_LORA_HIGH_POWER_FIX
#define RADIOLIB_LR11X0_LORA_HIGH_POWER_FIX                     (0x00UL << 30)  //  30    30    fix for errata

// RADIOLIB_LR11X0_REG_LNA_MODE
#define RADIOLIB_LR11X0_LNA_MODE_SINGLE_RFI_N                   (0x01UL << 4)   //  7     4     LNA mode: single-ended RFI_N
#define RADIOLIB_LR11X0_LNA_MODE_SINGLE_RFI_P                   (0x02UL << 4)   //  7     4               single-ended RFI_P
#define RADIOLIB_LR11X0_LNA_MODE_DIFFERENTIAL                   (0x03UL << 4)   //  7     4               differential (default)

/*!
  \struct LR11x0WifiResult_t
  \brief Structure to save result of passive WiFi scan.
  This result only saves the basic information.
*/
struct LR11x0WifiResult_t {
  /*! \brief WiFi (802.11) signal type, 'b', 'n' or 'g' */
  char type;

  /*! \brief Data rate ID holding information about modulation and coding rate. See LR11x0 user manual for details. */
  uint8_t dataRateId;

  /*! \brief Channel frequency in MHz */
  uint16_t channelFreq;

  /*! \brief MAC address origin: from gateway (1), phone (2) or undetermined (3) */
  uint8_t origin;

  /*! \brief Whether this signal was sent by an access point (true) or end device (false) */
  bool ap;

  /*! \brief RSSI in dBm */
  float rssi;

  /*! \brief MAC address */
  uint8_t mac[RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN];
};

/*!
  \struct LR11x0WifiResultFull_t
  \brief Structure to save result of passive WiFi scan.
  This result saves additional information alongside that in LR11x0WifiResult_t.
*/
struct LR11x0WifiResultFull_t: public LR11x0WifiResult_t {
  /*! \brief Frame type. See LR11x0 user manual for details. */
  uint8_t frameType;

  /*! \brief Frame sub type. See LR11x0 user manual for details. */
  uint8_t frameSubType;

  /*! \brief Frame sent from client station to distribution system. */
  bool toDistributionSystem;

  /*! \brief Frame sent from distribution system to client station. */
  bool fromDistributionSystem;

  /*! \brief See LR11x0 user manual for details. */
  uint16_t phiOffset;

  /*! \brief Number of microseconds the AP has been active. */
  uint64_t timestamp;

  /*! \brief Beacon period in microseconds. */
  uint32_t periodBeacon;
};

/*!
  \struct LR11x0WifiResultExtended_t
  \brief Structure to save result of passive WiFi scan.
  This result saves additional information alongside that in LR11x0WifiResultFull_t.
  Only scans performed with RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON acquisition mode
  can yield this result!
*/
struct LR11x0WifiResultExtended_t: public LR11x0WifiResultFull_t {
  /*! \brief Data rate. See LR11x0 user manual for details. */
  uint8_t rate;

  /*! \brief Refer to IEEE Std 802.11, 2016, Part 11: Wireless LAN MAC and PHY Spec. */
  uint16_t service;

  /*! \brief Refer to IEEE Std 802.11, 2016, Part 11: Wireless LAN MAC and PHY Spec. */
  uint16_t length;

  /*! \brief MAC address 0 */
  uint8_t mac0[RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN];

  /*! \brief MAC address 2 */
  uint8_t mac2[RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN];

  /*! \brief Refer to IEEE Std 802.11, 2016, Part 11: Wireless LAN MAC and PHY Spec. */
  uint16_t seqCtrl;

  /*! \brief SSID */
  uint8_t ssid[RADIOLIB_LR11X0_WIFI_RESULT_SSID_LEN];

  /*! \brief WiFi channel number */
  uint8_t currentChannel;

  /*! \brief Two-letter country code (null-terminated string). */
  char countryCode[3];

  /*! \brief Refer to IEEE Std 802.11, 2016, Part 11: Wireless LAN MAC and PHY Spec. */
  uint8_t ioReg;

  /*! \brief True if frame check sequences is valid, false otherwise. */
  bool fcsCheckOk;
};

/*!
  \struct LR11x0VersionInfo_t
  \brief Structure to report information about versions of the LR11x0 hardware and firmware.
*/
struct LR11x0VersionInfo_t {
  /*! \brief Hardware revision. */
  uint8_t hardware;

  /*! \brief Which device this is - one of RADIOLIB_LR11X0_DEVICE_* macros. */
  uint8_t device;
  
  /*! \brief Major revision of the base firmware. */
  uint8_t fwMajor;
  
  /*! \brief Minor revision of the base firmware. */
  uint8_t fwMinor;

  /*! \brief Major revision of the WiFi firmware. */
  uint8_t fwMajorWiFi;
  
  /*! \brief Minor revision of the WiFi firmware. */
  uint8_t fwMinorWiFi;

  /*! \brief Revision of the GNSS firmware. */
  uint8_t fwGNSS;
  
  /*! \brief Almanac revision of the GNSS firmware. */
  uint8_t almanacGNSS;
};

/*!
  \struct LR11x0GnssResult_t
  \brief Structure to report information results of a GNSS scan.
*/
struct LR11x0GnssResult_t {
  /*! \brief Demodulator status. One of RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_* */
  int8_t demodStat;
  
  /*! \brief Number of satellites detected during the scan. */
  uint8_t numSatsDet;

  /*! \brief Result size, used when passing data to LoRa cloud. */
  uint16_t resSize;
};

/*!
  \struct LR11x0GnssPosition_t
  \brief Structure to report position from LR11x0 internal solver.
*/
struct LR11x0GnssPosition_t {
  /*! \brief Latitude in degrees. */
  float latitude;

  /*! \brief Longitude in degrees. */
  float longitude;

  /*! \brief Accuracy of this result. */
  uint16_t accuracy;

  /*! \brief Number of satellites used to solve this position. */
  uint8_t numSatsUsed;
};

/*!
  \struct LR11x0GnssSatellite_t
  \brief Structure to save information about a satellite found during GNSS scan.
*/
struct LR11x0GnssSatellite_t {
  /*! \brief Satellite vehicle (SV) identifier. */
  uint8_t svId;

  /*! \brief C/N0 in dB. */
  uint8_t c_n0;

  /*! \brief Doppler shift of the signal in Hz. */
  int16_t doppler;
};

/*!
  \struct LR11x0GnssAlmanacStatusPart_t
  \brief Structure to save information about one constellation of the GNSS almanac.
*/
struct LR11x0GnssAlmanacStatusPart_t {
  int8_t status;
  uint32_t timeUntilSubframe;
  uint8_t numSubframes;
  uint8_t nextSubframe4SvId;
  uint8_t nextSubframe5SvId;
  uint8_t nextSubframeStart;
  uint8_t numUpdateNeeded;
  uint32_t flagsUpdateNeeded[2];
  uint32_t flagsActive[2];
};

/*!
  \struct LR11x0GnssAlmanacStatus_t
  \brief Structure to save information about the GNSS almanac.
  This is not the actual almanac, just some context information about it.
*/
struct LR11x0GnssAlmanacStatus_t {
  /*! \brief GPS part of the almanac */
  LR11x0GnssAlmanacStatusPart_t gps;

  /*! \brief BeiDou part of the almanac */
  LR11x0GnssAlmanacStatusPart_t beidou;

  /*! \brief Extra flags present for BeiDou only */
  uint32_t beidouSvNoAlmanacFlags[2];

  /*! \brief Next almanac ID */
  uint8_t nextAlmanacId;

  /*! \brief Timestamp of when almanac status was retrieved - timeUntilSubframe is relative to this value. */
  RadioLibTime_t start;
};

/*!
  \class LR11x0
  \brief Base class for %LR11x0 series. All derived classes for %LR11x0 (e.g. LR1110 or LR1120) inherit from this base class.
  This class should not be instantiated directly from user code, only from its derived classes.
*/
class LR11x0: public PhysicalLayer {
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
    explicit LR11x0(Module* mod);

    /*!
      \brief Custom operation modes for LR11x0.
      Needed because LR11x0 has several modems (sub-GHz, 2.4 GHz etc.) in one package
    */
    enum OpMode_t {
        /*! End of table marker, use \ref END_OF_MODE_TABLE constant instead */
        MODE_END_OF_TABLE = Module::MODE_END_OF_TABLE,
        /*! Standby/idle mode */
        MODE_STBY = Module::MODE_IDLE,
        /*! Receive mode */
        MODE_RX = Module::MODE_RX,
        /*! Low power transmission mode */
        MODE_TX = Module::MODE_TX,
        /*! High power transmission mode */
        MODE_TX_HP,
        /*! High frequency transmission mode */
        MODE_TX_HF,
        /*! GNSS scanning mode */
        MODE_GNSS,
        /*! WiFi scanning mode */
        MODE_WIFI,
    };

    /*!
      \brief Whether the module has an XTAL (true) or TCXO (false). Defaults to false.
    */
    bool XTAL;
    
    /*!
      \brief Initialization method for LoRa modem.
      \param bw LoRa bandwidth in kHz.
      \param sf LoRa spreading factor.
      \param cr LoRa coding rate denominator.
      \param syncWord 1-byte LoRa sync word.
      \param preambleLength LoRa preamble length in symbols
      \param tcxoVoltage TCXO reference voltage to be set.
      \param high defaults to false for Sub-GHz band, true for frequencies above 1GHz
      \returns \ref status_codes
    */
    int16_t begin(float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, uint16_t preambleLength, float tcxoVoltage, bool high = false);

    /*!
      \brief Initialization method for FSK modem.
      \param br FSK bit rate in kbps.
      \param freqDev Frequency deviation from carrier frequency in kHz.
      \param rxBw Receiver bandwidth in kHz.
      \param preambleLength FSK preamble length in bits.
      \param tcxoVoltage TCXO reference voltage to be set.
      \returns \ref status_codes
    */
    int16_t beginGFSK(float br, float freqDev, float rxBw, uint16_t preambleLength, float tcxoVoltage);

    /*!
      \brief Initialization method for LR-FHSS modem.
      \param bw LR-FHSS bandwidth, one of RADIOLIB_LR11X0_LR_FHSS_BW_* values.
      \param cr LR-FHSS coding rate, one of RADIOLIB_LR11X0_LR_FHSS_CR_* values.
      \param narrowGrid Whether to use narrow (3.9 kHz) or wide (25.39 kHz) grid spacing.
      \param tcxoVoltage TCXO reference voltage to be set.
      \returns \ref status_codes
    */
    int16_t beginLRFHSS(uint8_t bw, uint8_t cr, bool narrowGrid, float tcxoVoltage);

    /*!
      \brief Initialization method for GNSS scanning.
      \param constellations GNSS constellations to use (GPS, BeiDou or both). Defaults to both.
      \param tcxoVoltage TCXO reference voltage to be set.
      \returns \ref status_codes
    */
    int16_t beginGNSS(uint8_t constellations = RADIOLIB_LR11X0_GNSS_CONSTELLATION_GPS | RADIOLIB_LR11X0_GNSS_CONSTELLATION_BEIDOU, float tcxoVoltage = 1.6);

    /*!
      \brief Reset method. Will reset the chip to the default state using RST pin.
      \returns \ref status_codes
    */
    int16_t reset();

    /*!
      \brief Blocking binary transmit method.
      Overloads for string-based transmissions are implemented in PhysicalLayer.
      \param data Binary data to be sent.
      \param len Number of bytes to send.
      \param addr Address to send the data to. Will only be added if address filtering was enabled.
      \returns \ref status_codes
    */
    int16_t transmit(const uint8_t* data, size_t len, uint8_t addr = 0) override;

    /*!
      \brief Blocking binary receive method.
      Overloads for string-based transmissions are implemented in PhysicalLayer.
      \param data Binary data to be sent.
      \param len Number of bytes to send.
      \returns \ref status_codes
    */
    int16_t receive(uint8_t* data, size_t len) override;

    /*!
      \brief Starts direct mode transmission.
      \param frf Raw RF frequency value. Defaults to 0, required for quick frequency shifts in RTTY.
      \returns \ref status_codes
    */
    int16_t transmitDirect(uint32_t frf = 0) override;

    /*!
      \brief Starts direct mode reception. Only implemented for PhysicalLayer compatibility, as %SX126x series does not support direct mode reception.
      Will always return RADIOLIB_ERR_UNKNOWN.
      \returns \ref status_codes
    */
    int16_t receiveDirect() override;

    /*!
      \brief Performs scan for LoRa transmission in the current channel. Detects both preamble and payload.
      \returns \ref status_codes
    */
    int16_t scanChannel() override;

    /*!
      \brief Performs scan for LoRa transmission in the current channel. Detects both preamble and payload.
      \param config CAD configuration structure.
      \returns \ref status_codes
    */
    int16_t scanChannel(const ChannelScanConfig_t &config) override;

    /*!
      \brief Sets the module to standby mode (overload for PhysicalLayer compatibility, uses 13 MHz RC oscillator).
      \returns \ref status_codes
    */
    int16_t standby() override;

    /*!
      \brief Sets the module to standby mode.
      \param mode Oscillator to be used in standby mode. Can be set to RADIOLIB_LR11X0_STANDBY_RC (13 MHz RC oscillator)
      or RADIOLIB_LR11X0_STANDBY_XOSC (32 MHz external crystal oscillator).
      \param wakeup Whether to force the module to wake up. Setting to true will immediately attempt to wake up the module.
      \returns \ref status_codes
    */
    int16_t standby(uint8_t mode, bool wakeup = true);

    /*!
      \brief Sets the module to sleep mode. To wake the device up, call standby().
      Overload with warm start enabled for PhysicalLayer compatibility.
      \returns \ref status_codes
    */
    int16_t sleep() override;

    /*!
      \brief Sets the module to sleep mode. To wake the device up, call standby().
      \param retainConfig Set to true to retain configuration of the currently active modem ("warm start")
      or to false to discard current configuration ("cold start"). Defaults to true.
      \param sleepTime Sleep duration (enables automatic wakeup), in multiples of 30.52 us. Ignored if set to 0.
      \returns \ref status_codes
    */
    int16_t sleep(bool retainConfig, uint32_t sleepTime);
    
    // interrupt methods

    /*!
      \brief Sets interrupt service routine to call when IRQ1 activates.
      \param func ISR to call.
    */
    void setIrqAction(void (*func)(void));

    /*!
      \brief Clears interrupt service routine to call when IRQ1 activates.
    */
    void clearIrqAction();

    /*!
      \brief Sets interrupt service routine to call when a packet is received.
      \param func ISR to call.
    */
    void setPacketReceivedAction(void (*func)(void)) override;

    /*!
      \brief Clears interrupt service routine to call when a packet is received.
    */
    void clearPacketReceivedAction() override;

    /*!
      \brief Sets interrupt service routine to call when a packet is sent.
      \param func ISR to call.
    */
    void setPacketSentAction(void (*func)(void)) override;

    /*!
      \brief Clears interrupt service routine to call when a packet is sent.
    */
    void clearPacketSentAction() override;

    /*!
      \brief Interrupt-driven binary transmit method.
      Overloads for string-based transmissions are implemented in PhysicalLayer.
      \param data Binary data to be sent.
      \param len Number of bytes to send.
      \param addr Address to send the data to. Will only be added if address filtering was enabled.
      \returns \ref status_codes
    */
    int16_t startTransmit(const uint8_t* data, size_t len, uint8_t addr = 0) override;

    /*!
      \brief Clean up after transmission is done.
      \returns \ref status_codes
    */
    int16_t finishTransmit() override;

    /*!
      \brief Interrupt-driven receive method with default parameters.
      Implemented for compatibility with PhysicalLayer.

      \returns \ref status_codes
    */
    int16_t startReceive() override;

    /*!
      \brief Interrupt-driven receive method. IRQ1 will be activated when full packet is received.
      \param timeout Raw timeout value, expressed as multiples of 1/32.768 kHz (approximately 30.52 us).
      Defaults to RADIOLIB_LR11X0_RX_TIMEOUT_INF for infinite timeout (Rx continuous mode),
      set to RADIOLIB_LR11X0_RX_TIMEOUT_NONE for no timeout (Rx single mode).
      If timeout other than infinite is set, signal will be generated on IRQ1.

      \param irqFlags Sets the IRQ flags that will trigger IRQ1, defaults to RADIOLIB_LR11X0_IRQ_RX_DONE.
      \param irqMask Only for PhysicalLayer compatibility, not used.
      \param len Only for PhysicalLayer compatibility, not used.
      \returns \ref status_codes
    */
    int16_t startReceive(uint32_t timeout, uint32_t irqFlags = RADIOLIB_LR11X0_IRQ_RX_DONE, uint32_t irqMask = 0, size_t len = 0);

    /*!
      \brief Reads the current IRQ status.
      \returns IRQ status bits
    */
    uint32_t getIrqStatus();

    /*!
      \brief Reads data received after calling startReceive method. When the packet length is not known in advance,
      getPacketLength method must be called BEFORE calling readData!
      \param data Pointer to array to save the received binary data.
      \param len Number of bytes that will be read. When set to 0, the packet length will be retrieved automatically.
      When more bytes than received are requested, only the number of bytes requested will be returned.
      \returns \ref status_codes
    */
    int16_t readData(uint8_t* data, size_t len) override;
    
    /*!
      \brief Interrupt-driven channel activity detection method. IRQ1 will be activated
      when LoRa preamble is detected, or upon timeout. Defaults to CAD parameter values recommended by AN1200.48.
      \returns \ref status_codes
    */
    int16_t startChannelScan() override;

    /*!
      \brief Interrupt-driven channel activity detection method. IRQ pin will be activated
      when LoRa preamble is detected, or upon timeout.
      \param config CAD configuration structure.
      \returns \ref status_codes
    */
    int16_t startChannelScan(const ChannelScanConfig_t &config) override;

    /*!
      \brief Read the channel scan result
      \returns \ref status_codes
    */
    int16_t getChannelScanResult() override;

    // configuration methods

    /*!
      \brief Sets LoRa bandwidth. Allowed values are 62.5, 125.0, 250.0 and 500.0 kHz. (default, high = false)
      \param bw LoRa bandwidth to be set in kHz.
      \param high if set to true, allowed bandwidth is 203.125, 406.25 and 812.5 kHz, frequency must be above 1GHz
      \returns \ref status_codes
    */
    int16_t setBandwidth(float bw, bool high = false);

    /*!
      \brief Sets LoRa spreading factor. Allowed values range from 5 to 12.
      \param sf LoRa spreading factor to be set.
      \param legacy Enable legacy mode for SF6 - this allows to communicate with SX127x at SF6.
      \returns \ref status_codes
    */
    int16_t setSpreadingFactor(uint8_t sf, bool legacy = false);

    /*!
      \brief Sets LoRa coding rate denominator. Allowed values range from 5 to 8.
      \param cr LoRa coding rate denominator to be set.
      \param longInterleave Enable long interleaver when set to true.
      Note that CR 4/7 is not possible with long interleaver enabled!
      \returns \ref status_codes
    */
    int16_t setCodingRate(uint8_t cr, bool longInterleave = false);

    /*!
      \brief Sets LoRa sync word.
      \param syncWord LoRa sync word to be set.
      \returns \ref status_codes
    */
    int16_t setSyncWord(uint8_t syncWord);

    /*!
      \brief Sets GFSK bit rate. Allowed values range from 0.6 to 300.0 kbps.
      \param br FSK bit rate to be set in kbps.
      \returns \ref status_codes
    */
    int16_t setBitRate(float br) override;

    /*!
      \brief Sets GFSK frequency deviation. Allowed values range from 0.0 to 200.0 kHz.
      \param freqDev GFSK frequency deviation to be set in kHz.
      \returns \ref status_codes
    */
    int16_t setFrequencyDeviation(float freqDev) override;

    /*!
      \brief Sets GFSK receiver bandwidth. Allowed values are 4.8, 5.8, 7.3, 9.7, 11.7, 14.6, 19.5,
      23.4, 29.3, 39.0, 46.9, 58.6, 78.2, 93.8, 117.3, 156.2, 187.2, 234.3, 312.0, 373.6 and 467.0 kHz.
      \param rxBw GFSK receiver bandwidth to be set in kHz.
      \returns \ref status_codes
    */
    int16_t setRxBandwidth(float rxBw);
    
    /*!
      \brief Sets GFSK sync word in the form of array of up to 8 bytes.
      \param syncWord GFSK sync word to be set.
      \param len GFSK sync word length in bytes.
      \returns \ref status_codes
    */
    int16_t setSyncWord(uint8_t* syncWord, size_t len) override;

    /*!
      \brief Sets GFSK sync word in the form of array of up to 8 bytes.
      \param syncWord GFSK sync word to be set.
      \param bitsLen GFSK sync word length in bits. If length is not divisible by 8,
      least significant bits of syncWord will be ignored.
      \returns \ref status_codes
    */
    int16_t setSyncBits(uint8_t *syncWord, uint8_t bitsLen);

    /*!
      \brief Sets node address. Calling this method will also enable address filtering for node address only.
      \param nodeAddr Node address to be set.
      \returns \ref status_codes
    */
    int16_t setNodeAddress(uint8_t nodeAddr);

    /*!
      \brief Sets broadcast address. Calling this method will also enable address
      filtering for node and broadcast address.
      \param broadAddr Node address to be set.
      \returns \ref status_codes
    */
    int16_t setBroadcastAddress(uint8_t broadAddr);

    /*!
      \brief Disables address filtering. Calling this method will also erase previously set addresses.
      \returns \ref status_codes
    */
    int16_t disableAddressFiltering();

    /*!
      \brief Sets time-bandwidth product of Gaussian filter applied for shaping.
      Allowed values are RADIOLIB_SHAPING_0_3, RADIOLIB_SHAPING_0_5, RADIOLIB_SHAPING_0_7 or RADIOLIB_SHAPING_1_0.
      Set to RADIOLIB_SHAPING_NONE to disable data shaping.
      \param sh Time-bandwidth product of Gaussian filter to be set.
      \returns \ref status_codes
    */
    int16_t setDataShaping(uint8_t sh) override;

    /*!
      \brief Sets transmission encoding. Available in GFSK mode only. Serves only as alias for PhysicalLayer compatibility.
      \param encoding Encoding to be used. Set to 0 for NRZ, and 2 for whitening.
      \returns \ref status_codes
    */
    int16_t setEncoding(uint8_t encoding) override;

    /*!
      \brief Set modem in fixed packet length mode. Available in GFSK mode only.
      \param len Packet length.
      \returns \ref status_codes
    */
    int16_t fixedPacketLengthMode(uint8_t len = RADIOLIB_LR11X0_MAX_PACKET_LENGTH);

    /*!
      \brief Set modem in variable packet length mode. Available in GFSK mode only.
      \param maxLen Maximum packet length.
      \returns \ref status_codes
    */
    int16_t variablePacketLengthMode(uint8_t maxLen = RADIOLIB_LR11X0_MAX_PACKET_LENGTH);

    /*!
      \brief Sets GFSK whitening parameters.
      \param enabled True = Whitening enabled
      \param initial Initial value used for the whitening LFSR in GFSK mode.
      By default set to 0x01FF for compatibility with SX127x and LoRaWAN.
      \returns \ref status_codes
    */
    int16_t setWhitening(bool enabled, uint16_t initial = 0x01FF);

    /*!
      \brief Set data.
      \param dr Data rate struct. Interpretation depends on currently active modem (GFSK or LoRa).
      \returns \ref status_codes
    */
    int16_t setDataRate(DataRate_t dr) override;

    /*!
      \brief Check the data rate can be configured by this module.
      \param dr Data rate struct. Interpretation depends on currently active modem (GFSK or LoRa).
      \returns \ref status_codes
    */
    int16_t checkDataRate(DataRate_t dr) override;

    /*!
      \brief Sets preamble length for LoRa or GFSK modem. Allowed values range from 1 to 65535.
      \param preambleLength Preamble length to be set in symbols (LoRa) or bits (GFSK).
      \returns \ref status_codes
    */
    int16_t setPreambleLength(size_t preambleLength) override;

    /*!
      \brief Sets TCXO (Temperature Compensated Crystal Oscillator) configuration.
      \param voltage TCXO reference voltage in volts. Allowed values are 1.6, 1.7, 1.8, 2.2. 2.4, 2.7, 3.0 and 3.3 V.
      Set to 0 to disable TCXO.
      NOTE: After setting this parameter to 0, the module will be reset (since there's no other way to disable TCXO).
      \param delay TCXO timeout in us. Defaults to 5000 us.
      \returns \ref status_codes
    */
    int16_t setTCXO(float voltage, uint32_t delay = 5000);

    /*!
      \brief Sets CRC configuration.
      \param len CRC length in bytes, Allowed values are 1 or 2, set to 0 to disable CRC.
      \param initial Initial CRC value. GFSK only. Defaults to 0x1D0F (CCIT CRC).
      \param polynomial Polynomial for CRC calculation. GFSK only. Defaults to 0x1021 (CCIT CRC).
      \param inverted Invert CRC bytes. GFSK only. Defaults to true (CCIT CRC).
      \returns \ref status_codes
    */
    int16_t setCRC(uint8_t len, uint32_t initial = 0x00001D0FUL, uint32_t polynomial = 0x00001021UL, bool inverted = true);

    /*!
      \brief Enable/disable inversion of the I and Q signals
      \param enable QI inversion enabled (true) or disabled (false);
      \returns \ref status_codes
    */
    int16_t invertIQ(bool enable) override;

    /*!
      \brief Gets RSSI (Recorded Signal Strength Indicator) of the last received packet. Only available for LoRa or GFSK modem.
      \returns RSSI of the last received packet in dBm.
    */
    float getRSSI() override;

    /*!
      \brief Gets SNR (Signal to Noise Ratio) of the last received packet. Only available for LoRa modem.
      \returns SNR of the last received packet in dB.
    */
    float getSNR() override;

    /*!
      \brief Gets frequency error of the latest received packet.
      \returns Frequency error in Hz.
    */
    float getFrequencyError();

    /*!
      \brief Query modem for the packet length of received payload.
      \param update Update received packet length. Will return cached value when set to false.
      \returns Length of last received packet in bytes.
    */
    size_t getPacketLength(bool update = true) override;

    /*!
      \brief Query modem for the packet length of received payload.
      \param update Update received packet length. Will return cached value when set to false.
      \returns Length of last received packet in bytes.
    */
    size_t getPacketLength(bool update, uint8_t* offset);

    /*!
      \brief Get expected time-on-air for a given size of payload
      \param len Payload length in bytes.
      \returns Expected time-on-air in microseconds.
    */
    RadioLibTime_t getTimeOnAir(size_t len) override;

    /*!
      \brief Calculate the timeout value for this specific module / series (in number of symbols or units of time)
      \param timeoutUs Timeout in microseconds to listen for
      \returns Timeout value in a unit that is specific for the used module
    */
    RadioLibTime_t calculateRxTimeout(RadioLibTime_t timeoutUs) override;

    /*!
      \brief Read currently active IRQ flags.
      \returns IRQ flags.
    */
    uint32_t getIrqFlags() override;

    /*!
      \brief Set interrupt on IRQ pin to be sent on a specific IRQ bit (e.g. RxTimeout, CadDone).
      \param irq Module-specific IRQ flags.
      \returns \ref status_codes
    */
    int16_t setIrqFlags(uint32_t irq) override;

    /*!
      \brief Clear interrupt on a specific IRQ bit (e.g. RxTimeout, CadDone).
      \param irq Module-specific IRQ flags.
      \returns \ref status_codes
    */
    int16_t clearIrqFlags(uint32_t irq) override;

    /*!
      \brief Get one truly random byte from RSSI noise.
      \returns TRNG byte.
    */
    uint8_t randomByte() override;

    /*!
      \brief Set implicit header mode for future reception/transmission.
      \param len Payload length in bytes.
      \returns \ref status_codes
    */
    int16_t implicitHeader(size_t len);

    /*!
      \brief Set explicit header mode for future reception/transmission.
      \returns \ref status_codes
    */
    int16_t explicitHeader();

    /*!
      \brief Gets effective data rate for the last transmitted packet. The value is calculated only for payload bytes.
      \returns Effective data rate in bps.
    */
    float getDataRate() const;

    /*!
      \brief Set regulator mode to LDO.
      \returns \ref status_codes
    */
    int16_t setRegulatorLDO();

    /*!
      \brief Set regulator mode to DC-DC.
      \returns \ref status_codes
    */
    int16_t setRegulatorDCDC();

    /*!
      \brief Enables or disables Rx Boosted Gain mode (additional Rx gain for increased power consumption).
      \param en True for Rx Boosted Gain, false for Rx Power Saving Gain
      \returns \ref status_codes
    */
    int16_t setRxBoostedGainMode(bool en);

    /*! \copydoc Module::setRfSwitchTable */
    void setRfSwitchTable(const uint32_t (&pins)[Module::RFSWITCH_MAX_PINS], const Module::RfSwitchMode_t table[]);

    /*!
      \brief Forces LoRa low data rate optimization. Only available in LoRa mode. After calling this method, LDRO will always be set to
      the provided value, regardless of symbol length. To re-enable automatic LDRO configuration, call LR11x0::autoLDRO()
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

    /*!
      \brief Sets LR-FHSS configuration.
      \param bw LR-FHSS bandwidth, one of RADIOLIB_LR11X0_LR_FHSS_BW_* values.
      \param cr LR-FHSS coding rate, one of RADIOLIB_LR11X0_LR_FHSS_CR_* values.
      \param hdrCount Header packet count, 1 - 4. Defaults to 3.
      \param hopSeed 9-bit seed number for PRNG generation of the hopping sequence. Defaults to 0x13A.
      \returns \ref status_codes
    */
    int16_t setLrFhssConfig(uint8_t bw, uint8_t cr, uint8_t hdrCount = 3, uint16_t hopSeed = 0x13A);
    
    /*!
      \brief Start passive WiFi scan. BUSY pin will be de-activated when the scan is finished.
      \param wifiType Type of WiFi (802.11) signals to scan, 'b', 'n', 'g' or '*' for all signals.
      \param mode Scan acquisition mode, one of RADIOLIB_LR11X0_WIFI_ACQ_MODE_*.
      The type of results available after the scan depends on this mode.
      Defaults to RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON, which provides the most information.
      \param chanMask Bit mask of WiFi channels to scan, defaults to all channels.
      More channels leads to longer overall scan duration.
      \param numScans Number of scans to perform per each enabled channel. Defaults to 16 scans.
      More scans leads to longer overall scan duration.
      \param timeout Timeout of each scan in milliseconds. Defaults to 100 ms
      Longer timeout leads to longer overall scan duration.
      \returns \ref status_codes
    */
    int16_t startWifiScan(char wifiType, uint8_t mode = RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON, uint16_t chanMask = RADIOLIB_LR11X0_WIFI_ALL_CHANNELS, uint8_t numScans = 16, uint16_t timeout = 100);

    /*!
      \brief Sets interrupt service routine to call when a WiFi scan is completed.
      \param func ISR to call.
    */
    void setWiFiScanAction(void (*func)(void));

    /*!
      \brief Clears interrupt service routine to call when a WiFi scan is completed.
    */
    void clearWiFiScanAction();

    /*!
      \brief Get number of WiFi scan results after the scan is finished.
      \param count Pointer to a variable that will hold the number of scan results.
      \returns \ref status_codes
    */
    int16_t getWifiScanResultsCount(uint8_t* count);

    /*!
      \brief Retrieve passive WiFi scan result.
      \param result Pointer to structure to hold the result data.
      \param index Result index, starting from 0. The number of scan results can be retrieved by calling getWifiScanResultsCount.
      \param brief Whether to only retrieve the results in brief format. If set to false, only information in LR11x0WifiResult_t
      will be retrieved. If set to true, information in LR11x0WifiResultFull_t will be retrieved. In addition, if WiFi scan mode
      was set to RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON, all information in LR11x0WifiResultExtended_t will be retrieved.
      \returns \ref status_codes
    */
    int16_t getWifiScanResult(LR11x0WifiResult_t* result, uint8_t index, bool brief = false);
    
    /*!
      \brief Blocking WiFi scan method. Performs a full passive WiFi scan.
      This method may block for several seconds!
      \param wifiType Type of WiFi (802.11) signals to scan, 'b', 'n', 'g' or '*' for all signals.
      \param count Pointer to a variable that will hold the number of scan results.
      \param mode Scan acquisition mode, one of RADIOLIB_LR11X0_WIFI_ACQ_MODE_*.
      The type of results available after the scan depends on this mode.
      Defaults to RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON, which provides the most information.
      \param chanMask Bit mask of WiFi channels to scan, defaults to all channels.
      More channels leads to longer overall scan duration.
      \param numScans Number of scans to perform per each enabled channel. Defaults to 16 scans.
      More scans leads to longer overall scan duration.
      \param timeout Timeout of each scan in milliseconds. Defaults to 100 ms
      Longer timeout leads to longer overall scan duration.
      \returns \ref status_codes
    */
    int16_t wifiScan(uint8_t wifiType, uint8_t* count, uint8_t mode = RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON, uint16_t chanMask = RADIOLIB_LR11X0_WIFI_ALL_CHANNELS, uint8_t numScans = 16, uint16_t timeout = 100);
   
    /*!
      \brief Retrieve LR11x0 hardware, device and firmware version information.
      \param info Pointer to LR11x0VersionInfo_t structure to populate.
      \returns \ref status_codes
    */
    int16_t getVersionInfo(LR11x0VersionInfo_t* info);
    
    /*!
      \brief Method to upload new firmware image to the device.
      The device will be automatically erased, a new firmware will be uploaded,
      written to flash and executed.
      \param image Pointer to the image to upload.
      \param size Size of the image in 32-bit words.
      \param nonvolatile Set to true when the image is saved in non-volatile memory of the host processor,
      or to false when the patch is in its RAM. Defaults to true.
      \returns \ref status_codes
    */
    int16_t updateFirmware(const uint32_t* image, size_t size, bool nonvolatile = true);
    
    /*!
      \brief Method to check whether the device is capable of performing a GNSS scan.
      \returns \ref status_codes
    */
    int16_t isGnssScanCapable();

    /*!
      \brief Performs GNSS scan.
      \param res Pointer to LR11x0GnssPosition_t structure to populate.
      Will not be saved if set to NULL, defaults to NULL.
      \returns \ref status_codes
    */
    int16_t gnssScan(LR11x0GnssResult_t* res = NULL);

    /*!
      \brief Read information about the almanac.
      \param stat Pointer to structure to save the almanac status into.
      This is not the actual almanac, just a structure providing information about it.
      \returns \ref status_codes
    */
    int16_t getGnssAlmanacStatus(LR11x0GnssAlmanacStatus_t *stat);

    /*!
      \brief Blocking wait until the next subframe with almanac data is available.
      Used to control timing during almanac update from satellite.
      \param stat Pointer to structure containing the almanac status read by getGnssAlmanacStatus.
      This is not the actual almanac, just a structure providing information about it.
      \param constellation Constellation to wait for, one of RADIOLIB_LR11X0_GNSS_CONSTELLATION_*.
      Constellations cannot be updated at the same time, but rather must be updated sequentially!
      \returns \ref status_codes
    */
    int16_t gnssDelayUntilSubframe(LR11x0GnssAlmanacStatus_t *stat, uint8_t constellation);

    /*!
      \brief Perform almanac update. Must be called immediately after gnssDelayUntilSubframe.
      \param constellation Constellation to update, one of RADIOLIB_LR11X0_GNSS_CONSTELLATION_*.
      Constellations cannot be updated at the same time, but rather must be updated sequentially!
      \returns \ref status_codes
    */
    int16_t updateGnssAlmanac(uint8_t constellation);
    
    /*!
      \brief Get GNSS position. Called after gnssScan to retrieve the position calculated by the internal solver.
      \param pos Pointer to LR11x0GnssPosition_t structure to populate.
      \param filtered Whether to save the filtered, or unfiltered values. Defaults to true (filtered).
      \returns \ref status_codes
    */
    int16_t getGnssPosition(LR11x0GnssPosition_t* pos, bool filtered = true);

    /*!
      \brief Get GNSS satellites found during the last scan.
      \param sats Pointer to array of LR11x0GnssSatellite_t structures to populate.
      \param numSats Number of satellites to read. Can be retrieved from LR11x0GnssResult_t passed to gnssScan.
      \returns \ref status_codes
    */
    int16_t getGnssSatellites(LR11x0GnssSatellite_t* sats, uint8_t numSats);

    /*!
      \brief Get modem currently in use by the radio.
      \param modem Pointer to a variable to save the retrieved configuration into.
      \returns \ref status_codes
    */
    int16_t getModem(ModemType_t* modem) override;

    /*!
      \brief Perform image rejection calibration for the specified frequency band.
      WARNING: Use at your own risk! Setting incorrect values may lead to decreased performance
      \param freqMin Frequency band lower bound.
      \param freqMax Frequency band upper bound.
      \returns \ref status_codes
    */
    int16_t calibrateImageRejection(float freqMin, float freqMax);
    
#if !RADIOLIB_GODMODE && !RADIOLIB_LOW_LEVEL
  protected:
#endif
    Module* getMod() override;

    // LR11x0 SPI command implementations
    int16_t writeRegMem32(uint32_t addr, const uint32_t* data, size_t len);
    int16_t readRegMem32(uint32_t addr, uint32_t* data, size_t len);
    int16_t writeBuffer8(uint8_t* data, size_t len);
    int16_t readBuffer8(uint8_t* data, size_t len, size_t offset);
    int16_t clearRxBuffer(void);
    int16_t writeRegMemMask32(uint32_t addr, uint32_t mask, uint32_t data);

    int16_t getStatus(uint8_t* stat1, uint8_t* stat2, uint32_t* irq);
    int16_t getVersion(uint8_t* hw, uint8_t* device, uint8_t* major, uint8_t* minor);
    int16_t getErrors(uint16_t* err);
    int16_t clearErrors(void);
    int16_t calibrate(uint8_t params);
    int16_t setRegMode(uint8_t mode);
    int16_t setDioAsRfSwitch(uint8_t en, uint8_t stbyCfg, uint8_t rxCfg, uint8_t txCfg, uint8_t txHpCfg, uint8_t txHfCfg, uint8_t gnssCfg, uint8_t wifiCfg);
    int16_t setDioIrqParams(uint32_t irq1, uint32_t irq2);
    int16_t setDioIrqParams(uint32_t irq);
    int16_t clearIrqState(uint32_t irq);
    int16_t configLfClock(uint8_t setup);
    int16_t setTcxoMode(uint8_t tune, uint32_t delay);
    int16_t reboot(bool stay);
    int16_t getVbat(float* vbat);
    int16_t getTemp(float* temp);
    int16_t setFs(void);
    int16_t getRandomNumber(uint32_t* rnd);
    int16_t eraseInfoPage(void);
    int16_t writeInfoPage(uint16_t addr, const uint32_t* data, size_t len);
    int16_t readInfoPage(uint16_t addr, uint32_t* data, size_t len);
    int16_t getChipEui(uint8_t* eui);
    int16_t getSemtechJoinEui(uint8_t* eui);
    int16_t deriveRootKeysAndGetPin(uint8_t* pin);
    int16_t enableSpiCrc(bool en);
    int16_t driveDiosInSleepMode(bool en);

    int16_t resetStats(void);
    int16_t getStats(uint16_t* nbPktReceived, uint16_t* nbPktCrcError, uint16_t* data1, uint16_t* data2);
    int16_t getPacketType(uint8_t* type);
    int16_t getRxBufferStatus(uint8_t* len, uint8_t* startOffset);
    int16_t getPacketStatusLoRa(float* rssiPkt, float* snrPkt, float* signalRssiPkt);
    int16_t getPacketStatusGFSK(float* rssiSync, float* rssiAvg, uint8_t* rxLen, uint8_t* stat);
    int16_t getRssiInst(float* rssi);
    int16_t setGfskSyncWord(uint8_t* sync);
    int16_t setLoRaPublicNetwork(bool pub);
    int16_t setRx(uint32_t timeout);
    int16_t setTx(uint32_t timeout);
    int16_t setRfFrequency(uint32_t rfFreq);
    int16_t autoTxRx(uint32_t delay, uint8_t intMode, uint32_t timeout);
    int16_t setCadParams(uint8_t symNum, uint8_t detPeak, uint8_t detMin, uint8_t cadExitMode, uint32_t timeout);
    int16_t setPacketType(uint8_t type);
    int16_t setModulationParamsLoRa(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro);
    int16_t setModulationParamsGFSK(uint32_t br, uint8_t sh, uint8_t rxBw, uint32_t freqDev);
    int16_t setModulationParamsLrFhss(uint32_t br, uint8_t sh);
    int16_t setModulationParamsSigfox(uint32_t br, uint8_t sh);
    int16_t setPacketParamsLoRa(uint16_t preambleLen, uint8_t hdrType, uint8_t payloadLen, uint8_t crcType, uint8_t invertIQ);
    int16_t setPacketParamsGFSK(uint16_t preambleLen, uint8_t preambleDetectorLen, uint8_t syncWordLen, uint8_t addrCmp, uint8_t packType, uint8_t payloadLen, uint8_t crcType, uint8_t whiten);
    int16_t setPacketParamsSigfox(uint8_t payloadLen, uint16_t rampUpDelay, uint16_t rampDownDelay, uint16_t bitNum);
    int16_t setTxParams(int8_t pwr, uint8_t ramp);
    int16_t setPacketAdrs(uint8_t node, uint8_t broadcast);
    int16_t setRxTxFallbackMode(uint8_t mode);
    int16_t setRxDutyCycle(uint32_t rxPeriod, uint32_t sleepPeriod, uint8_t mode);
    int16_t setPaConfig(uint8_t paSel, uint8_t regPaSupply, uint8_t paDutyCycle, uint8_t paHpSel);
    int16_t stopTimeoutOnPreamble(bool stop);
    int16_t setCad(void);
    int16_t setTxCw(void);
    int16_t setTxInfinitePreamble(void);
    int16_t setLoRaSynchTimeout(uint8_t symbolNum);
    int16_t setRangingAddr(uint32_t addr, uint8_t checkLen);
    int16_t setRangingReqAddr(uint32_t addr);
    int16_t getRangingResult(uint8_t type, float* res);
    int16_t setRangingTxRxDelay(uint32_t delay);
    int16_t setGfskCrcParams(uint32_t init, uint32_t poly);
    int16_t setGfskWhitParams(uint16_t seed);
    int16_t setRangingParameter(uint8_t symbolNum);
    int16_t setRssiCalibration(const int8_t* tune, int16_t gainOffset);
    int16_t setLoRaSyncWord(uint8_t sync);
    int16_t lrFhssBuildFrame(uint8_t hdrCount, uint8_t cr, uint8_t grid, bool hop, uint8_t bw, uint16_t hopSeq, int8_t devOffset, const uint8_t* payload, size_t len);
    int16_t lrFhssSetSyncWord(uint32_t sync);
    int16_t configBleBeacon(uint8_t chan, const uint8_t* payload, size_t len);
    int16_t getLoRaRxHeaderInfos(uint8_t* info);
    int16_t bleBeaconSend(uint8_t chan, const uint8_t* payload, size_t len);

    int16_t wifiScan(uint8_t type, uint16_t mask, uint8_t acqMode, uint8_t nbMaxRes, uint8_t nbScanPerChan, uint16_t timeout, uint8_t abortOnTimeout);
    int16_t wifiScanTimeLimit(uint8_t type, uint16_t mask, uint8_t acqMode, uint8_t nbMaxRes, uint16_t timePerChan, uint16_t timeout);
    int16_t wifiCountryCode(uint16_t mask, uint8_t nbMaxRes, uint8_t nbScanPerChan, uint16_t timeout, uint8_t abortOnTimeout);
    int16_t wifiCountryCodeTimeLimit(uint16_t mask, uint8_t nbMaxRes, uint16_t timePerChan, uint16_t timeout);
    int16_t wifiGetNbResults(uint8_t* nbResults);
    int16_t wifiReadResults(uint8_t index, uint8_t nbResults, uint8_t format, uint8_t* results);
    int16_t wifiResetCumulTimings(void);
    int16_t wifiReadCumulTimings(uint32_t* detection, uint32_t* capture, uint32_t* demodulation);
    int16_t wifiGetNbCountryCodeResults(uint8_t* nbResults);
    int16_t wifiReadCountryCodeResults(uint8_t index, uint8_t nbResults, uint8_t* results);
    int16_t wifiCfgTimestampAPphone(uint32_t timestamp);
    int16_t wifiReadVersion(uint8_t* major, uint8_t* minor);

    int16_t gnssReadRssi(int8_t* rssi);
    int16_t gnssSetConstellationToUse(uint8_t mask);
    int16_t gnssReadConstellationToUse(uint8_t* mask);
    int16_t gnssSetAlmanacUpdate(uint8_t mask);
    int16_t gnssReadAlmanacUpdate(uint8_t* mask);
    int16_t gnssSetFreqSearchSpace(uint8_t freq);
    int16_t gnssReadFreqSearchSpace(uint8_t* freq);
    int16_t gnssReadVersion(uint8_t* fw, uint8_t* almanac);
    int16_t gnssReadSupportedConstellations(uint8_t* mask);
    int16_t gnssSetMode(uint8_t mode);
    int16_t gnssAutonomous(uint32_t gpsTime, uint8_t resMask, uint8_t nbSvMask);
    int16_t gnssAssisted(uint32_t gpsTime, uint8_t effort, uint8_t resMask, uint8_t nbSvMask);
    int16_t gnssSetAssistancePosition(float lat, float lon);
    int16_t gnssReadAssistancePosition(float* lat, float* lon);
    int16_t gnssPushSolverMsg(uint8_t* payload, size_t len);
    int16_t gnssPushDmMsg(uint8_t* payload, size_t len);
    int16_t gnssGetContextStatus(uint8_t* fwVersion, uint32_t* almanacCrc, uint8_t* errCode, uint8_t* almUpdMask, uint8_t* freqSpace);
    int16_t gnssGetNbSvDetected(uint8_t* nbSv);
    int16_t gnssGetSvDetected(uint8_t* svId, uint8_t* snr, int16_t* doppler, size_t nbSv);
    int16_t gnssGetConsumption(uint32_t* cpu, uint32_t* radio);
    int16_t gnssGetResultSize(uint16_t* size);
    int16_t gnssReadResults(uint8_t* result, uint16_t size);
    int16_t gnssAlmanacFullUpdateHeader(uint16_t date, uint32_t globalCrc);
    int16_t gnssAlmanacFullUpdateSV(uint8_t svn, const uint8_t* svnAlmanac);
    int16_t gnssAlmanacReadAddrSize(uint32_t* addr, uint16_t* size);
    int16_t gnssAlmanacReadSV(uint8_t svId, uint8_t* almanac);
    int16_t gnssGetNbSvVisible(uint32_t time, float lat, float lon, uint8_t constellation, uint8_t* nbSv);
    int16_t gnssGetSvVisible(uint8_t nbSv, uint8_t** svId, int16_t** doppler, int16_t** dopplerErr);
    int16_t gnssPerformScan(uint8_t effort, uint8_t resMask, uint8_t nbSvMax);
    int16_t gnssReadLastScanModeLaunched(uint8_t* lastScanMode);
    int16_t gnssFetchTime(uint8_t effort, uint8_t opt);
    int16_t gnssReadTime(uint8_t* err, uint32_t* time, uint32_t* nbUs, uint32_t* timeAccuracy);
    int16_t gnssResetTime(void);
    int16_t gnssResetPosition(void);
    int16_t gnssReadWeekNumberRollover(uint8_t* status, uint8_t* rollover);
    int16_t gnssReadDemodStatus(int8_t* status, uint8_t* info);
    int16_t gnssReadCumulTiming(uint32_t* timing, uint8_t* constDemod);
    int16_t gnssSetTime(uint32_t time, uint16_t accuracy);
    int16_t gnssReadDopplerSolverRes(uint8_t* error, uint8_t* nbSvUsed, float* lat, float* lon, uint16_t* accuracy, uint16_t* xtal, float* latFilt, float* lonFilt, uint16_t* accuracyFilt, uint16_t* xtalFilt);
    int16_t gnssReadDelayResetAP(uint32_t* delay);
    int16_t gnssAlmanacUpdateFromSat(uint8_t effort, uint8_t bitMask);
    int16_t gnssReadAlmanacStatus(uint8_t* status);
    int16_t gnssReadKeepSyncStatus(uint8_t mask, uint8_t* nbSvVisible, uint32_t* elapsed);
    int16_t gnssConfigAlmanacUpdatePeriod(uint8_t bitMask, uint8_t svType, uint16_t period);
    int16_t gnssReadAlmanacUpdatePeriod(uint8_t bitMask, uint8_t svType, uint16_t* period);
    int16_t gnssConfigDelayResetAP(uint32_t delay);
    int16_t gnssGetSvWarmStart(uint8_t bitMask, uint8_t* sv, uint8_t nbVisSat);
    int16_t gnssReadWarmStartStatus(uint8_t bitMask, uint8_t* nbVisSat, uint32_t* timeElapsed);
    int16_t gnssGetSvSync(uint8_t mask, uint8_t nbSv, uint8_t* syncList);
    int16_t gnssWriteBitMaskSatActivated(uint8_t bitMask, uint32_t* bitMaskActivated0, uint32_t* bitMaskActivated1);
    void gnssAbort();

    int16_t cryptoSetKey(uint8_t keyId, const uint8_t* key);
    int16_t cryptoDeriveKey(uint8_t srcKeyId, uint8_t dstKeyId, const uint8_t* key);
    int16_t cryptoProcessJoinAccept(uint8_t decKeyId, uint8_t verKeyId, uint8_t lwVer, const uint8_t* header, const uint8_t* dataIn, size_t len, uint8_t* dataOut);
    int16_t cryptoComputeAesCmac(uint8_t keyId, const uint8_t* data, size_t len, uint32_t* mic);
    int16_t cryptoVerifyAesCmac(uint8_t keyId, uint32_t micExp, const uint8_t* data, size_t len, bool* result);
    int16_t cryptoAesEncrypt01(uint8_t keyId, const uint8_t* dataIn, size_t len, uint8_t* dataOut);
    int16_t cryptoAesEncrypt(uint8_t keyId, const uint8_t* dataIn, size_t len, uint8_t* dataOut);
    int16_t cryptoAesDecrypt(uint8_t keyId, const uint8_t* dataIn, size_t len, uint8_t* dataOut);
    int16_t cryptoStoreToFlash(void);
    int16_t cryptoRestoreFromFlash(void);
    int16_t cryptoSetParam(uint8_t id, uint32_t value);
    int16_t cryptoGetParam(uint8_t id, uint32_t* value);
    int16_t cryptoCheckEncryptedFirmwareImage(uint32_t offset, const uint32_t* data, size_t len, bool nonvolatile);
    int16_t cryptoCheckEncryptedFirmwareImageResult(bool* result);

    int16_t bootEraseFlash(void);
    int16_t bootWriteFlashEncrypted(uint32_t offset, const uint32_t* data, size_t len, bool nonvolatile);
    int16_t bootReboot(bool stay);
    int16_t bootGetPin(uint8_t* pin);
    int16_t bootGetChipEui(uint8_t* eui);
    int16_t bootGetJoinEui(uint8_t* eui);
    
    int16_t SPIcommand(uint16_t cmd, bool write, uint8_t* data, size_t len, const uint8_t* out = NULL, size_t outLen = 0);
    
#if !RADIOLIB_GODMODE
  protected:
#endif
    uint8_t chipType = 0;
    float freqMHz = 0;

#if !RADIOLIB_GODMODE
  private:
#endif
    Module* mod;

    // cached LoRa parameters
    uint8_t bandwidth = 0, spreadingFactor = 0, codingRate = 0, ldrOptimize = 0, crcTypeLoRa = 0, headerType = 0;
    uint16_t preambleLengthLoRa = 0;
    float bandwidthKhz = 0;
    bool ldroAuto = true;
    size_t implicitLen = 0;
    bool invertIQEnabled = false;

    // cached GFSK parameters
    uint32_t bitRate = 0, frequencyDev = 0;
    uint8_t preambleDetLength = 0, rxBandwidth = 0, pulseShape = 0, crcTypeGFSK = 0, syncWordLength = 0, addrComp = 0, whitening = 0, packetType = 0, node = 0;
    uint16_t preambleLengthGFSK = 0;

    // cached LR-FHSS parameters
    uint8_t lrFhssCr = 0, lrFhssBw = 0, lrFhssHdrCount = 0, lrFhssGrid = 0;
    uint16_t lrFhssHopSeq = 0;

    float dataRateMeasured = 0;

    uint8_t wifiScanMode = 0;
    bool gnss = false;

    int16_t modSetup(float tcxoVoltage, uint8_t modem);
    static int16_t SPIparseStatus(uint8_t in);
    static int16_t SPIcheckStatus(Module* mod);
    bool findChip(uint8_t ver);
    int16_t config(uint8_t modem);
    int16_t setPacketMode(uint8_t mode, uint8_t len);
    int16_t startCad(uint8_t symbolNum, uint8_t detPeak, uint8_t detMin, uint8_t exitMode, RadioLibTime_t timeout);
    int16_t setHeaderType(uint8_t hdrType, size_t len = 0xFF);

    // common methods to avoid some copy-paste
    int16_t bleBeaconCommon(uint16_t cmd, uint8_t chan, const uint8_t* payload, size_t len);
    int16_t writeCommon(uint16_t cmd, uint32_t addrOffset, const uint32_t* data, size_t len, bool nonvolatile);
    int16_t cryptoCommon(uint16_t cmd, uint8_t keyId, const uint8_t* dataIn, size_t len, uint8_t* dataOut);
};

#endif

#endif
