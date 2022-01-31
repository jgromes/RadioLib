#include "Si443x.h"
#if !defined(RADIOLIB_EXCLUDE_SI443X)

Si443x::Si443x(Module* mod) : PhysicalLayer(RADIOLIB_SI443X_FREQUENCY_STEP_SIZE, RADIOLIB_SI443X_MAX_PACKET_LENGTH) {
  _mod = mod;
}

Module* Si443x::getMod() {
  return(_mod);
}

int16_t Si443x::begin(float br, float freqDev, float rxBw, uint8_t preambleLen) {
  // set module properties
  _mod->init();
  _mod->pinMode(_mod->getIrq(), INPUT);
  _mod->pinMode(_mod->getRst(), OUTPUT);
  _mod->digitalWrite(_mod->getRst(), LOW);

  // try to find the Si443x chip
  if(!Si443x::findChip()) {
    RADIOLIB_DEBUG_PRINTLN(F("No Si443x found!"));
    _mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  } else {
    RADIOLIB_DEBUG_PRINTLN(F("M\tSi443x"));
  }

  // reset the device
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_1, RADIOLIB_SI443X_SOFTWARE_RESET);

  // clear POR interrupt
  clearIRQFlags();

  // configure settings not accessible by API
  int16_t state = config();
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  state = setRxBandwidth(rxBw);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLen);
  RADIOLIB_ASSERT(state);

  uint8_t syncWord[] = {0x12, 0xAD};
  state = setSyncWord(syncWord, sizeof(syncWord));
  RADIOLIB_ASSERT(state);

  state = packetMode();
  RADIOLIB_ASSERT(state);

  state = setDataShaping(0);
  RADIOLIB_ASSERT(state);

  state = setEncoding(0);
  RADIOLIB_ASSERT(state);

  state = variablePacketLengthMode();

  return(state);
}

void Si443x::reset() {
  _mod->pinMode(_mod->getRst(), OUTPUT);
  _mod->digitalWrite(_mod->getRst(), HIGH);
  _mod->delay(1);
  _mod->digitalWrite(_mod->getRst(), LOW);
  _mod->delay(100);
}

int16_t Si443x::transmit(uint8_t* data, size_t len, uint8_t addr) {
  // calculate timeout (5ms + 500 % of expected time-on-air)
  uint32_t timeout = 5000000 + (uint32_t)((((float)(len * 8)) / (_br * 1000.0)) * 5000000.0);

  // start transmission
  int16_t state = startTransmit(data, len, addr);
  RADIOLIB_ASSERT(state);

  // wait for transmission end or timeout
  uint32_t start = _mod->micros();
  while(_mod->digitalRead(_mod->getIrq())) {
    _mod->yield();
    if(_mod->micros() - start > timeout) {
      standby();
      clearIRQFlags();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }

  // clear interrupt flags
  clearIRQFlags();

  // set mode to standby
  standby();

  return(state);
}

int16_t Si443x::receive(uint8_t* data, size_t len) {
  // calculate timeout (500 ms + 400 full 64-byte packets at current bit rate)
  uint32_t timeout = 500000 + (1.0/(_br*1000.0))*(RADIOLIB_SI443X_MAX_PACKET_LENGTH*400.0);

  // start reception
  int16_t state = startReceive();
  RADIOLIB_ASSERT(state);

  // wait for packet reception or timeout
  uint32_t start = _mod->micros();
  while(_mod->digitalRead(_mod->getIrq())) {
    if(_mod->micros() - start > timeout) {
      standby();
      clearIRQFlags();
      return(RADIOLIB_ERR_RX_TIMEOUT);
    }
  }

  // read packet data
  return(readData(data, len));
}

int16_t Si443x::sleep() {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, LOW);

  // disable wakeup timer interrupt
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_INTERRUPT_ENABLE_1, 0x00);
  RADIOLIB_ASSERT(state);
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_INTERRUPT_ENABLE_2, 0x00);
  RADIOLIB_ASSERT(state);

  // enable wakeup timer to set mode to sleep
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_1, RADIOLIB_SI443X_ENABLE_WAKEUP_TIMER);

  return(state);
}

int16_t Si443x::standby() {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, LOW);

  return(_mod->SPIsetRegValue(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_1, RADIOLIB_SI443X_XTAL_ON, 7, 0, 10));
}

