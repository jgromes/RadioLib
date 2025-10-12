#if !defined(RADIOLIB_LR2021_H)
#define RADIOLIB_LR2021_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR2021

#include "../../Module.h"

#include "../../protocols/PhysicalLayer/PhysicalLayer.h"

// LR2021 physical layer properties
#define RADIOLIB_LR2021_FREQUENCY_STEP_SIZE                     1.0
#define RADIOLIB_LR2021_MAX_PACKET_LENGTH                       255
#define RADIOLIB_LR2021_CRYSTAL_FREQ                            32.0
#define RADIOLIB_LR2021_DIV_EXPONENT                            25

// LR2021 SPI commands
#define RADIOLIB_LR2021_CMD_NOP                                 (0x0000)
#define RADIOLIB_LR2021_CMD_READ_RX_FIFO                        (0x0001)
#define RADIOLIB_LR2021_CMD_WRITE_TX_FIFO                       (0x0002)
#define RADIOLIB_LR2021_CMD_WRITE_REG_MEM_32                    (0x0104)
#define RADIOLIB_LR2021_CMD_WRITE_REG_MEM_MASK_32               (0x0105)
#define RADIOLIB_LR2021_CMD_READ_REG_MEM_32                     (0x0106)
#define RADIOLIB_LR2021_CMD_SET_SLEEP                           (0x0127)
#define RADIOLIB_LR2021_CMD_SET_STANDBY                         (0x0128)
#define RADIOLIB_LR2021_CMD_SET_FS                              (0x0129)
#define RADIOLIB_LR2021_CMD_SET_ADDITIONAL_REG_TO_RETAIN        (0x012A)
#define RADIOLIB_LR2021_CMD_SET_RX                              (0x020C)
#define RADIOLIB_LR2021_CMD_SET_TX                              (0x020D)
#define RADIOLIB_LR2021_CMD_SET_RX_TX_FALLBACK_MODE             (0x0206)
#define RADIOLIB_LR2021_CMD_SET_RX_DUTY_CYCLE                   (0x0210)
#define RADIOLIB_LR2021_CMD_SET_AUTO_RX_TX                      (0x0211)
#define RADIOLIB_LR2021_CMD_GET_RX_PKT_LENGTH                   (0x0212)
#define RADIOLIB_LR2021_CMD_STOP_TIMEOUT_ON_PREAMBLE            (0x0209)
#define RADIOLIB_LR2021_CMD_RESET_RX_STATS                      (0x020A)
#define RADIOLIB_LR2021_CMD_SET_DEFAULT_RX_TX_TIMEOUT           (0x0215)
#define RADIOLIB_LR2021_CMD_SET_REG_MODE                        (0x0121)
#define RADIOLIB_LR2021_CMD_CALIBRATE                           (0x0122)
#define RADIOLIB_LR2021_CMD_CALIB_FRONT_END                     (0x0123)
#define RADIOLIB_LR2021_CMD_GET_V_BAT                           (0x0124)
#define RADIOLIB_LR2021_CMD_GET_TEMP                            (0x0125)
#define RADIOLIB_LR2021_CMD_SET_EOL_CONFIG                      (0x0130)
#define RADIOLIB_LR2021_CMD_GET_RANDOM_NUMBER                   (0x0126)
#define RADIOLIB_LR2021_CMD_GET_STATUS                          (0x0100)
#define RADIOLIB_LR2021_CMD_GET_VERSION                         (0x0101)
#define RADIOLIB_LR2021_CMD_CLEAR_ERRORS                        (0x0111)
#define RADIOLIB_LR2021_CMD_GET_ERRORS                          (0x0110)
#define RADIOLIB_LR2021_CMD_SET_DIO_FUNCTION                    (0x0112)
#define RADIOLIB_LR2021_CMD_SET_DIO_IRQ_CONFIG                  (0x0115)
#define RADIOLIB_LR2021_CMD_CLEAR_IRQ                           (0x0116)
#define RADIOLIB_LR2021_CMD_GET_AND_CLEAR_IRQ_STATUS            (0x0117)
#define RADIOLIB_LR2021_CMD_CONFIG_FIFO_IRQ                     (0x011A)
#define RADIOLIB_LR2021_CMD_GET_FIFO_IRQ_FLAGS                  (0x011B)
#define RADIOLIB_LR2021_CMD_CLEAR_FIFO_IRQ_FLAGS                (0x0114)
#define RADIOLIB_LR2021_CMD_GET_AND_CLEAR_FIFO_IRQ_FLAGS        (0x012E)
#define RADIOLIB_LR2021_CMD_GET_RX_FIFO_LEVEL                   (0x011C)
#define RADIOLIB_LR2021_CMD_GET_TX_FIFO_LEVEL                   (0x011D)
#define RADIOLIB_LR2021_CMD_CLEAR_RX_FIFO                       (0x011E)
#define RADIOLIB_LR2021_CMD_CLEAR_TX_FIFO                       (0x011F)
#define RADIOLIB_LR2021_CMD_CONFIG_LF_CLOCK                     (0x0118)
#define RADIOLIB_LR2021_CMD_CONFIG_CLK_OUTPUTS                  (0x0119)
#define RADIOLIB_LR2021_CMD_SET_TCXO_MODE                       (0x0120)
#define RADIOLIB_LR2021_CMD_SET_XOSC_CP_TRIM                    (0x0131)
#define RADIOLIB_LR2021_CMD_SET_RF_FREQUENCY                    (0x0200)
#define RADIOLIB_LR2021_CMD_SET_RX_PATH                         (0x0201)
#define RADIOLIB_LR2021_CMD_GET_RSSI_INST                       (0x020B)
#define RADIOLIB_LR2021_CMD_SET_RSSI_CALIBRATION                (0x0205)
#define RADIOLIB_LR2021_CMD_SET_TIMESTAMP_SOURCE                (0x0216)
#define RADIOLIB_LR2021_CMD_GET_TIMESTAMP_VALUE                 (0x0217)
#define RADIOLIB_LR2021_CMD_SET_CCA                             (0x0218)
#define RADIOLIB_LR2021_CMD_GET_CCA_RESULT                      (0x0219)
#define RADIOLIB_LR2021_CMD_SET_AGC_GAIN_MANUAL                 (0x021A)
#define RADIOLIB_LR2021_CMD_SET_CAD_PARAMS                      (0x021B)
#define RADIOLIB_LR2021_CMD_SET_CAD                             (0x021C)
#define RADIOLIB_LR2021_CMD_SEL_PA                              (0x020F)
#define RADIOLIB_LR2021_CMD_SET_PA_CONFIG                       (0x0202)
#define RADIOLIB_LR2021_CMD_SET_TX_PARAMS                       (0x0203)
#define RADIOLIB_LR2021_CMD_SET_PACKET_TYPE                     (0x0207)
#define RADIOLIB_LR2021_CMD_GET_PACKET_TYPE                     (0x0208)
#define RADIOLIB_LR2021_CMD_SET_LORA_MODULATION_PARAMS          (0x0220)
#define RADIOLIB_LR2021_CMD_SET_LORA_PACKET_PARAMS              (0x0221)
#define RADIOLIB_LR2021_CMD_SET_LORA_SYNCH_TIMEOUT              (0x0222)
#define RADIOLIB_LR2021_CMD_SET_LORA_SYNCWORD                   (0x0223)
#define RADIOLIB_LR2021_CMD_SET_LORA_SIDE_DET_CONFIG            (0x0224)
#define RADIOLIB_LR2021_CMD_SET_LORA_SIDE_DET_SYNCWORD          (0x0225)
#define RADIOLIB_LR2021_CMD_SET_LORA_CAD_PARAMS                 (0x0227)
#define RADIOLIB_LR2021_CMD_GET_LORA_RX_STATS                   (0x0229)
#define RADIOLIB_LR2021_CMD_GET_LORA_PACKET_STATUS              (0x022A)
#define RADIOLIB_LR2021_CMD_SET_LORA_ADDRESS                    (0x022B)
#define RADIOLIB_LR2021_CMD_SET_LORA_HOPPING                    (0x022C)
#define RADIOLIB_LR2021_CMD_SET_LORA_TX_SYNC                    (0x021D)
#define RADIOLIB_LR2021_CMD_SET_LORA_SIDE_DET_CAD               (0x021E)
#define RADIOLIB_LR2021_CMD_SET_RANGING_ADDR                    (0x0278)
#define RADIOLIB_LR2021_CMD_SET_RANGING_REQ_ADDR                (0x0279)
#define RADIOLIB_LR2021_CMD_GET_RANGING_RESULT                  (0x027A)
#define RADIOLIB_LR2021_CMD_GET_RANGING_STATS                   (0x027D)
#define RADIOLIB_LR2021_CMD_SET_RANGING_TX_RX_DELAY             (0x027B)
#define RADIOLIB_LR2021_CMD_SET_RANGING_PARAMS                  (0x027C)
#define RADIOLIB_LR2021_CMD_SET_FSK_MODULATION_PARAMS           (0x0240)
#define RADIOLIB_LR2021_CMD_SET_FSK_PACKET_PARAMS               (0x0241)
#define RADIOLIB_LR2021_CMD_SET_FSK_WHITENING_PARAMS            (0x0242)
#define RADIOLIB_LR2021_CMD_SET_FSK_CRC_PARAMS                  (0x0243)
#define RADIOLIB_LR2021_CMD_SET_FSK_SYNCWORD                    (0x0244)
#define RADIOLIB_LR2021_CMD_SET_FSK_ADDRESS                     (0x0245)
#define RADIOLIB_LR2021_CMD_GET_FSK_RX_STATS                    (0x0246)
#define RADIOLIB_LR2021_CMD_GET_FSK_PACKET_STATUS               (0x0247)
#define RADIOLIB_LR2021_CMD_SET_WMBUS_PARAMS                    (0x026A)
#define RADIOLIB_LR2021_CMD_GET_WMBUS_RX_STATS                  (0x026C)
#define RADIOLIB_LR2021_CMD_GET_WMBUS_PACKET_STATUS             (0x026D)
#define RADIOLIB_LR2021_CMD_SET_WMBUS_FILTERING_ADDRESS         (0x026E)
#define RADIOLIB_LR2021_CMD_SET_WISUN_MODE                      (0x0270)
#define RADIOLIB_LR2021_CMD_SET_WISUN_PACKET_PARAMS             (0x0271)
#define RADIOLIB_LR2021_CMD_GET_WISUN_RX_STATS                  (0x0272)
#define RADIOLIB_LR2021_CMD_GET_WISUN_PACKET_STATUS             (0x0273)
#define RADIOLIB_LR2021_CMD_SET_WISUN_PACKET_LEN                (0x0274)
#define RADIOLIB_LR2021_CMD_SET_ZWAVE_PARAMS                    (0x0297)
#define RADIOLIB_LR2021_CMD_SET_ZWAVE_HOME_ID_FILTERING         (0x0298)
#define RADIOLIB_LR2021_CMD_GET_ZWAVE_RX_STATS                  (0x0299)
#define RADIOLIB_LR2021_CMD_GET_ZWAVE_PACKET_STATUS             (0x029A)
#define RADIOLIB_LR2021_CMD_SET_ZWAVE_BEAM_FILTERING            (0x029B)
#define RADIOLIB_LR2021_CMD_SET_ZWAVE_SCAN_CONFIG               (0x029C)
#define RADIOLIB_LR2021_CMD_SET_ZWAVE_SCAN                      (0x029D)
#define RADIOLIB_LR2021_CMD_SET_BLE_MODULATION_PARAMS           (0x0260)
#define RADIOLIB_LR2021_CMD_SET_BLE_CHANNEL_PARAMS              (0x0261)
#define RADIOLIB_LR2021_CMD_SET_BLE_PDU_LEN                     (0x0266)
#define RADIOLIB_LR2021_CMD_SET_BLE_TX                          (0x0262)
#define RADIOLIB_LR2021_CMD_GET_BLE_RX_STATS                    (0x0264)
#define RADIOLIB_LR2021_CMD_GET_BLE_PACKET_STATUS               (0x0265)
#define RADIOLIB_LR2021_CMD_SET_OQPSK_PARAMS                    (0x029F)
#define RADIOLIB_LR2021_CMD_GET_OQPSK_RX_STATS                  (0x02A0)
#define RADIOLIB_LR2021_CMD_GET_OQPSK_PACKET_STATUS             (0x02A1)
#define RADIOLIB_LR2021_CMD_SET_OQPSK_PACKET_LEN                (0x02A2)
#define RADIOLIB_LR2021_CMD_SET_OQPSK_ADDRESS                   (0x02A3)
#define RADIOLIB_LR2021_CMD_SET_BPSK_MODULATION_PARAMS          (0x0250)
#define RADIOLIB_LR2021_CMD_SET_BPSK_PACKET_PARAMS              (0x0251)
#define RADIOLIB_LR2021_CMD_SET_FLRC_MODULATION_PARAMS          (0x0248)
#define RADIOLIB_LR2021_CMD_SET_FLRC_PACKET_PARAMS              (0x0249)
#define RADIOLIB_LR2021_CMD_GET_FLRC_RX_STATS                   (0x024A)
#define RADIOLIB_LR2021_CMD_GET_FLRC_PACKET_STATUS              (0x024B)
#define RADIOLIB_LR2021_CMD_SET_FLRC_SYNCWORD                   (0x024C)
#define RADIOLIB_LR2021_CMD_LR_FHSS_BUILD_FRAME                 (0x0256)
#define RADIOLIB_LR2021_CMD_LR_FHSS_SET_SYNCWORD                (0x0257)
#define RADIOLIB_LR2021_CMD_SET_OOK_MODULATION_PARAMS           (0x0281)
#define RADIOLIB_LR2021_CMD_SET_OOK_PACKET_PARAMS               (0x0282)
#define RADIOLIB_LR2021_CMD_SET_OOK_CRC_PARAMS                  (0x0283)
#define RADIOLIB_LR2021_CMD_SET_OOK_SYNCWORD                    (0x0284)
#define RADIOLIB_LR2021_CMD_SET_OOK_ADDRESS                     (0x0285)
#define RADIOLIB_LR2021_CMD_GET_OOK_RX_STATS                    (0x0286)
#define RADIOLIB_LR2021_CMD_GET_OOK_PACKET_STATUS               (0x0287)
#define RADIOLIB_LR2021_CMD_SET_OOK_DETECTOR                    (0x0288)
#define RADIOLIB_LR2021_CMD_SET_OOK_WHITENING_PARAMS            (0x0289)
#define RADIOLIB_LR2021_CMD_SET_TX_TEST_MODE                    (0x020E)

