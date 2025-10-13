#include "LR_common.h"

LRxxxx::LRxxxx(Module* mod) {
  this->mod = mod;
}

int16_t LRxxxx::writeRegMem32(uint16_t cmd, uint32_t addr, const uint32_t* data, size_t len) {
  // check maximum size
  if(len > (RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->writeCommon(cmd, addr, data, len, false));
}

int16_t LRxxxx::writeRegMemMask32(uint16_t cmd, uint32_t addr, uint32_t mask, uint32_t data) {
  uint8_t buff[12] = {
    (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
    (uint8_t)((mask >> 24) & 0xFF), (uint8_t)((mask >> 16) & 0xFF), (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    (uint8_t)((data >> 24) & 0xFF), (uint8_t)((data >> 16) & 0xFF), (uint8_t)((data >> 8) & 0xFF), (uint8_t)(data & 0xFF),
  };
  return(this->SPIcommand(cmd, true, buff, sizeof(buff)));
}

int16_t LRxxxx::readRegMem32(uint16_t cmd, uint32_t addr, uint32_t* data, size_t len) {
  // check maximum size
  if(len >= (RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
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

  int16_t state = this->SPIcommand(cmd, false, rplBuff, len*sizeof(uint32_t), reqBuff, sizeof(reqBuff));

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

int16_t LRxxxx::writeCommon(uint16_t cmd, uint32_t addrOffset, const uint32_t* data, size_t len, bool nonvolatile) {
  // build buffers - later we need to ensure endians are correct, 
  // so there is probably no way to do this without copying buffers and iterating
  size_t buffLen = sizeof(uint32_t) + len*sizeof(uint32_t);
  #if RADIOLIB_STATIC_ONLY
    uint8_t dataBuff[sizeof(uint32_t) + RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* dataBuff = new uint8_t[buffLen];
  #endif

  // set the address or offset
  dataBuff[0] = (uint8_t)((addrOffset >> 24) & 0xFF);
  dataBuff[1] = (uint8_t)((addrOffset >> 16) & 0xFF);
  dataBuff[2] = (uint8_t)((addrOffset >> 8) & 0xFF);
  dataBuff[3] = (uint8_t)(addrOffset & 0xFF);

  // convert endians
  for(size_t i = 0; i < len; i++) {
    uint32_t bin = 0;
    if(nonvolatile) {
      uint32_t* ptr = const_cast<uint32_t*>(data) + i;
      bin = RADIOLIB_NONVOLATILE_READ_DWORD(ptr);
    } else {
      bin = data[i];
    }
    dataBuff[4 + i*sizeof(uint32_t)] = (uint8_t)((bin >> 24) & 0xFF);
    dataBuff[5 + i*sizeof(uint32_t)] = (uint8_t)((bin >> 16) & 0xFF);
    dataBuff[6 + i*sizeof(uint32_t)] = (uint8_t)((bin >> 8) & 0xFF);
    dataBuff[7 + i*sizeof(uint32_t)] = (uint8_t)(bin & 0xFF);
  }

  int16_t state = this->mod->SPIwriteStream(cmd, dataBuff, buffLen, true, false);
  #if !RADIOLIB_STATIC_ONLY
    delete[] dataBuff;
  #endif
  return(state);
}

int16_t LRxxxx::SPIcommand(uint16_t cmd, bool write, uint8_t* data, size_t len, const uint8_t* out, size_t outLen) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;
  if(!write) {
    // the SPI interface of LR11x0 requires two separate transactions for reading
    // send the 16-bit command
    state = this->mod->SPIwriteStream(cmd, out, outLen, true, false);
    RADIOLIB_ASSERT(state);

    // read the result without command
    this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_0;
    state = this->mod->SPIreadStream(RADIOLIB_LRXXXX_CMD_NOP, data, len, true, false);
    this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_16;

  } else {
    // write is just a single transaction
    state = this->mod->SPIwriteStream(cmd, data, len, true, true);
  
  }
  
  return(state);
}
