#include "SX126x.h"
#if !defined(RADIOLIB_EXCLUDE_SX126X)

SX126x::SX126x(Module* mod) : PhysicalLayer(RADIOLIB_SX126X_FREQUENCY_STEP_SIZE, RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
  _mod = mod;
}

Module* SX126x::getMod() {
  return(_mod);
}

int16_t SX126x::begin(uint8_t cr, uint8_t syncWord, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  // set module properties
  _mod->init();
  _mod->pinMode(_mod->getIrq(), INPUT);
  _mod->pinMode(_mod->getGpio(), INPUT);
  RADIOLIB_DEBUG_PRINTLN(F("M\tSX126x"));

  // BW in kHz and SF are required in order to calculate LDRO for setModulationParams
  // set the defaults, this will get overwritten later anyway
  _bwKhz = 500.0;
  _sf = 9;

  // initialize configuration variables (will be overwritten during public settings configuration)
  _bw = RADIOLIB_SX126X_LORA_BW_500_0;  // initialized to 500 kHz, since lower valeus will interfere with LLCC68
  _cr = RADIOLIB_SX126X_LORA_CR_4_7;
  _ldro = 0x00;
  _crcType = RADIOLIB_SX126X_LORA_CRC_ON;
  _preambleLength = preambleLength;
  _tcxoDelay = 0;
  _headerType = RADIOLIB_SX126X_LORA_HEADER_EXPLICIT;
  _implicitLen = 0xFF;

  // reset the module and verify startup
  int16_t state = reset();
  RADIOLIB_ASSERT(state);

  // set mode to standby
  state = standby();
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = config(RADIOLIB_SX126X_PACKET_TYPE_LORA);
  RADIOLIB_ASSERT(state);

  // set TCXO control, if requested
  if(tcxoVoltage > 0.0) {
    state = setTCXO(tcxoVoltage);
    RADIOLIB_ASSERT(state);
  }

  // configure publicly accessible settings
  state = setCodingRate(cr);
  RADIOLIB_ASSERT(state);

  state = setSyncWord(syncWord);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  state = setCurrentLimit(60.0);
  RADIOLIB_ASSERT(state);

  state = setDio2AsRfSwitch(true);
  RADIOLIB_ASSERT(state);

  if (useRegulatorLDO) {
      state = setRegulatorLDO();
  } else {
      state = setRegulatorDCDC();
  }

  return(state);
}

int16_t SX126x::beginFSK(float br, float freqDev, float rxBw, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  // set module properties
  _mod->init();
  _mod->pinMode(_mod->getIrq(), INPUT);
  _mod->pinMode(_mod->getGpio(), INPUT);
  RADIOLIB_DEBUG_PRINTLN(F("M\tSX126x"));

  // initialize configuration variables (will be overwritten during public settings configuration)
  _br = 21333;                                  // 48.0 kbps
  _freqDev = 52428;                             // 50.0 kHz
  _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_156_2;
  _rxBwKhz = 156.2;
  _pulseShape = RADIOLIB_SX126X_GFSK_FILTER_GAUSS_0_5;
  _crcTypeFSK = RADIOLIB_SX126X_GFSK_CRC_2_BYTE_INV;     // CCIT CRC configuration
  _preambleLengthFSK = preambleLength;
  _addrComp = RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF;

  // reset the module and verify startup
  int16_t state = reset();
  RADIOLIB_ASSERT(state);

  // set mode to standby
  state = standby();
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = config(RADIOLIB_SX126X_PACKET_TYPE_GFSK);
  RADIOLIB_ASSERT(state);

  // set TCXO control, if requested
  if(tcxoVoltage > 0.0) {
    state = setTCXO(tcxoVoltage);
    RADIOLIB_ASSERT(state);
  }

  // configure publicly accessible settings
  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  state = setRxBandwidth(rxBw);
  RADIOLIB_ASSERT(state);

  state = setCurrentLimit(60.0);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  if(useRegulatorLDO) {
    state = setRegulatorLDO();
  } else {
    state = setRegulatorDCDC();
  }
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  uint8_t sync[] = {0x12, 0xAD};
  state = setSyncWord(sync, 2);
  RADIOLIB_ASSERT(state);

  state = setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);

  state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  state = variablePacketLengthMode(RADIOLIB_SX126X_MAX_PACKET_LENGTH);
  RADIOLIB_ASSERT(state);

  state = setCRC(2);
  RADIOLIB_ASSERT(state);

  state = setDio2AsRfSwitch(false);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX126x::reset(bool verify) {
  // run the reset sequence
  _mod->pinMode(_mod->getRst(), OUTPUT);
  _mod->digitalWrite(_mod->getRst(), LOW);
  _mod->delay(1);
  _mod->digitalWrite(_mod->getRst(), HIGH);

  // return immediately when verification is disabled
  if(!verify) {
    return(RADIOLIB_ERR_NONE);
  }

  // set mode to standby - SX126x often refuses first few commands after reset
  uint32_t start = _mod->millis();
  while(true) {
    // try to set mode to standby
    int16_t state = standby();
    if(state == RADIOLIB_ERR_NONE) {
      // standby command successful
      return(RADIOLIB_ERR_NONE);
    }

    // standby command failed, check timeout and try again
    if(_mod->millis() - start >= 3000) {
      // timed out, possibly incorrect wiring
      return(state);
    }

    // wait a bit to not spam the module
    _mod->delay(10);
  }
}

int16_t SX126x::transmit(uint8_t* data, size_t len, uint8_t addr) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check packet length
  if(len > RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  uint32_t timeout = 0;

  // get currently active modem
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    // calculate timeout (150% of expected time-on-air)
    timeout = (getTimeOnAir(len) * 3) / 2;

  } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    // calculate timeout (500% of expected time-on-air)
    timeout = getTimeOnAir(len) * 5;

  } else {
    return(RADIOLIB_ERR_UNKNOWN);
  }

  RADIOLIB_DEBUG_PRINT(F("Timeout in "));
  RADIOLIB_DEBUG_PRINT(timeout);
  RADIOLIB_DEBUG_PRINTLN(F(" us"));

  // start transmission
  state = startTransmit(data, len, addr);
  RADIOLIB_ASSERT(state);

  // wait for packet transmission or timeout
  uint32_t start = _mod->micros();
  while(!_mod->digitalRead(_mod->getIrq())) {
    _mod->yield();
    if(_mod->micros() - start > timeout) {
      clearIrqStatus();
      standby();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }
  uint32_t elapsed = _mod->micros() - start;

  // update data rate
  _dataRate = (len*8.0)/((float)elapsed/1000000.0);

  // clear interrupt flags
  state = clearIrqStatus();
  RADIOLIB_ASSERT(state);

  // set mode to standby to disable transmitter
  state = standby();

  return(state);
}

