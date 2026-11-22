#include "LR2021.h"

#include "../LR11x0/LR_common.h"
#include "LR2021_registers.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::readRadioRxFifo(uint8_t* data, size_t len) {
  // FIFO read is just a single transaction sent without the status code
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_0;
  int16_t state = this->mod->SPIreadStream(RADIOLIB_LR2021_CMD_READ_RX_FIFO, data, len, true, false);
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_16;
  return(state);
}

int16_t LR2021::writeRadioTxFifo(const uint8_t* data, size_t len) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_WRITE_TX_FIFO, true, const_cast<uint8_t*>(data), len, NULL, 0));
}

int16_t LR2021::writeRegMem32(uint32_t addr, const uint32_t* data, size_t len) {
  // check maximum size
  if(len > (RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->writeCommon(RADIOLIB_LR2021_CMD_WRITE_REG_MEM_32, addr, data, len, false));
}

int16_t LR2021::writeRegMemMask32(uint32_t addr, uint32_t mask, uint32_t data) {
  uint8_t buff[11] = {
    (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
    (uint8_t)((mask >> 24) & 0xFF), (uint8_t)((mask >> 16) & 0xFF), (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    (uint8_t)((data >> 24) & 0xFF), (uint8_t)((data >> 16) & 0xFF), (uint8_t)((data >> 8) & 0xFF), (uint8_t)(data & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_WRITE_REG_MEM_MASK_32, true, buff, sizeof(buff)));
}

int16_t LR2021::readRegMem32(uint32_t addr, uint32_t* data, size_t len) {
  // check maximum size
  if(len > (RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // the request contains the address and length
  uint8_t reqBuff[4] = {
    (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF),
    (uint8_t)(addr & 0xFF), (uint8_t)len,
  };

  // build buffers - later we need to ensure endians are correct, 
  // so there is probably no way to do this without copying buffers and iterating
  #if RADIOLIB_STATIC_ONLY
    uint8_t rplBuff[RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* rplBuff = new uint8_t[len*sizeof(uint32_t)];
  #endif

  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_READ_REG_MEM_32, false, rplBuff, len*sizeof(uint32_t), reqBuff, sizeof(reqBuff));

  // convert endians
  if(data && (state == RADIOLIB_ERR_NONE)) {
    for(size_t i = 0; i < len; i++) {
      data[i] = ((uint32_t)rplBuff[i*sizeof(uint32_t)] << 24) | ((uint32_t)rplBuff[1 + i*sizeof(uint32_t)] << 16) | ((uint32_t)rplBuff[2 + i*sizeof(uint32_t)] << 8) | (uint32_t)rplBuff[3 + i*sizeof(uint32_t)];
    }
  }

  #if !RADIOLIB_STATIC_ONLY
    delete[] rplBuff;
  #endif
  
  return(state);
}

int16_t LR2021::setFs(void) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_FS, true, NULL, 0));
}

int16_t LR2021::setAdditionalRegToRetain(uint8_t slot, uint32_t addr) {
  uint8_t buff[] = { 
    slot, (uint8_t)((addr >> 16) & 0xFF),
    (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
  };

  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_ADDITIONAL_REG_TO_RETAIN, true, buff, sizeof(buff)));
}

int16_t LR2021::setRx(uint32_t timeout) {
  uint8_t buff[] = {
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RX, true, buff, sizeof(buff)));
}

int16_t LR2021::setTx(uint32_t timeout) {
  uint8_t buff[] = {
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_TX, true, buff, sizeof(buff)));
}

int16_t LR2021::setRxTxFallbackMode(uint8_t mode) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RX_TX_FALLBACK_MODE, true, &mode, sizeof(mode)));
}

int16_t LR2021::setRxDutyCycle(uint32_t rxMaxTime, uint32_t cycleTime, uint8_t mode) {
  uint8_t buff[] = {
    (uint8_t)((rxMaxTime >> 16) & 0xFF), (uint8_t)((rxMaxTime >> 8) & 0xFF), (uint8_t)(rxMaxTime & 0xFF),
    (uint8_t)((cycleTime >> 16) & 0xFF), (uint8_t)((cycleTime >> 8) & 0xFF), (uint8_t)(cycleTime & 0xFF),
    (uint8_t)(mode << 4),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RX_DUTY_CYCLE, true, buff, sizeof(buff)));
}

