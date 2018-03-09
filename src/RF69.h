#ifndef _KITELIB_RF69_H
#define _KITELIB_RF69_H

#include <EEPROM.h>

#include "TypeDef.h"
#include "Module.h"
#include "Packet.h"

//RF69 register map
#define RF69_REG_FIFO                                 0x00
#define RF69_REG_OP_MODE                              0x01
#define RF69_REG_DATA_MODUL                           0x02
#define RF69_REG_BITRATE_MSB                          0x03
#define RF69_REG_BITRATE_LSB                          0x04
#define RF69_REG_FDEV_MSB                             0x05
#define RF69_REG_FDEV_LSB                             0x06
#define RF69_REG_FRF_MSB                              0x07
#define RF69_REG_FRF_MID                              0x08
#define RF69_REG_FRF_LSB                              0x09
#define RF69_REG_OSC_1                                0x0A
#define RF69_REG_AFC_CTRL                             0x0B
#define RF69_REG_LISTEN_1                             0x0D
#define RF69_REG_LISTEN_2                             0x0E
#define RF69_REG_LISTEN_3                             0x0F
#define RF69_REG_VERSION                              0x10
#define RF69_REG_PA_LEVEL                             0x11
#define RF69_REG_PA_RAMP                              0x12
#define RF69_REG_OCP                                  0x13
#define RF69_REG_LNA                                  0x18
#define RF69_REG_RX_BW                                0x19
#define RF69_REG_AFC_BW                               0x1A
#define RF69_REG_OOK_PEAK                             0x1B
#define RF69_REG_OOK_AVG                              0x1C
#define RF69_REG_OOK_FIX                              0x1D
#define RF69_REG_AFC_FEI                              0x1E
#define RF69_REG_AFC_MSB                              0x1F
#define RF69_REG_AFC_LSB                              0x20
#define RF69_REG_FEI_MSB                              0x21
#define RF69_REG_FEI_LSB                              0x22
#define RF69_REG_RSSI_CONFIG                          0x23
#define RF69_REG_RSSI_VALUE                           0x24
#define RF69_REG_DIO_MAPPING_1                        0x25
#define RF69_REG_DIO_MAPPING_2                        0x26
#define RF69_REG_IRQ_FLAGS_1                          0x27
#define RF69_REG_IRQ_FLAGS_2                          0x28
#define RF69_REG_RSSI_THRESH                          0x29
#define RF69_REG_RX_TIMEOUT_1                         0x2A
#define RF69_REG_RX_TIMEOUT_2                         0x2B
#define RF69_REG_PREAMBLE_MSB                         0x2C
#define RF69_REG_PREAMBLE_LSB                         0x2D
#define RF69_REG_SYNC_CONFIG                          0x2E
#define RF69_REG_SYNC_VALUE_1                         0x2F
#define RF69_REG_SYNC_VALUE_2                         0x30
#define RF69_REG_SYNC_VALUE_3                         0x31
#define RF69_REG_SYNC_VALUE_4                         0x32
#define RF69_REG_SYNC_VALUE_5                         0x33
#define RF69_REG_SYNC_VALUE_6                         0x34
#define RF69_REG_SYNC_VALUE_7                         0x35
#define RF69_REG_SYNC_VALUE_8                         0x36
#define RF69_REG_PACKET_CONFIG_1                      0x37
#define RF69_REG_PAYLOAD_LENGTH                       0x38
#define RF69_REG_NODE_ADRS                            0x39
#define RF69_REG_BROADCAST_ADRS                       0x3A
#define RF69_REG_AUTO_MODES                           0x3B
#define RF69_REG_FIFO_THRESH                          0x3C
#define RF69_REG_PACKET_CONFIG_2                      0x3D
#define RF69_REG_AES_KEY_1                            0x3E
#define RF69_REG_AES_KEY_2                            0x3F
#define RF69_REG_AES_KEY_3                            0x40
#define RF69_REG_AES_KEY_4                            0x41
#define RF69_REG_AES_KEY_5                            0x42
#define RF69_REG_AES_KEY_6                            0x43
#define RF69_REG_AES_KEY_7                            0x44
#define RF69_REG_AES_KEY_8                            0x45
#define RF69_REG_AES_KEY_9                            0x46
#define RF69_REG_AES_KEY_10                           0x47
#define RF69_REG_AES_KEY_11                           0x48
#define RF69_REG_AES_KEY_12                           0x49
#define RF69_REG_AES_KEY_13                           0x4A
#define RF69_REG_AES_KEY_14                           0x4B
#define RF69_REG_AES_KEY_15                           0x4C
#define RF69_REG_AES_KEY_16                           0x4D
#define RF69_REG_TEMP_1                               0x4E
#define RF69_REG_TEMP_2                               0x4F