int16_t SX126x::receive(uint8_t* data, size_t len) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  uint32_t timeout = 0;

  // get currently active modem
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    // calculate timeout (100 LoRa symbols, the default for SX127x series)
    float symbolLength = (float)(uint32_t(1) << _sf) / (float)_bwKhz;
    timeout = (uint32_t)(symbolLength * 100.0 * 1000.0);
  } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    // calculate timeout (500 % of expected time-one-air)
    size_t maxLen = len;
    if(len == 0) {
      maxLen = 0xFF;
    }
    float brBps = ((float)(RADIOLIB_SX126X_CRYSTAL_FREQ) * 1000000.0 * 32.0) / (float)_br;
    timeout = (uint32_t)(((maxLen * 8.0) / brBps) * 1000000.0 * 5.0);

  } else {
    return(RADIOLIB_ERR_UNKNOWN);
  }

  RADIOLIB_DEBUG_PRINT(F("Timeout in "));
  RADIOLIB_DEBUG_PRINT(timeout);
  RADIOLIB_DEBUG_PRINTLN(F(" us"));

  // start reception
  uint32_t timeoutValue = (uint32_t)((float)timeout / 15.625);
  state = startReceive(timeoutValue);
  RADIOLIB_ASSERT(state);

  // wait for packet reception or timeout
  uint32_t start = _mod->micros();
  while(!_mod->digitalRead(_mod->getIrq())) {
    _mod->yield();
    if(_mod->micros() - start > timeout) {
      fixImplicitTimeout();
      clearIrqStatus();
      standby();
      return(RADIOLIB_ERR_RX_TIMEOUT);
    }
  }

  // fix timeout in implicit LoRa mode
  if(((_headerType == RADIOLIB_SX126X_LORA_HEADER_IMPLICIT) && (getPacketType() == RADIOLIB_SX126X_PACKET_TYPE_LORA))) {
    state = fixImplicitTimeout();
    RADIOLIB_ASSERT(state);
  }

  // read the received data
  return(readData(data, len));
}

int16_t SX126x::transmitDirect(uint32_t frf) {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, HIGH);

  // user requested to start transmitting immediately (required for RTTY)
  int16_t state = RADIOLIB_ERR_NONE;
  if(frf != 0) {
    state = setRfFrequency(frf);
  }
  RADIOLIB_ASSERT(state);

  // start transmitting
  uint8_t data[] = {RADIOLIB_SX126X_CMD_NOP};
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_TX_CONTINUOUS_WAVE, data, 1));
}

int16_t SX126x::receiveDirect() {
  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // SX126x is unable to output received data directly
  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX126x::scanChannel() {
  // set mode to CAD
  int state = startChannelScan();
  RADIOLIB_ASSERT(state);

  // wait for channel activity detected or timeout
  while(!_mod->digitalRead(_mod->getIrq())) {
    _mod->yield();
  }

  // check CAD result
  return(getChannelScanResult());
}

int16_t SX126x::sleep(bool retainConfig) {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, LOW);

  uint8_t sleepMode = RADIOLIB_SX126X_SLEEP_START_WARM | RADIOLIB_SX126X_SLEEP_RTC_OFF;
  if(!retainConfig) {
    sleepMode = RADIOLIB_SX126X_SLEEP_START_COLD | RADIOLIB_SX126X_SLEEP_RTC_OFF;
  }
  int16_t state = SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_SLEEP, &sleepMode, 1, false);

  // wait for SX126x to safely enter sleep mode
  _mod->delay(1);

  return(state);
}

int16_t SX126x::standby() {
  return(SX126x::standby(RADIOLIB_SX126X_STANDBY_RC));
}

int16_t SX126x::standby(uint8_t mode) {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, LOW);

  uint8_t data[] = {mode};
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_STANDBY, data, 1));
}

void SX126x::setDio1Action(void (*func)(void)) {
  _mod->attachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getIrq()), func, RISING);
}

void SX126x::clearDio1Action() {
  _mod->detachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getIrq()));
}

int16_t SX126x::startTransmit(uint8_t* data, size_t len, uint8_t addr) {
  // suppress unused variable warning
  (void)addr;

  // check packet length
  if(len > RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // maximum packet length is decreased by 1 when address filtering is active
  if((_addrComp != RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF) && (len > RADIOLIB_SX126X_MAX_PACKET_LENGTH - 1)) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // set packet Length
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    state = setPacketParams(_preambleLength, _crcType, len, _headerType);
  } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType, len);
  } else {
    return(RADIOLIB_ERR_UNKNOWN);
  }
  RADIOLIB_ASSERT(state);

  // set DIO mapping
  state = setDioIrqParams(RADIOLIB_SX126X_IRQ_TX_DONE | RADIOLIB_SX126X_IRQ_TIMEOUT, RADIOLIB_SX126X_IRQ_TX_DONE);
  RADIOLIB_ASSERT(state);

  // set buffer pointers
  state = setBufferBaseAddress();
  RADIOLIB_ASSERT(state);

  // write packet to buffer
  state = writeBuffer(data, len);
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqStatus();
  RADIOLIB_ASSERT(state);

  // fix sensitivity
  state = fixSensitivity();
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, HIGH);

  // start transmission
  state = setTx(RADIOLIB_SX126X_TX_TIMEOUT_NONE);
  RADIOLIB_ASSERT(state);

  // wait for BUSY to go low (= PA ramp up done)
  while(_mod->digitalRead(_mod->getGpio())) {
    _mod->yield();
  }

  return(state);
}

int16_t SX126x::startReceive(uint32_t timeout) {
  int16_t state = startReceiveCommon(timeout);
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // set mode to receive
  state = setRx(timeout);

  return(state);
}

int16_t SX126x::startReceiveDutyCycle(uint32_t rxPeriod, uint32_t sleepPeriod) {
  // datasheet claims time to go to sleep is ~500us, same to wake up, compensate for that with 1 ms + TCXO delay
  uint32_t transitionTime = _tcxoDelay + 1000;
  sleepPeriod -= transitionTime;

  // divide by 15.625
  uint32_t rxPeriodRaw = (rxPeriod * 8) / 125;
  uint32_t sleepPeriodRaw = (sleepPeriod * 8) / 125;

  // check 24 bit limit and zero value (likely not intended)
  if((rxPeriodRaw & 0xFF000000) || (rxPeriodRaw == 0)) {
    return(RADIOLIB_ERR_INVALID_RX_PERIOD);
  }

  // this check of the high byte also catches underflow when we subtracted transitionTime
  if((sleepPeriodRaw & 0xFF000000) || (sleepPeriodRaw == 0)) {
    return(RADIOLIB_ERR_INVALID_SLEEP_PERIOD);
  }

  int16_t state = startReceiveCommon();
  RADIOLIB_ASSERT(state);

  uint8_t data[6] = {(uint8_t)((rxPeriodRaw >> 16) & 0xFF), (uint8_t)((rxPeriodRaw >> 8) & 0xFF), (uint8_t)(rxPeriodRaw & 0xFF),
                     (uint8_t)((sleepPeriodRaw >> 16) & 0xFF), (uint8_t)((sleepPeriodRaw >> 8) & 0xFF), (uint8_t)(sleepPeriodRaw & 0xFF)};
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_RX_DUTY_CYCLE, data, 6));
}

int16_t SX126x::startReceiveDutyCycleAuto(uint16_t senderPreambleLength, uint16_t minSymbols) {
  if(senderPreambleLength == 0) {
    senderPreambleLength = _preambleLength;
  }

  // worst case is that the sender starts transmitting when we're just less than minSymbols from going back to sleep.
  // in this case, we don't catch minSymbols before going to sleep,
  // so we must be awake for at least that long before the sender stops transmitting.
  uint16_t sleepSymbols = senderPreambleLength - 2 * minSymbols;

  // if we're not to sleep at all, just use the standard startReceive.
  if(2 * minSymbols > senderPreambleLength) {
    return(startReceive());
  }

  uint32_t symbolLength = ((uint32_t)(10 * 1000) << _sf) / (10 * _bwKhz);
  uint32_t sleepPeriod = symbolLength * sleepSymbols;
  RADIOLIB_DEBUG_PRINT(F("Auto sleep period: "));
  RADIOLIB_DEBUG_PRINTLN(sleepPeriod);

  // when the unit detects a preamble, it starts a timer that will timeout if it doesn't receive a header in time.
  // the duration is sleepPeriod + 2 * wakePeriod.
  // The sleepPeriod doesn't take into account shutdown and startup time for the unit (~1ms)
  // We need to ensure that the timeout is longer than senderPreambleLength.
  // So we must satisfy: wakePeriod > (preamblePeriod - (sleepPeriod - 1000)) / 2. (A)
  // we also need to ensure the unit is awake to see at least minSymbols. (B)
  uint32_t wakePeriod = max(
    (symbolLength * (senderPreambleLength + 1) - (sleepPeriod - 1000)) / 2, // (A)
    symbolLength * (minSymbols + 1)); //(B)
  RADIOLIB_DEBUG_PRINT(F("Auto wake period: "));
  RADIOLIB_DEBUG_PRINTLN(wakePeriod);

  // If our sleep period is shorter than our transition time, just use the standard startReceive
  if(sleepPeriod < _tcxoDelay + 1016) {
    return(startReceive());
  }

  return(startReceiveDutyCycle(wakePeriod, sleepPeriod));
}

