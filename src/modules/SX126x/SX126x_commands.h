#if !defined(_RADIOLIB_SX126X_COMMANDS_H)
#define _RADIOLIB_SX126X_COMMANDS_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SX126X

// SX126X SPI commands
// operational modes commands
#define RADIOLIB_SX126X_CMD_NOP                                 0x00
#define RADIOLIB_SX126X_CMD_SET_SLEEP                           0x84
#define RADIOLIB_SX126X_CMD_SET_STANDBY                         0x80
#define RADIOLIB_SX126X_CMD_SET_FS                              0xC1
#define RADIOLIB_SX126X_CMD_SET_TX                              0x83
#define RADIOLIB_SX126X_CMD_SET_RX                              0x82
#define RADIOLIB_SX126X_CMD_STOP_TIMER_ON_PREAMBLE              0x9F
#define RADIOLIB_SX126X_CMD_SET_RX_DUTY_CYCLE                   0x94
#define RADIOLIB_SX126X_CMD_SET_CAD                             0xC5
#define RADIOLIB_SX126X_CMD_SET_TX_CONTINUOUS_WAVE              0xD1
#define RADIOLIB_SX126X_CMD_SET_TX_INFINITE_PREAMBLE            0xD2
#define RADIOLIB_SX126X_CMD_SET_REGULATOR_MODE                  0x96
#define RADIOLIB_SX126X_CMD_CALIBRATE                           0x89
#define RADIOLIB_SX126X_CMD_CALIBRATE_IMAGE                     0x98
#define RADIOLIB_SX126X_CMD_SET_PA_CONFIG                       0x95
#define RADIOLIB_SX126X_CMD_SET_RX_TX_FALLBACK_MODE             0x93

// register and buffer access commands
#define RADIOLIB_SX126X_CMD_WRITE_REGISTER                      0x0D
#define RADIOLIB_SX126X_CMD_READ_REGISTER                       0x1D
#define RADIOLIB_SX126X_CMD_WRITE_BUFFER                        0x0E
#define RADIOLIB_SX126X_CMD_READ_BUFFER                         0x1E

// DIO and IRQ control
#define RADIOLIB_SX126X_CMD_SET_DIO_IRQ_PARAMS                  0x08
#define RADIOLIB_SX126X_CMD_GET_IRQ_STATUS                      0x12
#define RADIOLIB_SX126X_CMD_CLEAR_IRQ_STATUS                    0x02
#define RADIOLIB_SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL          0x9D
#define RADIOLIB_SX126X_CMD_SET_DIO3_AS_TCXO_CTRL               0x97

// RF, modulation and packet commands
#define RADIOLIB_SX126X_CMD_SET_RF_FREQUENCY                    0x86
#define RADIOLIB_SX126X_CMD_SET_PACKET_TYPE                     0x8A
#define RADIOLIB_SX126X_CMD_GET_PACKET_TYPE                     0x11
#define RADIOLIB_SX126X_CMD_SET_TX_PARAMS                       0x8E
#define RADIOLIB_SX126X_CMD_SET_MODULATION_PARAMS               0x8B
#define RADIOLIB_SX126X_CMD_SET_PACKET_PARAMS                   0x8C
#define RADIOLIB_SX126X_CMD_SET_CAD_PARAMS                      0x88
#define RADIOLIB_SX126X_CMD_SET_BUFFER_BASE_ADDRESS             0x8F
#define RADIOLIB_SX126X_CMD_SET_LORA_SYMB_NUM_TIMEOUT           0xA0

// status commands
#define RADIOLIB_SX126X_CMD_GET_STATUS                          0xC0
#define RADIOLIB_SX126X_CMD_GET_RSSI_INST                       0x15
#define RADIOLIB_SX126X_CMD_GET_RX_BUFFER_STATUS                0x13
#define RADIOLIB_SX126X_CMD_GET_PACKET_STATUS                   0x14
#define RADIOLIB_SX126X_CMD_GET_DEVICE_ERRORS                   0x17
#define RADIOLIB_SX126X_CMD_CLEAR_DEVICE_ERRORS                 0x07
#define RADIOLIB_SX126X_CMD_GET_STATS                           0x10
#define RADIOLIB_SX126X_CMD_RESET_STATS                         0x00

#define RADIOLIB_SX126X_CMD_PRAM_UPDATE                         0xD9
#define RADIOLIB_SX126X_CMD_SET_LBT_SCAN_PARAMS                 0x9A
#define RADIOLIB_SX126X_CMD_SET_SPECTR_SCAN_PARAMS              0x9B

#endif

#endif
