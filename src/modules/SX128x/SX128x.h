#if !defined(_RADIOLIB_SX128X_H)
#define _RADIOLIB_SX128X_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_SX128X)

#include "../../Module.h"

#include "../../protocols/PhysicalLayer/PhysicalLayer.h"

// SX128X physical layer properties
#define SX128X_FREQUENCY_STEP_SIZE                    198.3642578
#define SX128X_MAX_PACKET_LENGTH                      255
#define SX128X_CRYSTAL_FREQ                           52.0
#define SX128X_DIV_EXPONENT                           18

// SX128X SPI commands
#define SX128X_CMD_NOP                                0x00
#define SX128X_CMD_GET_STATUS                         0xC0
#define SX128X_CMD_WRITE_REGISTER                     0x18
#define SX128X_CMD_READ_REGISTER                      0x19
#define SX128X_CMD_WRITE_BUFFER                       0x1A
#define SX128X_CMD_READ_BUFFER                        0x1B
#define SX128X_CMD_SET_SLEEP                          0x84
#define SX128X_CMD_SET_STANDBY                        0x80
#define SX128X_CMD_SET_FS                             0xC1
#define SX128X_CMD_SET_TX                             0x83
#define SX128X_CMD_SET_RX                             0x82
#define SX128X_CMD_SET_RX_DUTY_CYCLE                  0x94
#define SX128X_CMD_SET_CAD                            0xC5
#define SX128X_CMD_SET_TX_CONTINUOUS_WAVE             0xD1
#define SX128X_CMD_SET_TX_CONTINUOUS_PREAMBLE         0xD2
#define SX128X_CMD_SET_PACKET_TYPE                    0x8A
#define SX128X_CMD_GET_PACKET_TYPE                    0x03
#define SX128X_CMD_SET_RF_FREQUENCY                   0x86
#define SX128X_CMD_SET_TX_PARAMS                      0x8E
#define SX128X_CMD_SET_CAD_PARAMS                     0x88
#define SX128X_CMD_SET_BUFFER_BASE_ADDRESS            0x8F
#define SX128X_CMD_SET_MODULATION_PARAMS              0x8B
#define SX128X_CMD_SET_PACKET_PARAMS                  0x8C
#define SX128X_CMD_GET_RX_BUFFER_STATUS               0x17
#define SX128X_CMD_GET_PACKET_STATUS                  0x1D
#define SX128X_CMD_GET_RSSI_INST                      0x1F
#define SX128X_CMD_SET_DIO_IRQ_PARAMS                 0x8D
#define SX128X_CMD_GET_IRQ_STATUS                     0x15
#define SX128X_CMD_CLEAR_IRQ_STATUS                   0x97
#define SX128X_CMD_SET_REGULATOR_MODE                 0x96
#define SX128X_CMD_SET_SAVE_CONTEXT                   0xD5
#define SX128X_CMD_SET_AUTO_TX                        0x98
#define SX128X_CMD_SET_AUTO_FS                        0x9E
#define SX128X_CMD_SET_PERF_COUNTER_MODE              0x9C
#define SX128X_CMD_SET_LONG_PREAMBLE                  0x9B
#define SX128X_CMD_SET_UART_SPEED                     0x9D
#define SX128X_CMD_SET_RANGING_ROLE                   0xA3
#define SX128X_CMD_SET_ADVANCED_RANGING               0x9A

