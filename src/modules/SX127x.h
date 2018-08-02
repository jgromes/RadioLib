#ifndef _KITELIB_SX127X_H
#define _KITELIB_SX127X_H

#include "TypeDef.h"
#include "Module.h"

// SX127x series common registers
#define SX127X_REG_FIFO                               0x00
#define SX127X_REG_OP_MODE                            0x01
#define SX127X_REG_FRF_MSB                            0x06
#define SX127X_REG_FRF_MID                            0x07
#define SX127X_REG_FRF_LSB                            0x08
#define SX127X_REG_PA_CONFIG                          0x09
#define SX127X_REG_PA_RAMP                            0x0A
#define SX127X_REG_OCP                                0x0B
#define SX127X_REG_LNA                                0x0C
#define SX127X_REG_FIFO_ADDR_PTR                      0x0D
#define SX127X_REG_FIFO_TX_BASE_ADDR                  0x0E
#define SX127X_REG_FIFO_RX_BASE_ADDR                  0x0F
#define SX127X_REG_FIFO_RX_CURRENT_ADDR               0x10
#define SX127X_REG_IRQ_FLAGS_MASK                     0x11
#define SX127X_REG_IRQ_FLAGS                          0x12
#define SX127X_REG_RX_NB_BYTES                        0x13
#define SX127X_REG_RX_HEADER_CNT_VALUE_MSB            0x14
#define SX127X_REG_RX_HEADER_CNT_VALUE_LSB            0x15
#define SX127X_REG_RX_PACKET_CNT_VALUE_MSB            0x16
#define SX127X_REG_RX_PACKET_CNT_VALUE_LSB            0x17
#define SX127X_REG_MODEM_STAT                         0x18
#define SX127X_REG_PKT_SNR_VALUE                      0x19
#define SX127X_REG_PKT_RSSI_VALUE                     0x1A
#define SX127X_REG_RSSI_VALUE                         0x1B
#define SX127X_REG_HOP_CHANNEL                        0x1C
#define SX127X_REG_MODEM_CONFIG_1                     0x1D
#define SX127X_REG_MODEM_CONFIG_2                     0x1E
#define SX127X_REG_SYMB_TIMEOUT_LSB                   0x1F
#define SX127X_REG_PREAMBLE_MSB                       0x20
#define SX127X_REG_PREAMBLE_LSB                       0x21
#define SX127X_REG_PAYLOAD_LENGTH                     0x22
#define SX127X_REG_MAX_PAYLOAD_LENGTH                 0x23
#define SX127X_REG_HOP_PERIOD                         0x24
#define SX127X_REG_FIFO_RX_BYTE_ADDR                  0x25
#define SX127X_REG_FEI_MSB                            0x28
#define SX127X_REG_FEI_MID                            0x29
#define SX127X_REG_FEI_LSB                            0x2A
#define SX127X_REG_RSSI_WIDEBAND                      0x2C
#define SX127X_REG_DETECT_OPTIMIZE                    0x31
#define SX127X_REG_INVERT_IQ                          0x33
#define SX127X_REG_DETECTION_THRESHOLD                0x37
#define SX127X_REG_SYNC_WORD                          0x39
#define SX127X_REG_DIO_MAPPING_1                      0x40
#define SX127X_REG_DIO_MAPPING_2                      0x41
#define SX127X_REG_VERSION                            0x42

// SX127x common modem settings
// SX127X_REG_OP_MODE                                                 MSB   LSB   DESCRIPTION
#define SX127X_FSK_OOK                                0b00000000  //  7     7     FSK/OOK mode
#define SX127X_LORA                                   0b10000000  //  7     7     LoRa mode
#define SX127X_ACCESS_SHARED_REG_OFF                  0b00000000  //  6     6     access LoRa registers (0x0D:0x3F) in LoRa mode
#define SX127X_ACCESS_SHARED_REG_ON                   0b01000000  //  6     6     access FSK registers (0x0D:0x3F) in LoRa mode
#define SX127X_SLEEP                                  0b00000000  //  2     0     sleep
#define SX127X_STANDBY                                0b00000001  //  2     0     standby
#define SX127X_FSTX                                   0b00000010  //  2     0     frequency synthesis TX
#define SX127X_TX                                     0b00000011  //  2     0     transmit
#define SX127X_FSRX                                   0b00000100  //  2     0     frequency synthesis RX
#define SX127X_RXCONTINUOUS                           0b00000101  //  2     0     receive continuous
#define SX127X_RXSINGLE                               0b00000110  //  2     0     receive single
#define SX127X_CAD                                    0b00000111  //  2     0     channel activity detection

