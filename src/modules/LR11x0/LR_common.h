#if !defined(RADIOLIB_LR_COMMON_H)
#define RADIOLIB_LR_COMMON_H

#include "../../Module.h"

#define RADIOLIB_LRXXXX_CMD_NOP                         (0x0000)
#define RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN          (256)

class LRxxxx {
  public:
    LRxxxx(Module* mod);

  protected:
    int16_t writeRegMem32(uint16_t cmd, uint32_t addr, const uint32_t* data, size_t len);
    int16_t writeRegMemMask32(uint16_t cmd, uint32_t addr, uint32_t mask, uint32_t data);
    int16_t readRegMem32(uint16_t cmd, uint32_t addr, uint32_t* data, size_t len);
    
    int16_t writeCommon(uint16_t cmd, uint32_t addrOffset, const uint32_t* data, size_t len, bool nonvolatile);
    int16_t SPIcommand(uint16_t cmd, bool write, uint8_t* data, size_t len, const uint8_t* out = NULL, size_t outLen = 0);

  private:
    Module* mod;
};

#endif