// SX128X register map
#define SX128X_REG_SYNC_WORD_1_BYTE_4                 0x09CE
#define SX128X_REG_SYNC_WORD_1_BYTE_3                 0x09CF
#define SX128X_REG_SYNC_WORD_1_BYTE_2                 0x09D0
#define SX128X_REG_SYNC_WORD_1_BYTE_1                 0x09D1
#define SX128X_REG_SYNC_WORD_1_BYTE_0                 0x09D2
#define SX128X_REG_SYNC_WORD_2_BYTE_4                 0x09D3
#define SX128X_REG_SYNC_WORD_2_BYTE_3                 0x09D4
#define SX128X_REG_SYNC_WORD_2_BYTE_2                 0x09D5
#define SX128X_REG_SYNC_WORD_2_BYTE_1                 0x09D6
#define SX128X_REG_SYNC_WORD_2_BYTE_0                 0x09D7
#define SX128X_REG_SYNC_WORD_3_BYTE_4                 0x09D8
#define SX128X_REG_SYNC_WORD_3_BYTE_3                 0x09D9
#define SX128X_REG_SYNC_WORD_3_BYTE_2                 0x09DA
#define SX128X_REG_SYNC_WORD_3_BYTE_1                 0x09DB
#define SX128X_REG_SYNC_WORD_3_BYTE_0                 0x09DC
#define SX128X_REG_CRC_INITIAL_MSB                    0x09C8
#define SX128X_REG_CRC_INITIAL_LSB                    0x09C9
#define SX128X_REG_CRC_POLYNOMIAL_MSB                 0x09C6
#define SX128X_REG_CRC_POLYNOMIAL_LSB                 0x09C7
#define SX128X_REG_ACCESS_ADDRESS_BYTE_3              (SX128X_REG_SYNC_WORD_1_BYTE_3)
#define SX128X_REG_ACCESS_ADDRESS_BYTE_2              (SX128X_REG_SYNC_WORD_1_BYTE_2)
#define SX128X_REG_ACCESS_ADDRESS_BYTE_1              (SX128X_REG_SYNC_WORD_1_BYTE_1)
#define SX128X_REG_ACCESS_ADDRESS_BYTE_0              (SX128X_REG_SYNC_WORD_1_BYTE_0)
#define SX128X_REG_BLE_CRC_INITIAL_MSB                0x09C7
#define SX128X_REG_BLE_CRC_INITIAL_MID                (SX128X_REG_CRC_INITIAL_MSB)
#define SX128X_REG_BLE_CRC_INITIAL_LSB                (SX128X_REG_CRC_INITIAL_LSB)
#define SX128X_REG_SLAVE_RANGING_ADDRESS_BYTE_3       0x0916
#define SX128X_REG_SLAVE_RANGING_ADDRESS_BYTE_2       0x0917
#define SX128X_REG_SLAVE_RANGING_ADDRESS_BYTE_1       0x0918
#define SX128X_REG_SLAVE_RANGING_ADDRESS_BYTE_0       0x0919
#define SX128X_REG_SLAVE_RANGING_ADDRESS_WIDTH        0x0931
#define SX128X_REG_MASTER_RANGING_ADDRESS_BYTE_3      0x0912
#define SX128X_REG_MASTER_RANGING_ADDRESS_BYTE_2      0x0913
#define SX128X_REG_MASTER_RANGING_ADDRESS_BYTE_1      0x0914
#define SX128X_REG_MASTER_RANGING_ADDRESS_BYTE_0      0x0915
#define SX128X_REG_RANGING_CALIBRATION_MSB            0x092C
#define SX128X_REG_RANGING_CALIBRATION_LSB            0x092D
#define SX128X_REG_RANGING_RESULT_MSB                 0x0961
#define SX128X_REG_RANGING_RESULT_MID                 0x0962
#define SX128X_REG_RANGING_RESULT_LSB                 0x0963
#define SX128X_REG_MANUAL_GAIN_CONTROL_ENABLE_1       0x089F
#define SX128X_REG_MANUAL_GAIN_CONTROL_ENABLE_2       0x0895
#define SX128X_REG_MANUAL_GAIN_SETTING                0x089E
#define SX128X_REG_GAIN_MODE                          0x0891
#define SX128X_REG_LORA_FIXED_PAYLOAD_LENGTH          0x0901
#define SX128X_REG_LORA_SF_CONFIG                     0x0925
#define SX128X_REG_FEI_MSB                            0x0954
#define SX128X_REG_FEI_MID                            0x0955
#define SX128X_REG_FEI_LSB                            0x0956
#define SX128X_REG_RANGING_FILTER_WINDOW_SIZE         0x091E
#define SX128X_REG_RANGING_FILTER_RSSI_OFFSET         0x0953
#define SX128X_REG_RANGING_FILTER_RESET               0x0923
#define SX128X_REG_RANGING_LORA_CLOCK_ENABLE          0x097F
#define SX128X_REG_RANGING_TYPE                       0x0924
#define SX128X_REG_RANGING_ADDRESS_SWITCH             0x0927
#define SX128X_REG_RANGING_ADDRESS_MSB                0x095F
#define SX128X_REG_RANGING_ADDRESS_LSB                0x0960


// SX128X SPI command variables
//SX128X_CMD_GET_STATUS                                               MSB   LSB   DESCRIPTION
#define SX128X_STATUS_MODE_STDBY_RC                   0b01000000  //  7     5     current chip mode: STDBY_RC
#define SX128X_STATUS_MODE_STDBY_XOSC                 0b01100000  //  7     5                        STDBY_XOSC
#define SX128X_STATUS_MODE_FS                         0b10000000  //  7     5                        FS
#define SX128X_STATUS_MODE_RX                         0b10100000  //  7     5                        Rx
#define SX128X_STATUS_MODE_TX                         0b11000000  //  7     5                        Tx
#define SX128X_STATUS_CMD_PROCESSED                   0b00000100  //  4     2     command status: processing OK
#define SX128X_STATUS_DATA_AVAILABLE                  0b00001000  //  4     2                     data available
#define SX128X_STATUS_CMD_TIMEOUT                     0b00001100  //  4     2                     timeout
#define SX128X_STATUS_CMD_ERROR                       0b00010000  //  4     2                     processing error
#define SX128X_STATUS_CMD_FAILED                      0b00010100  //  4     2                     failed to execute
#define SX128X_STATUS_TX_DONE                         0b00011000  //  4     2                     transmission finished
#define SX128X_STATUS_BUSY                            0b00000001  //  0     0     chip busy
#define SX128X_STATUS_SPI_FAILED                      0b11111111  //  7     0     SPI transaction failed

//SX128X_CMD_SET_SLEEP
#define SX128X_SLEEP_DATA_BUFFER_FLUSH                0b00000000  //  1     1     data buffer behavior in sleep mode: flush
#define SX128X_SLEEP_DATA_BUFFER_RETAIN               0b00000010  //  1     1                                         retain
#define SX128X_SLEEP_DATA_RAM_FLUSH                   0b00000000  //  0     0     data RAM (configuration) behavior in sleep mode: flush
#define SX128X_SLEEP_DATA_RAM_RETAIN                  0b00000001  //  0     0                                                      retain

//SX128X_CMD_SET_STANDBY
#define SX128X_STANDBY_RC                             0x00        //  7     0     standby mode: 13 MHz RC oscillator
#define SX128X_STANDBY_XOSC                           0x01        //  7     0                   52 MHz crystal oscillator

//SX128X_CMD_SET_TX + SX128X_CMD_SET_RX + SX128X_CMD_SET_RX_DUTY_CYCLE
#define SX128X_PERIOD_BASE_15_625_US                  0x00        //  7     0     time period step: 15.625 us
#define SX128X_PERIOD_BASE_62_5_US                    0x01        //  7     0                       62.5 us
#define SX128X_PERIOD_BASE_1_MS                       0x02        //  7     0                       1 ms
#define SX128X_PERIOD_BASE_4_MS                       0x03        //  7     0                       4 ms

//SX128X_CMD_SET_TX
#define SX128X_TX_TIMEOUT_NONE                        0x0000      //  15    0     Tx timeout duration: no timeout (Tx single mode)

//SX128X_CMD_SET_RX
#define SX128X_RX_TIMEOUT_NONE                        0x0000      //  15    0     Rx timeout duration: no timeout (Rx single mode)
#define SX128X_RX_TIMEOUT_INF                         0xFFFF      //  15    0                          infinite (Rx continuous mode)