int16_t Si443x::transmitDirect(uint32_t frf) {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, HIGH);

  // user requested to start transmitting immediately (required for RTTY)
  if(frf != 0) {
    // convert the 24-bit frequency to the format accepted by the module
    /// \todo integers only
    float newFreq = frf / 6400.0;

    // check high/low band
    uint8_t bandSelect = RADIOLIB_SI443X_BAND_SELECT_LOW;
    uint8_t freqBand = (newFreq / 10) - 24;
    if(newFreq >= 480.0) {
      bandSelect = RADIOLIB_SI443X_BAND_SELECT_HIGH;
      freqBand = (newFreq / 20) - 24;
    }

    // calculate register values
    uint16_t freqCarrier = ((newFreq / (10 * ((bandSelect >> 5) + 1))) - freqBand - 24) * (uint32_t)64000;

    // update registers
    _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_FREQUENCY_BAND_SELECT, RADIOLIB_SI443X_SIDE_BAND_SELECT_LOW | bandSelect | freqBand);
    _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_NOM_CARRIER_FREQUENCY_1, (uint8_t)((freqCarrier & 0xFF00) >> 8));
    _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_NOM_CARRIER_FREQUENCY_0, (uint8_t)(freqCarrier & 0xFF));

    // start direct transmission
    directMode();
    _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_1, RADIOLIB_SI443X_TX_ON | RADIOLIB_SI443X_XTAL_ON);

    return(RADIOLIB_ERR_NONE);
  }

  // activate direct mode
  int16_t state = directMode();
  RADIOLIB_ASSERT(state);

  // start transmitting
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_1, RADIOLIB_SI443X_TX_ON | RADIOLIB_SI443X_XTAL_ON);
  return(state);
}

int16_t Si443x::receiveDirect() {
  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // activate direct mode
  int16_t state = directMode();
  RADIOLIB_ASSERT(state);

  // start receiving
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_1, RADIOLIB_SI443X_RX_ON | RADIOLIB_SI443X_XTAL_ON);
  return(state);
}

int16_t Si443x::packetMode() {
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_MODULATION_MODE_CONTROL_2, RADIOLIB_SI443X_MODULATION_FSK, 1, 0);
  RADIOLIB_ASSERT(state);

  return(_mod->SPIsetRegValue(RADIOLIB_SI443X_REG_MODULATION_MODE_CONTROL_2, RADIOLIB_SI443X_TX_DATA_SOURCE_FIFO, 5, 4));
}

void Si443x::setIrqAction(void (*func)(void)) {
  _mod->attachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getIrq()), func, FALLING);
}

void Si443x::clearIrqAction() {
  _mod->detachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getIrq()));
}

int16_t Si443x::startTransmit(uint8_t* data, size_t len, uint8_t addr) {
  // check packet length
  if(len > RADIOLIB_SI443X_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // clear Tx FIFO
  _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_2, RADIOLIB_SI443X_TX_FIFO_RESET, 0, 0);
  _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_2, RADIOLIB_SI443X_TX_FIFO_CLEAR, 0, 0);

  // clear interrupt flags
  clearIRQFlags();

  // set packet length
  if (_packetLengthConfig == RADIOLIB_SI443X_FIXED_PACKET_LENGTH_OFF) {
    _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_TRANSMIT_PACKET_LENGTH, len);
  }

  /// \todo use header as address field?
  (void)addr;

  // write packet to FIFO
  _mod->SPIwriteRegisterBurst(RADIOLIB_SI443X_REG_FIFO_ACCESS, data, len);

  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, HIGH);

  // set interrupt mapping
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_INTERRUPT_ENABLE_1, RADIOLIB_SI443X_PACKET_SENT_ENABLED);
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_INTERRUPT_ENABLE_2, 0x00);

  // set mode to transmit
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_1, RADIOLIB_SI443X_TX_ON | RADIOLIB_SI443X_XTAL_ON);

  return(state);
}

int16_t Si443x::startReceive() {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // clear Rx FIFO
  _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_2, RADIOLIB_SI443X_RX_FIFO_RESET, 1, 1);
  _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_2, RADIOLIB_SI443X_RX_FIFO_CLEAR, 1, 1);

  // clear interrupt flags
  clearIRQFlags();

  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // set interrupt mapping
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_INTERRUPT_ENABLE_1, RADIOLIB_SI443X_VALID_PACKET_RECEIVED_ENABLED | RADIOLIB_SI443X_CRC_ERROR_ENABLED);
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_INTERRUPT_ENABLE_2, 0x00);

  // set mode to receive
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_1, RADIOLIB_SI443X_RX_ON | RADIOLIB_SI443X_XTAL_ON);

  return(state);
}