//RF69 modem settings
//RF69_REG_OP_MODE                                                    MSB   LSB   DESCRIPTION
#define RF69_SEQUENCER_OFF                            0b00000000  //  7     7     disable automatic sequencer
#define RF69_SEQUENCER_ON                             0b10000000  //  7     7     enable automatic sequencer
#define RF69_LISTEN_OFF                               0b00000000  //  6     6     disable Listen mode
#define RF69_LISTEN_ON                                0b01000000  //  6     6     enable Listen mode
#define RF69_LISTEN_ABORT                             0b00100000  //  5     5     abort Listen mode (has to be set together with RF69_LISTEN_OFF)
#define RF69_SLEEP                                    0b00000000  //  4     2     sleep
#define RF69_STANDBY                                  0b00000100  //  4     2     standby
#define RF69_FS                                       0b00001000  //  4     2     frequency synthesis
#define RF69_TX                                       0b00001100  //  4     2     transmit
#define RF69_RX                                       0b00010000  //  4     2     receive

//RF69_REG_DATA_MODUL
#define RF69_PACKET_MODE                              0b00000000  //  6     5     packet mode
#define RF69_CONTINUOUS_MODE_WITH_SYNC                0b01000000  //  6     5     continuous mode with bit synchronizer
#define RF69_CONTINUOUS_MODE                          0b01100000  //  6     5     continuous mode without bit synchronizer
#define RF69_FSK                                      0b00000000  //  4     3     FSK modulation
#define RF69_OOK                                      0b00001000  //  4     3     OOK modulation
#define RF69_NO_SHAPING                               0b00000000  //  1     0     modulation shaping: no shaping
#define RF69_FSK_GAUSSIAN_1_0                         0b00000001  //  1     0                         FSK modulation Gaussian filter, BT = 1.0
#define RF69_FSK_GAUSSIAN_0_5                         0b00000010  //  1     0                         FSK modulation Gaussian filter, BT = 0.5
#define RF69_FSK_GAUSSIAN_0_3                         0b00000011  //  1     0                         FSK modulation Gaussian filter, BT = 0.3
#define RF69_OOK_FILTER_BR                            0b00000001  //  1     0                         OOK modulation filter, f_cutoff = BR
#define RF69_OOK_FILTER_2BR                           0b00000010  //  1     0                         OOK modulation filter, f_cutoff = 2*BR

//RF69_REG_BITRATE_MSB + REG_BITRATE_LSB
#define RF69_BITRATE_MSB                              0x1A        //  7     0     bit rate setting: rate = F(XOSC) / BITRATE
#define RF69_BITRATE_LSB                              0x0B        //  7     0         default value: 4.8 kbps

//RF69_REG_FDEV_MSB + REG_FDEV_LSB
#define RF69_FDEV_MSB                                 0x00        //  5     0     frequency deviation: f_dev = f_step * FDEV
#define RF69_FDEV_LSB                                 0x52        //  7     0         default value: 5 kHz

//RF69_REG_FRF_MSB + REG_FRF_MID + REG_FRF_LSB
#define SX1278_FRF_MSB                                0xE4        //  7     0     carrier frequency setting: f_RF = (F(XOSC) * FRF)/2^19
#define SX1278_FRF_MID                                0xC0        //  7     0         where F(XOSC) = 32 MHz
#define SX1278_FRF_LSB                                0x00        //  7     0         default value: 915 MHz

//RF69_REG_OSC_1
#define RF69_RC_CAL_START                             0b10000000  //  7     7     force RC oscillator calibration
#define RF69_RC_CAL_RUNNING                           0b00000000  //  6     6     RC oscillator calibration is still running
#define RF69_RC_CAL_DONE                              0b00000000  //  5     5     RC oscillator calibration has finished

//RF69_REG_AFC_CTRL
#define RF69_AFC_LOW_BETA_OFF                         0b00000000  //  5     5     standard AFC routine
#define RF69_AFC_LOW_BETA_ON                          0b00100000  //  5     5     improved AFC routine for signals with modulation index less than 2

//RF69_REG_LISTEN_1
#define RF69_LISTEN_RES_IDLE_64_US                    0b01000000  //  7     6     resolution of Listen mode idle time: 64 us
#define RF69_LISTEN_RES_IDLE_4_1_MS                   0b10000000  //  7     6                                          4.1 ms (default)
#define RF69_LISTEN_RES_IDLE_262_MS                   0b11000000  //  7     6                                          262 ms
#define RF69_LISTEN_RES_RX_64_US                      0b00010000  //  5     4     resolution of Listen mode ry time: 64 us (default)
#define RF69_LISTEN_RES_RX_4_1_MS                     0b00100000  //  5     4                                        4.1 ms
#define RF69_LISTEN_RES_RX_262_MS                     0b00110000  //  5     4                                        262 ms
#define RF69_LISTEN_ACCEPT_ABOVE_RSSI_THRESH          0b00000000  //  3     3     packet acceptance criteria: RSSI above threshold
#define RF69_LISTEN_ACCEPT_MATCH_SYNC_ADDRESS         0b00001000  //  3     3                                 RSSI above threshold AND sync address matched
#define RF69_LISTEN_END_KEEP_RX                       0b00000000  //  2     1     action after packet acceptance: stay in Rx mode
#define RF69_LISTEN_END_KEEP_RX_TIMEOUT               0b00000010  //  2     1                                     stay in Rx mode until timeout (default)
#define RF69_LISTEN_END_KEEP_RX_TIMEOUT_RESUME        0b00000100  //  2     1                                     stay in Rx mode until timeout, Listen mode will resume