//SX128X_CMD_SET_PACKET_TYPE
#define SX128X_PACKET_TYPE_GFSK                       0x00        //  7     0     packet type: (G)FSK
#define SX128X_PACKET_TYPE_LORA                       0x01        //  7     0                  LoRa
#define SX128X_PACKET_TYPE_RANGING                    0x02        //  7     0                  ranging engine
#define SX128X_PACKET_TYPE_FLRC                       0x03        //  7     0                  FLRC
#define SX128X_PACKET_TYPE_BLE                        0x04        //  7     0                  BLE

//SX128X_CMD_SET_TX_PARAMS
#define SX128X_PA_RAMP_02_US                          0x00        //  7     0     PA ramp time: 2 us
#define SX128X_PA_RAMP_04_US                          0x20        //  7     0                   4 us
#define SX128X_PA_RAMP_06_US                          0x40        //  7     0                   6 us
#define SX128X_PA_RAMP_08_US                          0x60        //  7     0                   8 us
#define SX128X_PA_RAMP_10_US                          0x80        //  7     0                   10 us
#define SX128X_PA_RAMP_12_US                          0xA0        //  7     0                   12 us
#define SX128X_PA_RAMP_16_US                          0xC0        //  7     0                   16 us
#define SX128X_PA_RAMP_20_US                          0xE0        //  7     0                   20 us

//SX128X_CMD_SET_CAD_PARAMS
#define SX128X_CAD_ON_1_SYMB                          0x00        //  7     0     number of symbols used for CAD: 1
#define SX128X_CAD_ON_2_SYMB                          0x20        //  7     0                                     2
#define SX128X_CAD_ON_4_SYMB                          0x40        //  7     0                                     4
#define SX128X_CAD_ON_8_SYMB                          0x60        //  7     0                                     8
#define SX128X_CAD_ON_16_SYMB                         0x80        //  7     0                                     16

//SX128X_CMD_SET_MODULATION_PARAMS
#define SX128X_BLE_GFSK_BR_2_000_BW_2_4               0x04        //  7     0     GFSK/BLE bit rate and bandwidth setting: 2.0 Mbps   2.4 MHz
#define SX128X_BLE_GFSK_BR_1_600_BW_2_4               0x28        //  7     0                                              1.6 Mbps   2.4 MHz
#define SX128X_BLE_GFSK_BR_1_000_BW_2_4               0x4C        //  7     0                                              1.0 Mbps   2.4 MHz
#define SX128X_BLE_GFSK_BR_1_000_BW_1_2               0x45        //  7     0                                              1.0 Mbps   1.2 MHz
#define SX128X_BLE_GFSK_BR_0_800_BW_2_4               0x70        //  7     0                                              0.8 Mbps   2.4 MHz
#define SX128X_BLE_GFSK_BR_0_800_BW_1_2               0x69        //  7     0                                              0.8 Mbps   1.2 MHz
#define SX128X_BLE_GFSK_BR_0_500_BW_1_2               0x8D        //  7     0                                              0.5 Mbps   1.2 MHz
#define SX128X_BLE_GFSK_BR_0_500_BW_0_6               0x86        //  7     0                                              0.5 Mbps   0.6 MHz
#define SX128X_BLE_GFSK_BR_0_400_BW_1_2               0xB1        //  7     0                                              0.4 Mbps   1.2 MHz
#define SX128X_BLE_GFSK_BR_0_400_BW_0_6               0xAA        //  7     0                                              0.4 Mbps   0.6 MHz
#define SX128X_BLE_GFSK_BR_0_250_BW_0_6               0xCE        //  7     0                                              0.25 Mbps  0.6 MHz
#define SX128X_BLE_GFSK_BR_0_250_BW_0_3               0xC7        //  7     0                                              0.25 Mbps  0.3 MHz
#define SX128X_BLE_GFSK_BR_0_125_BW_0_3               0xEF        //  7     0                                              0.125 Mbps 0.3 MHz
#define SX128X_BLE_GFSK_MOD_IND_0_35                  0x00        //  7     0     GFSK/BLE modulation index: 0.35
#define SX128X_BLE_GFSK_MOD_IND_0_50                  0x01        //  7     0                                0.50
#define SX128X_BLE_GFSK_MOD_IND_0_75                  0x02        //  7     0                                0.75
#define SX128X_BLE_GFSK_MOD_IND_1_00                  0x03        //  7     0                                1.00
#define SX128X_BLE_GFSK_MOD_IND_1_25                  0x04        //  7     0                                1.25
#define SX128X_BLE_GFSK_MOD_IND_1_50                  0x05        //  7     0                                1.50
#define SX128X_BLE_GFSK_MOD_IND_1_75                  0x06        //  7     0                                1.75
#define SX128X_BLE_GFSK_MOD_IND_2_00                  0x07        //  7     0                                2.00
#define SX128X_BLE_GFSK_MOD_IND_2_25                  0x08        //  7     0                                2.25
#define SX128X_BLE_GFSK_MOD_IND_2_50                  0x09        //  7     0                                2.50
#define SX128X_BLE_GFSK_MOD_IND_2_75                  0x0A        //  7     0                                2.75
#define SX128X_BLE_GFSK_MOD_IND_3_00                  0x0B        //  7     0                                3.00
#define SX128X_BLE_GFSK_MOD_IND_3_25                  0x0C        //  7     0                                3.25
#define SX128X_BLE_GFSK_MOD_IND_3_50                  0x0D        //  7     0                                3.50
#define SX128X_BLE_GFSK_MOD_IND_3_75                  0x0E        //  7     0                                3.75
#define SX128X_BLE_GFSK_MOD_IND_4_00                  0x0F        //  7     0                                4.00
#define SX128X_BLE_GFSK_BT_OFF                        0x00        //  7     0     GFSK Gaussian filter BT product: filter disabled
#define SX128X_BLE_GFSK_BT_1_0                        0x10        //  7     0                                      1.0
#define SX128X_BLE_GFSK_BT_0_5                        0x20        //  7     0                                      0.5
#define SX128X_FLRC_BR_1_300_BW_1_2                   0x45        //  7     0     FLRC bit rate and bandwidth setting: 1.3 Mbps   1.2 MHz
#define SX128X_FLRC_BR_1_000_BW_1_2                   0x69        //  7     0                                          1.04 Mbps  1.2 MHz
#define SX128X_FLRC_BR_0_650_BW_0_6                   0x86        //  7     0                                          0.65 Mbps  0.6 MHz
#define SX128X_FLRC_BR_0_520_BW_0_6                   0xAA        //  7     0                                          0.52 Mbps  0.6 MHz
#define SX128X_FLRC_BR_0_325_BW_0_3                   0xC7        //  7     0                                          0.325 Mbps 0.3 MHz
#define SX128X_FLRC_BR_0_260_BW_0_3                   0xEB        //  7     0                                          0.260 Mbps 0.3 MHz
#define SX128X_FLRC_CR_1_2                            0x00        //  7     0     FLRC coding rate: 1/2
#define SX128X_FLRC_CR_3_4                            0x02        //  7     0                       3/4
#define SX128X_FLRC_CR_1_0                            0x04        //  7     0                       1/1
#define SX128X_FLRC_BT_OFF                            0x00        //  7     0     FLRC Gaussian filter BT product: filter disabled
#define SX128X_FLRC_BT_1_0                            0x10        //  7     0                                      1.0
#define SX128X_FLRC_BT_0_5                            0x20        //  7     0                                      0.5
#define SX128X_LORA_SF_5                              0x50        //  7     0     LoRa spreading factor: 5
#define SX128X_LORA_SF_6                              0x60        //  7     0                            6
#define SX128X_LORA_SF_7                              0x70        //  7     0                            7
#define SX128X_LORA_SF_8                              0x80        //  7     0                            8
#define SX128X_LORA_SF_9                              0x90        //  7     0                            9
#define SX128X_LORA_SF_10                             0xA0        //  7     0                            10
#define SX128X_LORA_SF_11                             0xB0        //  7     0                            11
#define SX128X_LORA_SF_12                             0xC0        //  7     0                            12
#define SX128X_LORA_BW_1625_00                        0x0A        //  7     0     LoRa bandwidth: 1625.0 kHz
#define SX128X_LORA_BW_812_50                         0x18        //  7     0                     812.5 kHz
#define SX128X_LORA_BW_406_25                         0x26        //  7     0                     406.25 kHz
#define SX128X_LORA_BW_203_125                        0x34        //  7     0                     203.125 kHz
#define SX128X_LORA_CR_4_5                            0x01        //  7     0     LoRa coding rate: 4/5
#define SX128X_LORA_CR_4_6                            0x02        //  7     0                       4/6
#define SX128X_LORA_CR_4_7                            0x03        //  7     0                       4/7
#define SX128X_LORA_CR_4_8                            0x04        //  7     0                       4/8
#define SX128X_LORA_CR_4_5_LI                         0x05        //  7     0                       4/5, long interleaving
#define SX128X_LORA_CR_4_6_LI                         0x06        //  7     0                       4/6, long interleaving
#define SX128X_LORA_CR_4_7_LI                         0x07        //  7     0                       4/7, long interleaving

