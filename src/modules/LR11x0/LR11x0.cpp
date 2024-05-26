#include "LR11x0.h"

#include "../../utils/CRC.h"
#include "../../utils/Cryptography.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR11X0

LR11x0::LR11x0(Module* mod) : PhysicalLayer(RADIOLIB_LR11X0_FREQUENCY_STEP_SIZE, RADIOLIB_LR11X0_MAX_PACKET_LENGTH) {
  this->mod = mod;
  this->XTAL = false;
}

int16_t LR11x0::begin(float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  // set module properties and perform initial setup
  int16_t state = this->modSetup(tcxoVoltage, RADIOLIB_LR11X0_PACKET_TYPE_LORA);
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
  
  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  state = setCRC(2);
  RADIOLIB_ASSERT(state);

  state = invertIQ(false);
  RADIOLIB_ASSERT(state);

  state = setRegulatorLDO();
  RADIOLIB_ASSERT(state);

  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::beginGFSK(float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  // set module properties and perform initial setup
  int16_t state = this->modSetup(tcxoVoltage, RADIOLIB_LR11X0_PACKET_TYPE_GFSK);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  state = setRxBandwidth(rxBw);
  RADIOLIB_ASSERT(state);
  
  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  uint8_t sync[] = { 0x12, 0xAD };
  state = setSyncWord(sync, 2);
  RADIOLIB_ASSERT(state);

  state = setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);

  state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  state = variablePacketLengthMode(RADIOLIB_LR11X0_MAX_PACKET_LENGTH);
  RADIOLIB_ASSERT(state);

  state = setCRC(2);
  RADIOLIB_ASSERT(state);

  state = setRegulatorLDO();
  RADIOLIB_ASSERT(state);

  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::beginLRFHSS(uint8_t bw, uint8_t cr, int8_t power, float tcxoVoltage) {
  // set module properties and perform initial setup
  int16_t state = this->modSetup(tcxoVoltage, RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setLrFhssConfig(bw, cr);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = setSyncWord(0x12AD101B);
  RADIOLIB_ASSERT(state);

  state = setRegulatorLDO();
  RADIOLIB_ASSERT(state);

  // set fixed configuration
  return(setModulationParamsLrFhss(RADIOLIB_LR11X0_LR_FHSS_BIT_RATE_RAW, RADIOLIB_LR11X0_LR_FHSS_SHAPING_GAUSSIAN_BT_1_0));
}

int16_t LR11x0::reset() {
  // run the reset sequence
  this->mod->hal->pinMode(this->mod->getRst(), this->mod->hal->GpioModeOutput);
  this->mod->hal->digitalWrite(this->mod->getRst(), this->mod->hal->GpioLevelLow);
  this->mod->hal->delay(10);
  this->mod->hal->digitalWrite(this->mod->getRst(), this->mod->hal->GpioLevelHigh);

  // the typical transition duration should be 273 ms
  this->mod->hal->delay(300);
  
  // wait for BUSY to go low
  RadioLibTime_t start = this->mod->hal->millis();
  while(this->mod->hal->digitalRead(this->mod->getGpio())) {
    this->mod->hal->yield();
    if(this->mod->hal->millis() - start >= 3000) {
      RADIOLIB_DEBUG_BASIC_PRINTLN("BUSY pin timeout after reset!");
      return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
    }
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::transmit(uint8_t* data, size_t len, uint8_t addr) {
   // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check packet length
  if(len > RADIOLIB_LR11X0_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // get currently active modem
  uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  RadioLibTime_t timeout = getTimeOnAir(len);
  if(modem == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    // calculate timeout (150% of expected time-on-air)
    timeout = (timeout * 3) / 2;

  } else if((modem == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) || (modem == RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS)) {
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
  this->dataRateMeasured = (len*8.0)/((float)elapsed/1000000.0);

  return(finishTransmit());
}

int16_t LR11x0::receive(uint8_t* data, size_t len) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  RadioLibTime_t timeout = 0;

  // get currently active modem
  uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if(modem == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    // calculate timeout (100 LoRa symbols, the default for SX127x series)
    float symbolLength = (float)(uint32_t(1) << this->spreadingFactor) / (float)this->bandwidthKhz;
    timeout = (RadioLibTime_t)(symbolLength * 100.0);
  
  } else if(modem == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    // calculate timeout (500 % of expected time-one-air)
    size_t maxLen = len;
    if(len == 0) { 
      maxLen = 0xFF;
    }
    float brBps = ((float)(RADIOLIB_LR11X0_CRYSTAL_FREQ) * 1000000.0 * 32.0) / (float)this->bitRate;
    timeout = (RadioLibTime_t)(((maxLen * 8.0) / brBps) * 1000.0 * 5.0);
  
  } else if(modem == RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS) {
    size_t maxLen = len;
    if(len == 0) { 
      maxLen = 0xFF;
    }
    timeout = (RadioLibTime_t)(((maxLen * 8.0) / (RADIOLIB_LR11X0_LR_FHSS_BIT_RATE)) * 1000.0 * 5.0);

  } else {
    return(RADIOLIB_ERR_UNKNOWN);
  
  }

  RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout in %lu ms", timeout);

  // start reception
  uint32_t timeoutValue = (uint32_t)(((float)timeout * 1000.0) / 30.52);
  state = startReceive(timeoutValue);
  RADIOLIB_ASSERT(state);

  // wait for packet reception or timeout
  bool softTimeout = false;
  RadioLibTime_t start = this->mod->hal->millis();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    // safety check, the timeout should be done by the radio
    if(this->mod->hal->millis() - start > timeout) {
      softTimeout = true;
      break;
    }
  }

  // if it was a timeout, this will return an error code
  // TODO taken from SX126x, does this really work?
  state = standby();
  if((state != RADIOLIB_ERR_NONE) && (state != RADIOLIB_ERR_SPI_CMD_TIMEOUT)) {
    return(state);
  }

  // check whether this was a timeout or not
  if((getIrqStatus() & RADIOLIB_LR11X0_IRQ_TIMEOUT) || softTimeout) {
    standby();
    clearIrq(RADIOLIB_LR11X0_IRQ_ALL);
    return(RADIOLIB_ERR_RX_TIMEOUT);
  }

  // read the received data
  return(readData(data, len));
}

int16_t LR11x0::transmitDirect(uint32_t frf) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_TX);

  // user requested to start transmitting immediately (required for RTTY)
  int16_t state = RADIOLIB_ERR_NONE;
  if(frf != 0) {
    state = setRfFrequency(frf);
  }
  RADIOLIB_ASSERT(state);

  // start transmitting
  return(setTxCw());
}

int16_t LR11x0::receiveDirect() {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // LR11x0 is unable to output received data directly
  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t LR11x0::scanChannel() {
  return(this->scanChannel(RADIOLIB_LR11X0_CAD_PARAM_DEFAULT, RADIOLIB_LR11X0_CAD_PARAM_DEFAULT, RADIOLIB_LR11X0_CAD_PARAM_DEFAULT));
}

int16_t LR11x0::scanChannel(uint8_t symbolNum, uint8_t detPeak, uint8_t detMin) {
  // set mode to CAD
  int state = startChannelScan(symbolNum, detPeak, detMin);
  RADIOLIB_ASSERT(state);

  // wait for channel activity detected or timeout
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
  }

  // check CAD result
  return(getChannelScanResult());
}

int16_t LR11x0::standby() {
  return(LR11x0::standby(RADIOLIB_LR11X0_STANDBY_RC));
}

int16_t LR11x0::standby(uint8_t mode, bool wakeup) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  if(wakeup) {
    // pull NSS low for a while to wake up
    this->mod->hal->digitalWrite(this->mod->getCs(), this->mod->hal->GpioLevelLow);
    this->mod->hal->delay(1);
    this->mod->hal->digitalWrite(this->mod->getCs(), this->mod->hal->GpioLevelHigh);
  }

  uint8_t buff[] = { mode };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_STANDBY, true, buff, 1));
}

int16_t LR11x0::sleep() {
  return(LR11x0::sleep(true, 0));
}

int16_t LR11x0::sleep(bool retainConfig, uint32_t sleepTime) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  uint8_t buff[] = { 
    (uint8_t)retainConfig,
    (uint8_t)((sleepTime >> 24) & 0xFF), (uint8_t)((sleepTime >> 16) & 0xFF),
    (uint8_t)((sleepTime >> 16) & 0xFF), (uint8_t)(sleepTime & 0xFF),
  };
  if(sleepTime) {
    buff[0] |= RADIOLIB_LR11X0_SLEEP_WAKEUP_ENABLED;
  }

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_SLEEP, true, buff, sizeof(buff));

  // wait for the module to safely enter sleep mode
  this->mod->hal->delay(1);

  return(state);
}

void LR11x0::setIrqAction(void (*func)(void)) {
  this->mod->hal->attachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getIrq()), func, this->mod->hal->GpioInterruptRising);
}

void LR11x0::clearIrqAction() {
  this->mod->hal->detachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getIrq()));
}

void LR11x0::setPacketReceivedAction(void (*func)(void)) {
  this->setIrqAction(func);
}

void LR11x0::clearPacketReceivedAction() {
  this->clearIrqAction();
}

void LR11x0::setPacketSentAction(void (*func)(void)) {
  this->setIrqAction(func);
}

void LR11x0::clearPacketSentAction() {
  this->clearIrqAction();
}

int16_t LR11x0::startTransmit(uint8_t* data, size_t len, uint8_t addr) {
  // suppress unused variable warning
  (void)addr;

  // check packet length
  if(len > RADIOLIB_LR11X0_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // maximum packet length is decreased by 1 when address filtering is active
  if((this->addrComp != RADIOLIB_LR11X0_GFSK_ADDR_FILTER_DISABLED) && (len > RADIOLIB_LR11X0_MAX_PACKET_LENGTH - 1)) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // set packet Length
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if(modem == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    state = setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, len, this->crcTypeLoRa, this->invertIQEnabled);
  
  } else if(modem == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, len, this->crcTypeGFSK, this->whitening);
  
  } else if(modem != RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS) {
    return(RADIOLIB_ERR_UNKNOWN);
  
  }
  RADIOLIB_ASSERT(state);

  // set DIO mapping
  state = setDioIrqParams(RADIOLIB_LR11X0_IRQ_TX_DONE | RADIOLIB_LR11X0_IRQ_TIMEOUT, 0);
  RADIOLIB_ASSERT(state);

  if(modem == RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS) {
    // in LR-FHSS mode, the packet is built by the device
    // TODO add configurable grid step and device offset
    state = lrFhssBuildFrame(this->lrFhssHdrCount, this->lrFhssCr, RADIOLIB_LR11X0_LR_FHSS_GRID_STEP_FCC, true, this->lrFhssBw, this->lrFhssHopSeq, 0, data, len);
    RADIOLIB_ASSERT(state);

  } else {
    // write packet to buffer
    state = writeBuffer8(data, len);
    RADIOLIB_ASSERT(state);

  }

  // clear interrupt flags
  state = clearIrq(RADIOLIB_LR11X0_IRQ_ALL);
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_TX);

  // start transmission
  state = setTx(RADIOLIB_LR11X0_TX_TIMEOUT_NONE);
  RADIOLIB_ASSERT(state);

  // wait for BUSY to go low (= PA ramp up done)
  while(this->mod->hal->digitalRead(this->mod->getGpio())) {
    this->mod->hal->yield();
  }

  return(state);
}

int16_t LR11x0::finishTransmit() {
  // clear interrupt flags
  clearIrq(RADIOLIB_LR11X0_IRQ_ALL);

  // set mode to standby to disable transmitter/RF switch
  return(standby());
}

int16_t LR11x0::startReceive() {
  return(this->startReceive(RADIOLIB_LR11X0_RX_TIMEOUT_INF, RADIOLIB_LR11X0_IRQ_RX_DONE, 0, 0));
}

int16_t LR11x0::startReceive(uint32_t timeout, uint32_t irqFlags, uint32_t irqMask, size_t len) {
  (void)irqMask;
  (void)len;
  
  // check active modem
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if((modem != RADIOLIB_LR11X0_PACKET_TYPE_LORA) && 
     (modem != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) &&
     (modem != RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS)) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set DIO mapping
  uint32_t irq = irqFlags;
  if(timeout != RADIOLIB_LR11X0_RX_TIMEOUT_INF) {
    irq |= RADIOLIB_LR11X0_IRQ_TIMEOUT;
  }

  state = setDioIrqParams(irq, RADIOLIB_LR11X0_IRQ_NONE);
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrq(RADIOLIB_LR11X0_IRQ_ALL);
  RADIOLIB_ASSERT(state);

  // set implicit mode and expected len if applicable
  if((this->headerType == RADIOLIB_LR11X0_LORA_HEADER_IMPLICIT) && (modem == RADIOLIB_LR11X0_PACKET_TYPE_LORA)) {
    state = setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, this->invertIQEnabled);
    RADIOLIB_ASSERT(state);
  }

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // set mode to receive
  state = setRx(timeout);

  return(state);
}

uint32_t LR11x0::getIrqStatus() {
  // there is no dedicated "get IRQ" command, the IRQ bits are sent after the status bytes
  uint8_t buff[6] = { 0 };
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_0;
  mod->SPItransferStream(NULL, 0, false, NULL, buff, sizeof(buff), true);
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_8;
  uint32_t irq = ((uint32_t)(buff[2]) << 24) | ((uint32_t)(buff[3]) << 16) | ((uint32_t)(buff[4]) << 8) | (uint32_t)buff[5];
  return(irq);
}

int16_t LR11x0::readData(uint8_t* data, size_t len) {
  // check active modem
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if((modem != RADIOLIB_LR11X0_PACKET_TYPE_LORA) && 
     (modem != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) &&
     (modem != RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS)) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check integrity CRC
  uint32_t irq = getIrqStatus();
  int16_t crcState = RADIOLIB_ERR_NONE;
  if((irq & RADIOLIB_LR11X0_IRQ_CRC_ERR) || (irq & RADIOLIB_LR11X0_IRQ_HEADER_ERR)) {
    crcState = RADIOLIB_ERR_CRC_MISMATCH;
  }

  // get packet length
  // the offset is needed since LR11x0 seems to move the buffer base by 4 bytes on every packet
  uint8_t offset = 0;
  size_t length = getPacketLength(true, &offset);
  if((len != 0) && (len < length)) {
    // user requested less data than we got, only return what was requested
    length = len;
  }

  // read packet data
  state = readBuffer8(data, length, offset);
  RADIOLIB_ASSERT(state);

  // clear the Rx buffer
  state = clearRxBuffer();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrq(RADIOLIB_LR11X0_IRQ_ALL);

  // check if CRC failed - this is done after reading data to give user the option to keep them
  RADIOLIB_ASSERT(crcState);

  return(state);
}

int16_t LR11x0::startChannelScan() {
  return(this->startChannelScan(RADIOLIB_LR11X0_CAD_PARAM_DEFAULT, RADIOLIB_LR11X0_CAD_PARAM_DEFAULT, RADIOLIB_LR11X0_CAD_PARAM_DEFAULT));
}