int16_t LR2021::autoTxRx(uint32_t delay, uint8_t mode, uint32_t timeout) {
  uint8_t buff[] = {
    (uint8_t)((delay >> 16) & 0xFF), (uint8_t)((delay >> 8) & 0xFF), (uint8_t)(delay & 0xFF), mode,
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_AUTO_RX_TX, true, buff, sizeof(buff)));
}

int16_t LR2021::getRxPktLength(uint16_t* len) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_RX_PKT_LENGTH, false, buff, sizeof(buff));
  if(len) { *len = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1]; }
  return(state);
}

int16_t LR2021::resetRxStats(void) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_RESET_RX_STATS, true, NULL, 0));
}

int16_t LR2021::setDefaultRxTxTimeout(uint32_t rxTimeout, uint32_t txTimeout) {
  uint8_t buff[] = {
    (uint8_t)((rxTimeout >> 16) & 0xFF), (uint8_t)((rxTimeout >> 8) & 0xFF), (uint8_t)(rxTimeout & 0xFF),
    (uint8_t)((txTimeout >> 16) & 0xFF), (uint8_t)((txTimeout >> 8) & 0xFF), (uint8_t)(txTimeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_DEFAULT_RX_TX_TIMEOUT, true, buff, sizeof(buff)));
}

int16_t LR2021::setRegMode(uint8_t mode) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_REG_MODE, true, &mode, sizeof(mode)));
}

int16_t LR2021::calibrate(uint8_t blocks) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CALIBRATE, true, &blocks, sizeof(blocks)));
}

int16_t LR2021::calibrateFrontEnd(const uint16_t freq[3]) {
  uint8_t buff[] = {
    (uint8_t)((freq[0] >> 8) & 0xFF), (uint8_t)(freq[0] & 0xFF),
    (uint8_t)((freq[1] >> 8) & 0xFF), (uint8_t)(freq[1] & 0xFF),
    (uint8_t)((freq[2] >> 8) & 0xFF), (uint8_t)(freq[2] & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CALIB_FRONT_END, true, buff, sizeof(buff)));
}

int16_t LR2021::getVbat(uint8_t resolution, uint16_t* vbat) {
  uint8_t reqBuff[] = { (uint8_t)(RADIOLIB_LR2021_VBAT_FORMAT_MV | ((resolution - RADIOLIB_LR2021_MEAS_RESOLUTION_OFFSET) & 0x07)) };
  uint8_t rplBuff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_V_BAT, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  if(vbat) { *vbat = ((uint16_t)(rplBuff[0]) << 8) | (uint16_t)rplBuff[1]; }
  return(state);
}

int16_t LR2021::getTemp(uint8_t source, uint8_t resolution, float* temp) {
  // reading of temperature in degrees seems broken and the datasheet disagrees with reference implementation
  // so we read out the raw value and convert it here
  uint8_t reqBuff[] = { (uint8_t)((source & 0x30) | RADIOLIB_LR2021_TEMP_FORMAT_RAW | ((resolution - RADIOLIB_LR2021_MEAS_RESOLUTION_OFFSET) & 0x07)) };
  uint8_t rplBuff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_TEMP, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  if(temp) { 
    uint16_t raw = ((uint16_t)(rplBuff[0]) << 8) | (uint16_t)rplBuff[1];
    *temp = (25.0f + (0.7295f * (1000.0f / 1.7f))) - raw * ((1.35f / 8192.0f) * (1000.0f / 1.7f));
  }
  return(state);
}

int16_t LR2021::setEolConfig(bool enable, uint8_t trim) {
  uint8_t buff[] = { (uint8_t)((trim & 0x06) | (uint8_t)enable) };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_EOL_CONFIG, true, buff, sizeof(buff)));
}