//SX128X_CMD_SET_PACKET_PARAMS
#define SX128X_GFSK_FLRC_SYNC_WORD_OFF                0x00        //  7     0     GFSK/FLRC sync word used: none
#define SX128X_GFSK_FLRC_SYNC_WORD_1                  0x10        //  7     0                               sync word 1
#define SX128X_GFSK_FLRC_SYNC_WORD_2                  0x20        //  7     0                               sync word 2
#define SX128X_GFSK_FLRC_SYNC_WORD_1_2                0x30        //  7     0                               sync words 1 and 2
#define SX128X_GFSK_FLRC_SYNC_WORD_3                  0x40        //  7     0                               sync word 3
#define SX128X_GFSK_FLRC_SYNC_WORD_1_3                0x50        //  7     0                               sync words 1 and 3
#define SX128X_GFSK_FLRC_SYNC_WORD_2_3                0x60        //  7     0                               sync words 2 and 3
#define SX128X_GFSK_FLRC_SYNC_WORD_1_2_3              0x70        //  7     0                               sync words 1, 2 and 3
#define SX128X_GFSK_FLRC_PACKET_FIXED                 0x00        //  7     0     GFSK/FLRC packet length mode: fixed
#define SX128X_GFSK_FLRC_PACKET_VARIABLE              0x20        //  7     0                                   variable
#define SX128X_GFSK_FLRC_CRC_OFF                      0x00        //  7     0     GFSK/FLRC packet CRC: none
#define SX128X_GFSK_FLRC_CRC_1_BYTE                   0x10        //  7     0                           1 byte
#define SX128X_GFSK_FLRC_CRC_2_BYTE                   0x20        //  7     0                           2 bytes
#define SX128X_GFSK_FLRC_CRC_3_BYTE                   0x30        //  7     0                           3 bytes (FLRC only)
#define SX128X_GFSK_BLE_WHITENING_ON                  0x00        //  7     0     GFSK/BLE whitening: enabled
#define SX128X_GFSK_BLE_WHITENING_OFF                 0x08        //  7     0                         disabled
#define SX128X_BLE_PAYLOAD_LENGTH_MAX_31              0x00        //  7     0     BLE maximum payload length: 31 bytes
#define SX128X_BLE_PAYLOAD_LENGTH_MAX_37              0x20        //  7     0                                 37 bytes
#define SX128X_BLE_PAYLOAD_LENGTH_TEST                0x40        //  7     0                                 63 bytes (test mode)
#define SX128X_BLE_PAYLOAD_LENGTH_MAX_255             0x80        //  7     0                                 255 bytes (Bluetooth 4.2 and above)
#define SX128X_BLE_CRC_OFF                            0x00        //  7     0     BLE packet CRC: none
#define SX128X_BLE_CRC_3_BYTE                         0x10        //  7     0                     3 byte
#define SX128X_BLE_PRBS_9                             0x00        //  7     0     BLE test payload contents: PRNG sequence using x^9 + x^5 + x
#define SX128X_BLE_EYELONG                            0x04        //  7     0                                repeated 0xF0
#define SX128X_BLE_EYESHORT                           0x08        //  7     0                                repeated 0xAA
#define SX128X_BLE_PRBS_15                            0x0C        //  7     0                                PRNG sequence using x^15 + x^14 + x^13 + x^12 + x^2 + x + 1
#define SX128X_BLE_ALL_1                              0x10        //  7     0                                repeated 0xFF
#define SX128X_BLE_ALL_0                              0x14        //  7     0                                repeated 0x00
#define SX128X_BLE_EYELONG_INV                        0x18        //  7     0                                repeated 0x0F
#define SX128X_BLE_EYESHORT_INV                       0x1C        //  7     0                                repeated 0x55
#define SX128X_FLRC_SYNC_WORD_OFF                     0x00        //  7     0     FLRC sync word: disabled
#define SX128X_FLRC_SYNC_WORD_ON                      0x04        //  7     0                     enabled
#define SX128X_LORA_HEADER_EXPLICIT                   0x00        //  7     0     LoRa header mode: explicit
#define SX128X_LORA_HEADER_IMPLICIT                   0x80        //  7     0                       implicit
#define SX128X_LORA_CRC_OFF                           0x00        //  7     0     LoRa packet CRC: disabled
#define SX128X_LORA_CRC_ON                            0x20        //  7     0                      enabled
#define SX128X_LORA_IQ_STANDARD                       0x40        //  7     0     LoRa IQ: standard
#define SX128X_LORA_IQ_INVERTED                       0x00        //  7     0              inverted