int16_t SX126x::startReceiveCommon(uint32_t timeout) {
  // set DIO mapping
  uint16_t mask = RADIOLIB_SX126X_IRQ_RX_DONE;
  if(timeout != RADIOLIB_SX126X_RX_TIMEOUT_INF) {
    mask |= RADIOLIB_SX126X_IRQ_TIMEOUT;
  }
  int16_t state = setDioIrqParams(RADIOLIB_SX126X_IRQ_RX_DONE | RADIOLIB_SX126X_IRQ_TIMEOUT | RADIOLIB_SX126X_IRQ_CRC_ERR | RADIOLIB_SX126X_IRQ_HEADER_ERR, mask);
  RADIOLIB_ASSERT(state);

  // set buffer pointers
  state = setBufferBaseAddress();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqStatus();

  // restore original packet length
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    state = setPacketParams(_preambleLength, _crcType, _implicitLen, _headerType);
  } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);
  } else {
    return(RADIOLIB_ERR_UNKNOWN);
  }

  return(state);
}

int16_t SX126x::readData(uint8_t* data, size_t len) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check integrity CRC
  uint16_t irq = getIrqStatus();
  int16_t crcState = RADIOLIB_ERR_NONE;
  if((irq & RADIOLIB_SX126X_IRQ_CRC_ERR) || (irq & RADIOLIB_SX126X_IRQ_HEADER_ERR)) {
    crcState = RADIOLIB_ERR_CRC_MISMATCH;
  }

  // get packet length
  size_t length = getPacketLength();
  if((len != 0) && (len < length)) {
    // user requested less data than we got, only return what was requested
    length = len;
  }

  // read packet data
  state = readBuffer(data, length);
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqStatus();

  // check if CRC failed - this is done after reading data to give user the option to keep them
  RADIOLIB_ASSERT(crcState);

  return(state);
}

int16_t SX126x::startChannelScan() {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // set DIO pin mapping
  state = setDioIrqParams(RADIOLIB_SX126X_IRQ_CAD_DETECTED | RADIOLIB_SX126X_IRQ_CAD_DONE, RADIOLIB_SX126X_IRQ_CAD_DETECTED | RADIOLIB_SX126X_IRQ_CAD_DONE);
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqStatus();
  RADIOLIB_ASSERT(state);

  // set mode to CAD
  state = setCad();
  return(state);
}

int16_t SX126x::getChannelScanResult() {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check CAD result
  uint16_t cadResult = getIrqStatus();
  if(cadResult & RADIOLIB_SX126X_IRQ_CAD_DETECTED) {
    // detected some LoRa activity
    clearIrqStatus();
    return(RADIOLIB_LORA_DETECTED);
  } else if(cadResult & RADIOLIB_SX126X_IRQ_CAD_DONE) {
    // channel is free
    clearIrqStatus();
    return(RADIOLIB_CHANNEL_FREE);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX126x::setBandwidth(float bw) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // ensure byte conversion doesn't overflow
  RADIOLIB_CHECK_RANGE(bw, 0.0, 510.0, RADIOLIB_ERR_INVALID_BANDWIDTH);

  // check allowed bandwidth values
  uint8_t bw_div2 = bw / 2 + 0.01;
  switch (bw_div2)  {
    case 3: // 7.8:
      _bw = RADIOLIB_SX126X_LORA_BW_7_8;
      break;
    case 5: // 10.4:
      _bw = RADIOLIB_SX126X_LORA_BW_10_4;
      break;
    case 7: // 15.6:
      _bw = RADIOLIB_SX126X_LORA_BW_15_6;
      break;
    case 10: // 20.8:
      _bw = RADIOLIB_SX126X_LORA_BW_20_8;
      break;
    case 15: // 31.25:
      _bw = RADIOLIB_SX126X_LORA_BW_31_25;
      break;
    case 20: // 41.7:
      _bw = RADIOLIB_SX126X_LORA_BW_41_7;
      break;
    case 31: // 62.5:
      _bw = RADIOLIB_SX126X_LORA_BW_62_5;
      break;
    case 62: // 125.0:
      _bw = RADIOLIB_SX126X_LORA_BW_125_0;
      break;
    case 125: // 250.0
      _bw = RADIOLIB_SX126X_LORA_BW_250_0;
      break;
    case 250: // 500.0
      _bw = RADIOLIB_SX126X_LORA_BW_500_0;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_BANDWIDTH);
  }

  // update modulation parameters
  _bwKhz = bw;
  return(setModulationParams(_sf, _bw, _cr, _ldro));
}

int16_t SX126x::setSpreadingFactor(uint8_t sf) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(sf, 5, 12, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);

  // update modulation parameters
  _sf = sf;
  return(setModulationParams(_sf, _bw, _cr, _ldro));
}

int16_t SX126x::setCodingRate(uint8_t cr) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(cr, 5, 8, RADIOLIB_ERR_INVALID_CODING_RATE);

  // update modulation parameters
  _cr = cr - 4;
  return(setModulationParams(_sf, _bw, _cr, _ldro));
}

int16_t SX126x::setSyncWord(uint8_t syncWord, uint8_t controlBits) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update register
  uint8_t data[2] = {(uint8_t)((syncWord & 0xF0) | ((controlBits & 0xF0) >> 4)), (uint8_t)(((syncWord & 0x0F) << 4) | (controlBits & 0x0F))};
  return(writeRegister(RADIOLIB_SX126X_REG_LORA_SYNC_WORD_MSB, data, 2));
}

int16_t SX126x::setCurrentLimit(float currentLimit) {
  // check allowed range
  if(!((currentLimit >= 0) && (currentLimit <= 140))) {
    return(RADIOLIB_ERR_INVALID_CURRENT_LIMIT);
  }

  // calculate raw value
  uint8_t rawLimit = (uint8_t)(currentLimit / 2.5);

  // update register
  return(writeRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &rawLimit, 1));
}

float SX126x::getCurrentLimit() {
  // get the raw value
  uint8_t ocp = 0;
  readRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1);

  // return the actual value
  return((float)ocp * 2.5);
}

int16_t SX126x::setPreambleLength(uint16_t preambleLength) {
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    _preambleLength = preambleLength;
    return(setPacketParams(_preambleLength, _crcType, _implicitLen, _headerType));
  } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    _preambleLengthFSK = preambleLength;
    return(setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType));
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX126x::setFrequencyDeviation(float freqDev) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set frequency deviation to lowest available setting (required for digimodes)
  float newFreqDev = freqDev;
  if(freqDev < 0.0) {
    newFreqDev = 0.6;
  }

  RADIOLIB_CHECK_RANGE(newFreqDev, 0.6, 200.0, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);

  // calculate raw frequency deviation value
  uint32_t freqDevRaw = (uint32_t)(((newFreqDev * 1000.0) * (float)((uint32_t)(1) << 25)) / (RADIOLIB_SX126X_CRYSTAL_FREQ * 1000000.0));

  // check modulation parameters
  /*if(2 * freqDevRaw + _br > _rxBwKhz * 1000.0) {
    return(RADIOLIB_ERR_INVALID_MODULATION_PARAMETERS);
  }*/
  _freqDev = freqDevRaw;

  // update modulation parameters
  return(setModulationParamsFSK(_br, _pulseShape, _rxBw, _freqDev));
}