int16_t LR11x0::startChannelScan(uint8_t symbolNum, uint8_t detPeak, uint8_t detMin) {
  // check active modem
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if(modem != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  state = standby();
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // set DIO pin mapping
  state = setDioIrqParams(RADIOLIB_LR11X0_IRQ_CAD_DETECTED | RADIOLIB_LR11X0_IRQ_CAD_DONE, RADIOLIB_LR11X0_IRQ_NONE);
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrq(RADIOLIB_LR11X0_IRQ_ALL);
  RADIOLIB_ASSERT(state);

  // set mode to CAD
  return(startCad(symbolNum, detPeak, detMin));
}

int16_t LR11x0::getChannelScanResult() {
  // check active modem
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if(modem != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check CAD result
  uint32_t cadResult = getIrqStatus();
  if(cadResult & RADIOLIB_LR11X0_IRQ_CAD_DETECTED) {
    // detected some LoRa activity
    return(RADIOLIB_LORA_DETECTED);
  } else if(cadResult & RADIOLIB_LR11X0_IRQ_CAD_DONE) {
    // channel is free
    return(RADIOLIB_CHANNEL_FREE);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t LR11x0::setOutputPower(int8_t power) {
  return(this->setOutputPower(power, false));
}

int16_t LR11x0::setOutputPower(int8_t power, bool forceHighPower) {
  // check if power value is configurable
  int16_t state = checkOutputPower(power, NULL, forceHighPower);
  RADIOLIB_ASSERT(state);

  // determine whether to use HP or LP PA and check range accordingly
  bool useHp = forceHighPower || (power > 14);
  
  // TODO how and when to configure OCP?

  // update PA config - always use VBAT for high-power PA
  state = setPaConfig((uint8_t)useHp, (uint8_t)useHp, 0x04, 0x07);
  RADIOLIB_ASSERT(state);

  // set output power
  state = setTxParams(power, RADIOLIB_LR11X0_PA_RAMP_48U);
  return(state);
}

int16_t LR11x0::checkOutputPower(int8_t power, int8_t* clipped) {
  return(checkOutputPower(power, clipped, false));
}

int16_t LR11x0::checkOutputPower(int8_t power, int8_t* clipped, bool forceHighPower) {
  if(forceHighPower || (power > 14)) {
    if(clipped) {
      *clipped = RADIOLIB_MAX(-9, RADIOLIB_MIN(22, power));
    }
    RADIOLIB_CHECK_RANGE(power, -9, 22, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  
  } else {
    if(clipped) {
      *clipped = RADIOLIB_MAX(-17, RADIOLIB_MIN(14, power));
    }
    RADIOLIB_CHECK_RANGE(power, -17, 14, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  
  }
  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::setBandwidth(float bw) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // ensure byte conversion doesn't overflow
  RADIOLIB_CHECK_RANGE(bw, 0.0, 510.0, RADIOLIB_ERR_INVALID_BANDWIDTH);

  // check allowed bandwidth values
  uint8_t bw_div2 = bw / 2 + 0.01;
  switch (bw_div2)  {
    case 31: // 62.5:
      this->bandwidth = RADIOLIB_LR11X0_LORA_BW_62_5;
      break;
    case 62: // 125.0:
      this->bandwidth = RADIOLIB_LR11X0_LORA_BW_125_0;
      break;
    case 125: // 250.0
      this->bandwidth = RADIOLIB_LR11X0_LORA_BW_250_0;
      break;
    case 250: // 500.0
      this->bandwidth = RADIOLIB_LR11X0_LORA_BW_500_0;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_BANDWIDTH);
  }

  // update modulation parameters
  this->bandwidthKhz = bw;
  return(setModulationParamsLoRa(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t LR11x0::setSpreadingFactor(uint8_t sf, bool legacy) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(sf, 5, 12, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);

  // TODO enable SF6 legacy mode
  if(legacy && (sf == 6)) {
    //this->mod->SPIsetRegValue(RADIOLIB_LR11X0_REG_SF6_SX127X_COMPAT, RADIOLIB_LR11X0_SF6_SX127X, 18, 18);
  }

  // update modulation parameters
  this->spreadingFactor = sf;
  return(setModulationParamsLoRa(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t LR11x0::setCodingRate(uint8_t cr, bool longInterleave) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(cr, 5, 8, RADIOLIB_ERR_INVALID_CODING_RATE);

  if(longInterleave) {
    switch(cr) {
      case 5:
      case 6:
        this->codingRate = cr;
        break;
      case 8: 
        this->codingRate = cr - 1;
        break;
      default:
        return(RADIOLIB_ERR_INVALID_CODING_RATE);
    }
  
  } else {
    this->codingRate = cr - 4;
  
  }

  // update modulation parameters
  return(setModulationParamsLoRa(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t LR11x0::setSyncWord(uint32_t syncWord) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(setLoRaSyncWord(syncWord & 0xFF));
  
  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS) {
    return(lrFhssSetSyncWord(syncWord));
 
  }
  
  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR11x0::setBitRate(float br) {
  RADIOLIB_CHECK_RANGE(br, 0.6, 300.0, RADIOLIB_ERR_INVALID_BIT_RATE);

  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set bit rate value
  // TODO implement fractional bit rate configuration
  this->bitRate = br * 1000.0;
  return(setModulationParamsGFSK(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev));
}

int16_t LR11x0::setFrequencyDeviation(float freqDev) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set frequency deviation to lowest available setting (required for digimodes)
  float newFreqDev = freqDev;
  if(freqDev < 0.0) {
    newFreqDev = 0.6;
  }

  RADIOLIB_CHECK_RANGE(newFreqDev, 0.6, 200.0, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);
  this->frequencyDev = newFreqDev * 1000.0;
  return(setModulationParamsGFSK(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev));
}

int16_t LR11x0::setRxBandwidth(float rxBw) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check modulation parameters
  /*if(2 * this->frequencyDev + this->bitRate > rxBw * 1000.0) {
    return(RADIOLIB_ERR_INVALID_MODULATION_PARAMETERS);
  }*/

  // check allowed receiver bandwidth values
  if(fabs(rxBw - 4.8) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_4_8;
  } else if(fabs(rxBw - 5.8) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_5_8;
  } else if(fabs(rxBw - 7.3) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_7_3;
  } else if(fabs(rxBw - 9.7) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_9_7;
  } else if(fabs(rxBw - 11.7) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_11_7;
  } else if(fabs(rxBw - 14.6) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_14_6;
  } else if(fabs(rxBw - 19.5) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_19_5;
  } else if(fabs(rxBw - 23.4) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_23_4;
  } else if(fabs(rxBw - 29.3) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_29_3;
  } else if(fabs(rxBw - 39.0) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_39_0;
  } else if(fabs(rxBw - 46.9) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_46_9;
  } else if(fabs(rxBw - 58.6) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_58_6;
  } else if(fabs(rxBw - 78.2) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_78_2;
  } else if(fabs(rxBw - 93.8) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_93_8;
  } else if(fabs(rxBw - 117.3) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_117_3;
  } else if(fabs(rxBw - 156.2) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_156_2;
  } else if(fabs(rxBw - 187.2) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_187_2;
  } else if(fabs(rxBw - 234.3) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_234_3;
  } else if(fabs(rxBw - 312.0) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_312_0;
  } else if(fabs(rxBw - 373.6) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_373_6;
  } else if(fabs(rxBw - 467.0) <= 0.001) {
    this->rxBandwidth = RADIOLIB_LR11X0_GFSK_RX_BW_467_0;
  } else {
    return(RADIOLIB_ERR_INVALID_RX_BANDWIDTH);
  }

  // update modulation parameters
  return(setModulationParamsGFSK(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev));
}

int16_t LR11x0::setSyncWord(uint8_t* syncWord, size_t len) {
  if((!syncWord) || (!len) || (len > RADIOLIB_LR11X0_GFSK_SYNC_WORD_LEN)) {
    return(RADIOLIB_ERR_INVALID_SYNC_WORD);
  }

  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    // with length set to 1 and LoRa modem active, assume it is the LoRa sync word
    if(len > 1) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }
    return(setSyncWord(syncWord[0]));

  } else if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  
  }

  // update sync word length
  this->syncWordLength = len*8;
  state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening);
  RADIOLIB_ASSERT(state);

  // sync word is passed most-significant byte first
  uint8_t fullSyncWord[RADIOLIB_LR11X0_GFSK_SYNC_WORD_LEN] = { 0 };
  memcpy(fullSyncWord, syncWord, len);
  return(setGfskSyncWord(fullSyncWord));
}

int16_t LR11x0::setSyncBits(uint8_t *syncWord, uint8_t bitsLen) {
  if((!syncWord) || (!bitsLen) || (bitsLen > 8*RADIOLIB_LR11X0_GFSK_SYNC_WORD_LEN)) {
    return(RADIOLIB_ERR_INVALID_SYNC_WORD);
  }

  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  uint8_t bytesLen = bitsLen / 8;
  if ((bitsLen % 8) != 0) {
    bytesLen++;
  }

  return(setSyncWord(syncWord, bytesLen));
}

int16_t LR11x0::setNodeAddress(uint8_t nodeAddr) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // enable address filtering (node only)
  this->addrComp = RADIOLIB_LR11X0_GFSK_ADDR_FILTER_NODE;
  state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening);
  RADIOLIB_ASSERT(state);
  
  // set node address
  this->node = nodeAddr;
  return(setPacketAdrs(this->node, 0));
}

int16_t LR11x0::setBroadcastAddress(uint8_t broadAddr) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // enable address filtering (node and broadcast)
  this->addrComp = RADIOLIB_LR11X0_GFSK_ADDR_FILTER_NODE_BROADCAST;
  state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening);
  RADIOLIB_ASSERT(state);
  
  // set node and broadcast address
  return(setPacketAdrs(this->node, broadAddr));
}

int16_t LR11x0::disableAddressFiltering() {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // disable address filterin
  this->addrComp = RADIOLIB_LR11X0_GFSK_ADDR_FILTER_DISABLED;
  return(setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
}

int16_t LR11x0::setDataShaping(uint8_t sh) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set data shaping
  switch(sh) {
    case RADIOLIB_SHAPING_NONE:
      this->pulseShape = RADIOLIB_LR11X0_GFSK_SHAPING_NONE;
      break;
    case RADIOLIB_SHAPING_0_3:
      this->pulseShape = RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_0_3;
      break;
    case RADIOLIB_SHAPING_0_5:
      this->pulseShape = RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_0_5;
      break;
    case RADIOLIB_SHAPING_0_7:
      this->pulseShape = RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_0_7;
      break;
    case RADIOLIB_SHAPING_1_0:
      this->pulseShape = RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_1_0;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_DATA_SHAPING);
  }

  // update modulation parameters
  return(setModulationParamsGFSK(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev));
}

int16_t LR11x0::setEncoding(uint8_t encoding) {
  return(setWhitening(encoding));
}

int16_t LR11x0::fixedPacketLengthMode(uint8_t len) {
  return(setPacketMode(RADIOLIB_LR11X0_GFSK_PACKET_LENGTH_FIXED, len));
}

int16_t LR11x0::variablePacketLengthMode(uint8_t maxLen) {
  return(setPacketMode(RADIOLIB_LR11X0_GFSK_PACKET_LENGTH_VARIABLE, maxLen));
}

int16_t LR11x0::setWhitening(bool enabled, uint16_t initial) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  if(!enabled) {
    // disable whitening
    this->whitening = RADIOLIB_LR11X0_GFSK_WHITENING_DISABLED;

  } else {
    // enable whitening
    this->whitening = RADIOLIB_LR11X0_GFSK_WHITENING_ENABLED;

    // write initial whitening value
    state = setGfskWhitParams(initial);
    RADIOLIB_ASSERT(state);
  }

  return(setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
}

int16_t LR11x0::setDataRate(DataRate_t dr) {
  // select interpretation based on active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    // set the bit rate
    state = this->setBitRate(dr.fsk.bitRate);
    RADIOLIB_ASSERT(state);

    // set the frequency deviation
    state = this->setFrequencyDeviation(dr.fsk.freqDev);

  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    // set the spreading factor
    state = this->setSpreadingFactor(dr.lora.spreadingFactor);
    RADIOLIB_ASSERT(state);

    // set the bandwidth
    state = this->setBandwidth(dr.lora.bandwidth);
    RADIOLIB_ASSERT(state);

    // set the coding rate
    state = this->setCodingRate(dr.lora.codingRate);
  }

  return(state);
}

int16_t LR11x0::checkDataRate(DataRate_t dr) {
  // select interpretation based on active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);

  if(type == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    RADIOLIB_CHECK_RANGE(dr.fsk.bitRate, 0.6, 300.0, RADIOLIB_ERR_INVALID_BIT_RATE);
    RADIOLIB_CHECK_RANGE(dr.fsk.freqDev, 0.6, 200.0, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);
    return(RADIOLIB_ERR_NONE);

  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    RADIOLIB_CHECK_RANGE(dr.lora.spreadingFactor, 5, 12, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
    RADIOLIB_CHECK_RANGE(dr.lora.bandwidth, 0.0, 510.0, RADIOLIB_ERR_INVALID_BANDWIDTH);
    RADIOLIB_CHECK_RANGE(dr.lora.codingRate, 5, 8, RADIOLIB_ERR_INVALID_CODING_RATE);
    return(RADIOLIB_ERR_NONE);
  
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t LR11x0::setPreambleLength(size_t preambleLength) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    this->preambleLengthLoRa = preambleLength;
    return(setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType,  this->implicitLen, this->crcTypeLoRa, (uint8_t)this->invertIQEnabled));
  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    this->preambleLengthGFSK = preambleLength;
    this->preambleDetLength = RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_16_BITS;
    return(setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR11x0::setTCXO(float voltage, uint32_t delay) {
  // check if TCXO is enabled at all
  if(this->XTAL) {
    return(RADIOLIB_ERR_INVALID_TCXO_VOLTAGE);
  }

  // set mode to standby
  standby();

  // check RADIOLIB_LR11X0_ERROR_STAT_HF_XOSC_START_ERR flag and clear it
  uint16_t errors = 0;
  int16_t state = getErrors(&errors);
  RADIOLIB_ASSERT(state);
  if(errors & RADIOLIB_LR11X0_ERROR_STAT_HF_XOSC_START_ERR) {
    clearErrors();
  }

  // check 0 V disable
  if(fabs(voltage - 0.0) <= 0.001) {
    setTcxoMode(0, 0);
    return(reset());
  }

  // check allowed voltage values
  uint8_t tune = 0;
  if(fabs(voltage - 1.6) <= 0.001) {
    tune = RADIOLIB_LR11X0_TCXO_VOLTAGE_1_6;
  } else if(fabs(voltage - 1.7) <= 0.001) {
    tune = RADIOLIB_LR11X0_TCXO_VOLTAGE_1_7;
  } else if(fabs(voltage - 1.8) <= 0.001) {
    tune = RADIOLIB_LR11X0_TCXO_VOLTAGE_1_8;
  } else if(fabs(voltage - 2.2) <= 0.001) {
    tune = RADIOLIB_LR11X0_TCXO_VOLTAGE_2_2;
  } else if(fabs(voltage - 2.4) <= 0.001) {
    tune = RADIOLIB_LR11X0_TCXO_VOLTAGE_2_4;
  } else if(fabs(voltage - 2.7) <= 0.001) {
    tune = RADIOLIB_LR11X0_TCXO_VOLTAGE_2_7;
  } else if(fabs(voltage - 3.0) <= 0.001) {
    tune = RADIOLIB_LR11X0_TCXO_VOLTAGE_3_0;
  } else if(fabs(voltage - 3.3) <= 0.001) {
    tune = RADIOLIB_LR11X0_TCXO_VOLTAGE_3_3;
  } else {
    return(RADIOLIB_ERR_INVALID_TCXO_VOLTAGE);
  }

  // calculate delay value
  uint32_t delayValue = (uint32_t)((float)delay / 30.52f);
  if(delayValue == 0) {
    delayValue = 1;
  }
 
  // enable TCXO control
  return(setTcxoMode(tune, delayValue));
}

int16_t LR11x0::setCRC(uint8_t len, uint32_t initial, uint32_t polynomial, bool inverted) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    // LoRa CRC doesn't allow to set CRC polynomial, initial value, or inversion
    this->crcTypeLoRa = len > 0 ? RADIOLIB_LR11X0_LORA_CRC_ENABLED : RADIOLIB_LR11X0_LORA_CRC_DISABLED;
    state = setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, (uint8_t)this->invertIQEnabled);
  
  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    // update packet parameters
    switch(len) {
      case 0:
        this->crcTypeGFSK = RADIOLIB_LR11X0_GFSK_CRC_DISABLED;
        break;
      case 1:
        if(inverted) {
          this->crcTypeGFSK = RADIOLIB_LR11X0_GFSK_CRC_1_BYTE_INV;
        } else {
          this->crcTypeGFSK = RADIOLIB_LR11X0_GFSK_CRC_1_BYTE;
        }
        break;
      case 2:
        if(inverted) {
          this->crcTypeGFSK = RADIOLIB_LR11X0_GFSK_CRC_2_BYTE_INV;
        } else {
          this->crcTypeGFSK = RADIOLIB_LR11X0_GFSK_CRC_2_BYTE;
        }
        break;
      default:
        return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
    }

    state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening);
    RADIOLIB_ASSERT(state);

    state = setGfskCrcParams(initial, polynomial);
  
  }

  return(state);
}