// RADIOLIB_LR2021_CMD_SET_DIO_IRQ_CONFIG
#define RADIOLIB_LR2021_IRQ_RX_FIFO                             (0x01UL << 0)   //  31    0     interrupt: Rx FIFO threshold reached
#define RADIOLIB_LR2021_IRQ_TX_FIFO                             (0x01UL << 1)   //  31    0                Tx FIFO threshold reached
#define RADIOLIB_LR2021_IRQ_RNG_REQ_VALID                       (0x01UL << 2)   //  31    0                ranging slave received valid request
#define RADIOLIB_LR2021_IRQ_TX_TIMESTAMP                        (0x01UL << 3)   //  31    0                end of packet Tx timestamp
#define RADIOLIB_LR2021_IRQ_RX_TIMESTAMP                        (0x01UL << 4)   //  31    0                end of packet Rx timestamp
#define RADIOLIB_LR2021_IRQ_PREAMBLE_DETECTED                   (0x01UL << 5)   //  31    0                preamble detected
#define RADIOLIB_LR2021_IRQ_LORA_HEADER_VALID                   (0x01UL << 6)   //  31    0                LoRa header received and valid
#define RADIOLIB_LR2021_IRQ_SYNCWORD_VALID                      (0x01UL << 6)   //  31    0                sync word valid
#define RADIOLIB_LR2021_IRQ_CAD_DETECTED                        (0x01UL << 7)   //  31    0                channel activity detected
#define RADIOLIB_LR2021_IRQ_LORA_HDR_TIMESTAMP                  (0x01UL << 8)   //  31    0                LoRa header timestamp
#define RADIOLIB_LR2021_IRQ_LORA_HDR_CRC_ERROR                  (0x01UL << 9)   //  31    0                LoRa header CRC error
#define RADIOLIB_LR2021_IRQ_EOL                                 (0x01UL << 10)  //  31    0                end of life
#define RADIOLIB_LR2021_IRQ_PA_OCP_OVP                          (0x01UL << 11)  //  31    0                PA overcurrent/overvoltage triggered
#define RADIOLIB_LR2021_IRQ_LORA_TX_RX_HOP                      (0x01UL << 12)  //  31    0                LoRa intra-packet hopping
#define RADIOLIB_LR2021_IRQ_SYNC_FAIL                           (0x01UL << 13)  //  31    0                sync word match detection failed
#define RADIOLIB_LR2021_IRQ_LORA_SYMBOL_END                     (0x01UL << 14)  //  31    0                symbol end
#define RADIOLIB_LR2021_IRQ_LORA_TIMESTAMP_STAT                 (0x01UL << 15)  //  31    0                new stats available
#define RADIOLIB_LR2021_IRQ_ERROR                               (0x01UL << 16)  //  31    0                error other than command error
#define RADIOLIB_LR2021_IRQ_CMD_ERROR                           (0x01UL << 17)  //  31    0                command error
#define RADIOLIB_LR2021_IRQ_RX_DONE                             (0x01UL << 18)  //  31    0                packet received
#define RADIOLIB_LR2021_IRQ_TX_DONE                             (0x01UL << 19)  //  31    0                packet transmitted
#define RADIOLIB_LR2021_IRQ_CAD_DONE                            (0x01UL << 20)  //  31    0                CAD finished
#define RADIOLIB_LR2021_IRQ_TIMEOUT                             (0x01UL << 21)  //  31    0                Rx or Tx timeout
#define RADIOLIB_LR2021_IRQ_CRC_ERROR                           (0x01UL << 22)  //  31    0                CRC error
#define RADIOLIB_LR2021_IRQ_LEN_ERROR                           (0x01UL << 23)  //  31    0                length error on received packet
#define RADIOLIB_LR2021_IRQ_ADDR_ERROR                          (0x01UL << 24)  //  31    0                packet with incorrect address received
#define RADIOLIB_LR2021_IRQ_FHSS                                (0x01UL << 25)  //  31    0                FHSS intra-packet hopping
#define RADIOLIB_LR2021_IRQ_INTER_PACKET_FREQ                   (0x01UL << 26)  //  31    0                inter packet hopping can load new frequency table
#define RADIOLIB_LR2021_IRQ_INTER_NEW_PAYLOAD                   (0x01UL << 27)  //  31    0                inter packet hopping can load new payload
#define RADIOLIB_LR2021_IRQ_RNG_RESP_DONE                       (0x01UL << 28)  //  31    0                slave ranging response sent
#define RADIOLIB_LR2021_IRQ_RNG_REQ_DIS                         (0x01UL << 29)  //  31    0                ranging request discarded
#define RADIOLIB_LR2021_IRQ_RNG_EXCH_VALID                      (0x01UL << 30)  //  31    0                master receive valid ranging response
#define RADIOLIB_LR2021_IRQ_RNG_TIMEOUT                         (0x01UL << 31)  //  31    0                ranging timeout