int16_t SX126x::setBitRate(float br) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(br, 0.6, 300.0, RADIOLIB_ERR_INVALID_BIT_RATE);

  // calculate raw bit rate value
  uint32_t brRaw = (uint32_t)((RADIOLIB_SX126X_CRYSTAL_FREQ * 1000000.0 * 32.0) / (br * 1000.0));

  // check modulation parameters
  /*if(2 * _freqDev + brRaw > _rxBwKhz * 1000.0) {
    return(RADIOLIB_ERR_INVALID_MODULATION_PARAMETERS);
  }*/
  _br = brRaw;

  // update modulation parameters
  return(setModulationParamsFSK(_br, _pulseShape, _rxBw, _freqDev));
}

int16_t SX126x::setRxBandwidth(float rxBw) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check modulation parameters
  /*if(2 * _freqDev + _br > rxBw * 1000.0) {
    return(RADIOLIB_ERR_INVALID_MODULATION_PARAMETERS);
  }*/
  _rxBwKhz = rxBw;

  // check allowed receiver bandwidth values
  if(fabs(rxBw - 4.8) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_4_8;
  } else if(fabs(rxBw - 5.8) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_5_8;
  } else if(fabs(rxBw - 7.3) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_7_3;
  } else if(fabs(rxBw - 9.7) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_9_7;
  } else if(fabs(rxBw - 11.7) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_11_7;
  } else if(fabs(rxBw - 14.6) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_14_6;
  } else if(fabs(rxBw - 19.5) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_19_5;
  } else if(fabs(rxBw - 23.4) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_23_4;
  } else if(fabs(rxBw - 29.3) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_29_3;
  } else if(fabs(rxBw - 39.0) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_39_0;
  } else if(fabs(rxBw - 46.9) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_46_9;
  } else if(fabs(rxBw - 58.6) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_58_6;
  } else if(fabs(rxBw - 78.2) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_78_2;
  } else if(fabs(rxBw - 93.8) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_93_8;
  } else if(fabs(rxBw - 117.3) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_117_3;
  } else if(fabs(rxBw - 156.2) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_156_2;
  } else if(fabs(rxBw - 187.2) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_187_2;
  } else if(fabs(rxBw - 234.3) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_234_3;
  } else if(fabs(rxBw - 312.0) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_312_0;
  } else if(fabs(rxBw - 373.6) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_373_6;
  } else if(fabs(rxBw - 467.0) <= 0.001) {
    _rxBw = RADIOLIB_SX126X_GFSK_RX_BW_467_0;
  } else {
    return(RADIOLIB_ERR_INVALID_RX_BANDWIDTH);
  }

  // update modulation parameters
  return(setModulationParamsFSK(_br, _pulseShape, _rxBw, _freqDev));
}

int16_t SX126x::setDataShaping(uint8_t sh) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set data shaping
  switch(sh) {
    case RADIOLIB_SHAPING_NONE:
      _pulseShape = RADIOLIB_SX126X_GFSK_FILTER_NONE;
      break;
    case RADIOLIB_SHAPING_0_3:
      _pulseShape = RADIOLIB_SX126X_GFSK_FILTER_GAUSS_0_3;
      break;
    case RADIOLIB_SHAPING_0_5:
      _pulseShape = RADIOLIB_SX126X_GFSK_FILTER_GAUSS_0_5;
      break;
    case RADIOLIB_SHAPING_0_7:
      _pulseShape = RADIOLIB_SX126X_GFSK_FILTER_GAUSS_0_7;
      break;
    case RADIOLIB_SHAPING_1_0:
      _pulseShape = RADIOLIB_SX126X_GFSK_FILTER_GAUSS_1;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_DATA_SHAPING);
  }

  // update modulation parameters
  return(setModulationParamsFSK(_br, _pulseShape, _rxBw, _freqDev));
}

int16_t SX126x::setSyncWord(uint8_t* syncWord, uint8_t len) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check sync word Length
  if(len > 8) {
    return(RADIOLIB_ERR_INVALID_SYNC_WORD);
  }

  // write sync word
  int16_t state = writeRegister(RADIOLIB_SX126X_REG_SYNC_WORD_0, syncWord, len);
  RADIOLIB_ASSERT(state);

  // update packet parameters
  _syncWordLength = len * 8;
  state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);

  return(state);
}

int16_t SX126x::setSyncBits(uint8_t *syncWord, uint8_t bitsLen) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check sync word Length
  if(bitsLen > 0x40) {
    return(RADIOLIB_ERR_INVALID_SYNC_WORD);
  }

  uint8_t bytesLen = bitsLen / 8;
  if ((bitsLen % 8) != 0) {
    bytesLen++;
  }

  // write sync word
  int16_t state = writeRegister(RADIOLIB_SX126X_REG_SYNC_WORD_0, syncWord, bytesLen);
  RADIOLIB_ASSERT(state);

  // update packet parameters
  _syncWordLength = bitsLen;
  state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);

  return(state);
}

int16_t SX126x::setNodeAddress(uint8_t nodeAddr) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // enable address filtering (node only)
  _addrComp = RADIOLIB_SX126X_GFSK_ADDRESS_FILT_NODE;
  int16_t state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);
  RADIOLIB_ASSERT(state);

  // set node address
  state = writeRegister(RADIOLIB_SX126X_REG_NODE_ADDRESS, &nodeAddr, 1);

  return(state);
}

int16_t SX126x::setBroadcastAddress(uint8_t broadAddr) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // enable address filtering (node and broadcast)
  _addrComp = RADIOLIB_SX126X_GFSK_ADDRESS_FILT_NODE_BROADCAST;
  int16_t state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);
  RADIOLIB_ASSERT(state);

  // set broadcast address
  state = writeRegister(RADIOLIB_SX126X_REG_BROADCAST_ADDRESS, &broadAddr, 1);

  return(state);
}

int16_t SX126x::disableAddressFiltering() {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // disable address filtering
  _addrComp = RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF;
  return(setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening));
}

