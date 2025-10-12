#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::readRadioRxFifo(uint8_t* data, size_t len) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_READ_RX_FIFO, false, data, len, NULL, 0));
}

int16_t LR2021::writeRadioTxFifo(uint8_t* data, size_t len) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_WRITE_TX_FIFO, true, data, len, NULL, 0));
}

// TODO reuse LR11x0 method
int16_t LR2021::writeRegMem32(uint32_t addr, const uint32_t* data, size_t len) {
  // check maximum size
  if(len > (RADIOLIB_LR2021_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(LR_writeCommon(this->mod, RADIOLIB_LR2021_CMD_WRITE_REG_MEM_32, addr, data, len, false));
}

// TODO reuse LR11x0 method
int16_t LR2021::writeRegMemMask32(uint32_t addr, uint32_t mask, uint32_t data) {
  uint8_t buff[12] = {
    (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
    (uint8_t)((mask >> 24) & 0xFF), (uint8_t)((mask >> 16) & 0xFF), (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    (uint8_t)((data >> 24) & 0xFF), (uint8_t)((data >> 16) & 0xFF), (uint8_t)((data >> 8) & 0xFF), (uint8_t)(data & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_WRITE_REG_MEM_MASK_32, true, buff, sizeof(buff)));
}

// TODO reuse LR11x0 method
int16_t LR2021::readRegMem32(uint32_t addr, uint32_t* data, size_t len) {
  // check maximum size
  if(len >= (RADIOLIB_LR2021_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // the request contains the address and length
  uint8_t reqBuff[5] = {
    (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF),
    (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
    (uint8_t)len,
  };

  // build buffers - later we need to ensure endians are correct, 
  // so there is probably no way to do this without copying buffers and iterating
  #if RADIOLIB_STATIC_ONLY
    uint8_t rplBuff[RADIOLIB_LR2021_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* rplBuff = new uint8_t[len*sizeof(uint32_t)];
  #endif

  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_READ_REG_MEM_32, false, rplBuff, len*sizeof(uint32_t), reqBuff, sizeof(reqBuff));

  // convert endians
  if(data && (state == RADIOLIB_ERR_NONE)) {
    for(size_t i = 0; i < len; i++) {
      data[i] = ((uint32_t)rplBuff[2 + i*sizeof(uint32_t)] << 24) | ((uint32_t)rplBuff[3 + i*sizeof(uint32_t)] << 16) | ((uint32_t)rplBuff[4 + i*sizeof(uint32_t)] << 8) | (uint32_t)rplBuff[5 + i*sizeof(uint32_t)];
    }
  }

  #if !RADIOLIB_STATIC_ONLY
    delete[] rplBuff;
  #endif
  
  return(state);
}
#endif