int16_t Si443x::readData(uint8_t* data, size_t len) {
  // clear interrupt flags
  clearIRQFlags();

  // get packet length
  size_t length = getPacketLength();
  size_t dumpLen = 0;
  if((len != 0) && (len < length)) {
    // user requested less data than we got, only return what was requested
    dumpLen = length - len;
    length = len;
  }

  // read packet data
  _mod->SPIreadRegisterBurst(RADIOLIB_SI443X_REG_FIFO_ACCESS, length, data);

  // dump the bytes that weren't requested
  if(dumpLen != 0) {
    clearFIFO(dumpLen);
  }

  // clear internal flag so getPacketLength can return the new packet length
  _packetLengthQueried = false;

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  clearIRQFlags();

  return(RADIOLIB_ERR_NONE);
}

int16_t Si443x::setBitRate(float br) {
  RADIOLIB_CHECK_RANGE(br, 0.123, 256.0, RADIOLIB_ERR_INVALID_BIT_RATE);

  // check high data rate
  uint8_t dataRateMode = RADIOLIB_SI443X_LOW_DATA_RATE_MODE;
  uint8_t exp = 21;
  if(br >= 30.0) {
    // bit rate above 30 kbps
    dataRateMode = RADIOLIB_SI443X_HIGH_DATA_RATE_MODE;
    exp = 16;
  }

  // calculate raw data rate value
  uint16_t txDr = (br * ((uint32_t)1 << exp)) / 1000.0;

  // update registers
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_MODULATION_MODE_CONTROL_1, dataRateMode, 5, 5);
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_TX_DATA_RATE_1, (uint8_t)((txDr & 0xFF00) >> 8));
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_TX_DATA_RATE_0, (uint8_t)(txDr & 0xFF));

  if(state == RADIOLIB_ERR_NONE) {
    _br = br;
  }
  RADIOLIB_ASSERT(state);

  // update clock recovery
  state = updateClockRecovery();

  return(state);
}

int16_t Si443x::setFrequencyDeviation(float freqDev) {
  // set frequency deviation to lowest available setting (required for digimodes)
  float newFreqDev = freqDev;
  if(freqDev < 0.0) {
    newFreqDev = 0.625;
  }

  RADIOLIB_CHECK_RANGE(newFreqDev, 0.625, 320.0, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);

  // calculate raw frequency deviation value
  uint16_t fdev = (uint16_t)(newFreqDev / 0.625);

  // update registers
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_MODULATION_MODE_CONTROL_2, (uint8_t)((fdev & 0x0100) >> 6), 2, 2);
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_FREQUENCY_DEVIATION, (uint8_t)(fdev & 0xFF));

  if(state == RADIOLIB_ERR_NONE) {
    _freqDev = newFreqDev;
  }

  return(state);
}