int16_t LR11x0::invertIQ(bool enable) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  this->invertIQEnabled = enable;
  return(setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, (uint8_t)this->invertIQEnabled));
}

float LR11x0::getRSSI() {
  float val = 0;

  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  (void)getPacketType(&type);
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    (void)getPacketStatusLoRa(&val, NULL, NULL);

  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    (void)getPacketStatusGFSK(NULL, &val, NULL, NULL);
  
  }

  return(val);
}

float LR11x0::getSNR() {
  float val = 0;

  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  (void)getPacketType(&type);
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    (void)getPacketStatusLoRa(NULL, &val, NULL);
  }

  return(val);
}

float LR11x0::getFrequencyError() {
  // TODO implement this
  return(0);
}

size_t LR11x0::getPacketLength(bool update) {
  return(this->getPacketLength(update, NULL));
}

size_t LR11x0::getPacketLength(bool update, uint8_t* offset) {
  (void)update;

  // in implicit mode, return the cached value
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  (void)getPacketType(&type);
  if((type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) && (this->headerType == RADIOLIB_LR11X0_LORA_HEADER_IMPLICIT)) {
    return(this->implicitLen);
  }

  uint8_t len = 0;
  (void)getRxBufferStatus(&len, offset);
  return((size_t)len);
}

RadioLibTime_t LR11x0::getTimeOnAir(size_t len) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  (void)getPacketType(&type);
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    // calculate number of symbols
    float N_symbol = 0;
    if(this->codingRate <= RADIOLIB_LR11X0_LORA_CR_4_8_SHORT) {
      // legacy coding rate - nice and simple

      // get SF coefficients
      float coeff1 = 0;
      int16_t coeff2 = 0;
      int16_t coeff3 = 0;
      if(this->spreadingFactor < 7) {
        // SF5, SF6
        coeff1 = 6.25;
        coeff2 = 4*this->spreadingFactor;
        coeff3 = 4*this->spreadingFactor;
      } else if(this->spreadingFactor < 11) {
        // SF7. SF8, SF9, SF10
        coeff1 = 4.25;
        coeff2 = 4*this->spreadingFactor + 8;
        coeff3 = 4*this->spreadingFactor;
      } else {
        // SF11, SF12
        coeff1 = 4.25;
        coeff2 = 4*this->spreadingFactor + 8;
        coeff3 = 4*(this->spreadingFactor - 2);
      }

      // get CRC length
      int16_t N_bitCRC = 16;
      if(this->crcTypeLoRa == RADIOLIB_LR11X0_LORA_CRC_DISABLED) {
        N_bitCRC = 0;
      }

      // get header length
      int16_t N_symbolHeader = 20;
      if(this->headerType == RADIOLIB_LR11X0_LORA_HEADER_IMPLICIT) {
        N_symbolHeader = 0;
      }

      // calculate number of LoRa preamble symbols
      uint32_t N_symbolPreamble = (this->preambleLengthLoRa & 0x0F) * (uint32_t(1) << ((this->preambleLengthLoRa & 0xF0) >> 4));

      // calculate the number of symbols
      N_symbol = (float)N_symbolPreamble + coeff1 + 8.0 + ceil(RADIOLIB_MAX((int16_t)(8 * len + N_bitCRC - coeff2 + N_symbolHeader), (int16_t)0) / (float)coeff3) * (float)(this->codingRate + 4);

    } else {
      // long interleaving - abandon hope all ye who enter here
      /// \todo implement this mess - SX1280 datasheet v3.0 section 7.4.4.2

    }

    // get time-on-air in us
    return(((uint32_t(1) << this->spreadingFactor) / this->bandwidthKhz) * N_symbol * 1000.0);

  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(((uint32_t)len * 8 * 1000000UL) / this->bitRate);
  
  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS) {
    return(((uint32_t)len * 8 * 1000000UL) / RADIOLIB_LR11X0_LR_FHSS_BIT_RATE);
  }

  return(0);
}

RadioLibTime_t LR11x0::calculateRxTimeout(RadioLibTime_t timeoutUs) {
  // the timeout value is given in units of 30.52 microseconds
  // the calling function should provide some extra width, as this number of units is truncated to integer
  RadioLibTime_t timeout = timeoutUs / 30.52;
  return(timeout);
}

int16_t LR11x0::irqRxDoneRxTimeout(uint32_t &irqFlags, uint32_t &irqMask) {
  irqFlags = RADIOLIB_LR11X0_IRQ_RX_DONE | RADIOLIB_LR11X0_IRQ_TIMEOUT;  // flags that can appear in the IRQ register
  irqMask  = irqFlags; // on LR11x0, these are the same
  return(RADIOLIB_ERR_NONE);
}

bool LR11x0::isRxTimeout() {
  uint32_t irq = getIrqStatus();
  bool rxTimedOut = irq & RADIOLIB_LR11X0_IRQ_TIMEOUT;
  return(rxTimedOut);
}
  
uint8_t LR11x0::randomByte() {
  uint32_t num = 0;
  (void)getRandomNumber(&num);
  return((uint8_t)num);
}

int16_t LR11x0::implicitHeader(size_t len) {
  return(this->setHeaderType(RADIOLIB_LR11X0_LORA_HEADER_IMPLICIT, len));
}

int16_t LR11x0::explicitHeader() {
  return(this->setHeaderType(RADIOLIB_LR11X0_LORA_HEADER_EXPLICIT));
}

float LR11x0::getDataRate() const {
  return(this->dataRateMeasured);
}

int16_t LR11x0::setRegulatorLDO() {
  return(this->setRegMode(RADIOLIB_LR11X0_REG_MODE_LDO));
}

int16_t LR11x0::setRegulatorDCDC() {
  return(this->setRegMode(RADIOLIB_LR11X0_REG_MODE_DC_DC));
}

int16_t LR11x0::setRxBoostedGainMode(bool en) {
  uint8_t buff[1] = { (uint8_t)en };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RX_BOOSTED, true, buff, sizeof(buff)));
}

void LR11x0::setRfSwitchTable(const uint32_t (&pins)[Module::RFSWITCH_MAX_PINS], const Module::RfSwitchMode_t table[]) {
  // find which pins are used
  uint8_t enable = 0;
  for(size_t i = 0; i < Module::RFSWITCH_MAX_PINS; i++) {
    if((pins[i] == RADIOLIB_NC) || (pins[i] > RADIOLIB_LR11X0_DIO10)) {
      continue;
    }
    enable |= 1UL << pins[i];
  }

  // now get the configuration
  uint8_t modes[7] = { 0 };
  for(size_t i = 0; i < 7; i++) {
    for(size_t j = 0; j < Module::RFSWITCH_MAX_PINS; j++) {
      modes[i] |= (table[i].values[j] > 0) ? (1UL << j) : 0;
    }
  }

  // set it
  this->setDioAsRfSwitch(enable, modes[0], modes[1], modes[2], modes[3], modes[4], modes[5], modes[6]);
}

int16_t LR11x0::setLrFhssConfig(uint8_t bw, uint8_t cr, uint8_t hdrCount, uint16_t hopSeed) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check and cache all parameters
  RADIOLIB_CHECK_RANGE((int8_t)cr, (int8_t)RADIOLIB_LR11X0_LR_FHSS_CR_5_6, (int8_t)RADIOLIB_LR11X0_LR_FHSS_CR_1_3, RADIOLIB_ERR_INVALID_CODING_RATE);
  this->lrFhssCr = cr;
  RADIOLIB_CHECK_RANGE((int8_t)bw, (int8_t)RADIOLIB_LR11X0_LR_FHSS_BW_39_06, (int8_t)RADIOLIB_LR11X0_LR_FHSS_BW_1574_2, RADIOLIB_ERR_INVALID_BANDWIDTH);
  this->lrFhssBw = bw;
  RADIOLIB_CHECK_RANGE(hdrCount, 1, 4, RADIOLIB_ERR_INVALID_BIT_RANGE);
  this->lrFhssHdrCount = hdrCount;
  RADIOLIB_CHECK_RANGE((int16_t)hopSeed, (int16_t)0x000, (int16_t)0x1FF, RADIOLIB_ERR_INVALID_DATA_SHAPING);
  this->lrFhssHopSeq = hopSeed;
  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::startWifiScan(char wifiType, uint8_t mode, uint16_t chanMask, uint8_t numScans, uint16_t timeout) {
  uint8_t type;
  switch(wifiType) {
    case('b'):
      type = RADIOLIB_LR11X0_WIFI_SCAN_802_11_B;
      break;
    case('g'):
      type = RADIOLIB_LR11X0_WIFI_SCAN_802_11_G;
      break;
    case('n'):
      type = RADIOLIB_LR11X0_WIFI_SCAN_802_11_N;
      break;
    case('*'):
      type = RADIOLIB_LR11X0_WIFI_SCAN_ALL;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_WIFI_TYPE);
  }

  // go to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // reset cumulative timings
  state = wifiResetCumulTimings();
  RADIOLIB_ASSERT(state);

  // set DIO mapping
  state = setDioIrqParams(RADIOLIB_LR11X0_IRQ_WIFI_DONE, 0);
  RADIOLIB_ASSERT(state);

  // start scan with the maximum number of results and abort on timeout
  this->wifiScanMode = mode;
  state = wifiScan(type, chanMask, this->wifiScanMode, RADIOLIB_LR11X0_WIFI_MAX_NUM_RESULTS, numScans, timeout, RADIOLIB_LR11X0_WIFI_ABORT_ON_TIMEOUT_ENABLED);
  return(state);
}

void LR11x0::setWiFiScanAction(void (*func)(void)) {
  this->setIrqAction(func);
}

void LR11x0::clearWiFiScanAction() {
  this->clearIrqAction();
}

int16_t LR11x0::getWifiScanResultsCount(uint8_t* count) {
  // clear IRQ first, as this is likely to be called right after scan has finished
  int16_t state = clearIrq(RADIOLIB_LR11X0_IRQ_ALL);
  RADIOLIB_ASSERT(state);

  uint8_t buff[1] = { 0 };
  state = this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_GET_NB_RESULTS, false, buff, sizeof(buff));

  // pass the replies
  if(count) { *count = buff[0]; }

  return(state);
}