int16_t SX126x::setCRC(uint8_t len, uint16_t initial, uint16_t polynomial, bool inverted) {
  // check active modem
  uint8_t modem = getPacketType();

  if(modem == RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    // update packet parameters
    switch(len) {
      case 0:
        _crcTypeFSK = RADIOLIB_SX126X_GFSK_CRC_OFF;
        break;
      case 1:
        if(inverted) {
          _crcTypeFSK = RADIOLIB_SX126X_GFSK_CRC_1_BYTE_INV;
        } else {
          _crcTypeFSK = RADIOLIB_SX126X_GFSK_CRC_1_BYTE;
        }
        break;
      case 2:
        if(inverted) {
          _crcTypeFSK = RADIOLIB_SX126X_GFSK_CRC_2_BYTE_INV;
        } else {
          _crcTypeFSK = RADIOLIB_SX126X_GFSK_CRC_2_BYTE;
        }
        break;
      default:
        return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
    }

    int16_t state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);
    RADIOLIB_ASSERT(state);

    // write initial CRC value
    uint8_t data[2] = {(uint8_t)((initial >> 8) & 0xFF), (uint8_t)(initial & 0xFF)};
    state = writeRegister(RADIOLIB_SX126X_REG_CRC_INITIAL_MSB, data, 2);
    RADIOLIB_ASSERT(state);

    // write CRC polynomial value
    data[0] = (uint8_t)((polynomial >> 8) & 0xFF);
    data[1] = (uint8_t)(polynomial & 0xFF);
    state = writeRegister(RADIOLIB_SX126X_REG_CRC_POLYNOMIAL_MSB, data, 2);

    return(state);

  } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    // LoRa CRC doesn't allow to set CRC polynomial, initial value, or inversion

    // update packet parameters
    if(len) {
      _crcType = RADIOLIB_SX126X_LORA_CRC_ON;
    } else {
      _crcType = RADIOLIB_SX126X_LORA_CRC_OFF;
    }

    return(setPacketParams(_preambleLength, _crcType, _implicitLen, _headerType));
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX126x::setWhitening(bool enabled, uint16_t initial) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  int16_t state = RADIOLIB_ERR_NONE;
  if(!enabled) {
    // disable whitening
    _whitening = RADIOLIB_SX126X_GFSK_WHITENING_OFF;

    state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);
    RADIOLIB_ASSERT(state);

  } else {
    // enable whitening
    _whitening = RADIOLIB_SX126X_GFSK_WHITENING_ON;

    // write initial whitening value
    // as per note on pg. 65 of datasheet v1.2: "The user should not change the value of the 7 MSB's of this register"
    uint8_t data[2];
    // first read the actual value and mask 7 MSB which we can not change
    // if different value is written in 7 MSB, the Rx won't even work (tested on HW)
    state = readRegister(RADIOLIB_SX126X_REG_WHITENING_INITIAL_MSB, data, 1);
    RADIOLIB_ASSERT(state);

    data[0] = (data[0] & 0xFE) | (uint8_t)((initial >> 8) & 0x01);
    data[1] = (uint8_t)(initial & 0xFF);
    state = writeRegister(RADIOLIB_SX126X_REG_WHITENING_INITIAL_MSB, data, 2);
    RADIOLIB_ASSERT(state);

    state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);
    RADIOLIB_ASSERT(state);
  }
  return(state);
}

float SX126x::getDataRate() const {
  return(_dataRate);
}

float SX126x::getRSSI() {
  // get last packet RSSI from packet status
  uint32_t packetStatus = getPacketStatus();
  uint8_t rssiPkt = packetStatus & 0xFF;
  return(-1.0 * rssiPkt/2.0);
}

float SX126x::getSNR() {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // get last packet SNR from packet status
  uint32_t packetStatus = getPacketStatus();
  uint8_t snrPkt = (packetStatus >> 8) & 0xFF;
  if(snrPkt < 128) {
    return(snrPkt/4.0);
  } else {
    return((snrPkt - 256)/4.0);
  }
}

size_t SX126x::getPacketLength(bool update) {
  (void)update;
  uint8_t rxBufStatus[2] = {0, 0};
  SPIreadCommand(RADIOLIB_SX126X_CMD_GET_RX_BUFFER_STATUS, rxBufStatus, 2);
  return((size_t)rxBufStatus[0]);
}

int16_t SX126x::fixedPacketLengthMode(uint8_t len) {
  return(setPacketMode(RADIOLIB_SX126X_GFSK_PACKET_FIXED, len));
}

int16_t SX126x::variablePacketLengthMode(uint8_t maxLen) {
  return(setPacketMode(RADIOLIB_SX126X_GFSK_PACKET_VARIABLE, maxLen));
}

uint32_t SX126x::getTimeOnAir(size_t len) {
  // everything is in microseconds to allow integer arithmetic
  // some constants have .25, these are multiplied by 4, and have _x4 postfix to indicate that fact
  if(getPacketType() == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    uint32_t symbolLength_us = ((uint32_t)(1000 * 10) << _sf) / (_bwKhz * 10) ;
    uint8_t sfCoeff1_x4 = 17; // (4.25 * 4)
    uint8_t sfCoeff2 = 8;
    if(_sf == 5 || _sf == 6) {
      sfCoeff1_x4 = 25; // 6.25 * 4
      sfCoeff2 = 0;
    }
    uint8_t sfDivisor = 4*_sf;
    if(symbolLength_us >= 16000) {
      sfDivisor = 4*(_sf - 2);
    }
    const int8_t bitsPerCrc = 16;
    const int8_t N_symbol_header = _headerType == RADIOLIB_SX126X_LORA_HEADER_EXPLICIT ? 20 : 0;

    // numerator of equation in section 6.1.4 of SX1268 datasheet v1.1 (might not actually be bitcount, but it has len * 8)
    int16_t bitCount = (int16_t) 8 * len + _crcType * bitsPerCrc - 4 * _sf  + sfCoeff2 + N_symbol_header;
    if(bitCount < 0) {
      bitCount = 0;
    }
    // add (sfDivisor) - 1 to the numerator to give integer CEIL(...)
    uint16_t nPreCodedSymbols = (bitCount + (sfDivisor - 1)) / (sfDivisor);

    // preamble can be 65k, therefore nSymbol_x4 needs to be 32 bit
    uint32_t nSymbol_x4 = (_preambleLength + 8) * 4 + sfCoeff1_x4 + nPreCodedSymbols * (_cr + 4) * 4;

    return((symbolLength_us * nSymbol_x4) / 4);
  } else {
    return((len * 8 * _br) / (RADIOLIB_SX126X_CRYSTAL_FREQ * 32));
  }
}

float SX126x::getRSSIInst() {
  uint8_t data[3] = {0, 0, 0};  // RssiInst, Status, RFU
  SPIreadCommand(RADIOLIB_SX126X_CMD_GET_RSSI_INST, data, 3);

  return (float)data[0] / (-2.0);
}

int16_t SX126x::implicitHeader(size_t len) {
  return(setHeaderType(RADIOLIB_SX126X_LORA_HEADER_IMPLICIT, len));
}

int16_t SX126x::explicitHeader() {
  return(setHeaderType(RADIOLIB_SX126X_LORA_HEADER_EXPLICIT));
}

int16_t SX126x::setRegulatorLDO() {
  return(setRegulatorMode(RADIOLIB_SX126X_REGULATOR_LDO));
}

int16_t SX126x::setRegulatorDCDC() {
  return(setRegulatorMode(RADIOLIB_SX126X_REGULATOR_DC_DC));
}

int16_t SX126x::setEncoding(uint8_t encoding) {
  return(setWhitening(encoding));
}

void SX126x::setRfSwitchPins(RADIOLIB_PIN_TYPE rxEn, RADIOLIB_PIN_TYPE txEn) {
  _mod->setRfSwitchPins(rxEn, txEn);
}

int16_t SX126x::forceLDRO(bool enable) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update modulation parameters
  _ldroAuto = false;
  _ldro = (uint8_t)enable;
  return(setModulationParams(_sf, _bw, _cr, _ldro));
}

int16_t SX126x::autoLDRO() {
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  _ldroAuto = true;
  return(RADIOLIB_ERR_NONE);
}

uint8_t SX126x::randomByte() {
  // set mode to Rx
  setRx(RADIOLIB_SX126X_RX_TIMEOUT_INF);

  // wait a bit for the RSSI reading to stabilise
  _mod->delay(10);

  // read RSSI value 8 times, always keep just the least significant bit
  uint8_t randByte = 0x00;
  for(uint8_t i = 0; i < 8; i++) {
    uint8_t val = 0x00;
    readRegister(RADIOLIB_SX126X_REG_RANDOM_NUMBER_0, &val, sizeof(uint8_t));
    randByte |= ((val & 0x01) << i);
  }

  // set mode to standby
  standby();

  return(randByte);
}

void SX126x::setDirectAction(void (*func)(void)) {
  // SX126x is unable to perform direct mode reception
  // this method is implemented only for PhysicalLayer compatibility
  (void)func;
}

void SX126x::readBit(RADIOLIB_PIN_TYPE pin) {
  // SX126x is unable to perform direct mode reception
  // this method is implemented only for PhysicalLayer compatibility
  (void)pin;
}