// SX127X_REG_PA_CONFIG
#define SX127X_PA_SELECT_RFO                          0b00000000  //  7     7     RFO pin output, power limited to +14 dBm
#define SX127X_PA_SELECT_BOOST                        0b10000000  //  7     7     PA_BOOST pin output, power limited to +20 dBm
#define SX127X_OUTPUT_POWER                           0b00001111  //  3     0     output power: P_out = 2 + OUTPUT_POWER [dBm] for PA_SELECT_BOOST
                                                                  //                            P_out = -1 + OUTPUT_POWER [dBm] for PA_SELECT_RFO

// SX127X_REG_OCP
#define SX127X_OCP_OFF                                0b00000000  //  5     5     PA overload current protection disabled
#define SX127X_OCP_ON                                 0b00100000  //  5     5     PA overload current protection enabled
#define SX127X_OCP_TRIM                               0b00001011  //  4     0     OCP current: I_max(OCP_TRIM = 0b1011) = 100 mA

// SX127X_REG_LNA
#define SX127X_LNA_GAIN_1                             0b00100000  //  7     5     LNA gain setting:   max gain
#define SX127X_LNA_GAIN_2                             0b01000000  //  7     5                         .
#define SX127X_LNA_GAIN_3                             0b01100000  //  7     5                         .
#define SX127X_LNA_GAIN_4                             0b10000000  //  7     5                         .
#define SX127X_LNA_GAIN_5                             0b10100000  //  7     5                         .
#define SX127X_LNA_GAIN_6                             0b11000000  //  7     5                         min gain
#define SX127X_LNA_BOOST_OFF                          0b00000000  //  1     0     default LNA current
#define SX127X_LNA_BOOST_ON                           0b00000011  //  1     0     150% LNA current

// SX127X_REG_MODEM_CONFIG_2
#define SX127X_SF_6                                   0b01100000  //  7     4     spreading factor:   64 chips/bit
#define SX127X_SF_7                                   0b01110000  //  7     4                         128 chips/bit
#define SX127X_SF_8                                   0b10000000  //  7     4                         256 chips/bit
#define SX127X_SF_9                                   0b10010000  //  7     4                         512 chips/bit
#define SX127X_SF_10                                  0b10100000  //  7     4                         1024 chips/bit
#define SX127X_SF_11                                  0b10110000  //  7     4                         2048 chips/bit
#define SX127X_SF_12                                  0b11000000  //  7     4                         4096 chips/bit
#define SX127X_TX_MODE_SINGLE                         0b00000000  //  3     3     single TX
#define SX127X_TX_MODE_CONT                           0b00001000  //  3     3     continuous TX
#define SX127X_RX_TIMEOUT_MSB                         0b00000000  //  1     0

// SX127X_REG_SYMB_TIMEOUT_LSB
#define SX127X_RX_TIMEOUT_LSB                         0b01100100  //  7     0     10 bit RX operation timeout

// SX127X_REG_PREAMBLE_MSB + REG_PREAMBLE_LSB
#define SX127X_PREAMBLE_LENGTH_MSB                    0b00000000  //  7     0     2 byte preamble length setting: l_P = PREAMBLE_LENGTH + 4.25
#define SX127X_PREAMBLE_LENGTH_LSB                    0b00001000  //  7     0         where l_p = preamble length

// SX127X_REG_DETECT_OPTIMIZE
#define SX127X_DETECT_OPTIMIZE_SF_6                   0b00000101  //  2     0     SF6 detection optimization
#define SX127X_DETECT_OPTIMIZE_SF_7_12                0b00000011  //  2     0     SF7 to SF12 detection optimization

// SX127X_REG_DETECTION_THRESHOLD
#define SX127X_DETECTION_THRESHOLD_SF_6               0b00001100  //  7     0     SF6 detection threshold
#define SX127X_DETECTION_THRESHOLD_SF_7_12            0b00001010  //  7     0     SF7 to SF12 detection threshold

// SX127X_REG_PA_DAC
#define SX127X_PA_BOOST_OFF                           0b00000100  //  2     0     PA_BOOST disabled
#define SX127X_PA_BOOST_ON                            0b00000111  //  2     0     +20 dBm on PA_BOOST when OUTPUT_POWER = 0b1111

// SX127X_REG_HOP_PERIOD
#define SX127X_HOP_PERIOD_OFF                         0b00000000  //  7     0     number of periods between frequency hops; 0 = disabled
#define SX127X_HOP_PERIOD_MAX                         0b11111111  //  7     0

// SX127X_REG_DIO_MAPPING_1
#define SX127X_DIO0_RX_DONE                           0b00000000  //  7     6
#define SX127X_DIO0_TX_DONE                           0b01000000  //  7     6
#define SX127X_DIO0_CAD_DONE                          0b10000000  //  7     6
#define SX127X_DIO1_RX_TIMEOUT                        0b00000000  //  5     4
#define SX127X_DIO1_FHSS_CHANGE_CHANNEL               0b00010000  //  5     4
#define SX127X_DIO1_CAD_DETECTED                      0b00100000  //  5     4