//SX128X_CMD_GET_PACKET_STATUS
#define SX128X_PACKET_STATUS_SYNC_ERROR               0b01000000  //  6     6     packet status errors byte: sync word error
#define SX128X_PACKET_STATUS_LENGTH_ERROR             0b00100000  //  5     5                                packet length error
#define SX128X_PACKET_STATUS_CRC_ERROR                0b00010000  //  4     4                                CRC error
#define SX128X_PACKET_STATUS_ABORT_ERROR              0b00001000  //  3     3                                packet reception aborted
#define SX128X_PACKET_STATUS_HEADER_RECEIVED          0b00000100  //  2     2                                header received
#define SX128X_PACKET_STATUS_PACKET_RECEIVED          0b00000010  //  1     1                                packet received
#define SX128X_PACKET_STATUS_PACKET_CTRL_BUSY         0b00000001  //  0     0                                packet controller is busy
#define SX128X_PACKET_STATUS_RX_PID                   0b11000000  //  7     6     packet status status byte: PID field of the received packet
#define SX128X_PACKET_STATUS_NO_ACK                   0b00100000  //  5     5                                NO_ACK field of the received packet
#define SX128X_PACKET_STATUS_RX_PID_ERROR             0b00010000  //  4     4                                PID field error
#define SX128X_PACKET_STATUS_PACKET_SENT              0b00000001  //  0     0                                packet sent
#define SX128X_PACKET_STATUS_SYNC_DET_ERROR           0b00000000  //  2     0     packet status sync byte: sync word detection error
#define SX128X_PACKET_STATUS_SYNC_DET_1               0b00000001  //  2     0                              detected sync word 1
#define SX128X_PACKET_STATUS_SYNC_DET_2               0b00000010  //  2     0                              detected sync word 2
#define SX128X_PACKET_STATUS_SYNC_DET_3               0b00000100  //  2     0                              detected sync word 3

//SX128X_CMD_SET_DIO_IRQ_PARAMS
#define SX128X_IRQ_PREAMBLE_DETECTED                  0x8000      //  15    15    interrupt source: preamble detected
#define SX128X_IRQ_ADVANCED_RANGING_DONE              0x8000      //  15    15                      advanced ranging done
#define SX128X_IRQ_RX_TX_TIMEOUT                      0x4000      //  14    14                      Rx or Tx timeout
#define SX128X_IRQ_CAD_DETECTED                       0x2000      //  13    13                      channel activity detected
#define SX128X_IRQ_CAD_DONE                           0x1000      //  12    12                      CAD finished
#define SX128X_IRQ_RANGING_SLAVE_REQ_VALID            0x0800      //  11    11                      ranging request valid (slave)
#define SX128X_IRQ_RANGING_MASTER_TIMEOUT             0x0400      //  10    10                      ranging timeout (master)
#define SX128X_IRQ_RANGING_MASTER_RES_VALID           0x0200      //  9     9                       ranging result valid (master)
#define SX128X_IRQ_RANGING_SLAVE_REQ_DISCARD          0x0100      //  8     8                       ranging result valid (master)
#define SX128X_IRQ_RANGING_SLAVE_RESP_DONE            0x0080      //  7     7                       ranging response complete (slave)
#define SX128X_IRQ_CRC_ERROR                          0x0040      //  6     6                       CRC error
#define SX128X_IRQ_HEADER_ERROR                       0x0020      //  5     5                       header error
#define SX128X_IRQ_HEADER_VALID                       0x0010      //  4     4                       header valid
#define SX128X_IRQ_SYNC_WORD_ERROR                    0x0008      //  3     3                       sync word error
#define SX128X_IRQ_SYNC_WORD_VALID                    0x0004      //  2     2                       sync word valid
#define SX128X_IRQ_RX_DONE                            0x0002      //  1     1                       Rx done
#define SX128X_IRQ_TX_DONE                            0x0001      //  0     0                       Tx done
#define SX128X_IRQ_NONE                               0x0000      //  15    0                       none
#define SX128X_IRQ_ALL                                0xFFFF      //  15    0                       all