int16_t SX126x::setTCXO(float voltage, uint32_t delay) {
  // set mode to standby
  standby();

  // check RADIOLIB_SX126X_XOSC_START_ERR flag and clear it
  if(getDeviceErrors() & RADIOLIB_SX126X_XOSC_START_ERR) {
    clearDeviceErrors();
  }

  // check 0 V disable
  if(fabs(voltage - 0.0) <= 0.001) {
    return(reset(true));
  }

  // check alowed voltage values
  uint8_t data[4];
  if(fabs(voltage - 1.6) <= 0.001) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_1_6;
  } else if(fabs(voltage - 1.7) <= 0.001) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_1_7;
  } else if(fabs(voltage - 1.8) <= 0.001) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_1_8;
  } else if(fabs(voltage - 2.2) <= 0.001) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_2_2;
  } else if(fabs(voltage - 2.4) <= 0.001) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_2_4;
  } else if(fabs(voltage - 2.7) <= 0.001) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_2_7;
  } else if(fabs(voltage - 3.0) <= 0.001) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_3_0;
  } else if(fabs(voltage - 3.3) <= 0.001) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_3_3;
  } else {
    return(RADIOLIB_ERR_INVALID_TCXO_VOLTAGE);
  }

  // calculate delay
  uint32_t delayValue = (float)delay / 15.625;
  data[1] = (uint8_t)((delayValue >> 16) & 0xFF);
  data[2] = (uint8_t)((delayValue >> 8) & 0xFF);
  data[3] = (uint8_t)(delayValue & 0xFF);

  _tcxoDelay = delay;

  // enable TCXO control on DIO3
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_DIO3_AS_TCXO_CTRL, data, 4));
}

int16_t SX126x::setDio2AsRfSwitch(bool enable) {
  uint8_t data = 0;
  if(enable) {
    data = RADIOLIB_SX126X_DIO2_AS_RF_SWITCH;
  } else {
    data = RADIOLIB_SX126X_DIO2_AS_IRQ;
  }
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL, &data, 1));
}

int16_t SX126x::setTx(uint32_t timeout) {
  uint8_t data[] = { (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF)} ;
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_TX, data, 3));
}

int16_t SX126x::setRx(uint32_t timeout) {
  uint8_t data[] = { (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF) };
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_RX, data, 3));
}

int16_t SX126x::setCad() {
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_CAD, NULL, 0));
}

int16_t SX126x::setPaConfig(uint8_t paDutyCycle, uint8_t deviceSel, uint8_t hpMax, uint8_t paLut) {
  uint8_t data[] = { paDutyCycle, hpMax, deviceSel, paLut };
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_PA_CONFIG, data, 4));
}

int16_t SX126x::writeRegister(uint16_t addr, uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[] = { RADIOLIB_SX126X_CMD_WRITE_REGISTER, (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF) };
  return(SPIwriteCommand(cmd, 3, data, numBytes));
}

int16_t SX126x::readRegister(uint16_t addr, uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[] = { RADIOLIB_SX126X_CMD_READ_REGISTER, (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF) };
  return(SX126x::SPItransfer(cmd, 3, false, NULL, data, numBytes, true));
}

int16_t SX126x::writeBuffer(uint8_t* data, uint8_t numBytes, uint8_t offset) {
  uint8_t cmd[] = { RADIOLIB_SX126X_CMD_WRITE_BUFFER, offset };
  return(SPIwriteCommand(cmd, 2, data, numBytes));
}

int16_t SX126x::readBuffer(uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[] = { RADIOLIB_SX126X_CMD_READ_BUFFER, RADIOLIB_SX126X_CMD_NOP };
  return(SPIreadCommand(cmd, 2, data, numBytes));
}

int16_t SX126x::setDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask) {
  uint8_t data[8] = {(uint8_t)((irqMask >> 8) & 0xFF), (uint8_t)(irqMask & 0xFF),
                     (uint8_t)((dio1Mask >> 8) & 0xFF), (uint8_t)(dio1Mask & 0xFF),
                     (uint8_t)((dio2Mask >> 8) & 0xFF), (uint8_t)(dio2Mask & 0xFF),
                     (uint8_t)((dio3Mask >> 8) & 0xFF), (uint8_t)(dio3Mask & 0xFF)};
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_DIO_IRQ_PARAMS, data, 8));
}

uint16_t SX126x::getIrqStatus() {
  uint8_t data[] = { 0x00, 0x00 };
  SPIreadCommand(RADIOLIB_SX126X_CMD_GET_IRQ_STATUS, data, 2);
  return(((uint16_t)(data[0]) << 8) | data[1]);
}

int16_t SX126x::clearIrqStatus(uint16_t clearIrqParams) {
  uint8_t data[] = { (uint8_t)((clearIrqParams >> 8) & 0xFF), (uint8_t)(clearIrqParams & 0xFF) };
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_CLEAR_IRQ_STATUS, data, 2));
}

int16_t SX126x::setRfFrequency(uint32_t frf) {
  uint8_t data[] = { (uint8_t)((frf >> 24) & 0xFF), (uint8_t)((frf >> 16) & 0xFF), (uint8_t)((frf >> 8) & 0xFF), (uint8_t)(frf & 0xFF) };
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_RF_FREQUENCY, data, 4));
}

int16_t SX126x::calibrateImage(uint8_t* data) {
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_CALIBRATE_IMAGE, data, 2));
}

uint8_t SX126x::getPacketType() {
  uint8_t data = 0xFF;
  SPIreadCommand(RADIOLIB_SX126X_CMD_GET_PACKET_TYPE, &data, 1);
  return(data);
}

int16_t SX126x::setTxParams(uint8_t power, uint8_t rampTime) {
  uint8_t data[] = { power, rampTime };
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_TX_PARAMS, data, 2));
}

int16_t SX126x::setPacketMode(uint8_t mode, uint8_t len) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set requested packet mode
  int16_t state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, mode, len);
  RADIOLIB_ASSERT(state);

  // update cached value
  _packetType = mode;
  return(state);
}

int16_t SX126x::setHeaderType(uint8_t headerType, size_t len) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set requested packet mode
  int16_t state = setPacketParams(_preambleLength, _crcType, len, headerType);
  RADIOLIB_ASSERT(state);

  // update cached value
  _headerType = headerType;
  _implicitLen = len;

  return(state);
}

int16_t SX126x::setModulationParams(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro) {
  // calculate symbol length and enable low data rate optimization, if auto-configuration is enabled
  if(_ldroAuto) {
    float symbolLength = (float)(uint32_t(1) << _sf) / (float)_bwKhz;
    RADIOLIB_DEBUG_PRINT("Symbol length: ");
    RADIOLIB_DEBUG_PRINT(symbolLength);
    RADIOLIB_DEBUG_PRINTLN(" ms");
    if(symbolLength >= 16.0) {
      _ldro = RADIOLIB_SX126X_LORA_LOW_DATA_RATE_OPTIMIZE_ON;
    } else {
      _ldro = RADIOLIB_SX126X_LORA_LOW_DATA_RATE_OPTIMIZE_OFF;
    }
  } else {
    _ldro = ldro;
  }
  // 500/9/8  - 0x09 0x04 0x03 0x00 - SF9, BW125, 4/8
  // 500/11/8 - 0x0B 0x04 0x03 0x00 - SF11 BW125, 4/7
  uint8_t data[4] = {sf, bw, cr, _ldro};
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_MODULATION_PARAMS, data, 4));
}

int16_t SX126x::setModulationParamsFSK(uint32_t br, uint8_t pulseShape, uint8_t rxBw, uint32_t freqDev) {
  uint8_t data[8] = {(uint8_t)((br >> 16) & 0xFF), (uint8_t)((br >> 8) & 0xFF), (uint8_t)(br & 0xFF),
                     pulseShape, rxBw,
                     (uint8_t)((freqDev >> 16) & 0xFF), (uint8_t)((freqDev >> 8) & 0xFF), (uint8_t)(freqDev & 0xFF)};
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_MODULATION_PARAMS, data, 8));
}