int16_t LR11x0::getWifiScanResult(LR11x0WifiResult_t* result, uint8_t index, bool brief) {
  if(!result) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }

  // read a single result
  uint8_t format = brief ? RADIOLIB_LR11X0_WIFI_RESULT_TYPE_BASIC : RADIOLIB_LR11X0_WIFI_RESULT_TYPE_COMPLETE;
  uint8_t raw[RADIOLIB_LR11X0_WIFI_RESULT_MAX_LEN] = { 0 };
  int16_t state = wifiReadResults(index, 1, format, raw);
  RADIOLIB_ASSERT(state);

  // parse the information
  switch(raw[0] & 0x03) {
    case(RADIOLIB_LR11X0_WIFI_SCAN_802_11_B):
      result->type = 'b';
      break;
    case(RADIOLIB_LR11X0_WIFI_SCAN_802_11_G):
      result->type = 'g';
      break;
    case(RADIOLIB_LR11X0_WIFI_SCAN_802_11_N):
      result->type = 'n';
      break;
  }
  result->dataRateId = (raw[0] & 0xFC) >> 2;
  result->channelFreq = 2407 + (raw[1] & 0x0F)*5;
  result->origin = (raw[1] & 0x30) >> 4;
  result->ap = (raw[1] & 0x40) != 0;
  result->rssi = (float)raw[2] / -2.0f;;
  memcpy(result->mac, &raw[3], RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN);

  if(!brief) {
    if(this->wifiScanMode == RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON) {
      LR11x0WifiResultExtended_t* resultExtended = reinterpret_cast<LR11x0WifiResultExtended_t*>(result);
      resultExtended->rate = raw[3];
      resultExtended->service = (((uint16_t)raw[4] << 8) | ((uint16_t)raw[5]));
      resultExtended->length = (((uint16_t)raw[6] << 8) | ((uint16_t)raw[7]));
      resultExtended->frameType = raw[9] & 0x03;
      resultExtended->frameSubType = (raw[9] & 0x3C) >> 2;
      resultExtended->toDistributionSystem = (raw[9] & 0x40) != 0;
      resultExtended->fromDistributionSystem = (raw[9] & 0x80) != 0;
      memcpy(resultExtended->mac0, &raw[10], RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN);
      memcpy(resultExtended->mac, &raw[16], RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN);
      memcpy(resultExtended->mac2, &raw[22], RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN);
      resultExtended->timestamp = (((uint64_t)raw[28] << 56) | ((uint64_t)raw[29] << 48)) | 
                                  (((uint64_t)raw[30] << 40) | ((uint64_t)raw[31] << 32)) | 
                                  (((uint64_t)raw[32] << 24) | ((uint64_t)raw[33] << 16)) | 
                                  (((uint64_t)raw[34] << 8) | (uint64_t)raw[35]);
      resultExtended->periodBeacon = (((uint16_t)raw[36] << 8) | ((uint16_t)raw[37])) * 1024UL;
      resultExtended->seqCtrl = (((uint16_t)raw[38] << 8) | ((uint16_t)raw[39]));
      memcpy(resultExtended->ssid, &raw[40], RADIOLIB_LR11X0_WIFI_RESULT_SSID_LEN);
      resultExtended->currentChannel = raw[72];
      memcpy(resultExtended->countryCode, &raw[73], 2);
      resultExtended->countryCode[2] = '\0';
      resultExtended->ioReg = raw[75];
      resultExtended->fcsCheckOk = (raw[76] != 0);
      resultExtended->phiOffset = (((uint16_t)raw[77] << 8) | ((uint16_t)raw[78]));
      return(RADIOLIB_ERR_NONE);
    }

    LR11x0WifiResultFull_t* resultFull = reinterpret_cast<LR11x0WifiResultFull_t*>(result);
    resultFull->frameType = raw[3] & 0x03;
    resultFull->frameSubType = (raw[3] & 0x3C) >> 2;
    resultFull->toDistributionSystem = (raw[3] & 0x40) != 0;
    resultFull->fromDistributionSystem = (raw[3] & 0x80) != 0;
    memcpy(resultFull->mac, &raw[4], RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN);
    resultFull->phiOffset = (((uint16_t)raw[10] << 8) | ((uint16_t)raw[11]));
    resultFull->timestamp = (((uint64_t)raw[12] << 56) | ((uint64_t)raw[13] << 48)) | 
                            (((uint64_t)raw[14] << 40) | ((uint64_t)raw[15] << 32)) | 
                            (((uint64_t)raw[16] << 24) | ((uint64_t)raw[17] << 16)) | 
                            (((uint64_t)raw[18] << 8) | (uint64_t)raw[19]);
    resultFull->periodBeacon = (((uint16_t)raw[20] << 8) | ((uint16_t)raw[21])) * 1024UL;
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::wifiScan(uint8_t wifiType, uint8_t* count, uint8_t mode, uint16_t chanMask, uint8_t numScans, uint16_t timeout) {
  if(!count) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }

  // start scan
  RADIOLIB_DEBUG_BASIC_PRINTLN("WiFi scan start");
  int16_t state = startWifiScan(wifiType, mode, chanMask, numScans, timeout);
  RADIOLIB_ASSERT(state);

  // wait for scan finished or timeout
  RadioLibTime_t softTimeout = 30UL * 1000UL;
  RadioLibTime_t start = this->mod->hal->millis();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    if(this->mod->hal->millis() - start > softTimeout) {
      RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout waiting for IRQ");
      this->standby();
      return(RADIOLIB_ERR_RX_TIMEOUT);
    }
  }
  RADIOLIB_DEBUG_BASIC_PRINTLN("WiFi scan done in %lu ms", (long unsigned int)(this->mod->hal->millis() - start));

  // read number of results
  return(getWifiScanResultsCount(count));
}

int16_t LR11x0::getVersionInfo(LR11x0VersionInfo_t* info) {
  if(!info) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }

  int16_t state = this->getVersion(&info->hardware, &info->device, &info->fwMajor, &info->fwMinor);
  RADIOLIB_ASSERT(state);
  state = this->wifiReadVersion(&info->fwMajorWiFi, &info->fwMinorWiFi);
  RADIOLIB_ASSERT(state);
  return(this->gnssReadVersion(&info->fwGNSS, &info->almanacGNSS));
}

int16_t LR11x0::updateFirmware(const uint32_t* image, size_t size, bool nonvolatile) {
  if(!image) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }

  // put the device to bootloader mode
  int16_t state = this->reboot(true);
  RADIOLIB_ASSERT(state);
  this->mod->hal->delay(500);

  // check we're in bootloader
  uint8_t device = 0xFF;
  state = this->getVersion(NULL, &device, NULL, NULL);
  RADIOLIB_ASSERT(state);
  if(device != RADIOLIB_LR11X0_DEVICE_BOOT) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Failed to put device to bootloader mode, %02x != %02x", (unsigned int)device, (unsigned int)RADIOLIB_LR11X0_DEVICE_BOOT);
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }

  // erase the image
  state = this->bootEraseFlash();
  RADIOLIB_ASSERT(state);

  // wait for BUSY to go low
  RadioLibTime_t start = this->mod->hal->millis();
  while(this->mod->hal->digitalRead(this->mod->getGpio())) {
    this->mod->hal->yield();
    if(this->mod->hal->millis() - start >= 3000) {
      RADIOLIB_DEBUG_BASIC_PRINTLN("BUSY pin timeout after erase!");
      return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
    }
  }

  // upload the new image
  const size_t maxLen = 64;
  size_t rem = size % maxLen;
  size_t numWrites = (rem == 0) ? (size / maxLen) : ((size / maxLen) + 1);
  RADIOLIB_DEBUG_BASIC_PRINTLN("Writing image in %lu chunks, last chunk size is %lu words", (unsigned long)numWrites, (unsigned long)rem);
  for(size_t i = 0; i < numWrites; i ++) {
    uint32_t offset = i * maxLen;
    uint32_t len = (i == (numWrites - 1)) ? rem : maxLen;
    RADIOLIB_DEBUG_BASIC_PRINTLN("Writing chunk %d at offset %08lx (%u words)", (int)i, (unsigned long)offset, (unsigned int)len);
    this->bootWriteFlashEncrypted(offset*sizeof(uint32_t), (uint32_t*)&image[offset], len, nonvolatile);
  }

  // kick the device from bootloader
  state = this->reset();
  RADIOLIB_ASSERT(state);

  // verify we are no longer in bootloader
  state = this->getVersion(NULL, &device, NULL, NULL);
  RADIOLIB_ASSERT(state);
  if(device == RADIOLIB_LR11X0_DEVICE_BOOT) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Failed to kick device from bootloader mode, %02x == %02x", (unsigned int)device, (unsigned int)RADIOLIB_LR11X0_DEVICE_BOOT);
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }

  return(state);
}

int16_t LR11x0::modSetup(float tcxoVoltage, uint8_t modem) {
  this->mod->init();
  this->mod->hal->pinMode(this->mod->getIrq(), this->mod->hal->GpioModeInput);
  this->mod->hal->pinMode(this->mod->getGpio(), this->mod->hal->GpioModeInput);
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR] = Module::BITS_32;
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_16;
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_8;
  this->mod->spiConfig.statusPos = 0;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] = RADIOLIB_LR11X0_CMD_READ_REG_MEM;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] = RADIOLIB_LR11X0_CMD_WRITE_REG_MEM;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_NOP] = RADIOLIB_LR11X0_CMD_NOP;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_STATUS] = RADIOLIB_LR11X0_CMD_GET_STATUS;
  this->mod->spiConfig.stream = true;
  this->mod->spiConfig.parseStatusCb = SPIparseStatus;
  this->mod->spiConfig.checkStatusCb = SPIcheckStatus;

  // try to find the LR11x0 chip - this will also reset the module at least once
  if(!LR11x0::findChip(this->chipType)) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("No LR11x0 found!");
    this->mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  RADIOLIB_DEBUG_BASIC_PRINTLN("M\tLR11x0");

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set TCXO control, if requested
  if(!this->XTAL && tcxoVoltage > 0.0) {
    state = setTCXO(tcxoVoltage);
    RADIOLIB_ASSERT(state);
  }

  // configure settings not accessible by API
  return(config(modem));
}

int16_t LR11x0::SPIparseStatus(uint8_t in) {
  if((in & 0b00001110) == RADIOLIB_LR11X0_STAT_1_CMD_PERR) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  } else if((in & 0b00001110) == RADIOLIB_LR11X0_STAT_1_CMD_FAIL) {
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  } else if((in == 0x00) || (in == 0xFF)) {
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::SPIcheckStatus(Module* mod) {
  // the status check command doesn't return status in the same place as other read commands,
  // but only as the first byte (as with any other command), hence LR11x0::SPIcommand can't be used
  // it also seems to ignore the actual command, and just sending in bunch of NOPs will work 
  uint8_t buff[6] = { 0 };
  mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_0;
  int16_t state = mod->SPItransferStream(NULL, 0, false, NULL, buff, sizeof(buff), true);
  mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_8;
  RADIOLIB_ASSERT(state);
  return(LR11x0::SPIparseStatus(buff[0]));
}

int16_t LR11x0::SPIcommand(uint16_t cmd, bool write, uint8_t* data, size_t len, uint8_t* out, size_t outLen) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;
  if(!write) {
    // the SPI interface of LR11x0 requires two separate transactions for reading
    // send the 16-bit command
    state = this->mod->SPIwriteStream(cmd, out, outLen, true, false);
    RADIOLIB_ASSERT(state);

    // read the result without command
    this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_0;
    state = this->mod->SPIreadStream(RADIOLIB_LR11X0_CMD_NOP, data, len, true, false);
    this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_16;

  } else {
    // write is just a single transaction
    state = this->mod->SPIwriteStream(cmd, data, len, true, true);
  
  }
  
  return(state);
}

bool LR11x0::findChip(uint8_t ver) {
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    // reset the module
    reset();

    // read the version
    LR11x0VersionInfo_t info;
    int16_t state = getVersionInfo(&info);
    if((state == RADIOLIB_ERR_NONE) && (info.device == ver)) {
      RADIOLIB_DEBUG_BASIC_PRINTLN("Found LR11x0: RADIOLIB_LR11X0_CMD_GET_VERSION = 0x%02x", info.device);
      RADIOLIB_DEBUG_BASIC_PRINTLN("Base FW version: %d.%d", (int)info.fwMajor, (int)info.fwMinor);
      RADIOLIB_DEBUG_BASIC_PRINTLN("WiFi FW version: %d.%d", (int)info.fwMajorWiFi, (int)info.fwMinorWiFi);
      RADIOLIB_DEBUG_BASIC_PRINTLN("GNSS FW version: %d.%d", (int)info.fwGNSS, (int)info.almanacGNSS);
      flagFound = true;
    } else {
      RADIOLIB_DEBUG_BASIC_PRINTLN("LR11x0 not found! (%d of 10 tries) RADIOLIB_LR11X0_CMD_GET_VERSION = 0x%02x", i + 1, info.device);
      RADIOLIB_DEBUG_BASIC_PRINTLN("Expected: 0x%02x", ver);
      this->mod->hal->delay(10);
      i++;
    }
  }

  return(flagFound);
}

int16_t LR11x0::config(uint8_t modem) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;

  // set Rx/Tx fallback mode to STDBY_RC
  state = this->setRxTxFallbackMode(RADIOLIB_LR11X0_FALLBACK_MODE_STBY_RC);
  RADIOLIB_ASSERT(state);

  // clear IRQ
  state = this->clearIrq(RADIOLIB_LR11X0_IRQ_ALL);
  state |= this->setDioIrqParams(RADIOLIB_LR11X0_IRQ_NONE, RADIOLIB_LR11X0_IRQ_NONE);
  RADIOLIB_ASSERT(state);

  // calibrate all blocks
  (void)this->calibrate(RADIOLIB_LR11X0_CALIBRATE_ALL);

  // wait for calibration completion
  this->mod->hal->delay(5);
  while(this->mod->hal->digitalRead(this->mod->getGpio())) {
    this->mod->hal->yield();
  }
  
  // if something failed, show the device errors
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

int16_t LR11x0::setPacketMode(uint8_t mode, uint8_t len) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set requested packet mode
  state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, mode, len, this->crcTypeGFSK, this->whitening);
  RADIOLIB_ASSERT(state);

  // update cached value
  this->packetType = mode;
  return(state);
}

int16_t LR11x0::startCad(uint8_t symbolNum, uint8_t detPeak, uint8_t detMin) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // select CAD parameters
  // TODO the magic numbers are based on Semtech examples, this is probably suboptimal
  uint8_t num = symbolNum;
  if(num == RADIOLIB_LR11X0_CAD_PARAM_DEFAULT) {
    num = 2;
  }
  
  const uint8_t detPeakValues[8] = { 48, 48, 50, 55, 55, 59, 61, 65 };
  uint8_t peak = detPeak;
  if(peak == RADIOLIB_LR11X0_CAD_PARAM_DEFAULT) {
    peak = detPeakValues[this->spreadingFactor - 5];
  }

  uint8_t min = detMin;
  if(min == RADIOLIB_LR11X0_CAD_PARAM_DEFAULT) {
    min = 10;
  }

  // set CAD parameters
  // TODO add configurable exit mode and timeout
  state = setCadParams(num, peak, min, RADIOLIB_LR11X0_CAD_EXIT_MODE_STBY_RC, 0);
  RADIOLIB_ASSERT(state);

  // start CAD
  return(setCad());
}

int16_t LR11x0::setHeaderType(uint8_t hdrType, size_t len) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set requested packet mode
  state = setPacketParamsLoRa(this->preambleLengthLoRa, hdrType, len, this->crcTypeLoRa, this->invertIQEnabled);
  RADIOLIB_ASSERT(state);

  // update cached value
  this->headerType = hdrType;
  this->implicitLen = len;

  return(state);
}

Module* LR11x0::getMod() {
  return(this->mod);
}

int16_t LR11x0::writeRegMem32(uint32_t addr, uint32_t* data, size_t len) {
  // check maximum size
  if(len > (RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->writeCommon(RADIOLIB_LR11X0_CMD_WRITE_REG_MEM, addr, data, len, false));
}

int16_t LR11x0::readRegMem32(uint32_t addr, uint32_t* data, size_t len) {
  // check maximum size
  if(len >= (RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
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
    uint8_t rplBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* rplBuff = new uint8_t[len*sizeof(uint32_t)];
  #endif

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_READ_REG_MEM, false, rplBuff, len*sizeof(uint32_t), reqBuff, sizeof(reqBuff));

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

int16_t LR11x0::writeBuffer8(uint8_t* data, size_t len) {
  // check maximum size
  if(len > RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WRITE_BUFFER, true, data, len));
}

int16_t LR11x0::readBuffer8(uint8_t* data, size_t len, size_t offset) {
  // check maximum size
  if(len > RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // build buffers
  size_t reqLen = 2*sizeof(uint8_t) + len;
  #if RADIOLIB_STATIC_ONLY
    uint8_t reqBuff[sizeof(uint32_t) + RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* reqBuff = new uint8_t[reqLen];
  #endif

  // set the offset and length
  reqBuff[0] = (uint8_t)offset;
  reqBuff[1] = (uint8_t)len;

  // send the request
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_READ_BUFFER, false, data, len, reqBuff, reqLen);
  #if !RADIOLIB_STATIC_ONLY
    delete[] reqBuff;
  #endif
  return(state);
}

int16_t LR11x0::clearRxBuffer(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CLEAR_RX_BUFFER, true, NULL, 0));
}

int16_t LR11x0::writeRegMemMask32(uint32_t addr, uint32_t mask, uint32_t data) {
  uint8_t buff[12] = {
    (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
    (uint8_t)((mask >> 24) & 0xFF), (uint8_t)((mask >> 16) & 0xFF), (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    (uint8_t)((data >> 24) & 0xFF), (uint8_t)((data >> 16) & 0xFF), (uint8_t)((data >> 8) & 0xFF), (uint8_t)(data & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WRITE_REG_MEM_MASK, true, buff, sizeof(buff)));
}

int16_t LR11x0::getStatus(uint8_t* stat1, uint8_t* stat2, uint32_t* irq) {
  uint8_t buff[6] = { 0 };

  // the status check command doesn't return status in the same place as other read commands
  // but only as the first byte (as with any other command), hence LR11x0::SPIcommand can't be used
  // it also seems to ignore the actual command, and just sending in bunch of NOPs will work 
  int16_t state = this->mod->SPItransferStream(NULL, 0, false, NULL, buff, sizeof(buff), true);

  // pass the replies
  if(stat1) { *stat1 = buff[0]; }
  if(stat2) { *stat2 = buff[1]; }
  if(irq)   { *irq = ((uint32_t)(buff[2]) << 24) | ((uint32_t)(buff[3]) << 16) | ((uint32_t)(buff[4]) << 8) | (uint32_t)buff[5]; }

  return(state);
}

int16_t LR11x0::getVersion(uint8_t* hw, uint8_t* device, uint8_t* major, uint8_t* minor) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_VERSION, false, buff, sizeof(buff));

  // pass the replies
  if(hw)      { *hw = buff[0]; }
  if(device)  { *device = buff[1]; }
  if(major)   { *major = buff[2]; }
  if(minor)   { *minor = buff[3]; }

  return(state);
}

int16_t LR11x0::getErrors(uint16_t* err) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_ERRORS, false, buff, sizeof(buff));

  // pass the replies
  if(err) { *err = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1];  }

  return(state);
}

