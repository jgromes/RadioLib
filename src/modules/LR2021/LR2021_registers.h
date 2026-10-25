#if !defined(RADIOLIB_LR2021_REGISTERS_H)
#define RADIOLIB_LR2021_REGISTERS_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR2021

// LR2021 SPI registers
#define RADIOLIB_LR2021_REG_DCDC_FREQ_LF                        (0x80004C)
#define RADIOLIB_LR2021_REG_DCDC_SWITCHER                       (0xF20024)
#define RADIOLIB_LR2021_REG_RX_FIFO_BASE                        (0xF30028)
#define RADIOLIB_LR2021_REG_TX_FIFO_BASE                        (0xF3002C)
#define RADIOLIB_LR2021_REG_RX_FIFO_SIZE                        (0xF30030)
#define RADIOLIB_LR2021_REG_TX_FIFO_SIZE                        (0xF30034)
#define RADIOLIB_LR2021_REG_SRAM_STATUS                         (0xF30054)
#define RADIOLIB_LR2021_REG_RTTOF_RSSI_POWER_OFFSET             (0xF30128)
#define RADIOLIB_LR2021_REG_RESULT_DEVIATION_CHANNEL_FILTER     (0xF3013C)
#define RADIOLIB_LR2021_REG_RESULT_DEVIATION_DCC                (0xF30134)
#define RADIOLIB_LR2021_REG_RTTOF_RSSI_MAX_GAIN                 (0xF301A4)
#define RADIOLIB_LR2021_REG_LORA_MODEM_TXRX_CFG0                (0xF30A14)
#define RADIOLIB_LR2021_REG_LORA_MODEM_MAIN_TX_CFG1             (0xF30A24)
#define RADIOLIB_LR2021_REG_LORA_EXT_FREQ_ERR_CTRL              (0xF30A2C)
#define RADIOLIB_LR2021_REG_RTTOF_EXTENDED_STUCK                (0xF30B50)
#define RADIOLIB_LR2021_REG_BLE_PHY_CODED_FREQ_DRIFT            (0xF30C28)
#define RADIOLIB_LR2021_REG_OOK_DETECTION_THRESHOLD             (0xF30E14)
#define RADIOLIB_LR2021_REG_RTTOF_RF_FREQ                       (0xF40144)
#define RADIOLIB_LR2021_REG_DCDC_ADC_CTRL                       (0xF40200)
#define RADIOLIB_LR2021_REG_OCP_CONTROL_UNLOCK                  (0xF40338)
#define RADIOLIB_LR2021_REG_OCP_THRESHOLDS                      (0xF40300)
#define RADIOLIB_LR2021_REG_DCDC_RX_PATH                        (0xF40430)

// RAM addresses and magic values
#define RADIOLIB_LR2021_PRAM_BASE                               (0x801000)
#define RADIOLIB_LR2021_PRAM_ADDR_LOADED                        (0x800FF8)
#define RADIOLIB_LR2021_PRAM_LOADED_MAGIC                       (0x600DB002)
#define RADIOLIB_LR2021_PRAM_ADDR_VERSION                       (0x800FFC)
#define RADIOLIB_LR2021_SRAM_BASE                               (0x802000)

// FIFO base addresses
#define RADIOLIB_LR2021_RX_FIFO_BASE_ADDR                       (0x00804400)
#define RADIOLIB_LR2021_TX_FIFO_BASE_ADDR                       (0x00804000)

#endif

#endif