// SX127X_REG_IRQ_FLAGS
#define SX127X_CLEAR_IRQ_FLAG_RX_TIMEOUT              0b10000000  //  7     7     timeout
#define SX127X_CLEAR_IRQ_FLAG_RX_DONE                 0b01000000  //  6     6     packet reception complete
#define SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR       0b00100000  //  5     5     payload CRC error
#define SX127X_CLEAR_IRQ_FLAG_VALID_HEADER            0b00010000  //  4     4     valid header received
#define SX127X_CLEAR_IRQ_FLAG_TX_DONE                 0b00001000  //  3     3     payload transmission complete
#define SX127X_CLEAR_IRQ_FLAG_CAD_DONE                0b00000100  //  2     2     CAD complete
#define SX127X_CLEAR_IRQ_FLAG_FHSS_CHANGE_CHANNEL     0b00000010  //  1     1     FHSS change channel
#define SX127X_CLEAR_IRQ_FLAG_CAD_DETECTED            0b00000001  //  0     0     valid LoRa signal detected during CAD operation

// SX127X_REG_IRQ_FLAGS_MASK
#define SX127X_MASK_IRQ_FLAG_RX_TIMEOUT               0b01111111  //  7     7     timeout
#define SX127X_MASK_IRQ_FLAG_RX_DONE                  0b10111111  //  6     6     packet reception complete
#define SX127X_MASK_IRQ_FLAG_PAYLOAD_CRC_ERROR        0b11011111  //  5     5     payload CRC error
#define SX127X_MASK_IRQ_FLAG_VALID_HEADER             0b11101111  //  4     4     valid header received
#define SX127X_MASK_IRQ_FLAG_TX_DONE                  0b11110111  //  3     3     payload transmission complete
#define SX127X_MASK_IRQ_FLAG_CAD_DONE                 0b11111011  //  2     2     CAD complete
#define SX127X_MASK_IRQ_FLAG_FHSS_CHANGE_CHANNEL      0b11111101  //  1     1     FHSS change channel
#define SX127X_MASK_IRQ_FLAG_CAD_DETECTED             0b11111110  //  0     0     valid LoRa signal detected during CAD operation

// SX127X_REG_FIFO_TX_BASE_ADDR
#define SX127X_FIFO_TX_BASE_ADDR_MAX                  0b00000000  //  7     0     allocate the entire FIFO buffer for TX only

// SX127X_REG_FIFO_RX_BASE_ADDR
#define SX127X_FIFO_RX_BASE_ADDR_MAX                  0b00000000  //  7     0     allocate the entire FIFO buffer for RX only

// SX127X_REG_SYNC_WORD
#define SX127X_SYNC_WORD                              0x12        //  7     0     default LoRa sync word
#define SX127X_SYNC_WORD_LORAWAN                      0x34        //  7     0     sync word reserved for LoRaWAN networks

class SX127x {
  public:
    // constructor
    SX127x(Module* mod);
    
    // public member variables
    float dataRate;
    int8_t lastPacketRSSI;
    float lastPacketSNR;
    
    // basic methods
    int16_t begin(uint8_t chipVersion, uint8_t syncWord, uint8_t currentLimit, uint16_t preambleLength);
    int16_t transmit(String& str);
    int16_t transmit(const char* str);
    int16_t transmit(uint8_t* data, size_t len);
    int16_t receive(String& str, size_t len = 0);
    int16_t receive(uint8_t* data, size_t len);
    int16_t scanChannel();
    int16_t sleep();
    int16_t standby();
    
    // interrupt methods
    void setDio0Action(void (*func)(void));
    void setDio1Action(void (*func)(void));
    int16_t startTransmit(String& str);
    int16_t startTransmit(const char* str);
    int16_t startTransmit(uint8_t* data, size_t len);
    int16_t startReceive();
    int16_t readData(String& str, size_t len = 0);
    int16_t readData(uint8_t* data, size_t len);
    
    // configuration methods
    int16_t setSyncWord(uint8_t syncWord);
    int16_t setCurrentLimit(uint8_t currentLimit);
    int16_t setPreambleLength(uint16_t preambleLength);
    float getFrequencyError();
  
  protected:
    Module* _mod;
    
    float _bw;
    uint8_t _sf;
    uint8_t _cr;
    
    int16_t tx(char* data, uint8_t length);
    int16_t rxSingle(char* data, uint8_t* length);
    int16_t setFrequencyRaw(float newFreq);
    int16_t config();
  
  private:
    int16_t setMode(uint8_t mode);
    void clearIRQFlags();
};

#endif
