#include "LR2021.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

LR2021::LR2021(Module* mod) : PhysicalLayer() {
  this->freqStep = RADIOLIB_LR2021_FREQUENCY_STEP_SIZE;
  this->maxPacketLength = RADIOLIB_LR2021_MAX_PACKET_LENGTH;
  this->mod = mod;
  this->irqMap[RADIOLIB_IRQ_TX_DONE] = RADIOLIB_LR2021_IRQ_TX_DONE;
  this->irqMap[RADIOLIB_IRQ_RX_DONE] = RADIOLIB_LR2021_IRQ_RX_DONE;
  this->irqMap[RADIOLIB_IRQ_PREAMBLE_DETECTED] = RADIOLIB_LR2021_IRQ_PREAMBLE_DETECTED;
  this->irqMap[RADIOLIB_IRQ_SYNC_WORD_VALID] = RADIOLIB_LR2021_IRQ_LORA_HEADER_VALID;
  this->irqMap[RADIOLIB_IRQ_HEADER_VALID] = RADIOLIB_LR2021_IRQ_LORA_HEADER_VALID;
  this->irqMap[RADIOLIB_IRQ_HEADER_ERR] = RADIOLIB_LR2021_IRQ_LORA_HDR_CRC_ERROR;
  this->irqMap[RADIOLIB_IRQ_CRC_ERR] = RADIOLIB_LR2021_IRQ_CRC_ERROR;
  this->irqMap[RADIOLIB_IRQ_CAD_DONE] = RADIOLIB_LR2021_IRQ_CAD_DONE;
  this->irqMap[RADIOLIB_IRQ_CAD_DETECTED] = RADIOLIB_LR2021_IRQ_CAD_DETECTED;
  this->irqMap[RADIOLIB_IRQ_TIMEOUT] = RADIOLIB_LR2021_IRQ_TIMEOUT;
}

Module* LR2021::getMod() {
  return(this->mod);
}

int16_t LR2021::SPIparseStatus(uint8_t in) {
  /*if((in & 0b00001110) == RADIOLIB_LR11X0_STAT_1_CMD_PERR) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  } else if((in & 0b00001110) == RADIOLIB_LR11X0_STAT_1_CMD_FAIL) {
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  } else if((in == 0x00) || (in == 0xFF)) {
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }*/
  return(RADIOLIB_ERR_NONE);
}

int16_t LR2021::SPIcheckStatus(Module* mod) {
  // the status check command doesn't return status in the same place as other read commands,
  // but only as the first byte (as with any other command), hence LR2021::SPIcommand can't be used
  // it also seems to ignore the actual command, and just sending in bunch of NOPs will work 
  uint8_t buff[6] = { 0 };
  mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_0;
  int16_t state = mod->SPItransferStream(NULL, 0, false, NULL, buff, sizeof(buff), true);
  mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_8;
  RADIOLIB_ASSERT(state);
  return(LR2021::SPIparseStatus(buff[0]));
}

int16_t LR2021::SPIcommand(uint16_t cmd, bool write, uint8_t* data, size_t len, const uint8_t* out, size_t outLen) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;
  if(!write) {
    // the SPI interface of LR2021 requires two separate transactions for reading
    // send the 16-bit command
    state = this->mod->SPIwriteStream(cmd, out, outLen, true, false);
    RADIOLIB_ASSERT(state);

    // read the result without command
    this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_0;
    state = this->mod->SPIreadStream(RADIOLIB_LR2021_CMD_NOP, data, len, true, false);
    this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_16;

  } else {
    // write is just a single transaction
    state = this->mod->SPIwriteStream(cmd, data, len, true, true);
  
  }
  
  return(state);
}

#endif