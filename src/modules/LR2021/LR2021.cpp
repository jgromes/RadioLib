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