int16_t LR2021::getRandomNumber(uint32_t* rnd) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_RANDOM_NUMBER, false, buff, sizeof(buff));
  if(rnd) { *rnd = ((uint32_t)(buff[0]) << 24) | ((uint32_t)(buff[1]) << 16) | ((uint32_t)(buff[2]) << 8) | (uint32_t)buff[3];  }
  return(state);
}

int16_t LR2021::getVersion(uint8_t* major, uint8_t* minor) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_VERSION, false, buff, sizeof(buff));
  if(major)   { *major = buff[0]; }
  if(minor)   { *minor = buff[1]; }
  return(state);
}

int16_t LR2021::clearErrors(void) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CLEAR_ERRORS, true, NULL, 0));
}

int16_t LR2021::getErrors(uint16_t* err) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_ERRORS, false, buff, sizeof(buff));
  if(err) { *err = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1];  }
  return(state);
}

int16_t LR2021::setDioFunction(uint8_t dio, uint8_t func, uint8_t pullDrive) {
  uint8_t buff[] = { dio, (uint8_t)((func & 0xF0) | (pullDrive & 0x0F)) };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_DIO_FUNCTION, true, buff, sizeof(buff)));
}

int16_t LR2021::setDioRfSwitchConfig(uint8_t dio, uint8_t func) {
  uint8_t buff[] = { dio, func };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_DIO_RF_SWITCH_CONFIG, true, buff, sizeof(buff)));
}

int16_t LR2021::setDioIrqConfig(uint8_t dio, uint32_t irq) {
  uint8_t buff[] = { dio, 
    (uint8_t)((irq >> 24) & 0xFF), (uint8_t)((irq >> 16) & 0xFF),
    (uint8_t)((irq >> 8) & 0xFF), (uint8_t)(irq & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_DIO_IRQ_CONFIG, true, buff, sizeof(buff)));
}

int16_t LR2021::clearIrqState(uint32_t irq) {
  return(this->setU32(RADIOLIB_LR2021_CMD_CLEAR_IRQ, irq));
}

int16_t LR2021::getAndClearIrqStatus(uint32_t* irq) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_AND_CLEAR_IRQ_STATUS, false, buff, sizeof(buff));
  if(irq) { *irq = ((uint32_t)(buff[0]) << 24) | ((uint32_t)(buff[1]) << 16) | ((uint32_t)(buff[2]) << 8) |(uint32_t)buff[3]; }
  return(state);
}

int16_t LR2021::configFifoIrq(uint8_t rxFifoIrq, uint8_t txFifoIrq, uint16_t rxHighThreshold, uint16_t txLowThreshold, uint16_t rxLowThreshold, uint16_t txHighThreshold) {
  uint8_t buff[] = {
	rxFifoIrq,
	txFifoIrq,
	(uint8_t)((rxHighThreshold >> 8) & 0xFF),
	(uint8_t)(rxHighThreshold & 0xFF),
	(uint8_t)((txLowThreshold >> 8) & 0xFF),
	(uint8_t)(txLowThreshold & 0xFF),
	(uint8_t)((rxLowThreshold >> 8) & 0xFF),
	(uint8_t)(rxLowThreshold & 0xFF),
	(uint8_t)((txHighThreshold >> 8) & 0xFF),
	(uint8_t)(txHighThreshold & 0xFF),
	};
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CONFIG_FIFO_IRQ, true, buff, sizeof(buff)));
}

int16_t LR2021::getFifoIrqFlags(uint8_t* rxFifoFlags, uint8_t* txFifoFlags) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_FIFO_IRQ_FLAGS, false, buff, sizeof(buff));
  if(rxFifoFlags) { *rxFifoFlags = buff[0]; }
  if(txFifoFlags) { *txFifoFlags = buff[1]; }
  return(state);
}

int16_t LR2021::clearFifoIrqFlags(uint8_t rxFifoFlags, uint8_t txFifoFlags) {
  uint8_t buff[] = { rxFifoFlags, txFifoFlags };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CLEAR_FIFO_IRQ_FLAGS, true, buff, sizeof(buff)));
}