//RF69_REG_LISTEN_2
#define RF69_LISTEN_COEF_IDLE                         0xF5        //  7     0     duration of idle phase in Listen mode

//RF69_REG_LISTEN_3
#define RF69_LISTEN_COEF_RX                           0x20        //  7     0     duration of Rx phase in Listen mode

//RF69_REG_PA_LEVEL
#define RF69_PA0_OFF                                  0b00000000  //  7     7     PA0 disabled
#define RF69_PA0_ON                                   0b10000000  //  7     7     PA0 enabled (default)
#define RF69_PA1_OFF                                  0b00000000  //  6     6     PA1 disabled
#define RF69_PA1_ON                                   0b01000000  //  6     6     PA1 enabled
#define RF69_PA2_OFF                                  0b00000000  //  5     5     PA2 disabled
#define RF69_PA2_ON                                   0b00100000  //  5     5     PA2 enabled
#define RF69_OUTPUT_POWER                             0b00011111  //  4     0     output power: P_out = -18 + OUTPUT_POWER

//RF69_REG_PA_RAMP
#define RF69_PA_RAMP_3_4_MS                           0b00000000  //  3     0     PA ramp rise/fall time: 3.4 ms
#define RF69_PA_RAMP_2_MS                             0b00000001  //  3     0                             2 ms
#define RF69_PA_RAMP_1_MS                             0b00000010  //  3     0                             1 ms
#define RF69_PA_RAMP_500_US                           0b00000011  //  3     0                             500 us
#define RF69_PA_RAMP_250_US                           0b00000100  //  3     0                             250 us
#define RF69_PA_RAMP_125_US                           0b00000101  //  3     0                             125 us
#define RF69_PA_RAMP_100_US                           0b00000110  //  3     0                             100 us
#define RF69_PA_RAMP_62_US                            0b00000111  //  3     0                             62 us
#define RF69_PA_RAMP_50_US                            0b00001000  //  3     0                             50 us
#define RF69_PA_RAMP_40_US                            0b00001001  //  3     0                             40 us (default)
#define RF69_PA_RAMP_31_US                            0b00001010  //  3     0                             31 us
#define RF69_PA_RAMP_25_US                            0b00001011  //  3     0                             25 us
#define RF69_PA_RAMP_20_US                            0b00001100  //  3     0                             20 us
#define RF69_PA_RAMP_15_US                            0b00001101  //  3     0                             15 us
#define RF69_PA_RAMP_12_US                            0b00001110  //  3     0                             12 us
#define RF69_PA_RAMP_10_US                            0b00001111  //  3     0                             10 us

//RF69_REG_OCP
#define RF69_OCP_OFF                                  0b00000000  //  4     4     PA overload current protection disabled
#define RF69_OCP_ON                                   0b00100000  //  4     4     PA overload current protection enabled
#define RF69_OCP_TRIM                                 0b00001010  //  3     0     OCP current: I_max(OCP_TRIM = 0b1010) = 95 mA

//RF69_REG_LNA
//RF69_REG_RX_BW
//RF69_REG_AFC_BW
//RF69_REG_OOK_PEAK
//RF69_REG_OOK_AVG
//RF69_REG_OOK_FIX
//RF69_REG_AFC_FEI
//RF69_REG_AFC_MSB + REG_AFC_LSB
//RF69_REG_FEI_MSB + REG_FEI_LSB
//RF69_REG_RSSI_CONFIG

//RF69_REG_DIO_MAPPING_1
//RF69_REG_DIO_MAPPING_2
//RF69_REG_IRQ_FLAGS_1
//RF69_REG_IRQ_FLAGS_2
//RF69_REG_RSSI_THRESH
//RF69_REG_RX_TIMEOUT_1
//RF69_REG_RX_TIMEOUT_2

//RF69_REG_PREAMBLE_MSB + REG_PREAMBLE_MSB
//RF69_REG_SYNC_CONFIG
//RF69_REG_SYNC_VALUE_1
//RF69_REG_SYNC_VALUE_2
//RF69_REG_SYNC_VALUE_3
//RF69_REG_SYNC_VALUE_4
//RF69_REG_SYNC_VALUE_5
//RF69_REG_SYNC_VALUE_6
//RF69_REG_SYNC_VALUE_7
//RF69_REG_SYNC_VALUE_8
//RF69_REG_PACKET_CONFIG_1
//RF69_REG_PAYLOAD_LENGTH
//RF69_REG_NODE_ADRS
//RF69_REG_BROADCAST_ADRS
//RF69_REG_AUTO_MODES
//RF69_REG_FIFO_THRESH
//RF69_REG_PACKET_CONFIG_1
//RF69_REG_AES_KEY_1 - REG_AES_KEY_16

//RF69_REG_TEMP_1
//RF69_REG_TEMP_2

class RF69 {
  public:
    RF69(Module* module);
    
    void begin();
  
  private:
    Module* _mod;
};

#endif
