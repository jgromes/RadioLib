#if !defined(_RADIOLIB_SX1231_H)
#define _RADIOLIB_SX1231_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SX1231

#include "../../Module.h"
#include "../RF69/RF69.h"

#define RADIOLIB_SX123X_CHIP_REVISION_2_A                       0x21
#define RADIOLIB_SX123X_CHIP_REVISION_2_B                       0x22
#define RADIOLIB_SX123X_CHIP_REVISION_2_C                       0x23
#define RADIOLIB_SX123X_CHIP_REVISION_2_D                       0x24

// RADIOLIB_SX1231 specific register map
#define RADIOLIB_SX1231_REG_TEST_OOK                            0x6E

// RADIOLIB_SX1231_REG_TEST_OOK
#define RADIOLIB_SX1231_OOK_DELTA_THRESHOLD                     0x0C

// RADIOLIB_SX1231_REG_DIO_MAPPING_1
#define RADIOLIB_SX1231_DIO0_CONT_LOW_BAT                       0b10000000  //  7     6
#define RADIOLIB_SX1231_DIO0_CONT_MODE_READY                    0b11000000  //  7     6
#define RADIOLIB_SX1231_DIO0_CONT_PLL_LOCK                      0b00000000  //  7     6
#define RADIOLIB_SX1231_DIO0_CONT_SYNC_ADDRESS                  0b00000000  //  7     6
#define RADIOLIB_SX1231_DIO0_CONT_TIMEOUT                       0b01000000  //  7     6
#define RADIOLIB_SX1231_DIO0_CONT_RSSI                          0b10000000  //  7     6
#define RADIOLIB_SX1231_DIO0_CONT_MODE_READY                    0b11000000  //  7     6
#define RADIOLIB_SX1231_DIO0_CONT_TX_READY                      0b01000000  //  7     6
#define RADIOLIB_SX1231_DIO0_PACK_LOW_BAT                       0b10000000  //  7     6
#define RADIOLIB_SX1231_DIO0_PACK_PLL_LOCK                      0b11000000  //  7     6
#define RADIOLIB_SX1231_DIO0_PACK_CRC_OK                        0b00000000  //  7     6
#define RADIOLIB_SX1231_DIO0_PACK_PAYLOAD_READY                 0b01000000  //  7     6
#define RADIOLIB_SX1231_DIO0_PACK_SYNC_ADDRESS                  0b10000000  //  7     6
#define RADIOLIB_SX1231_DIO0_PACK_RSSI                          0b11000000  //  7     6
#define RADIOLIB_SX1231_DIO0_PACK_PACKET_SENT                   0b00000000  //  7     6
#define RADIOLIB_SX1231_DIO0_PACK_TX_READY                      0b01000000  //  7     6
#define RADIOLIB_SX1231_DIO1_CONT_LOW_BAT                       0b00100000  //  5     4
#define RADIOLIB_SX1231_DIO1_CONT_PLL_LOCK                      0b00110000  //  5     4
#define RADIOLIB_SX1231_DIO1_CONT_DCLK                          0b00000000  //  5     4
#define RADIOLIB_SX1231_DIO1_CONT_RX_READY                      0b00010000  //  5     4
#define RADIOLIB_SX1231_DIO1_CONT_SYNC_ADDRESS                  0b00110000  //  5     4
#define RADIOLIB_SX1231_DIO1_CONT_TX_READY                      0b00010000  //  5     4
#define RADIOLIB_SX1231_DIO1_PACK_FIFO_LEVEL                    0b00000000  //  5     4
#define RADIOLIB_SX1231_DIO1_PACK_FIFO_FULL                     0b00010000  //  5     4
#define RADIOLIB_SX1231_DIO1_PACK_FIFO_NOT_EMPTY                0b00100000  //  5     4
#define RADIOLIB_SX1231_DIO1_PACK_PLL_LOCK                      0b00110000  //  5     4
#define RADIOLIB_SX1231_DIO1_PACK_TIMEOUT                       0b00110000  //  5     4
#define RADIOLIB_SX1231_DIO2_CONT_DATA                          0b00000000  //  3     2
#define RADIOLIB_SX1231_DIO2_PACK_FIFO_NOT_EMPTY                0b00000000  //  3     2
#define RADIOLIB_SX1231_DIO2_PACK_LOW_BAT                       0b00001000  //  3     2
#define RADIOLIB_SX1231_DIO2_PACK_AUTO_MODE                     0b00001100  //  3     2
#define RADIOLIB_SX1231_DIO2_PACK_DATA                          0b00000100  //  3     2
#define RADIOLIB_SX1231_DIO3_CONT_AUTO_MODE                     0b00000010  //  0     1
#define RADIOLIB_SX1231_DIO3_CONT_RSSI                          0b00000000  //  0     1
#define RADIOLIB_SX1231_DIO3_CONT_RX_READY                      0b00000001  //  0     1
#define RADIOLIB_SX1231_DIO3_CONT_TIMEOUT                       0b00000011  //  0     1
#define RADIOLIB_SX1231_DIO3_CONT_TX_READY                      0b00000001  //  0     1
#define RADIOLIB_SX1231_DIO3_PACK_FIFO_FULL                     0b00000000  //  0     1
#define RADIOLIB_SX1231_DIO3_PACK_LOW_BAT                       0b00000010  //  0     1
#define RADIOLIB_SX1231_DIO3_PACK_PLL_LOCK                      0b00000011  //  0     1
#define RADIOLIB_SX1231_DIO3_PACK_RSSI                          0b00000001  //  0     1
#define RADIOLIB_SX1231_DIO3_PACK_SYNC_ADDRESSS                 0b00000010  //  0     1
#define RADIOLIB_SX1231_DIO3_PACK_TX_READY                      0b00000001  //  0     1