int16_t LR2021::getAndClearFifoIrqFlags(uint8_t* rxFifoFlags, uint8_t* txFifoFlags) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_AND_CLEAR_FIFO_IRQ_FLAGS, false, buff, sizeof(buff));
  if(rxFifoFlags) { *rxFifoFlags = buff[0]; }
  if(txFifoFlags) { *txFifoFlags = buff[1]; }
  return(state);
}

int16_t LR2021::getRxFifoLevel(uint16_t* level) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_RX_FIFO_LEVEL, false, buff, sizeof(buff));
  if(level) { *level = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1];  }
  return(state);
}

int16_t LR2021::getTxFifoLevel(uint16_t* level) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_TX_FIFO_LEVEL, false, buff, sizeof(buff));
  if(level) { *level = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1];  }
  return(state);
}

int16_t LR2021::clearRxFifo(void) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CLEAR_RX_FIFO, true, NULL, 0));
}

int16_t LR2021::clearTxFifo(void) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CLEAR_TX_FIFO, true, NULL, 0));
}

int16_t LR2021::configLfClock(uint8_t cfg) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CONFIG_LF_CLOCK, true, &cfg, sizeof(cfg)));
}

int16_t LR2021::configClkOutputs(uint8_t scaling) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CONFIG_CLK_OUTPUTS, true, &scaling, sizeof(scaling)));
}

int16_t LR2021::setTcxoMode(uint8_t tune, uint32_t startTime) {
  uint8_t buff[] = { (uint8_t)(tune & 0x07),
    (uint8_t)((startTime >> 24) & 0xFF), (uint8_t)((startTime >> 16) & 0xFF),
    (uint8_t)((startTime >> 8) & 0xFF), (uint8_t)(startTime & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_TCXO_MODE, true, buff, sizeof(buff)));
}

int16_t LR2021::setXoscCpTrim(uint8_t xta, uint8_t xtb, uint8_t startTime) {
  uint8_t buff[] = { (uint8_t)(xta & 0x3F), (uint8_t)(xtb & 0x3F), startTime };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_XOSC_CP_TRIM, true, buff, sizeof(buff)));
}

int16_t LR2021::activatePram(void) {
  uint8_t buff[] = { RADIOLIB_LR2021_CMD_NOP };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_ACTIVATE_PRAM, true, buff, sizeof(buff)));
}

int16_t LR2021::checkPramLoaded(bool* loaded) {
  uint32_t val = 0;
  int16_t state = this->readRegMem32(RADIOLIB_LR2021_PRAM_ADDR_LOADED, &val, 1);
  if(loaded) { *loaded = (val == RADIOLIB_LR2021_PRAM_LOADED_MAGIC);  }
  return(state);
}

int16_t LR2021::getPramVersion(uint16_t* version) {
  uint32_t val = 0;
  int16_t state = this->readRegMem32(RADIOLIB_LR2021_PRAM_ADDR_VERSION, &val, 1);
  if(version) { *version = ((val >>8) & 0xFFFF);  }
  return(state);
}

int16_t LR2021::setRegulatorLDO() {
  return(this->setRegMode(RADIOLIB_LR2021_REG_MODE_SIMO_OFF));
}

int16_t LR2021::setRegulatorDCDC() {
  return(this->setRegMode(RADIOLIB_LR2021_REG_MODE_SIMO_NORMAL));
}

