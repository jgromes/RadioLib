#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::setBpskModulationParams(uint32_t bitRate, uint8_t pulseShape, bool diff, uint8_t diffInit) {
  uint8_t buff[] = { 
    (uint8_t)((bitRate >> 24) & 0xFF), (uint8_t)((bitRate >> 16) & 0xFF),
    (uint8_t)((bitRate >> 8) & 0xFF), (uint8_t)(bitRate & 0xFF),
    (uint8_t)((pulseShape << 4) | ((uint8_t)diff << 2) | (diffInit & 0x03)),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_BPSK_MODULATION_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setBpskPacketParams(uint8_t payloadLen, uint8_t mode, bool sigFoxControlMsg, uint8_t sigFoxRank) {
  uint8_t buff[] = { 
    payloadLen,
    (uint8_t)((mode << 4) | ((uint8_t)sigFoxControlMsg << 2) | (sigFoxRank & 0x03)),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_BPSK_PACKET_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::lrFhssSetSyncword(uint32_t syncWord) {
  return(this->setU32(RADIOLIB_LR2021_CMD_LR_FHSS_SET_SYNCWORD, syncWord));
}

int16_t LR2021::readLrFhssHoppingTable(uint8_t* hopTable, size_t hopTableLen) {
  // the entry layout of the hopping table has not been decoded
  if(hopTableLen > RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_READ_LR_FHSS_HOPPING_TABLE, false, hopTable, hopTableLen));
}

int16_t LR2021::writeLrFhssHoppingTable(const uint8_t* hopTable, size_t hopTableLen) {
  // the ROM requires at least 9 bytes, the entry layout has not been decoded
  if(hopTableLen < 9) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_WRITE_LR_FHSS_HOPPING_TABLE, true, const_cast<uint8_t*>(hopTable), hopTableLen));
}

int16_t LR2021::setTxTestMode(uint8_t mode) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_TX_TEST_MODE, true, &mode, sizeof(mode)));
}

#endif