int16_t Si443x::setRxBandwidth(float rxBw) {
  RADIOLIB_CHECK_RANGE(rxBw, 2.6, 620.7, RADIOLIB_ERR_INVALID_RX_BANDWIDTH);

  // decide which approximation to use for decimation rate and filter tap calculation
  uint8_t bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_OFF;
  uint8_t decRate = RADIOLIB_SI443X_IF_FILTER_DEC_RATE;
  uint8_t filterSet = RADIOLIB_SI443X_IF_FILTER_COEFF_SET;

  // this is the "well-behaved" section - can be linearly approximated
  if((rxBw >= 2.6) && (rxBw <= 4.5)) {
    decRate = 5;
    filterSet = ((rxBw - 2.1429)/0.3250 + 0.5);
  } else if((rxBw > 4.5) && (rxBw <= 8.8)) {
    decRate = 4;
    filterSet = ((rxBw - 3.9857)/0.6643 + 0.5);
  } else if((rxBw > 8.8) && (rxBw <= 17.5)) {
    decRate = 3;
    filterSet = ((rxBw - 7.6714)/1.3536 + 0.5);
  } else if((rxBw > 17.5) && (rxBw <= 34.7)) {
    decRate = 2;
    filterSet = ((rxBw - 15.2000)/2.6893 + 0.5);
  } else if((rxBw > 34.7) && (rxBw <= 69.2)) {
    decRate = 1;
    filterSet = ((rxBw - 30.2430)/5.3679 + 0.5);
  } else if((rxBw > 69.2) && (rxBw <= 137.9)) {
    decRate = 0;
    filterSet = ((rxBw - 60.286)/10.7000 + 0.5);

  // this is the "Lord help thee who tread 'ere" section - no way to approximate this mess
  /// \todo float tolerance equality as macro?
  } else if(fabs(rxBw - 142.8) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 1;
    filterSet = 4;
  } else if(fabs(rxBw - 167.8) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 1;
    filterSet = 5;
  } else if(fabs(rxBw - 181.1) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 1;
    filterSet = 6;
  } else if(fabs(rxBw - 191.5) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 15;
  } else if(fabs(rxBw - 225.1) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 1;
  } else if(fabs(rxBw - 248.8) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 2;
  } else if(fabs(rxBw - 269.3) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 3;
  } else if(fabs(rxBw - 284.8) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 4;
  } else if(fabs(rxBw -335.5) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 8;
  } else if(fabs(rxBw - 391.8) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 9;
  } else if(fabs(rxBw - 420.2) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 10;
  } else if(fabs(rxBw - 468.4) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 11;
  } else if(fabs(rxBw - 518.8) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 12;
  } else if(fabs(rxBw - 577.0) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 13;
  } else if(fabs(rxBw - 620.7) <= 0.001) {
    bypass = RADIOLIB_SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 14;
  } else {
    return(RADIOLIB_ERR_INVALID_RX_BANDWIDTH);
  }

  // shift decimation rate bits
  decRate <<= 4;

  // update register
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_IF_FILTER_BANDWIDTH, bypass | decRate | filterSet);
  RADIOLIB_ASSERT(state);

  // update clock recovery
  state = updateClockRecovery();

  return(state);
}

int16_t Si443x::setSyncWord(uint8_t* syncWord, size_t len) {
  RADIOLIB_CHECK_RANGE(len, 1, 4, RADIOLIB_ERR_INVALID_SYNC_WORD);

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set sync word length
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_HEADER_CONTROL_2, (uint8_t)(len - 1) << 1, 2, 1);
  RADIOLIB_ASSERT(state);

  // set sync word bytes
  _mod->SPIwriteRegisterBurst(RADIOLIB_SI443X_REG_SYNC_WORD_3, syncWord, len);

  return(state);
}

int16_t Si443x::setPreambleLength(uint8_t preambleLen) {
  // Si443x configures preamble length in 4-bit nibbles
  if(preambleLen % 4 != 0) {
    return(RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);
  }

  // set default preamble length
  uint8_t preLenNibbles = preambleLen / 4;
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_PREAMBLE_LENGTH, preLenNibbles);
  RADIOLIB_ASSERT(state);

  // set default preamble detection threshold to 5/8 of preamble length (in units of 4 bits)
  uint8_t preThreshold = 5*preLenNibbles / 8;
  return(_mod->SPIsetRegValue(RADIOLIB_SI443X_REG_PREAMBLE_DET_CONTROL, preThreshold << 3, 7, 3));
}

size_t Si443x::getPacketLength(bool update) {
  if(!_packetLengthQueried && update) {
    if (_packetLengthConfig == RADIOLIB_SI443X_FIXED_PACKET_LENGTH_ON) {
      _packetLength = _mod->SPIreadRegister(RADIOLIB_SI443X_REG_TRANSMIT_PACKET_LENGTH);
    } else {
      _packetLength = _mod->SPIreadRegister(RADIOLIB_SI443X_REG_RECEIVED_PACKET_LENGTH);
    }
    _packetLengthQueried = true;
  }

  return(_packetLength);
}