// workaround: port of semtech's code altered to use local freq/highFreq - magic numbers are theirs
// https://github.com/Lora-net/usp/blob/351b20153506/smtc_rac_lib/radio_drivers/lr20xx_driver/src/lr20xx_workarounds.c
int16_t LR2021::setDCDCworkaround() {
  uint32_t adcCtrlRaw = 0;
  int16_t state = this->readRegMem32(RADIOLIB_LR2021_REG_DCDC_ADC_CTRL, &adcCtrlRaw, 1);
  RADIOLIB_ASSERT(state);
  const uint32_t anaDec = ( adcCtrlRaw >> 8 ) & 0x7;

  if (!this->highFreq && (anaDec == 1 || anaDec == 2)) {
    state = this->writeRegMemMask32(RADIOLIB_LR2021_REG_DCDC_SWITCHER, 0x0FUL << 20, 11UL << 20);
    RADIOLIB_ASSERT(state);
    state = this->writeRegMemMask32(RADIOLIB_LR2021_REG_DCDC_SWITCHER, 0x0FUL << 16, 13UL << 16);
    RADIOLIB_ASSERT(state);
  } else {
    state = this->writeRegMemMask32(RADIOLIB_LR2021_REG_DCDC_SWITCHER, 0x0FUL << 20, 15UL << 20);
    RADIOLIB_ASSERT(state);    
    state = this->writeRegMemMask32(RADIOLIB_LR2021_REG_DCDC_SWITCHER, 0x0FUL << 16, 15UL << 16);
    RADIOLIB_ASSERT(state);
  }

  // semtech number: 2800000 * 1.048576f (lines 558, 666)
  uint32_t freq_lf = 2936012;
  if (anaDec == 1) {
    // semtech number: 4300000 * 1.048576f (lines 554, 666)
    freq_lf = 4508876;
  } 
  state = this->writeRegMem32(RADIOLIB_LR2021_REG_DCDC_FREQ_LF, &freq_lf, 1);
  RADIOLIB_ASSERT(state);
  
  state = this->setFrequency(this->freqHz, true);
  return(state);
}

int16_t LR2021::resetDCDCworkaround() {
  int16_t state = this->writeRegMemMask32(RADIOLIB_LR2021_REG_DCDC_SWITCHER, 0x0FUL << 20, 15UL << 20);
  RADIOLIB_ASSERT(state);

  state = this->writeRegMemMask32(RADIOLIB_LR2021_REG_DCDC_SWITCHER, 0x0FUL << 16, 15UL << 16);
  RADIOLIB_ASSERT(state);

  // semtech number: 2800000 * 1.048576f (lines 558, 666)
  uint32_t freq_lf = 2936012;
  state = this->writeRegMem32(RADIOLIB_LR2021_REG_DCDC_FREQ_LF, &freq_lf, 1);
  RADIOLIB_ASSERT(state);

  if(this->freqHz) {
    state = this->setFrequency(this->freqHz, true);
  }
  return(state);
}

int16_t LR2021::setFifoSize(bool rx, size_t size) {
  // set the base
  uint32_t fifo_base = rx ? RADIOLIB_LR2021_RX_FIFO_BASE_ADDR : RADIOLIB_LR2021_TX_FIFO_BASE_ADDR;
  int16_t state = this->writeRegMem32(rx ? RADIOLIB_LR2021_REG_RX_FIFO_BASE : RADIOLIB_LR2021_REG_TX_FIFO_BASE, &fifo_base, 1);
  RADIOLIB_ASSERT(state);

  // set the size
  // Rx and Tx FIFO sizes are "coded" differently in the official driver,
  // and seem to suggest different size limits
  uint32_t fifo_size;
  if(rx) {
    if((size < 24) || (size > 1024)) {
      return(RADIOLIB_ERR_PACKET_TOO_LONG);
    }
    fifo_size = size - 24; 
  } else {
    if((size < 4) || (size > 1024)) {
      return(RADIOLIB_ERR_PACKET_TOO_LONG);
    }
    fifo_size = size - 4; 
  }
  state = this->writeRegMem32(rx ? RADIOLIB_LR2021_REG_RX_FIFO_SIZE : RADIOLIB_LR2021_REG_TX_FIFO_SIZE, &fifo_size, 1);

  //! \todo [LR2021] Implement FIFO config retention through sleep mode
  return(state);
}