int16_t LR11x0::clearErrors(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CLEAR_ERRORS, true, NULL, 0));
}

int16_t LR11x0::calibrate(uint8_t params) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CALIBRATE, true, &params, 1));
}

int16_t LR11x0::setRegMode(uint8_t mode) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_REG_MODE, true, &mode, 1));
}

int16_t LR11x0::calibImage(float freq1, float freq2) {
  uint8_t buff[2] = {
    (uint8_t)floor((freq1 - 1.0f) / 4.0f),
    (uint8_t)ceil((freq2 + 1.0f) / 4.0f)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CALIB_IMAGE, true, buff, sizeof(buff)));
}

int16_t LR11x0::setDioAsRfSwitch(uint8_t en, uint8_t stbyCfg, uint8_t rxCfg, uint8_t txCfg, uint8_t txHpCfg, uint8_t txHfCfg, uint8_t gnssCfg, uint8_t wifiCfg) {
  uint8_t buff[8] = { en, stbyCfg, rxCfg, txCfg, txHpCfg, txHfCfg, gnssCfg, wifiCfg };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_DIO_AS_RF_SWITCH, true, buff, sizeof(buff)));
}

int16_t LR11x0::setDioIrqParams(uint32_t irq1, uint32_t irq2) {
  uint8_t buff[8] = {
    (uint8_t)((irq1 >> 24) & 0xFF), (uint8_t)((irq1 >> 16) & 0xFF), (uint8_t)((irq1 >> 8) & 0xFF), (uint8_t)(irq1 & 0xFF),
    (uint8_t)((irq2 >> 24) & 0xFF), (uint8_t)((irq2 >> 16) & 0xFF), (uint8_t)((irq2 >> 8) & 0xFF), (uint8_t)(irq2 & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_DIO_IRQ_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::clearIrq(uint32_t irq) {
  uint8_t buff[4] = {
    (uint8_t)((irq >> 24) & 0xFF), (uint8_t)((irq >> 16) & 0xFF), (uint8_t)((irq >> 8) & 0xFF), (uint8_t)(irq & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CLEAR_IRQ, true, buff, sizeof(buff)));
}

int16_t LR11x0::configLfClock(uint8_t setup) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_REG_MODE, true, &setup, 1));
}

int16_t LR11x0::setTcxoMode(uint8_t tune, uint32_t delay) {
  uint8_t buff[4] = {
    tune, (uint8_t)((delay >> 16) & 0xFF), (uint8_t)((delay >> 8) & 0xFF), (uint8_t)(delay & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_TCXO_MODE, true, buff, sizeof(buff)));
}

int16_t LR11x0::reboot(bool stay) {
  uint8_t buff[1] = { (uint8_t)(stay*3) };
  return(this->mod->SPIwriteStream(RADIOLIB_LR11X0_CMD_REBOOT, buff, sizeof(buff), true, false));
}

int16_t LR11x0::getVbat(float* vbat) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_VBAT, false, buff, sizeof(buff));

  // pass the replies
  if(vbat) { *vbat = (((float)buff[0]/51.0f) - 1.0f)*1.35f; }

  return(state);
}

int16_t LR11x0::getTemp(float* temp) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_TEMP, false, buff, sizeof(buff));

  // pass the replies
  if(temp) {
    uint16_t raw = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1];
    *temp = 25.0f - (1000.0f/1.7f)*(((float)raw/2047.0f)*1350.0f - 0.7295f);
  }

  return(state);
}

int16_t LR11x0::setFs(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_FS, true, NULL, 0));
}

int16_t LR11x0::getRandomNumber(uint32_t* rnd) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_RANDOM_NUMBER, false, buff, sizeof(buff));

  // pass the replies
  if(rnd) { *rnd = ((uint32_t)(buff[0]) << 24) | ((uint32_t)(buff[1]) << 16) | ((uint32_t)(buff[2]) << 8) | (uint32_t)buff[3];  }

  return(state);
}

int16_t LR11x0::eraseInfoPage(void) {
  // only page 1 can be erased
  uint8_t buff[1] = { RADIOLIB_LR11X0_INFO_PAGE };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_ERASE_INFO_PAGE, true, buff, sizeof(buff)));
}

int16_t LR11x0::writeInfoPage(uint16_t addr, const uint32_t* data, size_t len) {
  // check maximum size
  if(len > (RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // build buffers - later we need to ensure endians are correct, 
  // so there is probably no way to do this without copying buffers and iterating
  size_t buffLen = sizeof(uint8_t) + sizeof(uint16_t) + len*sizeof(uint32_t);
  #if RADIOLIB_STATIC_ONLY
    uint8_t dataBuff[sizeof(uint8_t) + sizeof(uint16_t) + RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* dataBuff = new uint8_t[buffLen];
  #endif

  // set the address
  dataBuff[0] = RADIOLIB_LR11X0_INFO_PAGE;
  dataBuff[1] = (uint8_t)((addr >> 8) & 0xFF);
  dataBuff[2] = (uint8_t)(addr & 0xFF);

  // convert endians
  for(size_t i = 0; i < len; i++) {
    dataBuff[3 + i] = (uint8_t)((data[i] >> 24) & 0xFF);
    dataBuff[4 + i] = (uint8_t)((data[i] >> 16) & 0xFF);
    dataBuff[5 + i] = (uint8_t)((data[i] >> 8) & 0xFF);
    dataBuff[6 + i] = (uint8_t)(data[i] & 0xFF);
  }

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_WRITE_INFO_PAGE, true, dataBuff, buffLen);
  #if !RADIOLIB_STATIC_ONLY
    delete[] dataBuff;
  #endif
  return(state);
}

int16_t LR11x0::readInfoPage(uint16_t addr, uint32_t* data, size_t len) {
  // check maximum size
  if(len > (RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // the request contains the address and length
  uint8_t reqBuff[4] = {
    RADIOLIB_LR11X0_INFO_PAGE,
    (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
    (uint8_t)len,
  };

  // build buffers - later we need to ensure endians are correct, 
  // so there is probably no way to do this without copying buffers and iterating
  #if RADIOLIB_STATIC_ONLY
    uint8_t rplBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* rplBuff = new uint8_t[len*sizeof(uint32_t)];
  #endif

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_READ_INFO_PAGE, false, rplBuff, len*sizeof(uint32_t), reqBuff, sizeof(reqBuff));

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

int16_t LR11x0::getChipEui(uint8_t* eui) {
  if(!eui) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_CHIP_EUI, false, eui, RADIOLIB_LR11X0_EUI_LEN));
}

int16_t LR11x0::getSemtechJoinEui(uint8_t* eui) {
  if(!eui) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_SEMTECH_JOIN_EUI, false, eui, RADIOLIB_LR11X0_EUI_LEN));
}

int16_t LR11x0::deriveRootKeysAndGetPin(uint8_t* pin) {
  if(!pin) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_DERIVE_ROOT_KEYS_AND_GET_PIN, false, pin, RADIOLIB_LR11X0_PIN_LEN));
}

int16_t LR11x0::enableSpiCrc(bool en) {
  // TODO implement this
  (void)en;
  // LR11X0 CRC is gen 0xA6 (0x65 but reflected), init 0xFF, input and result reflected
  /*RadioLibCRCInstance.size = 8;
  RadioLibCRCInstance.poly = 0xA6;
  RadioLibCRCInstance.init = 0xFF;
  RadioLibCRCInstance.out = 0x00;
  RadioLibCRCInstance.refIn = true;
  RadioLibCRCInstance.refOut = true;*/
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t LR11x0::driveDiosInSleepMode(bool en) {
  uint8_t buff[1] = { (uint8_t)en };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_DRIVE_DIOS_IN_SLEEP_MODE, true, buff, sizeof(buff)));
}

int16_t LR11x0::resetStats(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_RESET_STATS, true, NULL, 0));
}

int16_t LR11x0::getStats(uint16_t* nbPktReceived, uint16_t* nbPktCrcError, uint16_t* data1, uint16_t* data2) {
  uint8_t buff[8] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_STATS, false, buff, sizeof(buff));

  // pass the replies
  if(nbPktReceived) { *nbPktReceived = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1]; }
  if(nbPktCrcError) { *nbPktCrcError = ((uint16_t)(buff[2]) << 8) | (uint16_t)buff[3]; }
  if(data1) { *data1 = ((uint16_t)(buff[4]) << 8) | (uint16_t)buff[5]; }
  if(data2) { *data2 = ((uint16_t)(buff[6]) << 8) | (uint16_t)buff[7]; }

  return(state);
}

int16_t LR11x0::getPacketType(uint8_t* type) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_PACKET_TYPE, false, buff, sizeof(buff));

  // pass the replies
  if(type) { *type = buff[0]; }

  return(state);
}

int16_t LR11x0::getRxBufferStatus(uint8_t* len, uint8_t* startOffset) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_RX_BUFFER_STATUS, false, buff, sizeof(buff));

  // pass the replies
  if(len) { *len = buff[0]; }
  if(startOffset) { *startOffset = buff[1]; }

  return(state);
}

int16_t LR11x0::getPacketStatusLoRa(float* rssiPkt, float* snrPkt, float* signalRssiPkt) {
  uint8_t buff[3] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_PACKET_STATUS, false, buff, sizeof(buff));

  // pass the replies
  if(rssiPkt) { *rssiPkt = (float)buff[0] / -2.0f; }
  if(snrPkt) { *snrPkt = (float)buff[1] / 4.0f; }
  if(signalRssiPkt) { *signalRssiPkt = buff[2]; }

  return(state);
}

int16_t LR11x0::getPacketStatusGFSK(float* rssiSync, float* rssiAvg, uint8_t* rxLen, uint8_t* stat) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_PACKET_STATUS, false, buff, sizeof(buff));

  // pass the replies
  if(rssiSync) { *rssiSync = (float)buff[0] / -2.0f; }
  if(rssiAvg) { *rssiAvg = (float)buff[1] / -2.0f; }
  if(rxLen) { *rxLen = buff[2]; }
  if(stat) { *stat = buff[3]; }

  return(state);
}

int16_t LR11x0::getRssiInst(float* rssi) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_RSSI_INST, false, buff, sizeof(buff));

  // pass the replies
  if(rssi) { *rssi = (float)buff[0] / -2.0f; }

  return(state);
}

int16_t LR11x0::setGfskSyncWord(uint8_t* sync) {
  if(!sync) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_GFSK_SYNC_WORD, true, sync, RADIOLIB_LR11X0_GFSK_SYNC_WORD_LEN));
}

int16_t LR11x0::setLoRaPublicNetwork(bool pub) {
  uint8_t buff[1] = { (uint8_t)pub };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_LORA_PUBLIC_NETWORK, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRx(uint32_t timeout) {
  uint8_t buff[3] = {
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RX, true, buff, sizeof(buff)));
}

