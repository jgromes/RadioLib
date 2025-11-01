#include "LR2021.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

LR2021::LR2021(Module* mod) : PhysicalLayer(), LRxxxx(mod) {
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
  this->mod->spiConfig.stream = true;
  this->mod->spiConfig.parseStatusCb = SPIparseStatus;
  this->mod->spiConfig.checkStatusCb = SPIcheckStatus;
}

int16_t LR2021::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength) {
  this->mod->init();
  this->mod->hal->pinMode(this->mod->getIrq(), this->mod->hal->GpioModeInput);
  this->mod->hal->pinMode(this->mod->getGpio(), this->mod->hal->GpioModeInput);
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR] = Module::BITS_32;
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_16;
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_8;
  this->mod->spiConfig.statusPos = 0;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] = RADIOLIB_LR2021_CMD_READ_REG_MEM_32;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] = RADIOLIB_LR2021_CMD_WRITE_REG_MEM_32;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_NOP] = RADIOLIB_LR2021_CMD_NOP;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_STATUS] = RADIOLIB_LR2021_CMD_GET_STATUS;
  this->mod->spiConfig.stream = true;
  this->mod->spiConfig.parseStatusCb = SPIparseStatus;
  this->mod->spiConfig.checkStatusCb = SPIcheckStatus;

  // reset the radio
  (void)reset();

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t LR2021::standby() {
  return(this->standby(RADIOLIB_LR2021_STANDBY_RC));
}

int16_t LR2021::standby(uint8_t mode, bool wakeup) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  if(wakeup) {
    // send a NOP command - this pulls the NSS low to exit the sleep mode,
    // while preventing interference with possible other SPI transactions
    (void)this->mod->SPIwriteStream((uint16_t)RADIOLIB_LR2021_CMD_NOP, NULL, 0, false, false);
  }
  
  uint8_t buff[] = { mode };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_STANDBY, true, buff, sizeof(buff)));
}

int16_t LR2021::sleep() {
  return(this->sleep(RADIOLIB_LR2021_SLEEP_RETENTION_ENABLED, 0));
}

int16_t LR2021::sleep(uint8_t cfg, uint32_t time) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  uint8_t buff[] = { cfg,
    (uint8_t)((time >> 24) & 0xFF), (uint8_t)((time >> 16) & 0xFF),
    (uint8_t)((time >> 8) & 0xFF), (uint8_t)(time & 0xFF),
  };

  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_SET_SLEEP, true, buff, sizeof(buff));

  // wait for the module to safely enter sleep mode
  this->mod->hal->delay(1);

  return(state);
}

Module* LR2021::getMod() {
  return(this->mod);
}

#endif