int16_t LR2021::setIQStream(uint32_t rate, uint8_t source, bool fifo) {
  // rate is configured via LoRa bandwidth
  int16_t state = this->setBandwidth(rate/4);
  RADIOLIB_ASSERT(state);

  // if the destination is FIFO, enable 1024-byte RX FIFO size
  if(fifo) {
    state = this->setFifoSize(true, 1024);
    RADIOLIB_ASSERT(state);
  }

  // setup config for the stream
  uint8_t cfg = RADIOLIB_LR2021_CMD_IQ_STREAM_BANK_ADDR | RADIOLIB_LR2021_CMD_IQ_STREAM_MODE_RAM | source;
  uint16_t len = 0xFF00; // magic number
  if(fifo) {
    cfg = RADIOLIB_LR2021_CMD_IQ_STREAM_MODE_FIFO | source;
    len = 0;
  }

  uint8_t buff[] = {
    RADIOLIB_LR2021_CMD_IQ_STREAM_TRIG_NONE,
    0x00, 0x00, 0x00, 0x00, // output rate, unused
    cfg,
    0x00, 0x00, // reserved
    (uint8_t)(len & 0xFF), (uint8_t)(len >> 8),
    (uint8_t)fifo,
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_IQ_STREAM, true, buff, sizeof(buff)));
}

int16_t LR2021::enableRxFifoInterrupt(uint16_t level) {
  // enable Rx FIFO interrupt on FIFO being nearly full
  int16_t state = this->configFifoIrq(RADIOLIB_LR2021_FIFO_IRQ_HIGH, 0, level, 0, 0, 0);
  RADIOLIB_ASSERT(state);

  // route the interrupt source to the DIO pin
  state = this->setDioIrqConfig(this->irqDioNum, RADIOLIB_LR2021_IRQ_RX_FIFO);
  return(state);
}

// undocumented commands below, credit to https://github.com/beala/lr2021-re

int16_t LR2021::writeAux(uint32_t aux, uint32_t value) {
  uint8_t buff[] = {
    (uint8_t)((aux >> 24) & 0xFF), (uint8_t)((aux >> 16) & 0xFF), (uint8_t)((aux >> 8) & 0xFF), (uint8_t)(aux & 0xFF),
    (uint8_t)((value >> 24) & 0xFF), (uint8_t)((value >> 16) & 0xFF), (uint8_t)((value >> 8) & 0xFF), (uint8_t)(value & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_WRITE_AUX, true, buff, sizeof(buff)));
}

int16_t LR2021::readAux(uint32_t aux, uint32_t* value) {
  uint8_t reqBuff[] = {
    (uint8_t)((aux >> 24) & 0xFF), (uint8_t)((aux >> 16) & 0xFF), (uint8_t)((aux >> 8) & 0xFF), (uint8_t)(aux & 0xFF),
  };
  uint8_t rplBuff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_READ_AUX, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  if(value && (state == RADIOLIB_ERR_NONE)) {
    *value = ((uint32_t)rplBuff[0] << 24) | ((uint32_t)rplBuff[1] << 16) | ((uint32_t)rplBuff[2] << 8) | (uint32_t)rplBuff[3];
  }
  return(state);
}

int16_t LR2021::setTestMux(const uint8_t* params, size_t len) {
  // the first byte selects one of 12 debug multiplexer cases, the ROM requires at least 12 bytes
  if(len < 12) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_TEST_MUX, true, const_cast<uint8_t*>(params), len));
}

int16_t LR2021::setActionPoint(const uint8_t* params, size_t len) {
  // programs one of the two hardware actionpoints (breakpoints), the ROM requires at least 9 bytes
  // the exact parameter layout has not been decoded, see lr2021-re docs/commands.md
  if(len < 9) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_ACTION_POINT, true, const_cast<uint8_t*>(params), len));
}

int16_t LR2021::setTestBus(uint8_t selector, uint16_t maskA, uint16_t maskB, uint16_t maskC) {
  // route the internal test bus onto the pads: mask A = which pads (bit i = pad i = DIOi),
  // mask B = output enable, mask C = input enable. The ROM's mask defaults include pads 0-4,
  // which are SPI and BUSY - always pass explicit masks! selector 0xFF restores the pads.
  uint8_t buff[] = {
    selector,
    (uint8_t)((maskA >> 8) & 0xFF), (uint8_t)(maskA & 0xFF),
    (uint8_t)((maskB >> 8) & 0xFF), (uint8_t)(maskB & 0xFF),
    (uint8_t)((maskC >> 8) & 0xFF), (uint8_t)(maskC & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_TEST_BUS, true, buff, sizeof(buff)));
}