//SX128X_CMD_SET_REGULATOR_MODE
#define SX128X_REGULATOR_LDO                          0x00        //  7     0     set regulator mode: LDO (default)
#define SX128X_REGULATOR_DC_DC                        0x01        //  7     0                         DC-DC

//SX128X_CMD_SET_RANGING_ROLE
#define SX128X_RANGING_ROLE_MASTER                    0x01        //  7     0     ranging role: master
#define SX128X_RANGING_ROLE_SLAVE                     0x00        //  7     0                   slave


/*!
  \class SX128x

  \brief Base class for %SX128x series. All derived classes for %SX128x (e.g. SX1280 or SX1281) inherit from this base class.
  This class should not be instantiated directly from Arduino sketch, only from its derived classes.
*/
class SX128x: public PhysicalLayer {
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
    SX128x(Module* mod);

    // basic methods

    /*!
      \brief Initialization method for LoRa modem.

      \param freq Carrier frequency in MHz. Defaults to 2400.0 MHz.

      \param bw LoRa bandwidth in kHz. Defaults to 812.5 kHz.

      \param sf LoRa spreading factor. Defaults to 9.

      \param cr LoRa coding rate denominator. Defaults to 7 (coding rate 4/7).

      \param power Output power in dBm. Defaults to 10 dBm.

      \param preambleLength LoRa preamble length in symbols. Defaults to 12 symbols.

      \returns \ref status_codes
    */
    int16_t begin(float freq = 2400.0, float bw = 812.5, uint8_t sf = 9, uint8_t cr = 7, int8_t power = 10, uint16_t preambleLength = 12);

    /*!
      \brief Initialization method for GFSK modem.

      \param freq Carrier frequency in MHz. Defaults to 2400.0 MHz.

      \param br FSK bit rate in kbps. Defaults to 800 kbps.

      \param freqDev Frequency deviation from carrier frequency in kHz. Defaults to 400.0 kHz.

      \param power Output power in dBm. Defaults to 10 dBm.

      \parma preambleLength FSK preamble length in bits. Defaults to 16 bits.

      \returns \ref status_codes
    */
    int16_t beginGFSK(float freq = 2400.0, uint16_t br = 800, float freqDev = 400.0, int8_t power = 10, uint16_t preambleLength = 16);

    /*!
      \brief Initialization method for BLE modem.

      \param freq Carrier frequency in MHz. Defaults to 2400.0 MHz.

      \param br BLE bit rate in kbps. Defaults to 800 kbps.

      \param freqDev Frequency deviation from carrier frequency in kHz. Defaults to 400.0 kHz.

      \param power Output power in dBm. Defaults to 10 dBm.

      \param dataShaping Time-bandwidth product of the Gaussian filter to be used for shaping. Defaults to 0.5.

      \returns \ref status_codes
    */
    int16_t beginBLE(float freq = 2400.0, uint16_t br = 800, float freqDev = 400.0, int8_t power = 10, uint8_t dataShaping = RADIOLIB_SHAPING_0_5);

    /*!
      \brief Initialization method for FLRC modem.

      \param freq Carrier frequency in MHz. Defaults to 2400.0 MHz.

      \param br FLRC bit rate in kbps. Defaults to 650 kbps.

      \param cr FLRC coding rate. Defaults to 3 (coding rate 3/4).

      \param power Output power in dBm. Defaults to 10 dBm.

      \parma preambleLength FLRC preamble length in bits. Defaults to 16 bits.

      \param dataShaping Time-bandwidth product of the Gaussian filter to be used for shaping. Defaults to 0.5.

      \returns \ref status_codes
    */
    int16_t beginFLRC(float freq = 2400.0, uint16_t br = 650, uint8_t cr = 3, int8_t power = 10, uint16_t preambleLength = 16, uint8_t dataShaping = RADIOLIB_SHAPING_0_5);

    /*!
      \brief Reset method. Will reset the chip to the default state using RST pin.

      \param verify Whether correct module startup should be verified. When set to true, RadioLib will attempt to verify the module has started correctly
      by repeatedly issuing setStandby command. Enabled by default.

      \returns \ref status_codes
    */
    int16_t reset(bool verify = true);

    /*!
      \brief Blocking binary transmit method.
      Overloads for string-based transmissions are implemented in PhysicalLayer.

      \param data Binary data to be sent.

      \param len Number of bytes to send.

      \param addr Address to send the data to. Unsupported, compatibility only.

      \returns \ref status_codes
    */
    int16_t transmit(uint8_t* data, size_t len, uint8_t addr = 0) override;

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
      \brief Starts direct mode reception. Only implemented for PhysicalLayer compatibility, as %SX128x series does not support direct mode reception.
      Will always return ERR_UNKNOWN.

      \returns \ref status_codes
    */
    int16_t receiveDirect() override;

    /*!
      \brief Performs scan for LoRa transmission in the current channel. Detects both preamble and payload.

      \returns \ref status_codes
    */
    int16_t scanChannel();

    /*!
      \brief Sets the module to sleep mode.

      \param retainConfig Set to true to retain configuration and data buffer or to false to discard current configuration and data buffer. Defaults to true.

      \returns \ref status_codes
    */
    int16_t sleep(bool retainConfig = true);

    /*!
      \brief Sets the module to standby mode (overload for PhysicalLayer compatibility, uses 13 MHz RC oscillator).

      \returns \ref status_codes
    */
    int16_t standby() override;

    /*!
      \brief Sets the module to standby mode.

      \param mode Oscillator to be used in standby mode. Can be set to SX128X_STANDBY_RC (13 MHz RC oscillator) or SX128X_STANDBY_XOSC (52 MHz external crystal oscillator).

      \returns \ref status_codes
    */
    int16_t standby(uint8_t mode);