// RADIOLIB_LR2021_CMD_WRITE_REG_MEM
#define RADIOLIB_LR2021_SPI_MAX_READ_WRITE_LEN                  (256)           //  7     0     maximum length of read/write SPI payload in bytes

// RADIOLIB_LR2021_CMD_SET_SLEEP
#define RADIOLIB_LR2021_SLEEP_32K_CLK_DISABLED                  (0x00UL << 0)   //  0     0     32 kHz clock: disabled
#define RADIOLIB_LR2021_SLEEP_32K_CLK_ENABLED                   (0x01UL << 0)   //  0     0                   enabled
#define RADIOLIB_LR2021_SLEEP_RETENTION_DISABLED                (0x00UL << 1)   //  1     1     configuration retention in sleep mode: disabled
#define RADIOLIB_LR2021_SLEEP_RETENTION_ENABLED                 (0x01UL << 1)   //  1     1                                            enabled


/*!
  \class LR2021
  \brief 
*/
class LR2021: public PhysicalLayer {
  public:
    // introduce PhysicalLayer overloads
    using PhysicalLayer::transmit;
    using PhysicalLayer::receive;
    using PhysicalLayer::startTransmit;
    using PhysicalLayer::startReceive;
    using PhysicalLayer::readData;

    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    explicit LR2021(Module* mod);
    
    int16_t sleep();
    int16_t sleep(uint8_t cfg, uint32_t time);
    
#if !RADIOLIB_GODMODE && !RADIOLIB_LOW_LEVEL
  protected:
#endif
    Module* getMod() override;
    
#if !RADIOLIB_GODMODE
  protected:
#endif
    int16_t SPIcommand(uint16_t cmd, bool write, uint8_t* data, size_t len, const uint8_t* out = NULL, size_t outLen = 0);

#if !RADIOLIB_GODMODE
  private:
#endif
    Module* mod;

    static int16_t SPIparseStatus(uint8_t in);
    static int16_t SPIcheckStatus(Module* mod);

    int16_t readRadioRxFifo(uint8_t* data, size_t len);
    int16_t writeRadioTxFifo(uint8_t* data, size_t len);
    int16_t writeRegMem32(uint32_t addr, const uint32_t* data, size_t len);
    int16_t writeRegMemMask32(uint32_t addr, uint32_t mask, uint32_t data);
    int16_t readRegMem32(uint32_t addr, uint32_t* data, size_t len);

};

#endif

#endif
