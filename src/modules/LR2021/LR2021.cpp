#include "LR2021.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

LR2021::LR2021(Module* mod) : LRxxxx(mod) {
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
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_16;

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
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setBandwidth(bw);
  RADIOLIB_ASSERT(state);

  state = setSpreadingFactor(sf);
  RADIOLIB_ASSERT(state);

  state = setCodingRate(cr);
  RADIOLIB_ASSERT(state);

  state = setSyncWord(syncWord);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
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

int16_t LR2021::transmit(const uint8_t* data, size_t len, uint8_t addr) {
   // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check packet length
  if (this->codingRate > RADIOLIB_LR2021_LORA_CR_4_8) {
    // Long Interleaver needs at least 8 bytes
    if(len < 8) {
      return(RADIOLIB_ERR_PACKET_TOO_SHORT);
    }

    // Long Interleaver supports up to 253 bytes if CRC is enabled
    if (this->crcTypeLoRa == RADIOLIB_LR2021_LORA_CRC_ENABLED && (len > RADIOLIB_LR2021_MAX_PACKET_LENGTH - 2)) {
      return(RADIOLIB_ERR_PACKET_TOO_LONG);
    }  
  } 
  if(len > RADIOLIB_LR2021_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // get currently active modem
  uint8_t modem = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  RadioLibTime_t timeout = getTimeOnAir(len);
  if(modem == RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    // calculate timeout (150% of expected time-on-air)
    timeout = (timeout * 3) / 2;

  } else if((modem == RADIOLIB_LR2021_PACKET_TYPE_GFSK) || (modem == RADIOLIB_LR2021_PACKET_TYPE_LR_FHSS)) {
    // calculate timeout (500% of expected time-on-air)
    timeout = timeout * 5;

  } else {
    return(RADIOLIB_ERR_UNKNOWN);
  }

  RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout in %lu us", timeout);

  // start transmission
  state = startTransmit(data, len, addr);
  RADIOLIB_ASSERT(state);

  // wait for packet transmission or timeout
  RadioLibTime_t start = this->mod->hal->micros();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    if(this->mod->hal->micros() - start > timeout) {
      finishTransmit();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }
  RadioLibTime_t elapsed = this->mod->hal->micros() - start;

  // update data rate
  this->dataRateMeasured = (len*8.0f)/((float)elapsed/1000000.0f);

  return(finishTransmit());
}

int16_t LR2021::receive(uint8_t* data, size_t len, RadioLibTime_t timeout) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // calculate timeout based on the configured modem
  RadioLibTime_t timeoutInternal = timeout;
  if(!timeoutInternal) {
    // get currently active modem
    uint8_t modem = RADIOLIB_LR2021_PACKET_TYPE_NONE;
    state = getPacketType(&modem);
    RADIOLIB_ASSERT(state);
    if((modem == RADIOLIB_LR2021_PACKET_TYPE_LORA) || (modem == RADIOLIB_LR2021_PACKET_TYPE_GFSK)) {
      // calculate timeout (500 % of expected time-one-air)
      size_t maxLen = len;
      if(len == 0) { maxLen = RADIOLIB_LR2021_MAX_PACKET_LENGTH; }
      timeoutInternal = (getTimeOnAir(maxLen) * 5) / 1000;
    
    } else if(modem == RADIOLIB_LR2021_PACKET_TYPE_LR_FHSS) {
      // this modem cannot receive
      return(RADIOLIB_ERR_WRONG_MODEM);

    } else {
      return(RADIOLIB_ERR_UNKNOWN);
    
    }
  }

  RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout in %lu ms", timeoutInternal);

  // start reception
  uint32_t timeoutValue = (uint32_t)(((float)timeoutInternal * 1000.0f) / 30.52f);
  state = startReceive(timeoutValue);
  RADIOLIB_ASSERT(state);

  // wait for packet reception or timeout
  bool softTimeout = false;
  RadioLibTime_t start = this->mod->hal->millis();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    // safety check, the timeout should be done by the radio
    if(this->mod->hal->millis() - start > timeoutInternal) {
      softTimeout = true;
      break;
    }
  }

  // if it was a timeout, this will return an error code
  //! \TODO: [LR2021] taken from SX126x, does this really work?
  state = standby();
  if((state != RADIOLIB_ERR_NONE) && (state != RADIOLIB_ERR_SPI_CMD_TIMEOUT)) {
    return(state);
  }

  // check whether this was a timeout or not
  if(softTimeout || (getIrqFlags() & this->irqMap[RADIOLIB_IRQ_TIMEOUT])) {
    (void)finishReceive();
    return(RADIOLIB_ERR_RX_TIMEOUT);
  }

  // read the received data
  return(readData(data, len));
}