int16_t LR2021::calibrateFreqListA(const uint16_t freq[3]) {
  // uses the same frequency list parser as calibrateFrontEnd, then a per-frequency calibration
  // whose stored parameters have not been decoded
  uint8_t buff[] = {
    (uint8_t)((freq[0] >> 8) & 0xFF), (uint8_t)(freq[0] & 0xFF),
    (uint8_t)((freq[1] >> 8) & 0xFF), (uint8_t)(freq[1] & 0xFF),
    (uint8_t)((freq[2] >> 8) & 0xFF), (uint8_t)(freq[2] & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CALIBRATE_FREQ_LIST_A, true, buff, sizeof(buff)));
}

int16_t LR2021::calibrateFreqListB(const uint16_t freq[3]) {
  uint8_t buff[] = {
    (uint8_t)((freq[0] >> 8) & 0xFF), (uint8_t)(freq[0] & 0xFF),
    (uint8_t)((freq[1] >> 8) & 0xFF), (uint8_t)(freq[1] & 0xFF),
    (uint8_t)((freq[2] >> 8) & 0xFF), (uint8_t)(freq[2] & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CALIBRATE_FREQ_LIST_B, true, buff, sizeof(buff)));
}

int16_t LR2021::calibrateFrontEndMasked(uint8_t blockMask, const uint16_t freq[3]) {
  // calibrateFrontEnd with a leading block mask, the ROM default is 0x3F (all blocks)
  uint8_t buff[] = {
    blockMask,
    (uint8_t)((freq[0] >> 8) & 0xFF), (uint8_t)(freq[0] & 0xFF),
    (uint8_t)((freq[1] >> 8) & 0xFF), (uint8_t)(freq[1] & 0xFF),
    (uint8_t)((freq[2] >> 8) & 0xFF), (uint8_t)(freq[2] & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CALIB_FRONT_END_MASKED, true, buff, sizeof(buff)));
}

int16_t LR2021::getCapturePointer(uint16_t* ptr) {
  uint8_t rplBuff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_CAPTURE_POINTER, false, rplBuff, sizeof(rplBuff));
  if(ptr && (state == RADIOLIB_ERR_NONE)) {
    *ptr = ((uint16_t)rplBuff[0] << 8) | (uint16_t)rplBuff[1];
  }
  return(state);
}

int16_t LR2021::readCapture(uint16_t offset, uint8_t* data, size_t len) {
  // reads back bytes written by the capture DMA, while the capture is not armed
  // the length parameter is a single byte and the transfer is also SPI-length limited
  if(len > RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  uint8_t reqBuff[] = { (uint8_t)((offset >> 8) & 0xFF), (uint8_t)(offset & 0xFF), (uint8_t)len };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_READ_CAPTURE, false, data, len, reqBuff, sizeof(reqBuff)));
}

int16_t LR2021::setRawTx(uint16_t count, uint32_t rate, uint8_t mode) {
  // raw transmit: count samples of the chosen format still have to be fed through the 1 KB Tx FIFO
  uint8_t buff[] = {
    (uint8_t)((count >> 8) & 0xFF), (uint8_t)(count & 0xFF),
    (uint8_t)((rate >> 24) & 0xFF), (uint8_t)((rate >> 16) & 0xFF), (uint8_t)((rate >> 8) & 0xFF), (uint8_t)(rate & 0xFF),
    mode,
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RAW_TX, true, buff, sizeof(buff)));
}

int16_t LR2021::setRxFrontEndTrim(const uint8_t* params, size_t len) {
  // writes 3-bit fields into 0xF30044/0xF30048 and two 9-bit values into 0xF30130,
  // the exact parameter layout has not been decoded
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RX_FRONT_END_TRIM, true, const_cast<uint8_t*>(params), len));
}

#endif