// RADIOLIB_SX1231_REG_DIO_MAPPING_2
#define RADIOLIB_SX1231_DIO4_CONT_LOW_BAT                       0b10000000  //  7     6
#define RADIOLIB_SX1231_DIO4_CONT_PLL_LOCK                      0b11000000  //  7     6
#define RADIOLIB_SX1231_DIO4_CONT_TIMEOUT                       0b00000000  //  7     6
#define RADIOLIB_SX1231_DIO4_CONT_RX_READY                      0b01000000  //  7     6
#define RADIOLIB_SX1231_DIO4_CONT_SYNC_ADDRESS                  0b10000000  //  7     6
#define RADIOLIB_SX1231_DIO4_CONT_TX_READY                      0b01000000  //  7     6
#define RADIOLIB_SX1231_DIO4_PACK_LOW_BAT                       0b10000000  //  7     6
#define RADIOLIB_SX1231_DIO4_PACK_PLL_LOCK                      0b11000000  //  7     6
#define RADIOLIB_SX1231_DIO4_PACK_TIMEOUT                       0b00000000  //  7     6
#define RADIOLIB_SX1231_DIO4_PACK_RSSI                          0b01000000  //  7     6
#define RADIOLIB_SX1231_DIO4_PACK_RX_READY                      0b10000000  //  7     6
#define RADIOLIB_SX1231_DIO4_PACK_MODE_READY                    0b00000000  //  7     6
#define RADIOLIB_SX1231_DIO4_PACK_TX_READY                      0b01000000  //  7     6
#define RADIOLIB_SX1231_DIO5_CONT_LOW_BAT                       0b00100000  //  5     4
#define RADIOLIB_SX1231_DIO5_CONT_MODE_READY                    0b00110000  //  5     4
#define RADIOLIB_SX1231_DIO5_CONT_CLK_OUT                       0b00000000  //  5     4
#define RADIOLIB_SX1231_DIO5_CONT_RSSI                          0b00010000  //  5     4
#define RADIOLIB_SX1231_DIO5_PACK_LOW_BAT                       0b00100000  //  5     4
#define RADIOLIB_SX1231_DIO5_PACK_MODE_READY                    0b00110000  //  5     4
#define RADIOLIB_SX1231_DIO5_PACK_CLK_OUT                       0b00000000  //  5     4
#define RADIOLIB_SX1231_DIO5_PACK_DATA                          0b00010000  //  5     4

/*!
  \class SX1231
  \brief Control class for %SX1231 module. Overrides some methods from RF69 due to different register values.
*/
class SX1231: public RF69  {
  public:
    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    SX1231(Module* mod); // cppcheck-suppress noExplicitConstructor

    /*!
      \brief Initialization method for FSK modem.
      \param config Initialization configuration.
      \details This method initializes the FSK modem with the specified configuration.
      Supports designated initializers when using C++14 or above.
      \returns \ref status_codes
    */
    virtual int16_t begin(const ConfigFSK_t& config);

#if !RADIOLIB_GODMODE
  protected:
#endif
    uint8_t chipRevision = 0;
};

#endif

#endif