int16_t LR2021::transmitDirect(uint32_t frf) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_TX);

  // user requested to start transmitting immediately (required for RTTY)
  int16_t state = RADIOLIB_ERR_NONE;
  if(frf != 0) {
    state = setRfFrequency(frf);
  }
  RADIOLIB_ASSERT(state);

  // start transmitting
  return(setTxTestMode(RADIOLIB_LR2021_TX_TEST_MODE_CW));
}

int16_t LR2021::receiveDirect() {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // LR2021 is unable to output received data directly
  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t LR2021::scanChannel() {
  ChannelScanConfig_t cfg = {
    .cad = {
      .symNum = RADIOLIB_LR2021_CAD_PARAM_DEFAULT,
      .detPeak = RADIOLIB_LR2021_CAD_PARAM_DEFAULT,
      .detMin = RADIOLIB_LR2021_CAD_PARAM_DEFAULT,
      .exitMode = RADIOLIB_LR2021_CAD_PARAM_DEFAULT,
      .timeout = 0,
      .irqFlags = RADIOLIB_IRQ_CAD_DEFAULT_FLAGS,
      .irqMask = RADIOLIB_IRQ_CAD_DEFAULT_MASK,
    },
  };
  return(this->scanChannel(cfg));
}

int16_t LR2021::scanChannel(const ChannelScanConfig_t &config) {
  // set mode to CAD
  int state = startChannelScan(config);
  RADIOLIB_ASSERT(state);

  // wait for channel activity detected or timeout
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
  }

  // check CAD result
  return(getChannelScanResult());
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
  return(this->sleep(true, 0));
}

int16_t LR2021::sleep(bool retainConfig, uint32_t sleepTime) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  uint8_t buff[] = { (uint8_t)(retainConfig ? RADIOLIB_LR2021_SLEEP_RETENTION_ENABLED : RADIOLIB_LR2021_SLEEP_RETENTION_DISABLED),
    (uint8_t)((sleepTime >> 24) & 0xFF), (uint8_t)((sleepTime >> 16) & 0xFF),
    (uint8_t)((sleepTime >> 8) & 0xFF), (uint8_t)(sleepTime & 0xFF),
  };

  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_SET_SLEEP, true, buff, sizeof(buff));

  // wait for the module to safely enter sleep mode
  this->mod->hal->delay(1);

  return(state);
}

size_t LR2021::getPacketLength(bool update) {
  (void)update;

  // in implicit mode, return the cached value
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  (void)getPacketType(&type);
  if((type == RADIOLIB_LR2021_PACKET_TYPE_LORA) && (this->headerType == RADIOLIB_LR2021_LORA_HEADER_IMPLICIT)) {
    return(this->implicitLen);
  }

  uint16_t len = 0;
  (void)getRxPktLength(&len);
  return((size_t)len);
}

int16_t LR2021::finishTransmit() {
  // clear interrupt flags
  clearIrq(RADIOLIB_LR2021_IRQ_ALL);

  // set mode to standby to disable transmitter/RF switch
  return(standby());
}

int16_t LR2021::startReceive() {
  return(this->startReceive(RADIOLIB_LR2021_RX_TIMEOUT_INF, RADIOLIB_IRQ_RX_DEFAULT_FLAGS, RADIOLIB_IRQ_RX_DEFAULT_MASK, 0));
}