    // interrupt methods

    /*!
      \brief Sets interrupt service routine to call when DIO1 activates.

      \param func ISR to call.
    */
    void setDio1Action(void (*func)(void));

    /*!
      \brief Clears interrupt service routine to call when DIO1 activates.
    */
    void clearDio1Action();

    /*!
      \brief Interrupt-driven binary transmit method.
      Overloads for string-based transmissions are implemented in PhysicalLayer.

      \param data Binary data to be sent.

      \param len Number of bytes to send.

      \param addr Address to send the data to. Unsupported, compatibility only.

      \returns \ref status_codes
    */
    int16_t startTransmit(uint8_t* data, size_t len, uint8_t addr = 0) override;

    /*!
      \brief Interrupt-driven receive method. DIO1 will be activated when full packet is received.

      \param timeout Raw timeout value, expressed as multiples of 15.625 us. Defaults to SX128X_RX_TIMEOUT_INF for infinite timeout (Rx continuous mode), set to SX128X_RX_TIMEOUT_NONE for no timeout (Rx single mode).

      \returns \ref status_codes
    */
    int16_t startReceive(uint16_t timeout = SX128X_RX_TIMEOUT_INF);

    /*!
      \brief Reads data received after calling startReceive method.

      \param data Pointer to array to save the received binary data.

      \param len Number of bytes that will be received. Must be known in advance for binary transmissions.

      \returns \ref status_codes
    */
    int16_t readData(uint8_t* data, size_t len) override;

    // configuration methods

    /*!
      \brief Sets carrier frequency. Allowed values are in range from 2400.0 to 2500.0 MHz.

      \param freq Carrier frequency to be set in MHz.

      \returns \ref status_codes
    */
    int16_t setFrequency(float freq);

    /*!
      \brief Sets LoRa bandwidth. Allowed values are 203.125, 406.25, 812.5 and 1625.0 kHz.

      \param bw LoRa bandwidth to be set in kHz.

      \returns \ref status_codes
    */
    int16_t setBandwidth(float bw);

    /*!
      \brief Sets LoRa spreading factor. Allowed values range from 5 to 12.

      \param sf LoRa spreading factor to be set.

      \returns \ref status_codes
    */
    int16_t setSpreadingFactor(uint8_t sf);

    /*!
      \brief Sets LoRa coding rate denominator. Allowed values range from 5 to 8.

      \param cr LoRa coding rate denominator to be set.

      \param longInterleaving Whether to enable long interleaving mode. Not available for coding rate 4/7, defaults to false.

      \returns \ref status_codes
    */
    int16_t setCodingRate(uint8_t cr, bool longInterleaving = false);

    /*!
      \brief Sets output power. Allowed values are in range from -18 to 13 dBm.

      \param power Output power to be set in dBm.

      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power);

    /*!
      \brief Sets preamble length for currently active modem. Allowed values range from 1 to 65535.

      \param preambleLength Preamble length to be set in symbols (LoRa) or bits (FSK/BLE/FLRC).

      \returns \ref status_codes
    */
    int16_t setPreambleLength(uint32_t preambleLength);

    /*!
      \brief Sets FSK or FLRC bit rate. Allowed values are 125, 250, 400, 500, 800, 1000, 1600 and 2000 kbps (for FSK modem) or 260, 325, 520, 650, 1000 and 1300 (for FLRC modem).

      \param br FSK/FLRC bit rate to be set in kbps.

      \returns \ref status_codes
    */
    int16_t setBitRate(uint16_t br);

    /*!
      \brief Sets FSK frequency deviation. Allowed values range from 0.0 to 3200.0 kHz.

      \param freqDev FSK frequency deviation to be set in kHz.

      \returns \ref status_codes
    */
    int16_t setFrequencyDeviation(float freqDev) override;

    /*!
      \brief Sets time-bandwidth product of Gaussian filter applied for shaping.
      Allowed values are RADIOLIB_SHAPING_0_5 or RADIOLIB_SHAPING_1_0. Set to RADIOLIB_SHAPING_NONE to disable data shaping.

      \param sh Time-bandwidth product of Gaussian filter to be set.

      \returns \ref status_codes
    */
    int16_t setDataShaping(uint8_t sh) override;

    /*!
      \brief Sets FSK/FLRC sync word in the form of array of up to 5 bytes (FSK). For FLRC modem, the sync word must be exactly 4 bytes long

      \param syncWord Sync word to be set.

      \param len Sync word length in bytes.

      \returns \ref status_codes
    */
    int16_t setSyncWord(uint8_t* syncWord, uint8_t len);

    /*!
      \brief Sets CRC configuration.

      \param len CRC length in bytes, Allowed values are 1, 2 or 3, set to 0 to disable CRC.

      \param initial Initial CRC value. Defaults to 0x1D0F (CCIT CRC), not available for LoRa modem.

      \param polynomial Polynomial for CRC calculation. Defaults to 0x1021 (CCIT CRC), not available for LoRa or BLE modem.

      \returns \ref status_codes
    */
    int16_t setCRC(uint8_t len, uint32_t initial = 0x1D0F, uint16_t polynomial = 0x1021);

    /*!
      \brief Sets whitening parameters, not available for LoRa or FLRC modem.

      \param enabled Set to true to enable whitening.

      \returns \ref status_codes
    */
    int16_t setWhitening(bool enabled);

    /*!
      \brief Sets BLE access address.

      \param addr BLE access address.

      \returns \ref status_codes
    */
    int16_t setAccessAddress(uint32_t addr);

    /*!
      \brief Gets RSSI (Recorded Signal Strength Indicator) of the last received packet.

      \returns RSSI of the last received packet in dBm.
    */
    float getRSSI();