int16_t Si443x::setEncoding(uint8_t encoding) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set encoding
  /// \todo - add inverted Manchester?
  switch(encoding) {
    case RADIOLIB_ENCODING_NRZ:
      return(_mod->SPIsetRegValue(RADIOLIB_SI443X_REG_MODULATION_MODE_CONTROL_1, RADIOLIB_SI443X_MANCHESTER_INVERTED_OFF | RADIOLIB_SI443X_MANCHESTER_OFF | RADIOLIB_SI443X_WHITENING_OFF, 2, 0));
    case RADIOLIB_ENCODING_MANCHESTER:
      return(_mod->SPIsetRegValue(RADIOLIB_SI443X_REG_MODULATION_MODE_CONTROL_1, RADIOLIB_SI443X_MANCHESTER_INVERTED_OFF | RADIOLIB_SI443X_MANCHESTER_ON | RADIOLIB_SI443X_WHITENING_OFF, 2, 0));
    case RADIOLIB_ENCODING_WHITENING:
      return(_mod->SPIsetRegValue(RADIOLIB_SI443X_REG_MODULATION_MODE_CONTROL_1, RADIOLIB_SI443X_MANCHESTER_INVERTED_OFF | RADIOLIB_SI443X_MANCHESTER_OFF | RADIOLIB_SI443X_WHITENING_ON, 2, 0));
    default:
      return(RADIOLIB_ERR_INVALID_ENCODING);
  }
}

int16_t Si443x::setDataShaping(uint8_t sh) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set data shaping
  switch(sh) {
    case RADIOLIB_SHAPING_NONE:
      return(_mod->SPIsetRegValue(RADIOLIB_SI443X_REG_MODULATION_MODE_CONTROL_1, RADIOLIB_SI443X_MANCHESTER_INVERTED_OFF | RADIOLIB_SI443X_MANCHESTER_OFF | RADIOLIB_SI443X_WHITENING_OFF, 2, 0));
    case RADIOLIB_SHAPING_0_3:
    case RADIOLIB_SHAPING_0_5:
    case RADIOLIB_SHAPING_1_0:
      /// \todo implement fiter configuration - docs claim this should be possible, but seems undocumented
      return(_mod->SPIsetRegValue(RADIOLIB_SI443X_REG_MODULATION_MODE_CONTROL_1, RADIOLIB_SI443X_MANCHESTER_INVERTED_OFF | RADIOLIB_SI443X_MANCHESTER_OFF | RADIOLIB_SI443X_WHITENING_ON, 2, 0));
    default:
      return(RADIOLIB_ERR_INVALID_ENCODING);
  }
}

void Si443x::setRfSwitchPins(RADIOLIB_PIN_TYPE rxEn, RADIOLIB_PIN_TYPE txEn) {
  _mod->setRfSwitchPins(rxEn, txEn);
}

uint8_t Si443x::randomByte() {
  // set mode to Rx
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_OP_FUNC_CONTROL_1, RADIOLIB_SI443X_RX_ON | RADIOLIB_SI443X_XTAL_ON);

  // wait a bit for the RSSI reading to stabilise
  _mod->delay(10);

  // read RSSI value 8 times, always keep just the least significant bit
  uint8_t randByte = 0x00;
  for(uint8_t i = 0; i < 8; i++) {
    randByte |= ((_mod->SPIreadRegister(RADIOLIB_SI443X_REG_RSSI) & 0x01) << i);
  }

  // set mode to standby
  standby();

  return(randByte);
}

int16_t Si443x::getChipVersion() {
  return(_mod->SPIgetRegValue(RADIOLIB_SI443X_REG_DEVICE_VERSION));
}

void Si443x::setDirectAction(void (*func)(void)) {
  setIrqAction(func);
}

void Si443x::readBit(RADIOLIB_PIN_TYPE pin) {
  updateDirectBuffer((uint8_t)digitalRead(pin));
}

int16_t Si443x::fixedPacketLengthMode(uint8_t len) {
  return(Si443x::setPacketMode(RADIOLIB_SI443X_FIXED_PACKET_LENGTH_ON, len));
}

int16_t Si443x::variablePacketLengthMode(uint8_t maxLen) {
  return(Si443x::setPacketMode(RADIOLIB_SI443X_FIXED_PACKET_LENGTH_OFF, maxLen));
}