int16_t LR2021::readData(uint8_t* data, size_t len) {
  // check active modem
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if((modem != RADIOLIB_LR2021_PACKET_TYPE_LORA) && 
     (modem != RADIOLIB_LR2021_PACKET_TYPE_GFSK)) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check integrity CRC
  uint32_t irq = getIrqStatus();
  int16_t crcState = RADIOLIB_ERR_NONE;
  // Report CRC mismatch when there's a payload CRC error, or a header error and no valid header (to avoid false alarm from previous packet)
  //! \TODO: [LR2021] legacy from LR11x0, is it still needed?
  if((irq & RADIOLIB_LR2021_IRQ_CRC_ERROR) || (!(irq & RADIOLIB_LR2021_IRQ_LORA_HEADER_VALID))) {
    crcState = RADIOLIB_ERR_CRC_MISMATCH;
  }

  // get packet length
  size_t length = getPacketLength();
  if((len != 0) && (len < length)) {
    // user requested less data than we got, only return what was requested
    length = len;
  }

  // read packet data
  state = readRadioRxFifo(data, length);
  RADIOLIB_ASSERT(state);

  // clear the Rx buffer
  state = clearRxFifo();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrq(RADIOLIB_LR2021_IRQ_ALL);

  // check if CRC failed - this is done after reading data to give user the option to keep them
  RADIOLIB_ASSERT(crcState);

  return(state);
}

int16_t LR2021::finishReceive() {
  // set mode to standby to disable RF switch
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  return(clearIrq(RADIOLIB_LR2021_IRQ_ALL));
}

int16_t LR2021::startChannelScan() {
  ChannelScanConfig_t cfg = {
    .cad = {
      .symNum = RADIOLIB_LR2021_CAD_PARAM_DEFAULT,
      .detPeak = RADIOLIB_LR2021_CAD_PARAM_DEFAULT,
      .detMin = RADIOLIB_LR2021_CAD_PARAM_DEFAULT,
      .exitMode = RADIOLIB_LR2021_CAD_PARAM_DEFAULT,
      .timeout = 0,
      .irqFlags = RADIOLIB_IRQ_CAD_DEFAULT_FLAGS,
      .irqMask = RADIOLIB_IRQ_CAD_DEFAULT_MASK,
    },
  };
  return(this->startChannelScan(cfg));
}