int16_t LR11x0::setTx(uint32_t timeout) {
  uint8_t buff[3] = {
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_TX, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRfFrequency(uint32_t rfFreq) {
  uint8_t buff[4] = {
    (uint8_t)((rfFreq >> 24) & 0xFF), (uint8_t)((rfFreq >> 16) & 0xFF),
    (uint8_t)((rfFreq >> 8) & 0xFF), (uint8_t)(rfFreq & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RF_FREQUENCY, true, buff, sizeof(buff)));
}

int16_t LR11x0::autoTxRx(uint32_t delay, uint8_t intMode, uint32_t timeout) {
  uint8_t buff[7] = {
    (uint8_t)((delay >> 16) & 0xFF), (uint8_t)((delay >> 8) & 0xFF), (uint8_t)(delay & 0xFF), intMode,
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_AUTO_TX_RX, true, buff, sizeof(buff)));
}

int16_t LR11x0::setCadParams(uint8_t symNum, uint8_t detPeak, uint8_t detMin, uint8_t cadExitMode, uint32_t timeout) {
  uint8_t buff[7] = {
    symNum, detPeak, detMin, cadExitMode,
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_CAD_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setPacketType(uint8_t type) {
  uint8_t buff[1] = { type };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_PACKET_TYPE, true, buff, sizeof(buff)));
}

int16_t LR11x0::setModulationParamsLoRa(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro) {
  uint8_t buff[4] = { sf, bw, cr, ldro };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_MODULATION_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setModulationParamsGFSK(uint32_t br, uint8_t sh, uint8_t rxBw, uint32_t freqDev) {
  uint8_t buff[10] = { 
    (uint8_t)((br >> 24) & 0xFF), (uint8_t)((br >> 16) & 0xFF),
    (uint8_t)((br >> 8) & 0xFF), (uint8_t)(br & 0xFF), sh, rxBw,
    (uint8_t)((freqDev >> 24) & 0xFF), (uint8_t)((freqDev >> 16) & 0xFF),
    (uint8_t)((freqDev >> 8) & 0xFF), (uint8_t)(freqDev & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_MODULATION_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setModulationParamsLrFhss(uint32_t br, uint8_t sh) {
  uint8_t buff[5] = { 
    (uint8_t)((br >> 24) & 0xFF), (uint8_t)((br >> 16) & 0xFF),
    (uint8_t)((br >> 8) & 0xFF), (uint8_t)(br & 0xFF), sh
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_MODULATION_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setModulationParamsSigfox(uint32_t br, uint8_t sh) {
  // same as for LR-FHSS
  return(this->setModulationParamsLrFhss(br, sh));
}

int16_t LR11x0::setPacketParamsLoRa(uint16_t preambleLen, uint8_t hdrType, uint8_t payloadLen, uint8_t crcType, uint8_t invertIQ) {
  uint8_t buff[6] = { 
    (uint8_t)((preambleLen >> 8) & 0xFF), (uint8_t)(preambleLen & 0xFF),
    hdrType, payloadLen, crcType, invertIQ
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_PACKET_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setPacketParamsGFSK(uint16_t preambleLen, uint8_t preambleDetectorLen, uint8_t syncWordLen, uint8_t addrCmp, uint8_t packType, uint8_t payloadLen, uint8_t crcType, uint8_t whiten) {
  uint8_t buff[9] = { 
    (uint8_t)((preambleLen >> 8) & 0xFF), (uint8_t)(preambleLen & 0xFF),
    preambleDetectorLen, syncWordLen, addrCmp, packType, payloadLen, crcType, whiten
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_PACKET_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setPacketParamsSigfox(uint8_t payloadLen, uint16_t rampUpDelay, uint16_t rampDownDelay, uint16_t bitNum) {
  uint8_t buff[7] = { 
    payloadLen, (uint8_t)((rampUpDelay >> 8) & 0xFF), (uint8_t)(rampUpDelay & 0xFF),
    (uint8_t)((rampDownDelay >> 8) & 0xFF), (uint8_t)(rampDownDelay & 0xFF),
    (uint8_t)((bitNum >> 8) & 0xFF), (uint8_t)(bitNum & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_PACKET_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setTxParams(int8_t pwr, uint8_t ramp) {
  uint8_t buff[2] = { (uint8_t)pwr, ramp };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_TX_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setPacketAdrs(uint8_t node, uint8_t broadcast) {
  uint8_t buff[2] = { node, broadcast };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_PACKET_ADRS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRxTxFallbackMode(uint8_t mode) {
  uint8_t buff[1] = { mode };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RX_TX_FALLBACK_MODE, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRxDutyCycle(uint32_t rxPeriod, uint32_t sleepPeriod, uint8_t mode) {
  uint8_t buff[7] = {
    (uint8_t)((rxPeriod >> 16) & 0xFF), (uint8_t)((rxPeriod >> 8) & 0xFF), (uint8_t)(rxPeriod & 0xFF),
    (uint8_t)((sleepPeriod >> 16) & 0xFF), (uint8_t)((sleepPeriod >> 8) & 0xFF), (uint8_t)(sleepPeriod & 0xFF),
    mode
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RX_DUTY_CYCLE, true, buff, sizeof(buff)));
}

int16_t LR11x0::setPaConfig(uint8_t paSel, uint8_t regPaSupply, uint8_t paDutyCycle, uint8_t paHpSel) {
  uint8_t buff[4] = { paSel, regPaSupply, paDutyCycle, paHpSel };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_PA_CONFIG, true, buff, sizeof(buff)));
}

int16_t LR11x0::stopTimeoutOnPreamble(bool stop) {
  uint8_t buff[1] = { (uint8_t)stop };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_STOP_TIMEOUT_ON_PREAMBLE, true, buff, sizeof(buff)));
}

int16_t LR11x0::setCad(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_CAD, true, NULL, 0));
}

int16_t LR11x0::setTxCw(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_TX_CW, true, NULL, 0));
}

int16_t LR11x0::setTxInfinitePreamble(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_TX_INFINITE_PREAMBLE, true, NULL, 0));
}

int16_t LR11x0::setLoRaSynchTimeout(uint8_t symbolNum) {
  uint8_t buff[1] = { symbolNum };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_LORA_SYNCH_TIMEOUT, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRangingAddr(uint32_t addr, uint8_t checkLen) {
  uint8_t buff[5] = {
    (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF),
    (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF), checkLen
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RANGING_ADDR, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRangingReqAddr(uint32_t addr) {
  uint8_t buff[4] = {
    (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF),
    (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RANGING_REQ_ADDR, true, buff, sizeof(buff)));
}

int16_t LR11x0::getRangingResult(uint8_t type, float* res) {
  uint8_t reqBuff[1] = { type };
  uint8_t rplBuff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_RANGING_RESULT, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);

  if(res) { 
    if(type == RADIOLIB_LR11X0_RANGING_RESULT_DISTANCE) {
      uint32_t raw = ((uint32_t)(rplBuff[0]) << 24) | ((uint32_t)(rplBuff[1]) << 16) | ((uint32_t)(rplBuff[2]) << 8) | (uint32_t)rplBuff[3];
      *res = ((float)(raw*3e8))/((float)(4096*this->bandwidthKhz*1000));
    } else {
      *res = (float)rplBuff[3]/2.0f;
    }
  }

  return(state);
}

int16_t LR11x0::setRangingTxRxDelay(uint32_t delay) {
  uint8_t buff[4] = {
    (uint8_t)((delay >> 24) & 0xFF), (uint8_t)((delay >> 16) & 0xFF),
    (uint8_t)((delay >> 8) & 0xFF), (uint8_t)(delay & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RANGING_TX_RX_DELAY, true, buff, sizeof(buff)));
}

int16_t LR11x0::setGfskCrcParams(uint32_t init, uint32_t poly) {
  uint8_t buff[8] = {
    (uint8_t)((init >> 24) & 0xFF), (uint8_t)((init >> 16) & 0xFF),
    (uint8_t)((init >> 8) & 0xFF), (uint8_t)(init & 0xFF),
    (uint8_t)((poly >> 24) & 0xFF), (uint8_t)((poly >> 16) & 0xFF),
    (uint8_t)((poly >> 8) & 0xFF), (uint8_t)(poly & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_GFSK_CRC_PARAMS, true, buff, sizeof(buff)));
  
}

int16_t LR11x0::setGfskWhitParams(uint16_t seed) {
  uint8_t buff[2] = {
    (uint8_t)((seed >> 8) & 0xFF), (uint8_t)(seed & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_GFSK_WHIT_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRangingParameter(uint8_t symbolNum) {
  // the first byte is reserved
  uint8_t buff[2] = { 0x00, symbolNum };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RANGING_PARAMETER, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRssiCalibration(const int8_t* tune, int16_t gainOffset) {
  uint8_t buff[11] = {
    (uint8_t)((tune[0] & 0x0F) | (uint8_t)(tune[1] & 0x0F) << 4),
    (uint8_t)((tune[2] & 0x0F) | (uint8_t)(tune[3] & 0x0F) << 4),
    (uint8_t)((tune[4] & 0x0F) | (uint8_t)(tune[5] & 0x0F) << 4),
    (uint8_t)((tune[6] & 0x0F) | (uint8_t)(tune[7] & 0x0F) << 4),
    (uint8_t)((tune[8] & 0x0F) | (uint8_t)(tune[9] & 0x0F) << 4),
    (uint8_t)((tune[10] & 0x0F) | (uint8_t)(tune[11] & 0x0F) << 4),
    (uint8_t)((tune[12] & 0x0F) | (uint8_t)(tune[13] & 0x0F) << 4),
    (uint8_t)((tune[14] & 0x0F) | (uint8_t)(tune[15] & 0x0F) << 4),
    (uint8_t)((tune[16] & 0x0F) | (uint8_t)(tune[17] & 0x0F) << 4),
    (uint8_t)(((uint16_t)gainOffset >> 8) & 0xFF), (uint8_t)(gainOffset & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RSSI_CALIBRATION, true, buff, sizeof(buff)));
}

int16_t LR11x0::setLoRaSyncWord(uint8_t sync) {
  uint8_t buff[1] = { sync };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_LORA_SYNC_WORD, true, buff, sizeof(buff)));
}

int16_t LR11x0::lrFhssBuildFrame(uint8_t hdrCount, uint8_t cr, uint8_t grid, bool hop, uint8_t bw, uint16_t hopSeq, int8_t devOffset, uint8_t* payload, size_t len) {
  // check maximum size
  const uint8_t maxLen[4][4] = {
    { 189, 178, 167, 155, },
    { 151, 142, 133, 123, },
    { 112, 105,  99,  92, },
    {  74,  69,  65,  60, },
  };
  if((cr > RADIOLIB_LR11X0_LR_FHSS_CR_1_3) || ((hdrCount - 1) > (int)sizeof(maxLen[0])) || (len > maxLen[cr][hdrCount - 1])) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // build buffers
  size_t buffLen = 9 + len;
  #if RADIOLIB_STATIC_ONLY
    uint8_t dataBuff[9 + 190];
  #else
    uint8_t* dataBuff = new uint8_t[buffLen];
  #endif

  // set properties of the packet
  dataBuff[0] = hdrCount;
  dataBuff[1] = cr;
  dataBuff[2] = RADIOLIB_LR11X0_LR_FHSS_MOD_TYPE_GMSK;
  dataBuff[3] = grid;
  dataBuff[4] = (uint8_t)hop;
  dataBuff[5] = bw;
  dataBuff[6] = (uint8_t)((hopSeq >> 8) & 0x01);
  dataBuff[7] = (uint8_t)(hopSeq & 0xFF);
  dataBuff[8] = devOffset;
  memcpy(&dataBuff[9], payload, len);

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_LR_FHSS_BUILD_FRAME, true, dataBuff, buffLen);
  #if !RADIOLIB_STATIC_ONLY
    delete[] dataBuff;
  #endif
  return(state);
}

int16_t LR11x0::lrFhssSetSyncWord(uint32_t sync) {
  uint8_t buff[4] = {
    (uint8_t)((sync >> 24) & 0xFF), (uint8_t)((sync >> 16) & 0xFF),
    (uint8_t)((sync >> 8) & 0xFF), (uint8_t)(sync & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_LR_FHSS_SET_SYNC_WORD, true, buff, sizeof(buff)));
}

int16_t LR11x0::configBleBeacon(uint8_t chan, uint8_t* payload, size_t len) {
  return(this->bleBeaconCommon(RADIOLIB_LR11X0_CMD_CONFIG_BLE_BEACON, chan, payload, len));
}

int16_t LR11x0::getLoRaRxHeaderInfos(uint8_t* info) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_LORA_RX_HEADER_INFOS, false, buff, sizeof(buff));

  // pass the replies
  if(info) { *info = buff[0]; }

  return(state);
}

int16_t LR11x0::bleBeaconSend(uint8_t chan, uint8_t* payload, size_t len) {
  return(this->bleBeaconCommon(RADIOLIB_LR11X0_CMD_BLE_BEACON_SEND, chan, payload, len));
}

int16_t LR11x0::bleBeaconCommon(uint16_t cmd, uint8_t chan, uint8_t* payload, size_t len) {
  // check maximum size
  // TODO what is the actual maximum?
  if(len > RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // build buffers
  #if RADIOLIB_STATIC_ONLY
    uint8_t dataBuff[sizeof(uint8_t) + RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* dataBuff = new uint8_t[sizeof(uint8_t) + len];
  #endif

  // set the channel
  dataBuff[0] = chan;
  memcpy(&dataBuff[1], payload, len);

  int16_t state = this->SPIcommand(cmd, true, dataBuff, sizeof(uint8_t) + len);
  #if !RADIOLIB_STATIC_ONLY
    delete[] dataBuff;
  #endif
  return(state);
}

int16_t LR11x0::wifiScan(uint8_t type, uint16_t mask, uint8_t acqMode, uint8_t nbMaxRes, uint8_t nbScanPerChan, uint16_t timeout, uint8_t abortOnTimeout) {
  uint8_t buff[9] = {
    type, (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    acqMode, nbMaxRes, nbScanPerChan,
    (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
    abortOnTimeout
  };

  // call the SPI write stream directly to skip waiting for BUSY - it will be set to high once the scan starts
  return(this->mod->SPIwriteStream(RADIOLIB_LR11X0_CMD_WIFI_SCAN, buff, sizeof(buff), false, false));
}

int16_t LR11x0::wifiScanTimeLimit(uint8_t type, uint16_t mask, uint8_t acqMode, uint8_t nbMaxRes, uint16_t timePerChan, uint16_t timeout) {
  uint8_t buff[9] = {
    type, (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    acqMode, nbMaxRes,
    (uint8_t)((timePerChan >> 8) & 0xFF), (uint8_t)(timePerChan & 0xFF),
    (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_SCAN_TIME_LIMIT, true, buff, sizeof(buff)));
}

int16_t LR11x0::wifiCountryCode(uint16_t mask, uint8_t nbMaxRes, uint8_t nbScanPerChan, uint16_t timeout, uint8_t abortOnTimeout) {
  uint8_t buff[7] = {
    (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    nbMaxRes, nbScanPerChan,
    (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
    abortOnTimeout
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_COUNTRY_CODE, true, buff, sizeof(buff)));
}

int16_t LR11x0::wifiCountryCodeTimeLimit(uint16_t mask, uint8_t nbMaxRes, uint16_t timePerChan, uint16_t timeout) {
  uint8_t buff[7] = {
    (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    nbMaxRes,
    (uint8_t)((timePerChan >> 8) & 0xFF), (uint8_t)(timePerChan & 0xFF),
    (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_COUNTRY_CODE_TIME_LIMIT, true, buff, sizeof(buff)));
}

int16_t LR11x0::wifiReadResults(uint8_t index, uint8_t nbResults, uint8_t format, uint8_t* results) {
  uint8_t buff[3] = { index, nbResults, format };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_READ_RESULTS, false, results, RADIOLIB_LR11X0_WIFI_RESULT_MAX_LEN, buff, sizeof(buff)));
}

int16_t LR11x0::wifiResetCumulTimings(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_RESET_CUMUL_TIMINGS, true, NULL, 0));
}

int16_t LR11x0::wifiReadCumulTimings(uint32_t* detection, uint32_t* capture, uint32_t* demodulation) {
  uint8_t buff[16] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_READ_CUMUL_TIMINGS, false, buff, sizeof(buff));

  // pass the replies
  if(detection) { *detection = ((uint32_t)(buff[4]) << 24) | ((uint32_t)(buff[5]) << 16) | ((uint32_t)(buff[6]) << 8) | (uint32_t)buff[7]; }
  if(capture) { *capture = ((uint32_t)(buff[8]) << 24) | ((uint32_t)(buff[9]) << 16) | ((uint32_t)(buff[10]) << 8) | (uint32_t)buff[11]; }
  if(demodulation) { *demodulation = ((uint32_t)(buff[12]) << 24) | ((uint32_t)(buff[13]) << 16) | ((uint32_t)(buff[14]) << 8) | (uint32_t)buff[15]; }

  return(state);
}

int16_t LR11x0::wifiGetNbCountryCodeResults(uint8_t* nbResults) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_GET_NB_COUNTRY_CODE_RESULTS, false, buff, sizeof(buff));

  // pass the replies
  if(nbResults) { *nbResults = buff[0]; }

  return(state);
}

int16_t LR11x0::wifiReadCountryCodeResults(uint8_t index, uint8_t nbResults, uint8_t* results) {
  uint8_t reqBuff[2] = { index, nbResults };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_READ_COUNTRY_CODE_RESULTS, false, results, nbResults, reqBuff, sizeof(reqBuff)));
}

int16_t LR11x0::wifiCfgTimestampAPphone(uint32_t timestamp) {
  uint8_t buff[4] = {
    (uint8_t)((timestamp >> 24) & 0xFF), (uint8_t)((timestamp >> 16) & 0xFF),
    (uint8_t)((timestamp >> 8) & 0xFF), (uint8_t)(timestamp & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_COUNTRY_CODE_TIME_LIMIT, true, buff, sizeof(buff)));
}

int16_t LR11x0::wifiReadVersion(uint8_t* major, uint8_t* minor) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_READ_VERSION, false, buff, sizeof(buff));

  // pass the replies
  if(major) { *major = buff[0]; }
  if(minor) { *minor = buff[1]; }

  return(state);
}

int16_t LR11x0::gnssSetConstellationToUse(uint8_t mask) {
  uint8_t buff[1] = { mask };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_SET_CONSTELLATION_TO_USE, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadConstellationToUse(uint8_t* mask) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_CONSTELLATION_TO_USE, false, buff, sizeof(buff));

  // pass the replies
  if(mask) { *mask = buff[0]; }

  return(state);
}

int16_t LR11x0::gnssSetAlmanacUpdate(uint8_t mask) {
  uint8_t buff[1] = { mask };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_SET_ALMANAC_UPDATE, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadAlmanacUpdate(uint8_t* mask) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_UPDATE, false, buff, sizeof(buff));

  // pass the replies
  if(mask) { *mask = buff[0]; }

  return(state);
}

int16_t LR11x0::gnssReadVersion(uint8_t* fw, uint8_t* almanac) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_VERSION, false, buff, sizeof(buff));

  // pass the replies
  if(fw) { *fw = buff[0]; }
  if(almanac) { *almanac = buff[1]; }

  return(state);
}

