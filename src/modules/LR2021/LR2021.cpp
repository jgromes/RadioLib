#include "LR2021.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

LR2021::LR2021(Module* mod) : PhysicalLayer(), LRxxxx(mod) {
  this->freqStep = RADIOLIB_LR2021_FREQUENCY_STEP_SIZE;
  this->maxPacketLength = RADIOLIB_LR2021_MAX_PACKET_LENGTH;
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

int16_t LR2021::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  this->mod->init();
  this->mod->hal->pinMode(this->mod->getIrq(), this->mod->hal->GpioModeInput);
  this->mod->hal->pinMode(this->mod->getGpio(), this->mod->hal->GpioModeInput);
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] = RADIOLIB_LR2021_CMD_READ_REG_MEM_32;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] = RADIOLIB_LR2021_CMD_WRITE_REG_MEM_32;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_NOP] = RADIOLIB_LR2021_CMD_NOP;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_STATUS] = RADIOLIB_LR2021_CMD_GET_STATUS;

  // try to find the chip - this will also reset the module at least once
  if(!this->findChip()) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("No LR2021 found!");
    this->mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  RADIOLIB_DEBUG_BASIC_PRINTLN("M\tLR2021");

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set TCXO control, if requested
  if(!this->XTAL && tcxoVoltage > 0.0f) {
    state = setTCXO(tcxoVoltage);
    RADIOLIB_ASSERT(state);
  }

  // configure settings not accessible by API
  state = config(RADIOLIB_LR2021_PACKET_TYPE_LORA);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setBandwidth(bw);
  RADIOLIB_ASSERT(state);

  state = setSpreadingFactor(sf);
  RADIOLIB_ASSERT(state);

  state = setCodingRate(cr);
  RADIOLIB_ASSERT(state);

  state = setSyncWord(syncWord);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  state = setCRC(2);
  RADIOLIB_ASSERT(state);

  state = invertIQ(false);
  RADIOLIB_ASSERT(state);

  return(RADIOLIB_ERR_NONE);
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

bool LR2021::findChip(void) {
  // this is the only version mentioned in datasheet
  const uint8_t expMajor = 0x01;
  const uint8_t expMinor = 0x18;

  uint8_t i = 0;
  bool flagFound = false;
  uint8_t fwMajor = 0, fwMinor = 0;
  while((i < 10) && !flagFound) {
    // reset the module
    reset();

    // read the version
    int16_t state = getVersion(&fwMajor, &fwMinor);
    RADIOLIB_ASSERT(state);

    if((fwMajor == expMajor) && (fwMajor == expMinor)) {
      RADIOLIB_DEBUG_BASIC_PRINTLN("Found LR2021");
      RADIOLIB_DEBUG_BASIC_PRINTLN("Base FW version: %d.%d", (int)fwMajor, (int)fwMinor);
      flagFound = true;
    } else {
      RADIOLIB_DEBUG_BASIC_PRINTLN("LR2021 not found! (%d of 10 tries) FW version: = %d.%d", (int)fwMajor, (int)fwMinor);
      RADIOLIB_DEBUG_BASIC_PRINTLN("Expected: %d.%d", (int)expMajor, (int)expMinor);
      this->mod->hal->delay(10);
      i++;
    }
  }

  return(flagFound);
}

int16_t LR2021::config(uint8_t modem) {
  // set Rx/Tx fallback mode to STDBY_RC
  int16_t state = this->setRxTxFallbackMode(RADIOLIB_LR2021_FALLBACK_MODE_STBY_RC);
  RADIOLIB_ASSERT(state);

  // clear IRQ
  state = this->clearIrq(RADIOLIB_LR2021_IRQ_ALL);
  RADIOLIB_ASSERT(state);

  // calibrate all blocks
  (void)this->calibrate(RADIOLIB_LR2021_CALIBRATE_ALL);

  // wait for calibration completion
  this->mod->hal->delay(5);
  while(this->mod->hal->digitalRead(this->mod->getGpio())) {
    this->mod->hal->yield();
  }
  
  // if something failed, show the device errors
  // TODO legacy from LR11x0, chech if this really works on LR2021
  #if RADIOLIB_DEBUG_BASIC
  if(state != RADIOLIB_ERR_NONE) {
    // unless mode is forced to standby, device errors will be 0
    standby();
    uint16_t errors = 0;
    getErrors(&errors);
    RADIOLIB_DEBUG_BASIC_PRINTLN("Calibration failed, device errors: 0x%X", errors);
  }
  #endif

  // set modem
  state = this->setPacketType(modem);
  return(state);
}

#endif