int16_t LR2021::startChannelScan(const ChannelScanConfig_t &config) {
  // check active modem
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if(modem != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  state = standby();
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // set DIO pin mapping
  uint32_t irqFlags = (config.cad.irqFlags == RADIOLIB_IRQ_NOT_SUPPORTED) ? RADIOLIB_LR2021_IRQ_CAD_DETECTED | RADIOLIB_LR2021_IRQ_CAD_DONE : config.cad.irqFlags;
  state = setDioIrqConfig(this->irqDioNum, getIrqMapped(irqFlags));
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrq(RADIOLIB_LR2021_IRQ_ALL);
  RADIOLIB_ASSERT(state);

  // set mode to CAD
  return(startCad(config.cad.symNum, config.cad.detPeak, config.cad.detMin, config.cad.exitMode, config.cad.timeout));
}

int16_t LR2021::getChannelScanResult() {
  // check active modem
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if(modem != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check CAD result
  uint32_t cadResult = getIrqStatus();
  if(cadResult & RADIOLIB_LR2021_IRQ_CAD_DETECTED) {
    // detected some LoRa activity
    return(RADIOLIB_LORA_DETECTED);
  } else if(cadResult & RADIOLIB_LR2021_IRQ_CAD_DONE) {
    // channel is free
    return(RADIOLIB_CHANNEL_FREE);
  }

  return(RADIOLIB_ERR_UNKNOWN);
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

    if((fwMajor == expMajor) && (fwMinor == expMinor)) {
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

  // validate DIO pin number
  if((this->irqDioNum < 5) || (this->irqDioNum > 11)) {
    return(RADIOLIB_ERR_INVALID_DIO_PIN);
  }

  // set the DIO to IRQ
  // DIO5 can only be pull up
  uint8_t pull = this->irqDioNum == 5 ? RADIOLIB_LR2021_DIO_SLEEP_PULL_UP : RADIOLIB_LR2021_DIO_SLEEP_PULL_NONE;
  state = this->setDioFunction(this->irqDioNum, RADIOLIB_LR2021_DIO_FUNCTION_IRQ, pull);
  RADIOLIB_ASSERT(state);

  // calibrate all blocks
  state = this->calibrate(RADIOLIB_LR2021_CALIBRATE_ALL);

  // wait for calibration completion
  this->mod->hal->delay(5);
  while(this->mod->hal->digitalRead(this->mod->getGpio())) {
    this->mod->hal->yield();
  }
  
  // if something failed, show the device errors
  #if RADIOLIB_DEBUG_BASIC
  if(state != RADIOLIB_ERR_NONE) {
    uint16_t errors = 0;
    getErrors(&errors);
    RADIOLIB_DEBUG_BASIC_PRINTLN("Calibration failed, device errors: 0x%X", errors);
  }
  #else
  RADIOLIB_ASSERT(state);
  #endif

  // set modem
  state = this->setPacketType(modem);
  return(state);
}

int16_t LR2021::startCad(uint8_t symbolNum, uint8_t detPeak, uint8_t detMin, uint8_t exitMode, RadioLibTime_t timeout) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // select CAD parameters
  //! \TODO: [LR2021] the magic numbers for CAD are based on Semtech examples, this is probably suboptimal
  uint8_t num = symbolNum;
  if(num == RADIOLIB_LR2021_CAD_PARAM_DEFAULT) {
    num = 2;
  }
  
  const uint8_t detPeakValues[8] = { 48, 48, 50, 55, 55, 59, 61, 65 };
  uint8_t peak = detPeak;
  if(peak == RADIOLIB_LR2021_CAD_PARAM_DEFAULT) {
    peak = detPeakValues[this->spreadingFactor - 5];
  }

  uint8_t min = detMin;
  if(min == RADIOLIB_LR2021_CAD_PARAM_DEFAULT) {
    min = 10;
  }

  uint8_t mode = exitMode; 
  if(mode == RADIOLIB_LR2021_CAD_PARAM_DEFAULT) {
    mode = RADIOLIB_LR2021_CAD_EXIT_MODE_FALLBACK;
  }

  uint32_t timeout_raw = (float)timeout / 30.52f;

  //! \TODO: [LR2021] The datasheet says this CAD is only based on RSSI, but the reference to the LoRa CAD is GetLoraRxStats ...?
  (void)peak;
  (void)min;

  // set CAD parameters
  //! \TODO: [LR2021] add configurable exit mode and timeout
  state = setCadParams(timeout_raw, num, mode, timeout_raw);
  RADIOLIB_ASSERT(state);

  // start CAD
  return(setCad());
}

int16_t LR2021::getModem(ModemType_t* modem) {
  RADIOLIB_ASSERT_PTR(modem);

  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);

  switch(type) {
    case(RADIOLIB_LR2021_PACKET_TYPE_LORA):
      *modem = ModemType_t::RADIOLIB_MODEM_LORA;
      return(RADIOLIB_ERR_NONE);
    case(RADIOLIB_LR2021_PACKET_TYPE_GFSK):
      *modem = ModemType_t::RADIOLIB_MODEM_FSK;
      return(RADIOLIB_ERR_NONE);
    case(RADIOLIB_LR2021_PACKET_TYPE_LR_FHSS):
      *modem = ModemType_t::RADIOLIB_MODEM_LRFHSS;
      return(RADIOLIB_ERR_NONE);
  }
  
  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR2021::stageMode(RadioModeType_t mode, RadioModeConfig_t* cfg) {
  int16_t state;

  switch(mode) {
    case(RADIOLIB_RADIO_MODE_RX): {
      // check active modem
      uint8_t modem = RADIOLIB_LR2021_PACKET_TYPE_NONE;
      state = getPacketType(&modem);
      RADIOLIB_ASSERT(state);
      if((modem != RADIOLIB_LR2021_PACKET_TYPE_LORA) && 
        (modem != RADIOLIB_LR2021_PACKET_TYPE_GFSK)) {
        return(RADIOLIB_ERR_WRONG_MODEM);
      }

      // set DIO mapping
      if(cfg->receive.timeout != RADIOLIB_LR2021_RX_TIMEOUT_INF) {
        cfg->receive.irqMask |= (1UL << RADIOLIB_IRQ_TIMEOUT);
      }
      state = setDioIrqConfig(this->irqDioNum, getIrqMapped(cfg->receive.irqFlags & cfg->receive.irqMask));
      RADIOLIB_ASSERT(state);

      // clear interrupt flags
      state = clearIrq(RADIOLIB_LR2021_IRQ_ALL);
      RADIOLIB_ASSERT(state);

      // set implicit mode and expected len if applicable
      if((this->headerType == RADIOLIB_LR2021_LORA_HEADER_IMPLICIT) && (modem == RADIOLIB_LR2021_PACKET_TYPE_LORA)) {
        state = setLoRaPacketParams(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, this->invertIQEnabled);
        RADIOLIB_ASSERT(state);
      }

      // if max(uint32_t) is used, revert to RxContinuous
      if(cfg->receive.timeout == 0xFFFFFFFF) {
        cfg->receive.timeout = 0xFFFFFF;
      }
      this->rxTimeout = cfg->receive.timeout;
    } break;
  
    case(RADIOLIB_RADIO_MODE_TX): {
      // check packet length
      if(cfg->transmit.len > RADIOLIB_LR2021_MAX_PACKET_LENGTH) {
        return(RADIOLIB_ERR_PACKET_TOO_LONG);
      }

      // maximum packet length is decreased by 1 when address filtering is active
      //! \todo [LR2021] implement GFSK address filtering

      // set packet Length
      state = RADIOLIB_ERR_NONE;
      uint8_t modem = RADIOLIB_LR2021_PACKET_TYPE_NONE;
      state = getPacketType(&modem);
      RADIOLIB_ASSERT(state);
      if(modem == RADIOLIB_LR2021_PACKET_TYPE_LORA) {
        state = setLoRaPacketParams(this->preambleLengthLoRa, this->headerType, cfg->transmit.len, this->crcTypeLoRa, this->invertIQEnabled);
      
      } else if(modem == RADIOLIB_LR2021_PACKET_TYPE_GFSK) {
        //! \todo [LR2021] implement GFSK
        //state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, cfg->transmit.len, this->crcTypeGFSK, this->whitening);
      
      } else if(modem != RADIOLIB_LR2021_PACKET_TYPE_LR_FHSS) {
        return(RADIOLIB_ERR_UNKNOWN);
      
      }
      RADIOLIB_ASSERT(state);

      // set DIO mapping
      state = setDioIrqConfig(this->irqDioNum, RADIOLIB_LR2021_IRQ_TX_DONE | RADIOLIB_LR2021_IRQ_TIMEOUT);
      RADIOLIB_ASSERT(state);

      if(modem == RADIOLIB_LR2021_PACKET_TYPE_LR_FHSS) {
        // in LR-FHSS mode, the packet is built by the device
        //! \todo [LR2021] add configurable LR-FHSS device offset
        state = LRxxxx::lrFhssBuildFrame(RADIOLIB_LR2021_CMD_LR_FHSS_BUILD_FRAME, this->lrFhssHdrCount, this->lrFhssCr, this->lrFhssGrid, true, this->lrFhssBw, this->lrFhssHopSeq, 0, cfg->transmit.data, cfg->transmit.len);
        RADIOLIB_ASSERT(state);

      } else {
        // write packet to buffer
        state = writeRadioTxFifo(cfg->transmit.data, cfg->transmit.len);
        RADIOLIB_ASSERT(state);

      }

      // clear interrupt flags
      state = clearIrq(RADIOLIB_LR2021_IRQ_ALL);
      RADIOLIB_ASSERT(state);
    } break;
    
    default:
      return(RADIOLIB_ERR_UNSUPPORTED);
  }

  this->stagedMode = mode;
  return(state);
}

int16_t LR2021::launchMode() {
  int16_t state;
  switch(this->stagedMode) {
    case(RADIOLIB_RADIO_MODE_RX): {
      this->mod->setRfSwitchState(Module::MODE_RX);
      state = setRx(this->rxTimeout);
    } break;
  
    case(RADIOLIB_RADIO_MODE_TX): {
      this->mod->setRfSwitchState(Module::MODE_TX);
      state = setTx(RADIOLIB_LR2021_TX_TIMEOUT_NONE);
      RADIOLIB_ASSERT(state);

      // wait for BUSY to go low (= PA ramp up done)
      while(this->mod->hal->digitalRead(this->mod->getGpio())) {
        this->mod->hal->yield();
      }
    } break;
    
    default:
      return(RADIOLIB_ERR_UNSUPPORTED);
  }

  this->stagedMode = RADIOLIB_RADIO_MODE_NONE;
  return(state);
}

#endif