int16_t Si443x::setFrequencyRaw(float newFreq) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check high/low band
  uint8_t bandSelect = RADIOLIB_SI443X_BAND_SELECT_LOW;
  uint8_t freqBand = (newFreq / 10) - 24;
  uint8_t afcLimiter = 80;
  _freq = newFreq;
  if(newFreq >= 480.0) {
    bandSelect = RADIOLIB_SI443X_BAND_SELECT_HIGH;
    freqBand = (newFreq / 20) - 24;
    afcLimiter = 40;
  }

  // calculate register values
  uint16_t freqCarrier = ((newFreq / (10 * ((bandSelect >> 5) + 1))) - freqBand - 24) * (uint32_t)64000;

  // update registers
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_FREQUENCY_BAND_SELECT, bandSelect | freqBand, 5, 0);
  state |= _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_NOM_CARRIER_FREQUENCY_1, (uint8_t)((freqCarrier & 0xFF00) >> 8));
  state |= _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_NOM_CARRIER_FREQUENCY_0, (uint8_t)(freqCarrier & 0xFF));
  state |= _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_AFC_LIMITER, afcLimiter);

  return(state);
}

int16_t Si443x::setPacketMode(uint8_t mode, uint8_t len) {
  // check packet length
  if (len > RADIOLIB_SI443X_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // set to fixed packet length
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_HEADER_CONTROL_2, mode, 3, 3);
  RADIOLIB_ASSERT(state);

  // set length to register
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_TRANSMIT_PACKET_LENGTH, len);
  RADIOLIB_ASSERT(state);

  // update cached value
  _packetLengthConfig = mode;
  return(state);
}

bool Si443x::findChip() {
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    // reset the module
    reset();

    // check version register
    uint8_t version = _mod->SPIreadRegister(RADIOLIB_SI443X_REG_DEVICE_VERSION);
    if(version == RADIOLIB_SI443X_DEVICE_VERSION) {
      flagFound = true;
    } else {
      #if defined(RADIOLIB_DEBUG)
        RADIOLIB_DEBUG_PRINT(F("Si443x not found! ("));
        RADIOLIB_DEBUG_PRINT(i + 1);
        RADIOLIB_DEBUG_PRINT(F(" of 10 tries) RADIOLIB_SI443X_REG_DEVICE_VERSION == "));

        char buffHex[5];
        sprintf(buffHex, "0x%02X", version);
        RADIOLIB_DEBUG_PRINT(buffHex);
        RADIOLIB_DEBUG_PRINT(F(", expected 0x00"));
        RADIOLIB_DEBUG_PRINTLN(RADIOLIB_SI443X_DEVICE_VERSION, HEX);
      #endif
      _mod->delay(10);
      i++;
    }
  }

  return(flagFound);
}

void Si443x::clearIRQFlags() {
  uint8_t buff[2];
  _mod->SPIreadRegisterBurst(RADIOLIB_SI443X_REG_INTERRUPT_STATUS_1, 2, buff);
}

void Si443x::clearFIFO(size_t count) {
  while(count) {
    _mod->SPIreadRegister(RADIOLIB_SI443X_REG_FIFO_ACCESS);
    count--;
  }
}

