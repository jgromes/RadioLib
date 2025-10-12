#include "LR_common.h"

int16_t LR_writeCommon(Module* mod, uint16_t cmd, uint32_t addrOffset, const uint32_t* data, size_t len, bool nonvolatile) {
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

  int16_t state = mod->SPIwriteStream(cmd, dataBuff, buffLen, true, false);
  #if !RADIOLIB_STATIC_ONLY
    delete[] dataBuff;
  #endif
  return(state);
}