int16_t SX126x::setPacketParams(uint16_t preambleLength, uint8_t crcType, uint8_t payloadLength, uint8_t headerType, uint8_t invertIQ) {
  int16_t state = fixInvertedIQ(invertIQ);
  RADIOLIB_ASSERT(state);
  uint8_t data[6] = {(uint8_t)((preambleLength >> 8) & 0xFF), (uint8_t)(preambleLength & 0xFF), headerType, payloadLength, crcType, invertIQ};
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_PACKET_PARAMS, data, 6));
}

int16_t SX126x::setPacketParamsFSK(uint16_t preambleLength, uint8_t crcType, uint8_t syncWordLength, uint8_t addrComp, uint8_t whitening, uint8_t packetType, uint8_t payloadLength, uint8_t preambleDetectorLength) {
  uint8_t data[9] = {(uint8_t)((preambleLength >> 8) & 0xFF), (uint8_t)(preambleLength & 0xFF),
                     preambleDetectorLength, syncWordLength, addrComp,
                     packetType, payloadLength, crcType, whitening};
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_PACKET_PARAMS, data, 9));
}

int16_t SX126x::setBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress) {
  uint8_t data[2] = {txBaseAddress, rxBaseAddress};
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_BUFFER_BASE_ADDRESS, data, 2));
}

int16_t SX126x::setRegulatorMode(uint8_t mode) {
  uint8_t data[1] = {mode};
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_REGULATOR_MODE, data, 1));
}

uint8_t SX126x::getStatus() {
  uint8_t data = 0;
  SPIreadCommand(RADIOLIB_SX126X_CMD_GET_STATUS, &data, 1);
  return(data);
}

uint32_t SX126x::getPacketStatus() {
  uint8_t data[3] = {0, 0, 0};
  SPIreadCommand(RADIOLIB_SX126X_CMD_GET_PACKET_STATUS, data, 3);
  return((((uint32_t)data[0]) << 16) | (((uint32_t)data[1]) << 8) | (uint32_t)data[2]);
}

uint16_t SX126x::getDeviceErrors() {
  uint8_t data[2] = {0, 0};
  SPIreadCommand(RADIOLIB_SX126X_CMD_GET_DEVICE_ERRORS, data, 2);
  uint16_t opError = (((uint16_t)data[0] & 0xFF) << 8) & ((uint16_t)data[1]);
  return(opError);
}

int16_t SX126x::clearDeviceErrors() {
  uint8_t data[2] = {RADIOLIB_SX126X_CMD_NOP, RADIOLIB_SX126X_CMD_NOP};
  return(SPIwriteCommand(RADIOLIB_SX126X_CMD_CLEAR_DEVICE_ERRORS, data, 2));
}

int16_t SX126x::setFrequencyRaw(float freq) {
  // calculate raw value
  uint32_t frf = (freq * (uint32_t(1) << RADIOLIB_SX126X_DIV_EXPONENT)) / RADIOLIB_SX126X_CRYSTAL_FREQ;
  return(setRfFrequency(frf));
}

int16_t SX126x::fixSensitivity() {
  // fix receiver sensitivity for 500 kHz LoRa
  // see SX1262/SX1268 datasheet, chapter 15 Known Limitations, section 15.1 for details

  // read current sensitivity configuration
  uint8_t sensitivityConfig = 0;
  int16_t state = readRegister(RADIOLIB_SX126X_REG_SENSITIVITY_CONFIG, &sensitivityConfig, 1);
  RADIOLIB_ASSERT(state);

  // fix the value for LoRa with 500 kHz bandwidth
  if((getPacketType() == RADIOLIB_SX126X_PACKET_TYPE_LORA) && (fabs(_bwKhz - 500.0) <= 0.001)) {
    sensitivityConfig &= 0xFB;
  } else {
    sensitivityConfig |= 0x04;
  }
  return(writeRegister(RADIOLIB_SX126X_REG_SENSITIVITY_CONFIG, &sensitivityConfig, 1));
}

int16_t SX126x::fixPaClamping() {
  // fixes overly eager PA clamping
  // see SX1262/SX1268 datasheet, chapter 15 Known Limitations, section 15.2 for details

  // read current clamping configuration
  uint8_t clampConfig = 0;
  int16_t state = readRegister(RADIOLIB_SX126X_REG_TX_CLAMP_CONFIG, &clampConfig, 1);
  RADIOLIB_ASSERT(state);

  // update with the new value
  clampConfig |= 0x1E;
  return(writeRegister(RADIOLIB_SX126X_REG_TX_CLAMP_CONFIG, &clampConfig, 1));
}

int16_t SX126x::fixImplicitTimeout() {
  // fixes timeout in implicit header mode
  // see SX1262/SX1268 datasheet, chapter 15 Known Limitations, section 15.3 for details

  //check if we're in implicit LoRa mode
  if(!((_headerType == RADIOLIB_SX126X_LORA_HEADER_IMPLICIT) && (getPacketType() == RADIOLIB_SX126X_PACKET_TYPE_LORA))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // stop RTC counter
  uint8_t rtcStop = 0x00;
  int16_t state = writeRegister(RADIOLIB_SX126X_REG_RTC_STOP, &rtcStop, 1);
  RADIOLIB_ASSERT(state);

  // read currently active event
  uint8_t rtcEvent = 0;
  state = readRegister(RADIOLIB_SX126X_REG_RTC_EVENT, &rtcEvent, 1);
  RADIOLIB_ASSERT(state);

  // clear events
  rtcEvent |= 0x02;
  return(writeRegister(RADIOLIB_SX126X_REG_RTC_EVENT, &rtcEvent, 1));
}

int16_t SX126x::fixInvertedIQ(uint8_t iqConfig) {
  // fixes IQ configuration for inverted IQ
  // see SX1262/SX1268 datasheet, chapter 15 Known Limitations, section 15.4 for details

  // read current IQ configuration
  uint8_t iqConfigCurrent = 0;
  int16_t state = readRegister(RADIOLIB_SX126X_REG_IQ_CONFIG, &iqConfigCurrent, 1);
  RADIOLIB_ASSERT(state);

  // set correct IQ configuration
  if(iqConfig == RADIOLIB_SX126X_LORA_IQ_INVERTED) {
    iqConfigCurrent &= 0xFB;
  } else {
    iqConfigCurrent |= 0x04;
  }

  // update with the new value
  return(writeRegister(RADIOLIB_SX126X_REG_IQ_CONFIG, &iqConfigCurrent, 1));
}

int16_t SX126x::config(uint8_t modem) {
  // reset buffer base address
  int16_t state = setBufferBaseAddress();
  RADIOLIB_ASSERT(state);

  // set modem
  uint8_t data[7];
  data[0] = modem;
  state = SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_PACKET_TYPE, data, 1);
  RADIOLIB_ASSERT(state);

  // set Rx/Tx fallback mode to STDBY_RC
  data[0] = RADIOLIB_SX126X_RX_TX_FALLBACK_MODE_STDBY_RC;
  state = SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_RX_TX_FALLBACK_MODE, data, 1);
  RADIOLIB_ASSERT(state);

  // set CAD parameters
  data[0] = RADIOLIB_SX126X_CAD_ON_8_SYMB;
  data[1] = _sf + 13;
  data[2] = 10;
  data[3] = RADIOLIB_SX126X_CAD_GOTO_STDBY;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = 0x00;
  state = SPIwriteCommand(RADIOLIB_SX126X_CMD_SET_CAD_PARAMS, data, 7);
  RADIOLIB_ASSERT(state);

  // clear IRQ
  state = clearIrqStatus();
  state |= setDioIrqParams(RADIOLIB_SX126X_IRQ_NONE, RADIOLIB_SX126X_IRQ_NONE);
  RADIOLIB_ASSERT(state);

  // calibrate all blocks
  data[0] = RADIOLIB_SX126X_CALIBRATE_ALL;
  state = SPIwriteCommand(RADIOLIB_SX126X_CMD_CALIBRATE, data, 1);
  RADIOLIB_ASSERT(state);

  // wait for calibration completion
  _mod->delay(5);
  while(_mod->digitalRead(_mod->getGpio())) {
    _mod->yield();
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t SX126x::SPIwriteCommand(uint8_t* cmd, uint8_t cmdLen, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  return(SX126x::SPItransfer(cmd, cmdLen, true, data, NULL, numBytes, waitForBusy));
}

int16_t SX126x::SPIwriteCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  return(SX126x::SPItransfer(&cmd, 1, true, data, NULL, numBytes, waitForBusy));
}