int16_t Si443x::config() {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // disable POR and chip ready interrupts
  _mod->SPIwriteRegister(RADIOLIB_SI443X_REG_INTERRUPT_ENABLE_2, 0x00);

  // enable AGC
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_AGC_OVERRIDE_1, RADIOLIB_SI443X_AGC_GAIN_INCREASE_ON | RADIOLIB_SI443X_AGC_ON, 6, 5);
  RADIOLIB_ASSERT(state);

  // disable packet header
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_HEADER_CONTROL_2, RADIOLIB_SI443X_SYNC_WORD_TIMEOUT_OFF | RADIOLIB_SI443X_HEADER_LENGTH_HEADER_NONE, 7, 4);
  RADIOLIB_ASSERT(state);

  // set antenna switching
  _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_GPIO0_CONFIG, RADIOLIB_SI443X_GPIOX_TX_STATE_OUT, 4, 0);
  _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_GPIO1_CONFIG, RADIOLIB_SI443X_GPIOX_RX_STATE_OUT, 4, 0);

  // disable packet header checking
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_HEADER_CONTROL_1, RADIOLIB_SI443X_BROADCAST_ADDR_CHECK_NONE | RADIOLIB_SI443X_RECEIVED_HEADER_CHECK_NONE);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t Si443x::updateClockRecovery() {
  // get the parameters
  uint8_t bypass = _mod->SPIgetRegValue(RADIOLIB_SI443X_REG_IF_FILTER_BANDWIDTH, 7, 7) >> 7;
  uint8_t decRate = _mod->SPIgetRegValue(RADIOLIB_SI443X_REG_IF_FILTER_BANDWIDTH, 6, 4) >> 4;
  uint8_t manch = _mod->SPIgetRegValue(RADIOLIB_SI443X_REG_MODULATION_MODE_CONTROL_1, 1, 1) >> 1;

  // calculate oversampling ratio, NCO offset and clock recovery gain
  int8_t ndecExp = (int8_t)decRate - 3;
  float ndec = 0;
  if(ndecExp > 0) {
    ndec = (uint16_t)1 << ndecExp;
  } else {
    ndecExp *= -1;
    ndec = 1.0/(float)((uint16_t)1 << ndecExp);
  }
  float rxOsr = ((float)(500 * (1 + 2*bypass))) / (ndec * _br * ((float)(1 + manch)));
  uint32_t ncoOff = (_br * (1 + manch) * ((uint32_t)(1) << (20 + decRate))) / (500 * (1 + 2*bypass));
  uint16_t crGain = 2 + (((float)(65536.0 * (1 + manch)) * _br) / (rxOsr * (_freqDev / 0.625)));
  uint16_t rxOsr_fixed = (uint16_t)rxOsr;

  // print that whole mess
  RADIOLIB_DEBUG_PRINTLN(bypass, HEX);
  RADIOLIB_DEBUG_PRINTLN(decRate, HEX);
  RADIOLIB_DEBUG_PRINTLN(manch, HEX);
  RADIOLIB_DEBUG_PRINT(rxOsr, 3);
  RADIOLIB_DEBUG_PRINT('\t');
  RADIOLIB_DEBUG_PRINT(rxOsr_fixed);
  RADIOLIB_DEBUG_PRINT('\t');
  RADIOLIB_DEBUG_PRINTLN(rxOsr_fixed, HEX);
  RADIOLIB_DEBUG_PRINT(ncoOff);
  RADIOLIB_DEBUG_PRINT('\t');
  RADIOLIB_DEBUG_PRINTLN(ncoOff, HEX);
  RADIOLIB_DEBUG_PRINT(crGain);
  RADIOLIB_DEBUG_PRINT('\t');
  RADIOLIB_DEBUG_PRINTLN(crGain, HEX);

  // update oversampling ratio
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_CLOCK_REC_OFFSET_2, (uint8_t)((rxOsr_fixed & 0x0700) >> 3), 7, 5);
  RADIOLIB_ASSERT(state);
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_CLOCK_REC_OVERSAMP_RATIO, (uint8_t)(rxOsr_fixed & 0x00FF));
  RADIOLIB_ASSERT(state);

  // update NCO offset
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_CLOCK_REC_OFFSET_2, (uint8_t)((ncoOff & 0x0F0000) >> 16), 3, 0);
  RADIOLIB_ASSERT(state);
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_CLOCK_REC_OFFSET_1, (uint8_t)((ncoOff & 0x00FF00) >> 8));
  RADIOLIB_ASSERT(state);
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_CLOCK_REC_OFFSET_0, (uint8_t)(ncoOff & 0x0000FF));
  RADIOLIB_ASSERT(state);

  // update clock recovery loop gain
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_CLOCK_REC_TIMING_LOOP_GAIN_1, (uint8_t)((crGain & 0x0700) >> 8), 2, 0);
  RADIOLIB_ASSERT(state);
  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_CLOCK_REC_TIMING_LOOP_GAIN_0, (uint8_t)(crGain & 0x00FF));
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t Si443x::directMode() {
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_MODULATION_MODE_CONTROL_2, RADIOLIB_SI443X_TX_DATA_SOURCE_GPIO, 5, 4);
  RADIOLIB_ASSERT(state);

  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_GPIO1_CONFIG, RADIOLIB_SI443X_GPIOX_TX_RX_DATA_CLK_OUT, 4, 0);
  RADIOLIB_ASSERT(state);

  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_GPIO2_CONFIG, RADIOLIB_SI443X_GPIOX_TX_DATA_IN, 4, 0);
  RADIOLIB_ASSERT(state);

  state = _mod->SPIsetRegValue(RADIOLIB_SI443X_REG_MODULATION_MODE_CONTROL_2, RADIOLIB_SI443X_MODULATION_FSK, 1, 0);
  return(state);
}

#endif