    /*!
      \brief Gets SNR (Signal to Noise Ratio) of the last received packet. Only available for LoRa or ranging modem.

      \returns SNR of the last received packet in dB.
    */
    float getSNR();

    /*!
      \brief Query modem for the packet length of received payload.

      \param update Update received packet length. Will return cached value when set to false.

      \returns Length of last received packet in bytes.
    */
    size_t getPacketLength(bool update = true) override;

    /*!
      \brief Get expected time-on-air for a given size of payload.

      \param len Payload length in bytes.

      \returns Expected time-on-air in microseconds.
    */
    uint32_t getTimeOnAir(size_t len);

    /*!
      \brief Set implicit header mode for future reception/transmission.

      \returns \ref status_codes
    */
    int16_t implicitHeader(size_t len);

    /*!
      \brief Set explicit header mode for future reception/transmission.

      \param len Payload length in bytes.

      \returns \ref status_codes
    */
    int16_t explicitHeader();

    /*!
      \brief Sets transmission encoding. Serves only as alias for PhysicalLayer compatibility.

      \param encoding Encoding to be used. Set to 0 for NRZ, and 2 for whitening.

      \returns \ref status_codes
    */
    int16_t setEncoding(uint8_t encoding) override;

    /*!
      \brief Some modules contain external RF switch controlled by two pins. This function gives RadioLib control over those two pins to automatically switch Rx and Tx state.
      When using automatic RF switch control, DO NOT change the pin mode of rxEn or txEn from Arduino sketch!

      \param rxEn RX enable pin.

      \param txEn TX enable pin.
    */
    void setRfSwitchPins(RADIOLIB_PIN_TYPE rxEn, RADIOLIB_PIN_TYPE txEn);

    /*!
     \brief Dummy random method, to ensure PhysicalLayer compatibility.

     \returns Always returns 0.
   */
    uint8_t random();

#ifndef RADIOLIB_GODMODE
  protected:
#endif
    Module* _mod;

    // cached LoRa parameters
    float _bwKhz = 0;
    uint8_t _bw = 0, _sf = 0, _cr = 0;
    uint8_t _preambleLengthLoRa = 0, _headerType = 0, _payloadLen = 0, _crcLoRa = 0;

    // SX128x SPI command implementations
    uint8_t getStatus();
    int16_t writeRegister(uint16_t addr, uint8_t* data, uint8_t numBytes);
    int16_t readRegister(uint16_t addr, uint8_t* data, uint8_t numBytes);
    int16_t writeBuffer(uint8_t* data, uint8_t numBytes, uint8_t offset = 0x00);
    int16_t readBuffer(uint8_t* data, uint8_t numBytes);
    int16_t setTx(uint16_t periodBaseCount = SX128X_TX_TIMEOUT_NONE, uint8_t periodBase = SX128X_PERIOD_BASE_15_625_US);
    int16_t setRx(uint16_t periodBaseCount, uint8_t periodBase = SX128X_PERIOD_BASE_15_625_US);
    int16_t setCad();
    uint8_t getPacketType();
    int16_t setRfFrequency(uint32_t frf);
    int16_t setTxParams(uint8_t power, uint8_t rampTime = SX128X_PA_RAMP_10_US);
    int16_t setBufferBaseAddress(uint8_t txBaseAddress = 0x00, uint8_t rxBaseAddress = 0x00);
    int16_t setModulationParams(uint8_t modParam1, uint8_t modParam2, uint8_t modParam3);
    int16_t setPacketParamsGFSK(uint8_t preambleLen, uint8_t syncWordLen, uint8_t syncWordMatch, uint8_t crcLen, uint8_t whitening, uint8_t payloadLen = 0xFF, uint8_t headerType = SX128X_GFSK_FLRC_PACKET_VARIABLE);
    int16_t setPacketParamsBLE(uint8_t connState, uint8_t crcLen, uint8_t bleTestPayload, uint8_t whitening);
    int16_t setPacketParamsLoRa(uint8_t preambleLen, uint8_t headerType, uint8_t payloadLen, uint8_t crc, uint8_t invertIQ = SX128X_LORA_IQ_STANDARD);
    int16_t setDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask = SX128X_IRQ_NONE, uint16_t dio3Mask = SX128X_IRQ_NONE);
    uint16_t getIrqStatus();
    int16_t clearIrqStatus(uint16_t clearIrqParams = SX128X_IRQ_ALL);
    int16_t setRangingRole(uint8_t role);
    int16_t setPacketType(uint8_t type);

    int16_t setHeaderType(uint8_t headerType, size_t len = 0xFF);

#ifndef RADIOLIB_GODMODE
  private:
#endif
    // common parameters
    uint8_t _pwr = 0;

    // cached GFSK parameters
    float _modIndexReal = 0;
    uint16_t _brKbps = 0;
    uint8_t _br = 0, _modIndex = 0, _shaping = 0;
    uint8_t _preambleLengthGFSK = 0, _syncWordLen = 0, _syncWordMatch = 0, _crcGFSK = 0, _whitening = 0;

    // cached FLRC parameters
    uint8_t _crFLRC = 0;

    // cached BLE parameters
    uint8_t _connectionState = 0, _crcBLE = 0, _bleTestPayload = 0;

    int16_t config(uint8_t modem);

    // common low-level SPI interface
    int16_t SPIwriteCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy = true);
    int16_t SPIwriteCommand(uint8_t* cmd, uint8_t cmdLen, uint8_t* data, uint8_t numBytes, bool waitForBusy = true);
    int16_t SPIreadCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy = true);
    int16_t SPIreadCommand(uint8_t* cmd, uint8_t cmdLen, uint8_t* data, uint8_t numBytes, bool waitForBusy = true);
    int16_t SPItransfer(uint8_t* cmd, uint8_t cmdLen, bool write, uint8_t* dataOut, uint8_t* dataIn, uint8_t numBytes, bool waitForBusy, uint32_t timeout = 5000);
};

#endif

#endif
