#if !defined(RADIOLIB_LR_COMMON_H)
#define RADIOLIB_LR_COMMON_H

#include "../../Module.h"

#define RADIOLIB_LRXXXX_CMD_NOP                                 (0x0000)
#define RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN                  (256)

// RADIOLIB_LR11X0_CMD_GET_STATUS                                                   MSB   LSB   DESCRIPTION
#define RADIOLIB_LRXXXX_STAT_1_CMD_FAIL                         (0x00UL << 1)   //  3     1     command status: last command could not be executed
#define RADIOLIB_LRXXXX_STAT_1_CMD_PERR                         (0x01UL << 1)   //  3     1                     processing error
#define RADIOLIB_LRXXXX_STAT_1_CMD_OK                           (0x02UL << 1)   //  3     1                     successfully processed
#define RADIOLIB_LRXXXX_STAT_1_CMD_DAT                          (0x03UL << 1)   //  3     1                     successfully processed, data is being transmitted

class LRxxxx {
  public:
    LRxxxx(Module* mod);

  protected:
    // a lot of SPI commands have the same structure and arguments on both LR11xx as well as LR2021
    // the only difference is the 16-bit command code - however, having everything in this base class
    // will actually increase the binary size, because of the extra method calls that are needed
    // for that reason, only the methods that are 100% the same are kept here
    int16_t getStatus(uint8_t* stat1, uint8_t* stat2, uint32_t* irq);

    int16_t writeCommon(uint16_t cmd, uint32_t addrOffset, const uint32_t* data, size_t len, bool nonvolatile);
    int16_t SPIcommand(uint16_t cmd, bool write, uint8_t* data, size_t len, const uint8_t* out = NULL, size_t outLen = 0);

    static int16_t SPIparseStatus(uint8_t in);
    static int16_t SPIcheckStatus(Module* mod);

  private:
    Module* mod;
};

#endif