int16_t SX126x::SPIreadCommand(uint8_t* cmd, uint8_t cmdLen, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  return(SX126x::SPItransfer(cmd, cmdLen, false, NULL, data, numBytes, waitForBusy));
}

int16_t SX126x::SPIreadCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  return(SX126x::SPItransfer(&cmd, 1, false, NULL, data, numBytes, waitForBusy));
}

int16_t SX126x::SPItransfer(uint8_t* cmd, uint8_t cmdLen, bool write, uint8_t* dataOut, uint8_t* dataIn, uint8_t numBytes, bool waitForBusy, uint32_t timeout) {
  #if defined(RADIOLIB_VERBOSE)
    uint8_t debugBuff[256];
  #endif

  // pull NSS low
  _mod->digitalWrite(_mod->getCs(), LOW);

  // ensure BUSY is low (state machine ready)
  uint32_t start = _mod->millis();
  while(_mod->digitalRead(_mod->getGpio())) {
    _mod->yield();
    if(_mod->millis() - start >= timeout) {
      _mod->digitalWrite(_mod->getCs(), HIGH);
      return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
    }
  }

  // start transfer
  _mod->SPIbeginTransaction();

  // send command byte(s)
  for(uint8_t n = 0; n < cmdLen; n++) {
    _mod->SPItransfer(cmd[n]);
  }

  // variable to save error during SPI transfer
  uint8_t status = 0;

  // send/receive all bytes
  if(write) {
    for(uint8_t n = 0; n < numBytes; n++) {
      // send byte
      uint8_t in = _mod->SPItransfer(dataOut[n]);
      #if defined(RADIOLIB_VERBOSE)
        debugBuff[n] = in;
      #endif

      // check status
      if(((in & 0b00001110) == RADIOLIB_SX126X_STATUS_CMD_TIMEOUT) ||
         ((in & 0b00001110) == RADIOLIB_SX126X_STATUS_CMD_INVALID) ||
         ((in & 0b00001110) == RADIOLIB_SX126X_STATUS_CMD_FAILED)) {
        status = in & 0b00001110;
        break;
      } else if(in == 0x00 || in == 0xFF) {
        status = RADIOLIB_SX126X_STATUS_SPI_FAILED;
        break;
      }
    }

  } else {
    // skip the first byte for read-type commands (status-only)
    uint8_t in = _mod->SPItransfer(RADIOLIB_SX126X_CMD_NOP);
    #if defined(RADIOLIB_VERBOSE)
      debugBuff[0] = in;
    #endif

    // check status
    if(((in & 0b00001110) == RADIOLIB_SX126X_STATUS_CMD_TIMEOUT) ||
       ((in & 0b00001110) == RADIOLIB_SX126X_STATUS_CMD_INVALID) ||
       ((in & 0b00001110) == RADIOLIB_SX126X_STATUS_CMD_FAILED)) {
      status = in & 0b00001110;
    } else if(in == 0x00 || in == 0xFF) {
      status = RADIOLIB_SX126X_STATUS_SPI_FAILED;
    } else {
      for(uint8_t n = 0; n < numBytes; n++) {
        dataIn[n] = _mod->SPItransfer(RADIOLIB_SX126X_CMD_NOP);
      }
    }
  }

  // stop transfer
  _mod->SPIendTransaction();
  _mod->digitalWrite(_mod->getCs(), HIGH);

  // wait for BUSY to go high and then low
  if(waitForBusy) {
    _mod->delayMicroseconds(1);
    start = _mod->millis();
    while(_mod->digitalRead(_mod->getGpio())) {
      _mod->yield();
      if(_mod->millis() - start >= timeout) {
        status = RADIOLIB_SX126X_STATUS_CMD_TIMEOUT;
        break;
      }
    }
  }

  // print debug output
  #if defined(RADIOLIB_VERBOSE)
    // print command byte(s)
    RADIOLIB_VERBOSE_PRINT("CMD\t");
    for(uint8_t n = 0; n < cmdLen; n++) {
      RADIOLIB_VERBOSE_PRINT(cmd[n], HEX);
      RADIOLIB_VERBOSE_PRINT('\t');
    }
    RADIOLIB_VERBOSE_PRINTLN();

    // print data bytes
    RADIOLIB_VERBOSE_PRINT("DAT");
    if(write) {
      RADIOLIB_VERBOSE_PRINT("W\t");
      for(uint8_t n = 0; n < numBytes; n++) {
        RADIOLIB_VERBOSE_PRINT(dataOut[n], HEX);
        RADIOLIB_VERBOSE_PRINT('\t');
        RADIOLIB_VERBOSE_PRINT(debugBuff[n], HEX);
        RADIOLIB_VERBOSE_PRINT('\t');
      }
      RADIOLIB_VERBOSE_PRINTLN();
    } else {
      RADIOLIB_VERBOSE_PRINT("R\t");
      // skip the first byte for read-type commands (status-only)
      RADIOLIB_VERBOSE_PRINT(RADIOLIB_SX126X_CMD_NOP, HEX);
      RADIOLIB_VERBOSE_PRINT('\t');
      RADIOLIB_VERBOSE_PRINT(debugBuff[0], HEX);
      RADIOLIB_VERBOSE_PRINT('\t')

      for(uint8_t n = 0; n < numBytes; n++) {
        RADIOLIB_VERBOSE_PRINT(RADIOLIB_SX126X_CMD_NOP, HEX);
        RADIOLIB_VERBOSE_PRINT('\t');
        RADIOLIB_VERBOSE_PRINT(dataIn[n], HEX);
        RADIOLIB_VERBOSE_PRINT('\t');
      }
      RADIOLIB_VERBOSE_PRINTLN();
    }
    RADIOLIB_VERBOSE_PRINTLN();
  #else
    // some faster platforms require a short delay here
    // not sure why, but it seems that long enough SPI transaction
    // (e.g. setPacketParams for GFSK) will fail without it
    #if defined(RADIOLIB_SPI_SLOWDOWN)
      _mod->delay(1);
    #endif
  #endif

  // parse status
  switch(status) {
    case RADIOLIB_SX126X_STATUS_CMD_TIMEOUT:
      return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
    case RADIOLIB_SX126X_STATUS_CMD_INVALID:
      return(RADIOLIB_ERR_SPI_CMD_INVALID);
    case RADIOLIB_SX126X_STATUS_CMD_FAILED:
      return(RADIOLIB_ERR_SPI_CMD_FAILED);
    case RADIOLIB_SX126X_STATUS_SPI_FAILED:
      return(RADIOLIB_ERR_CHIP_NOT_FOUND);
    default:
      return(RADIOLIB_ERR_NONE);
  }
}

#endif