int16_t LR11x0::gnssReadSupportedConstellations(uint8_t* mask) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_SUPPORTED_CONSTELLATIONS, false, buff, sizeof(buff));

  // pass the replies
  if(mask) { *mask = buff[0]; }

  return(state);
}

int16_t LR11x0::gnssSetMode(uint8_t mode) {
  uint8_t buff[1] = { mode };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_SET_MODE, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssAutonomous(uint32_t gpsTime, uint8_t resMask, uint8_t nbSvMask) {
  uint8_t buff[7] = {
    (uint8_t)((gpsTime >> 24) & 0xFF), (uint8_t)((gpsTime >> 16) & 0xFF),
    (uint8_t)((gpsTime >> 8) & 0xFF), (uint8_t)(gpsTime & 0xFF),
    RADIOLIB_LR11X0_GNSS_AUTO_EFFORT_MODE, resMask, nbSvMask
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_AUTONOMOUS, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssAssisted(uint32_t gpsTime, uint8_t effort, uint8_t resMask, uint8_t nbSvMask) {
  uint8_t buff[7] = {
    (uint8_t)((gpsTime >> 24) & 0xFF), (uint8_t)((gpsTime >> 16) & 0xFF),
    (uint8_t)((gpsTime >> 8) & 0xFF), (uint8_t)(gpsTime & 0xFF),
    effort, resMask, nbSvMask
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_ASSISTED, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssSetAssistancePosition(float lat, float lon) {
  uint16_t latRaw = (lat*2048.0f)/90.0f + 0.5f;
  uint16_t lonRaw = (lon*2048.0f)/180.0f + 0.5f;
  uint8_t buff[4] = {
    (uint8_t)((latRaw >> 8) & 0xFF), (uint8_t)(latRaw & 0xFF),
    (uint8_t)((lonRaw >> 8) & 0xFF), (uint8_t)(lonRaw & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_SET_ASSISTANCE_POSITION, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadAssistancePosition(float* lat, float* lon) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_ASSISTANCE_POSITION, false, buff, sizeof(buff));

  // pass the replies
  if(lat) {
    uint16_t latRaw = ((uint16_t)(buff[0]) << 8) | (uint16_t)(buff[1]);
    *lat = ((float)latRaw*90.0f)/2048.0f;
  }
  if(lon) {
    uint16_t lonRaw = ((uint16_t)(buff[2]) << 8) | (uint16_t)(buff[3]);
    *lon = ((float)lonRaw*180.0f)/2048.0f;
  }

  return(state);
}

int16_t LR11x0::gnssPushSolverMsg(uint8_t* payload, size_t len) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_PUSH_SOLVER_MSG, true, payload, len));
}

int16_t LR11x0::gnssPushDmMsg(uint8_t* payload, size_t len) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_PUSH_DM_MSG, true, payload, len));
}

int16_t LR11x0::gnssGetContextStatus(uint8_t* fwVersion, uint32_t* almanacCrc, uint8_t* errCode, uint8_t* almUpdMask, uint8_t* freqSpace) {
  // send the command
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_CONTEXT_STATUS, true, NULL, 0);
  RADIOLIB_ASSERT(state);

  // read the result - this requires some magic bytes first, that's why LR11x0::SPIcommand cannot be used
  uint8_t cmd_buff[3] = { 0x00, 0x02, 0x18 };
  uint8_t buff[9] = { 0 };
  state = this->mod->SPItransferStream(cmd_buff, sizeof(cmd_buff), false, NULL, buff, sizeof(buff), true);

  // pass the replies
  if(fwVersion) { *fwVersion = buff[0]; }
  if(almanacCrc) { *almanacCrc = ((uint32_t)(buff[1]) << 24) | ((uint32_t)(buff[2]) << 16) | ((uint32_t)(buff[3]) << 8) | (uint32_t)buff[4]; }
  if(errCode) { *errCode = (buff[5] & 0xF0) >> 4; }
  if(almUpdMask) { *almUpdMask = (buff[5] & 0x0E) >> 1; }
  if(freqSpace) { *freqSpace = ((buff[5] & 0x01) << 1) | ((buff[6] & 0x80) >> 7); }

  return(state);
}

int16_t LR11x0::gnssGetNbSvDetected(uint8_t* nbSv) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_NB_SV_DETECTED, false, buff, sizeof(buff));

  // pass the replies
  if(nbSv) { *nbSv = buff[0]; }

  return(state);
}

int16_t LR11x0::gnssGetSvDetected(uint8_t* svId, uint8_t* snr, uint16_t* doppler, size_t nbSv) {
  // TODO this is arbitrary - is there an actual maximum?
  if(nbSv > RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t)) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // build buffers
  size_t buffLen = nbSv*sizeof(uint32_t);
  #if RADIOLIB_STATIC_ONLY
    uint8_t dataBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* dataBuff = new uint8_t[buffLen];
  #endif

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_SV_DETECTED, false, dataBuff, buffLen);
  if(state == RADIOLIB_ERR_NONE) {
    for(size_t i = 0; i < nbSv; i++) {
      if(svId) { svId[i] = dataBuff[4*i]; }
      if(snr) { snr[i] = dataBuff[4*i + 1]; }
      if(doppler) { doppler[i] = ((uint16_t)(dataBuff[4*i + 2]) << 8) | (uint16_t)dataBuff[4*i + 3]; }
    }
  }

  #if !RADIOLIB_STATIC_ONLY
    delete[] dataBuff;
  #endif
  return(state);
}

int16_t LR11x0::gnssGetConsumption(uint32_t* cpu, uint32_t* radio) {
  uint8_t buff[8] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_CONSUMPTION, false, buff, sizeof(buff));

  // pass the replies
  if(cpu) { *cpu = ((uint32_t)(buff[0]) << 24) | ((uint32_t)(buff[1]) << 16) | ((uint32_t)(buff[2]) << 8) | (uint32_t)buff[3]; }
  if(radio) { *radio = ((uint32_t)(buff[4]) << 24) | ((uint32_t)(buff[5]) << 16) | ((uint32_t)(buff[6]) << 8) | (uint32_t)buff[7]; }

  return(state);
}

int16_t LR11x0::gnssGetResultSize(uint16_t* size) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_RESULT_SIZE, false, buff, sizeof(buff));

  // pass the replies
  if(size) { *size = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1]; }
  
  return(state);
}

int16_t LR11x0::gnssReadResults(uint8_t* result, uint16_t size) {
  if(!result) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_RESULTS, false, result, size));
}

int16_t LR11x0::gnssAlmanacFullUpdateHeader(uint16_t date, uint32_t globalCrc) {
  uint8_t buff[RADIOLIB_LR11X0_GNSS_ALMANAC_BLOCK_SIZE] = {
    RADIOLIB_LR11X0_GNSS_ALMANAC_HEADER_ID,
    (uint8_t)((date >> 8) & 0xFF), (uint8_t)(date & 0xFF),
    (uint8_t)((globalCrc >> 24) & 0xFF), (uint8_t)((globalCrc >> 16) & 0xFF), 
    (uint8_t)((globalCrc >> 8) & 0xFF), (uint8_t)(globalCrc & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_FULL_UPDATE, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssAlmanacFullUpdateSV(uint8_t svn, uint8_t* svnAlmanac) {
  uint8_t buff[RADIOLIB_LR11X0_GNSS_ALMANAC_BLOCK_SIZE] = { svn };
  memcpy(&buff[1], svnAlmanac, RADIOLIB_LR11X0_GNSS_ALMANAC_BLOCK_SIZE - 1);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_FULL_UPDATE, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssGetSvVisible(uint32_t time, float lat, float lon, uint8_t constellation, uint8_t* nbSv) {
  uint16_t latRaw = (lat*2048.0f)/90.0f + 0.5f;
  uint16_t lonRaw = (lon*2048.0f)/180.0f + 0.5f;
  uint8_t reqBuff[9] = { 
    (uint8_t)((time >> 24) & 0xFF), (uint8_t)((time >> 16) & 0xFF),
    (uint8_t)((time >> 8) & 0xFF), (uint8_t)(time & 0xFF),
    (uint8_t)((latRaw >> 8) & 0xFF), (uint8_t)(latRaw & 0xFF),
    (uint8_t)((lonRaw >> 8) & 0xFF), (uint8_t)(lonRaw & 0xFF),
    constellation,
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_SV_VISIBLE, false, nbSv, 1, reqBuff, sizeof(reqBuff)));
}

// TODO check version > 02.01
int16_t LR11x0::gnssScan(uint8_t effort, uint8_t resMask, uint8_t nbSvMax) {
  uint8_t buff[3] = { effort, resMask, nbSvMax };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_SCAN, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadLastScanModeLaunched(uint8_t* lastScanMode) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_LAST_SCAN_MODE_LAUNCHED, false, buff, sizeof(buff));

  // pass the replies
  if(lastScanMode) { *lastScanMode = buff[0]; }
  
  return(state);
}

int16_t LR11x0::gnssFetchTime(uint8_t effort, uint8_t opt) {
  uint8_t buff[2] = { effort, opt };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_FETCH_TIME, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadTime(uint8_t* err, uint32_t* time, uint32_t* nbUs, uint32_t* timeAccuracy) {
  uint8_t buff[12] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_TIME, false, buff, sizeof(buff));

  // pass the replies
  if(err) { *err = buff[0]; }
  if(time) { *time = ((uint32_t)(buff[1]) << 24) | ((uint32_t)(buff[2]) << 16) | ((uint32_t)(buff[3]) << 8) | (uint32_t)buff[4]; }
  if(nbUs) { *nbUs = ((uint32_t)(buff[5]) << 16) | ((uint32_t)(buff[6]) << 8) | (uint32_t)buff[7]; }
  if(timeAccuracy) { *timeAccuracy = ((uint32_t)(buff[8]) << 24) | ((uint32_t)(buff[9]) << 16) | ((uint32_t)(buff[10]) << 8) | (uint32_t)buff[11]; }
  
  return(state);
}

int16_t LR11x0::gnssResetTime(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_RESET_TIME, true, NULL, 0));
}

int16_t LR11x0::gnssResetPosition(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_RESET_POSITION, true, NULL, 0));
}

int16_t LR11x0::gnssReadDemodStatus(int8_t* status, uint8_t* info) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_DEMOD_STATUS, false, buff, sizeof(buff));

  // pass the replies
  if(status) { *status = (int8_t)buff[0]; }
  if(info) { *info = buff[1]; }
  
  return(state);
}

int16_t LR11x0::gnssReadCumulTiming(uint32_t* timing, uint8_t* constDemod) {
  uint8_t rplBuff[125] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_READ_REG_MEM, false, rplBuff, 125);
  RADIOLIB_ASSERT(state);

  // convert endians
  if(timing) {
    for(size_t i = 0; i < 31; i++) {
      timing[i] = ((uint32_t)rplBuff[i*sizeof(uint32_t)] << 24) | ((uint32_t)rplBuff[1 + i*sizeof(uint32_t)] << 16) | ((uint32_t)rplBuff[2 + i*sizeof(uint32_t)] << 8) | (uint32_t)rplBuff[3 + i*sizeof(uint32_t)];
    }
  }

  if(constDemod) { *constDemod = rplBuff[124]; }
  
  return(state);
}

int16_t LR11x0::gnssSetTime(uint32_t time, uint16_t accuracy) {
  uint8_t buff[6] = {
    (uint8_t)((time >> 24) & 0xFF), (uint8_t)((time >> 16) & 0xFF),
    (uint8_t)((time >> 8) & 0xFF), (uint8_t)(time & 0xFF),
    (uint8_t)((accuracy >> 8) & 0xFF), (uint8_t)(accuracy & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_SET_TIME, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadDopplerSolverRes(uint8_t* error, uint8_t* nbSvUsed, float* lat, float* lon, uint16_t* accuracy, uint16_t* xtal, float* latFilt, float* lonFilt, uint16_t* accuracyFilt, uint16_t* xtalFilt) {
  uint8_t buff[18] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_DOPPLER_SOLVER_RES, false, buff, sizeof(buff));

  // pass the replies
  if(error) { *error = buff[0]; }
  if(nbSvUsed) { *nbSvUsed = buff[1]; }
  if(lat) {
    uint16_t latRaw = ((uint16_t)(buff[2]) << 8) | (uint16_t)buff[3];
    *lat = ((float)latRaw * 90.0f)/2048.0f;
  }
  if(lon) {
    uint16_t lonRaw = ((uint16_t)(buff[4]) << 8) | (uint16_t)buff[5];
    *lon = ((float)lonRaw * 180.0f)/2048.0f;
  }
  if(accuracy) { *accuracy = ((uint16_t)(buff[6]) << 8) | (uint16_t)buff[7]; }
  if(xtal) { *xtal = ((uint16_t)(buff[8]) << 8) | (uint16_t)buff[9]; }
  if(latFilt) {
    uint16_t latRaw = ((uint16_t)(buff[10]) << 8) | (uint16_t)buff[11];
    *latFilt = ((float)latRaw * 90.0f)/2048.0f;
  }
  if(lonFilt) {
    uint16_t lonRaw = ((uint16_t)(buff[12]) << 8) | (uint16_t)buff[13];
    *lonFilt = ((float)lonRaw * 180.0f)/2048.0f;
  }
  if(accuracyFilt) { *accuracyFilt = ((uint16_t)(buff[14]) << 8) | (uint16_t)buff[15]; }
  if(xtalFilt) { *xtalFilt = ((uint16_t)(buff[16]) << 8) | (uint16_t)buff[17]; }
  
  return(state);
}

int16_t LR11x0::gnssReadDelayResetAP(uint32_t* delay) {
  uint8_t buff[3] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_DELAY_RESET_AP, false, buff, sizeof(buff));

  if(delay) { *delay = ((uint32_t)(buff[0]) << 16) | ((uint32_t)(buff[1]) << 8) | (uint32_t)buff[2]; }
  
  return(state);
}

int16_t LR11x0::gnssAlmanacUpdateFromSat(uint8_t effort, uint8_t bitMask) {
  uint8_t buff[2] = { effort, bitMask };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_UPDATE_FROM_SAT, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadAlmanacStatus(uint8_t* status) {
  // TODO parse the reply into some structure
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_STATUS, false, status, 53));
}

int16_t LR11x0::gnssConfigAlmanacUpdatePeriod(uint8_t bitMask, uint8_t svType, uint16_t period) {
  uint8_t buff[4] = { bitMask, svType, (uint8_t)((period >> 8) & 0xFF), (uint8_t)(period & 0xFF) };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_CONFIG_ALMANAC_UPDATE_PERIOD, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadAlmanacUpdatePeriod(uint8_t bitMask, uint8_t svType, uint16_t* period) {
  uint8_t reqBuff[2] = { bitMask, svType };
  uint8_t rplBuff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_UPDATE_PERIOD, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);

  if(period) { *period = ((uint16_t)(rplBuff[0]) << 8) | (uint16_t)rplBuff[1]; }

  return(state);
}

int16_t LR11x0::gnssConfigDelayResetAP(uint32_t delay) {
  uint8_t buff[3] = { (uint8_t)((delay >> 16) & 0xFF), (uint8_t)((delay >> 8) & 0xFF), (uint8_t)(delay & 0xFF) };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_CONFIG_DELAY_RESET_AP, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssGetSvWarmStart(uint8_t bitMask, uint8_t* sv, uint8_t nbVisSat) {
  uint8_t reqBuff[1] = { bitMask };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_SV_WARM_START, false, sv, nbVisSat, reqBuff, sizeof(reqBuff)));
}

int16_t LR11x0::gnssReadWNRollover(uint8_t* status, uint8_t* rollover) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_WN_ROLLOVER, false, buff, sizeof(buff));

  if(status) { *status = buff[0]; }
  if(rollover) { *rollover = buff[1]; }
  
  return(state);
}

int16_t LR11x0::gnssReadWarmStartStatus(uint8_t bitMask, uint8_t* nbVisSat, uint32_t* timeElapsed) {
  uint8_t reqBuff[1] = { bitMask };
  uint8_t rplBuff[5] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_WARM_START_STATUS, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);

  if(nbVisSat) { *nbVisSat = rplBuff[0]; }
  if(timeElapsed) { *timeElapsed = ((uint32_t)(rplBuff[1]) << 24) | ((uint32_t)(rplBuff[2]) << 16) | ((uint32_t)(rplBuff[3]) << 8) | (uint32_t)rplBuff[4]; }

  return(state);
}

int16_t LR11x0::gnssWriteBitMaskSatActivated(uint8_t bitMask, uint32_t* bitMaskActivated0, uint32_t* bitMaskActivated1) {
  uint8_t reqBuff[1] = { bitMask };
  uint8_t rplBuff[8] = { 0 };
  size_t rplLen = (bitMask & 0x01) ? 8 : 4; // GPS only has the first bit mask
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_WARM_START_STATUS, false, rplBuff, rplLen, reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);

  if(bitMaskActivated0) { *bitMaskActivated0 = ((uint32_t)(rplBuff[0]) << 24) | ((uint32_t)(rplBuff[1]) << 16) | ((uint32_t)(rplBuff[2]) << 8) | (uint32_t)rplBuff[3]; }
  if(bitMaskActivated1) { *bitMaskActivated1 = ((uint32_t)(rplBuff[4]) << 24) | ((uint32_t)(rplBuff[5]) << 16) | ((uint32_t)(rplBuff[6]) << 8) | (uint32_t)rplBuff[7]; }

  return(state);
}

int16_t LR11x0::cryptoSetKey(uint8_t keyId, uint8_t* key) {
  if(!key) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }
  uint8_t buff[1 + RADIOLIB_AES128_KEY_SIZE] = { 0 };
  buff[0] = keyId;
  memcpy(&buff[1], key, RADIOLIB_AES128_KEY_SIZE);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_SET_KEY, false, buff, sizeof(buff)));
}

int16_t LR11x0::cryptoDeriveKey(uint8_t srcKeyId, uint8_t dstKeyId, uint8_t* key) {
  if(!key) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }
  uint8_t buff[2 + RADIOLIB_AES128_KEY_SIZE] = { 0 };
  buff[0] = srcKeyId;
  buff[1] = dstKeyId;
  memcpy(&buff[2], key, RADIOLIB_AES128_KEY_SIZE);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_DERIVE_KEY, false, buff, sizeof(buff)));
}

int16_t LR11x0::cryptoProcessJoinAccept(uint8_t decKeyId, uint8_t verKeyId, uint8_t lwVer, uint8_t* header, uint8_t* dataIn, size_t len, uint8_t* dataOut) {
  // calculate buffer sizes
  size_t headerLen = 1;
  if(lwVer) {
    headerLen += 11; // LoRaWAN 1.1 header is 11 bytes longer than 1.0
  }
  size_t reqLen = 3*sizeof(uint8_t) + headerLen + len;
  size_t rplLen = sizeof(uint8_t) + len;

  // build buffers
  #if RADIOLIB_STATIC_ONLY
    uint8_t reqBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
    uint8_t rplBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* reqBuff = new uint8_t[reqLen];
    uint8_t* rplBuff = new uint8_t[rplLen];
  #endif
  
  // set the request fields
  reqBuff[0] = decKeyId;
  reqBuff[1] = verKeyId;
  reqBuff[2] = lwVer;
  memcpy(&reqBuff[3], header, headerLen);
  memcpy(&reqBuff[3 + headerLen], dataIn, len);

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_PROCESS_JOIN_ACCEPT, false, rplBuff, rplLen, reqBuff, reqLen);
  #if !RADIOLIB_STATIC_ONLY
    delete[] reqBuff;
  #endif
  if(state != RADIOLIB_ERR_NONE) {
    #if !RADIOLIB_STATIC_ONLY
      delete[] rplBuff;
    #endif
    return(state);
  }

  // check the crypto engine state
  if(rplBuff[0] != RADIOLIB_LR11X0_CRYPTO_STATUS_SUCCESS) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Crypto Engine error: %02x", rplBuff[0]);
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  }

  // pass the data
  memcpy(dataOut, &rplBuff[1], len);
  return(state);
}

int16_t LR11x0::cryptoComputeAesCmac(uint8_t keyId, uint8_t* data, size_t len, uint32_t* mic) {
  size_t reqLen = sizeof(uint8_t) + len;
  #if RADIOLIB_STATIC_ONLY
    uint8_t reqBuff[sizeof(uint8_t) + RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* reqBuff = new uint8_t[reqLen];
  #endif
  uint8_t rplBuff[5] = { 0 };
  
  reqBuff[0] = keyId;
  memcpy(&reqBuff[1], data, len);

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_COMPUTE_AES_CMAC, false, rplBuff, sizeof(rplBuff), reqBuff, reqLen);
  #if !RADIOLIB_STATIC_ONLY
    delete[] reqBuff;
  #endif

  // check the crypto engine state
  if(rplBuff[0] != RADIOLIB_LR11X0_CRYPTO_STATUS_SUCCESS) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Crypto Engine error: %02x", rplBuff[0]);
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  }

  if(mic) { *mic = ((uint32_t)(rplBuff[1]) << 24) |  ((uint32_t)(rplBuff[2]) << 16) | ((uint32_t)(rplBuff[3]) << 8) | (uint32_t)rplBuff[4]; }
  return(state);
}

int16_t LR11x0::cryptoVerifyAesCmac(uint8_t keyId, uint32_t micExp, uint8_t* data, size_t len, bool* result) {
   size_t reqLen = sizeof(uint8_t) + sizeof(uint32_t) + len;
  #if RADIOLIB_STATIC_ONLY
    uint8_t reqBuff[sizeof(uint8_t) + sizeof(uint32_t) + RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* reqBuff = new uint8_t[reqLen];
  #endif
  uint8_t rplBuff[1] = { 0 };
  
  reqBuff[0] = keyId;
  reqBuff[1] = (uint8_t)((micExp >> 24) & 0xFF);
  reqBuff[2] = (uint8_t)((micExp >> 16) & 0xFF);
  reqBuff[3] = (uint8_t)((micExp >> 8) & 0xFF);
  reqBuff[4] = (uint8_t)(micExp & 0xFF);
  memcpy(&reqBuff[5], data, len);

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_VERIFY_AES_CMAC, false, rplBuff, sizeof(rplBuff), reqBuff, reqLen);
  #if !RADIOLIB_STATIC_ONLY
    delete[] reqBuff;
  #endif

  // check the crypto engine state
  if(rplBuff[0] != RADIOLIB_LR11X0_CRYPTO_STATUS_SUCCESS) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Crypto Engine error: %02x", rplBuff[0]);
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  }

  if(result) { *result = (rplBuff[0] == RADIOLIB_LR11X0_CRYPTO_STATUS_SUCCESS); }
  return(state);
}

int16_t LR11x0::cryptoAesEncrypt01(uint8_t keyId, uint8_t* dataIn, size_t len, uint8_t* dataOut) {
  return(this->cryptoCommon(RADIOLIB_LR11X0_CMD_CRYPTO_AES_ENCRYPT_01, keyId, dataIn, len, dataOut));
}

int16_t LR11x0::cryptoAesEncrypt(uint8_t keyId, uint8_t* dataIn, size_t len, uint8_t* dataOut) {
  return(this->cryptoCommon(RADIOLIB_LR11X0_CMD_CRYPTO_AES_ENCRYPT, keyId, dataIn, len, dataOut));
}

int16_t LR11x0::cryptoAesDecrypt(uint8_t keyId, uint8_t* dataIn, size_t len, uint8_t* dataOut) {
  return(this->cryptoCommon(RADIOLIB_LR11X0_CMD_CRYPTO_AES_DECRYPT, keyId, dataIn, len, dataOut));
}

int16_t LR11x0::cryptoStoreToFlash(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_STORE_TO_FLASH, true, NULL, 0));
}

int16_t LR11x0::cryptoRestoreFromFlash(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_RESTORE_FROM_FLASH, true, NULL, 0));
}

int16_t LR11x0::cryptoSetParam(uint8_t id, uint32_t value) {
  uint8_t buff[5] = {
    id,
    (uint8_t)((value >> 24) & 0xFF), (uint8_t)((value >> 16) & 0xFF),
    (uint8_t)((value >> 8) & 0xFF), (uint8_t)(value & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_SET_PARAM, true, buff, sizeof(buff)));
}

int16_t LR11x0::cryptoGetParam(uint8_t id, uint32_t* value) {
  uint8_t reqBuff[1] = { id };
  uint8_t rplBuff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_GET_PARAM, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);
  if(value) { *value = ((uint32_t)(rplBuff[0]) << 24) | ((uint32_t)(rplBuff[1]) << 16) | ((uint32_t)(rplBuff[2]) << 8) | (uint32_t)rplBuff[3]; }
  return(state);
}

int16_t LR11x0::cryptoCheckEncryptedFirmwareImage(uint32_t offset, uint32_t* data, size_t len, bool nonvolatile) {
  // check maximum size
  if(len > (RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->writeCommon(RADIOLIB_LR11X0_CMD_CRYPTO_CHECK_ENCRYPTED_FIRMWARE_IMAGE, offset, data, len, nonvolatile));
}

int16_t LR11x0::cryptoCheckEncryptedFirmwareImageResult(bool* result) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_CRYPTO_CHECK_ENCRYPTED_FIRMWARE_IMAGE_RESULT, false, buff, sizeof(buff));

  // pass the replies
  if(result) { *result = (bool)buff[0]; }
  
  return(state);
}

int16_t LR11x0::bootEraseFlash(void) {
  // erasing flash takes about 2.5 seconds, temporarily tset SPI timeout to 3 seconds
  RadioLibTime_t timeout = this->mod->spiConfig.timeout;
  this->mod->spiConfig.timeout = 3000;
  int16_t state = this->mod->SPIwriteStream(RADIOLIB_LR11X0_CMD_BOOT_ERASE_FLASH, NULL, 0, false, false);
  this->mod->spiConfig.timeout = timeout;
  return(state);
}

int16_t LR11x0::bootWriteFlashEncrypted(uint32_t offset, uint32_t* data, size_t len, bool nonvolatile) {
  // check maximum size
  if(len > (RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->writeCommon(RADIOLIB_LR11X0_CMD_BOOT_WRITE_FLASH_ENCRYPTED, offset, data, len, nonvolatile));
}

int16_t LR11x0::bootReboot(bool stay) {
  uint8_t buff[1] = { (uint8_t)stay };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_BOOT_REBOOT, true, buff, sizeof(buff)));
}

int16_t LR11x0::bootGetPin(uint8_t* pin) {
  if(!pin) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_BOOT_GET_PIN, false, pin, RADIOLIB_LR11X0_PIN_LEN));
}

int16_t LR11x0::bootGetChipEui(uint8_t* eui) {
  if(!eui) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_BOOT_GET_CHIP_EUI, false, eui, RADIOLIB_LR11X0_EUI_LEN));
}

int16_t LR11x0::bootGetJoinEui(uint8_t* eui) {
  if(!eui) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_BOOT_GET_JOIN_EUI, false, eui, RADIOLIB_LR11X0_EUI_LEN));
}

int16_t LR11x0::writeCommon(uint16_t cmd, uint32_t addrOffset, const uint32_t* data, size_t len, bool nonvolatile) {
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
      bin = RADIOLIB_NONVOLATILE_READ_DWORD(data + i);
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

int16_t LR11x0::cryptoCommon(uint16_t cmd, uint8_t keyId, uint8_t* dataIn, size_t len, uint8_t* dataOut) {
  // build buffers
  #if RADIOLIB_STATIC_ONLY
    uint8_t reqBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
    uint8_t rplBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* reqBuff = new uint8_t[sizeof(uint8_t) + len];
    uint8_t* rplBuff = new uint8_t[sizeof(uint8_t) + len];
  #endif
  
  // set the request fields
  reqBuff[0] = keyId;
  memcpy(&reqBuff[1], dataIn, len);

  int16_t state = this->SPIcommand(cmd, false, rplBuff, sizeof(uint8_t) + len, reqBuff, sizeof(uint8_t) + len);
  #if !RADIOLIB_STATIC_ONLY
    delete[] reqBuff;
  #endif
  if(state != RADIOLIB_ERR_NONE) {
    #if !RADIOLIB_STATIC_ONLY
      delete[] rplBuff;
    #endif
    return(state);
  }

  // check the crypto engine state
  if(rplBuff[0] != RADIOLIB_LR11X0_CRYPTO_STATUS_SUCCESS) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Crypto Engine error: %02x", rplBuff[0]);
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  }

  // pass the data
  memcpy(dataOut, &rplBuff[1], len);
  return(state);
}

#endif
