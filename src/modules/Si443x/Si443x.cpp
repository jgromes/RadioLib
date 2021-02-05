#include "Si443x.h"
#if !defined(RADIOLIB_EXCLUDE_SI443X)

Si443x::Si443x(Module* mod) : PhysicalLayer(SI443X_FREQUENCY_STEP_SIZE, SI443X_MAX_PACKET_LENGTH) {
  _mod = mod;
}

int16_t Si443x::begin(float br, float freqDev, float rxBw, uint8_t preambleLen) {
  // set module properties
  _mod->init(RADIOLIB_USE_SPI);
  Module::pinMode(_mod->getIrq(), INPUT);
  Module::pinMode(_mod->getRst(), OUTPUT);
  Module::digitalWrite(_mod->getRst(), LOW);

  // try to find the Si443x chip
  if(!Si443x::findChip()) {
    RADIOLIB_DEBUG_PRINTLN(F("No Si443x found!"));
    _mod->term(RADIOLIB_USE_SPI);
    return(ERR_CHIP_NOT_FOUND);
  } else {
    RADIOLIB_DEBUG_PRINTLN(F("M\tSi443x"));
  }

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

  return(state);
}

void Si443x::reset() {
  Module::pinMode(_mod->getRst(), OUTPUT);
  Module::digitalWrite(_mod->getRst(), HIGH);
  Module::delay(1);
  Module::digitalWrite(_mod->getRst(), LOW);
  Module::delay(100);
}

int16_t Si443x::transmit(uint8_t* data, size_t len, uint8_t addr) {
  // calculate timeout (5ms + 500 % of expected time-on-air)
  uint32_t timeout = 5000000 + (uint32_t)((((float)(len * 8)) / (_br * 1000.0)) * 5000000.0);

  // start transmission
  int16_t state = startTransmit(data, len, addr);
  RADIOLIB_ASSERT(state);

  // wait for transmission end or timeout
  uint32_t start = Module::micros();
  while(Module::digitalRead(_mod->getIrq())) {
    Module::yield();
    if(Module::micros() - start > timeout) {
      standby();
      clearIRQFlags();
      return(ERR_TX_TIMEOUT);
    }
  }

  // clear interrupt flags
  clearIRQFlags();

  // set mode to standby
  standby();

  // the next transmission will timeout without the following
  _mod->SPIwriteRegister(SI443X_REG_INTERRUPT_ENABLE_2, 0x00);
  _mod->SPIsetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_2, SI443X_TX_DATA_SOURCE_FIFO, 5, 4);
  state = setFrequencyRaw(_freq);

  return(state);
}

int16_t Si443x::receive(uint8_t* data, size_t len) {
  // calculate timeout (500 ms + 400 full 64-byte packets at current bit rate)
  uint32_t timeout = 500000 + (1.0/(_br*1000.0))*(SI443X_MAX_PACKET_LENGTH*400.0);

  // start reception
  int16_t state = startReceive();
  RADIOLIB_ASSERT(state);

  // wait for packet reception or timeout
  uint32_t start = Module::micros();
  while(Module::digitalRead(_mod->getIrq())) {
    if(Module::micros() - start > timeout) {
      standby();
      clearIRQFlags();
      return(ERR_RX_TIMEOUT);
    }
  }

  // read packet data
  return(readData(data, len));
}

int16_t Si443x::sleep() {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, LOW);

  // disable wakeup timer interrupt
  int16_t state = _mod->SPIsetRegValue(SI443X_REG_INTERRUPT_ENABLE_1, 0x00);
  RADIOLIB_ASSERT(state);
  state = _mod->SPIsetRegValue(SI443X_REG_INTERRUPT_ENABLE_2, 0x00);
  RADIOLIB_ASSERT(state);

  // enable wakeup timer to set mode to sleep
  _mod->SPIwriteRegister(SI443X_REG_OP_FUNC_CONTROL_1, SI443X_ENABLE_WAKEUP_TIMER);

  return(state);
}

int16_t Si443x::standby() {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, LOW);

  //return(_mod->SPIsetRegValue(SI443X_REG_OP_FUNC_CONTROL_1, SI443X_XTAL_ON, 7, 0, 10));
  return(ERR_NONE);
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
    uint8_t bandSelect = SI443X_BAND_SELECT_LOW;
    uint8_t freqBand = (newFreq / 10) - 24;
    if(newFreq >= 480.0) {
      bandSelect = SI443X_BAND_SELECT_HIGH;
      freqBand = (newFreq / 20) - 24;
    }

    // calculate register values
    uint16_t freqCarrier = ((newFreq / (10 * ((bandSelect >> 5) + 1))) - freqBand - 24) * (uint32_t)64000;

    // update registers
    _mod->SPIwriteRegister(SI443X_REG_FREQUENCY_BAND_SELECT, SI443X_SIDE_BAND_SELECT_LOW | bandSelect | freqBand);
    _mod->SPIwriteRegister(SI443X_REG_NOM_CARRIER_FREQUENCY_1, (uint8_t)((freqCarrier & 0xFF00) >> 8));
    _mod->SPIwriteRegister(SI443X_REG_NOM_CARRIER_FREQUENCY_0, (uint8_t)(freqCarrier & 0xFF));

    // start direct transmission
    directMode();
    _mod->SPIwriteRegister(SI443X_REG_OP_FUNC_CONTROL_1, SI443X_TX_ON | SI443X_XTAL_ON);

    return(ERR_NONE);
  }

  // activate direct mode
  int16_t state = directMode();
  RADIOLIB_ASSERT(state);

  // start transmitting
  _mod->SPIwriteRegister(SI443X_REG_OP_FUNC_CONTROL_1, SI443X_TX_ON | SI443X_XTAL_ON);
  return(state);
}

int16_t Si443x::receiveDirect() {
  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // activate direct mode
  int16_t state = directMode();
  RADIOLIB_ASSERT(state);

  // start receiving
  _mod->SPIwriteRegister(SI443X_REG_OP_FUNC_CONTROL_1, SI443X_RX_ON | SI443X_XTAL_ON);
  return(state);
}

int16_t Si443x::packetMode() {
  return(_mod->SPIsetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_2, SI443X_TX_DATA_SOURCE_FIFO, 5, 4));
}

void Si443x::setIrqAction(void (*func)(void)) {
  Module::attachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getIrq()), func, FALLING);
}

void Si443x::clearIrqAction() {
  Module::detachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getIrq()));
}

int16_t Si443x::startTransmit(uint8_t* data, size_t len, uint8_t addr) {
  // check packet length
  if(len > SI443X_MAX_PACKET_LENGTH) {
    return(ERR_PACKET_TOO_LONG);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // clear Tx FIFO
  _mod->SPIsetRegValue(SI443X_REG_OP_FUNC_CONTROL_2, SI443X_TX_FIFO_RESET, 0, 0);
  _mod->SPIsetRegValue(SI443X_REG_OP_FUNC_CONTROL_2, SI443X_TX_FIFO_CLEAR, 0, 0);

  // clear interrupt flags
  clearIRQFlags();

  // set packet length
  /// \todo variable packet length
  _mod->SPIwriteRegister(SI443X_REG_TRANSMIT_PACKET_LENGTH, len);

  /// \todo use header as address field?
  (void)addr;

  // write packet to FIFO
  _mod->SPIwriteRegisterBurst(SI443X_REG_FIFO_ACCESS, data, len);

  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, HIGH);

  // set interrupt mapping
  _mod->SPIwriteRegister(SI443X_REG_INTERRUPT_ENABLE_1, SI443X_PACKET_SENT_ENABLED);
  _mod->SPIwriteRegister(SI443X_REG_INTERRUPT_ENABLE_2, 0x00);

  // set mode to transmit
  _mod->SPIwriteRegister(SI443X_REG_OP_FUNC_CONTROL_1, SI443X_TX_ON | SI443X_XTAL_ON);

  return(state);
}

int16_t Si443x::startReceive() {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // clear Rx FIFO
  _mod->SPIsetRegValue(SI443X_REG_OP_FUNC_CONTROL_2, SI443X_RX_FIFO_RESET, 1, 1);
  _mod->SPIsetRegValue(SI443X_REG_OP_FUNC_CONTROL_2, SI443X_RX_FIFO_CLEAR, 1, 1);

  // clear interrupt flags
  clearIRQFlags();

  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // set interrupt mapping
  _mod->SPIwriteRegister(SI443X_REG_INTERRUPT_ENABLE_1, SI443X_PACKET_SENT_ENABLED);
  _mod->SPIwriteRegister(SI443X_REG_INTERRUPT_ENABLE_2, 0x00);

  // set mode to receive
  _mod->SPIwriteRegister(SI443X_REG_OP_FUNC_CONTROL_1, SI443X_RX_ON | SI443X_XTAL_ON);

  return(state);
}

int16_t Si443x::readData(uint8_t* data, size_t len) {
  // clear interrupt flags
  clearIRQFlags();

  // get packet length
  size_t length = len;
  if(len == SI443X_MAX_PACKET_LENGTH) {
    length = getPacketLength();
  }

  // read packet data
  _mod->SPIreadRegisterBurst(SI443X_REG_FIFO_ACCESS, length, data);

  // clear internal flag so getPacketLength can return the new packet length
  _packetLengthQueried = false;

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  clearIRQFlags();

  return(ERR_NONE);
}

int16_t Si443x::setBitRate(float br) {
  RADIOLIB_CHECK_RANGE(br, 0.123, 256.0, ERR_INVALID_BIT_RATE);

  // check high data rate
  uint8_t dataRateMode = SI443X_LOW_DATA_RATE_MODE;
  uint8_t exp = 21;
  if(br >= 30.0) {
    // bit rate above 30 kbps
    dataRateMode = SI443X_HIGH_DATA_RATE_MODE;
    exp = 16;
  }

  // calculate raw data rate value
  uint16_t txDr = (br * ((uint32_t)1 << exp)) / 1000.0;

  // update registers
  int16_t state = _mod->SPIsetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_1, dataRateMode, 5, 5);
  _mod->SPIwriteRegister(SI443X_REG_TX_DATA_RATE_1, (uint8_t)((txDr & 0xFF00) >> 8));
  _mod->SPIwriteRegister(SI443X_REG_TX_DATA_RATE_0, (uint8_t)(txDr & 0xFF));

  if(state == ERR_NONE) {
    _br = br;
  }
  RADIOLIB_ASSERT(state);

  // update clock recovery
  state = updateClockRecovery();

  return(state);
}

int16_t Si443x::setFrequencyDeviation(float freqDev) {
  // set frequency deviation to lowest available setting (required for RTTY)
  if(freqDev == 0.0) {
    int16_t state = _mod->SPIsetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_2, 0x00, 2, 2);
    _mod->SPIwriteRegister(SI443X_REG_FREQUENCY_DEVIATION, 0x00);

    if(state == ERR_NONE) {
      _freqDev = freqDev;
    }
    return(state);
  }

  RADIOLIB_CHECK_RANGE(freqDev, 0.625, 320.0, ERR_INVALID_FREQUENCY_DEVIATION);

  // calculate raw frequency deviation value
  uint16_t fdev = (uint16_t)(freqDev / 0.625);

  // update registers
  int16_t state = _mod->SPIsetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_2, (uint8_t)((fdev & 0x0100) >> 6), 2, 2);
  _mod->SPIwriteRegister(SI443X_REG_FREQUENCY_DEVIATION, (uint8_t)(fdev & 0xFF));

  if(state == ERR_NONE) {
    _freqDev = freqDev;
  }

  return(state);
}

int16_t Si443x::setRxBandwidth(float rxBw) {
  RADIOLIB_CHECK_RANGE(rxBw, 2.6, 620.7, ERR_INVALID_RX_BANDWIDTH);

  // decide which approximation to use for decimation rate and filter tap calculation
  uint8_t bypass = SI443X_BYPASS_DEC_BY_3_OFF;
  uint8_t decRate = SI443X_IF_FILTER_DEC_RATE;
  uint8_t filterSet = SI443X_IF_FILTER_COEFF_SET;

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
  } else if(abs(rxBw - 142.8) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 1;
    filterSet = 4;
  } else if(abs(rxBw - 167.8) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 1;
    filterSet = 5;
  } else if(abs(rxBw - 181.1) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 1;
    filterSet = 6;
  } else if(abs(rxBw - 191.5) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 15;
  } else if(abs(rxBw - 225.1) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 1;
  } else if(abs(rxBw - 248.8) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 2;
  } else if(abs(rxBw - 269.3) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 3;
  } else if(abs(rxBw - 284.8) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 4;
  } else if(abs(rxBw -335.5) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 8;
  } else if(abs(rxBw - 391.8) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 9;
  } else if(abs(rxBw - 420.2) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 10;
  } else if(abs(rxBw - 468.4) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 11;
  } else if(abs(rxBw - 518.8) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 12;
  } else if(abs(rxBw - 577.0) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 13;
  } else if(abs(rxBw - 620.7) <= 0.001) {
    bypass = SI443X_BYPASS_DEC_BY_3_ON;
    decRate = 0;
    filterSet = 14;
  } else {
    return(ERR_INVALID_RX_BANDWIDTH);
  }

  // shift decimation rate bits
  decRate <<= 4;

  // update register
  int16_t state = _mod->SPIsetRegValue(SI443X_REG_IF_FILTER_BANDWIDTH, bypass | decRate | filterSet);
  RADIOLIB_ASSERT(state);

  // update clock recovery
  state = updateClockRecovery();

  return(state);
}

int16_t Si443x::setSyncWord(uint8_t* syncWord, size_t len) {
  RADIOLIB_CHECK_RANGE(len, 1, 4, ERR_INVALID_SYNC_WORD);

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set sync word length
  state = _mod->SPIsetRegValue(SI443X_REG_HEADER_CONTROL_2, (uint8_t)(len - 1) << 1, 2, 1);
  RADIOLIB_ASSERT(state);

  // set sync word bytes
  _mod->SPIwriteRegisterBurst(SI443X_REG_SYNC_WORD_3, syncWord, len);

  return(state);
}

int16_t Si443x::setPreambleLength(uint8_t preambleLen) {
  // Si443x configures preamble length in bytes
  if(preambleLen % 8 != 0) {
    return(ERR_INVALID_PREAMBLE_LENGTH);
  }

  // set default preamble length
  uint8_t preLenBytes = preambleLen / 8;
  int16_t state = _mod->SPIsetRegValue(SI443X_REG_PREAMBLE_LENGTH, preLenBytes);
  RADIOLIB_ASSERT(state);

  // set default preamble detection threshold to 50% of preamble length (in units of 4 bits)
  uint8_t preThreshold = preambleLen / 4;
  return(_mod->SPIsetRegValue(SI443X_REG_PREAMBLE_DET_CONTROL, preThreshold << 4, 7, 3));
}

size_t Si443x::getPacketLength(bool update) {
  /// \todo variable length mode
  if(!_packetLengthQueried && update) {
    _packetLength = _mod->SPIreadRegister(SI443X_REG_RECEIVED_PACKET_LENGTH);
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
      return(_mod->SPIsetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_1, SI443X_MANCHESTER_INVERTED_OFF | SI443X_MANCHESTER_OFF | SI443X_WHITENING_OFF, 2, 0));
    case RADIOLIB_ENCODING_MANCHESTER:
      return(_mod->SPIsetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_1, SI443X_MANCHESTER_INVERTED_OFF | SI443X_MANCHESTER_ON | SI443X_WHITENING_OFF, 2, 0));
    case RADIOLIB_ENCODING_WHITENING:
      return(_mod->SPIsetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_1, SI443X_MANCHESTER_INVERTED_OFF | SI443X_MANCHESTER_OFF | SI443X_WHITENING_ON, 2, 0));
    default:
      return(ERR_INVALID_ENCODING);
  }
}

int16_t Si443x::setDataShaping(uint8_t sh) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set data shaping
  switch(sh) {
    case RADIOLIB_SHAPING_NONE:
      return(_mod->SPIsetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_1, SI443X_MANCHESTER_INVERTED_OFF | SI443X_MANCHESTER_OFF | SI443X_WHITENING_OFF, 2, 0));
    case RADIOLIB_SHAPING_0_3:
    case RADIOLIB_SHAPING_0_5:
    case RADIOLIB_SHAPING_1_0:
      /// \todo implement fiter configuration - docs claim this should be possible, but seems undocumented
      return(_mod->SPIsetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_1, SI443X_MANCHESTER_INVERTED_OFF | SI443X_MANCHESTER_OFF | SI443X_WHITENING_ON, 2, 0));
    default:
      return(ERR_INVALID_ENCODING);
  }
}

void Si443x::setRfSwitchPins(RADIOLIB_PIN_TYPE rxEn, RADIOLIB_PIN_TYPE txEn) {
  _mod->setRfSwitchPins(rxEn, txEn);
}

uint8_t Si443x::random() {
  // set mode to Rx
  _mod->SPIwriteRegister(SI443X_REG_OP_FUNC_CONTROL_1, SI443X_RX_ON | SI443X_XTAL_ON);

  // wait a bit for the RSSI reading to stabilise
  Module::delay(10);

  // read RSSI value 8 times, always keep just the least significant bit
  uint8_t randByte = 0x00;
  for(uint8_t i = 0; i < 8; i++) {
    randByte |= ((_mod->SPIreadRegister(SI443X_REG_RSSI) & 0x01) << i);
  }

  // set mode to standby
  standby();

  return(randByte);
}

int16_t Si443x::getChipVersion() {
  return(_mod->SPIgetRegValue(SI443X_REG_DEVICE_VERSION));
}

int16_t Si443x::setFrequencyRaw(float newFreq) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check high/low band
  uint8_t bandSelect = SI443X_BAND_SELECT_LOW;
  uint8_t freqBand = (newFreq / 10) - 24;
  _freq = newFreq;
  if(newFreq >= 480.0) {
    bandSelect = SI443X_BAND_SELECT_HIGH;
    freqBand = (newFreq / 20) - 24;
  }

  // calculate register values
  uint16_t freqCarrier = ((newFreq / (10 * ((bandSelect >> 5) + 1))) - freqBand - 24) * (uint32_t)64000;

  // update registers
  state = _mod->SPIsetRegValue(SI443X_REG_FREQUENCY_BAND_SELECT, bandSelect | freqBand, 5, 0);
  state |= _mod->SPIsetRegValue(SI443X_REG_NOM_CARRIER_FREQUENCY_1, (uint8_t)((freqCarrier & 0xFF00) >> 8));
  state |= _mod->SPIsetRegValue(SI443X_REG_NOM_CARRIER_FREQUENCY_0, (uint8_t)(freqCarrier & 0xFF));

  return(state);
}

bool Si443x::findChip() {
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    // reset the module
    reset();

    // check version register
    uint8_t version = _mod->SPIreadRegister(SI443X_REG_DEVICE_VERSION);
    if(version == SI443X_DEVICE_VERSION) {
      flagFound = true;
    } else {
      #ifdef RADIOLIB_DEBUG
        RADIOLIB_DEBUG_PRINT(F("Si443x not found! ("));
        RADIOLIB_DEBUG_PRINT(i + 1);
        RADIOLIB_DEBUG_PRINT(F(" of 10 tries) SI443X_REG_DEVICE_VERSION == "));

        char buffHex[5];
        sprintf(buffHex, "0x%02X", version);
        RADIOLIB_DEBUG_PRINT(buffHex);
        RADIOLIB_DEBUG_PRINT(F(", expected 0x00"));
        RADIOLIB_DEBUG_PRINTLN(SI443X_DEVICE_VERSION, HEX);
      #endif
      Module::delay(10);
      i++;
    }
  }

  return(flagFound);
}

void Si443x::clearIRQFlags() {
  uint8_t buff[2];
  _mod->SPIreadRegisterBurst(SI443X_REG_INTERRUPT_STATUS_1, 2, buff);
}

int16_t Si443x::config() {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // disable POR and chip ready interrupts
  _mod->SPIwriteRegister(SI443X_REG_INTERRUPT_ENABLE_2, 0x00);

  // disable packet header
  state = _mod->SPIsetRegValue(SI443X_REG_HEADER_CONTROL_2, SI443X_SYNC_WORD_TIMEOUT_ON | SI443X_HEADER_LENGTH_HEADER_NONE, 7, 4);
  RADIOLIB_ASSERT(state);

  // disable packet header checking
  state = _mod->SPIsetRegValue(SI443X_REG_HEADER_CONTROL_1, SI443X_BROADCAST_ADDR_CHECK_NONE | SI443X_RECEIVED_HEADER_CHECK_NONE);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t Si443x::updateClockRecovery() {
  // get the parameters
  uint8_t bypass = _mod->SPIgetRegValue(SI443X_REG_IF_FILTER_BANDWIDTH, 7, 7) >> 7;
  uint8_t decRate = _mod->SPIgetRegValue(SI443X_REG_IF_FILTER_BANDWIDTH, 6, 4) >> 4;
  uint8_t manch = _mod->SPIgetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_1, 1, 1) >> 1;

  // calculate oversampling ratio, NCO offset and clock recovery gain
  float rxOsr = ((float)(500 * (1 + 2*bypass))) / (((float)((uint16_t)(1) << decRate)) * _br * ((float)(1 + manch)));
  uint32_t ncoOff = (_br * (1 + manch) * ((uint32_t)(1) << (20 + decRate))) / (500 * (1 + 2*bypass));
  uint16_t crGain = 2 + (((float)(65536.0 * (1 + manch)) * _br) / (rxOsr * (_freqDev / 0.625)));

  // convert oversampling ratio from float to fixed point
  uint8_t rxOsr_int = (uint8_t)rxOsr;
  uint8_t rxOsr_dec = 0;
  float rxOsr_temp = rxOsr;
  if((rxOsr_temp - rxOsr_int) >= 0.5) {
    rxOsr_dec |= 0x04;
    rxOsr_temp -= 0.5;
  }
  if((rxOsr_temp - rxOsr_int) >= 0.25) {
    rxOsr_dec |= 0x02;
    rxOsr_temp -= 0.25;
  }
  if((rxOsr_temp - rxOsr_int) >= 0.125) {
    rxOsr_dec |= 0x01;
  }
  uint16_t rxOsr_fixed = ((uint16_t)rxOsr_int << 3) | ((uint16_t)rxOsr_dec);

  // print that whole mess
  RADIOLIB_DEBUG_PRINTLN(bypass, HEX);
  RADIOLIB_DEBUG_PRINTLN(decRate, HEX);
  RADIOLIB_DEBUG_PRINTLN(manch, HEX);
  RADIOLIB_DEBUG_PRINT(rxOsr, 3);
  RADIOLIB_DEBUG_PRINT('\t');
  RADIOLIB_DEBUG_PRINT(rxOsr_int);
  RADIOLIB_DEBUG_PRINT('\t');
  RADIOLIB_DEBUG_PRINT(rxOsr_int, HEX);
  RADIOLIB_DEBUG_PRINT('\t');
  RADIOLIB_DEBUG_PRINT(rxOsr_dec);
  RADIOLIB_DEBUG_PRINT('\t');
  RADIOLIB_DEBUG_PRINT(rxOsr_dec, HEX);
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
  int16_t state = _mod->SPIsetRegValue(SI443X_REG_CLOCK_REC_OFFSET_2, (uint8_t)((rxOsr_fixed & 0x0700) >> 3), 7, 5);
  RADIOLIB_ASSERT(state);
  state = _mod->SPIsetRegValue(SI443X_REG_CLOCK_REC_OVERSAMP_RATIO, (uint8_t)(rxOsr_fixed & 0x00FF));
  RADIOLIB_ASSERT(state);

  // update NCO offset
  state = _mod->SPIsetRegValue(SI443X_REG_CLOCK_REC_OFFSET_2, (uint8_t)((ncoOff & 0x0F0000) >> 16), 3, 0);
  RADIOLIB_ASSERT(state);
  state = _mod->SPIsetRegValue(SI443X_REG_CLOCK_REC_OFFSET_1, (uint8_t)((ncoOff & 0x00FF00) >> 8));
  RADIOLIB_ASSERT(state);
  state = _mod->SPIsetRegValue(SI443X_REG_CLOCK_REC_OFFSET_0, (uint8_t)(ncoOff & 0x0000FF));
  RADIOLIB_ASSERT(state);

  // update clock recovery loop gain
  state = _mod->SPIsetRegValue(SI443X_REG_CLOCK_REC_TIMING_LOOP_GAIN_1, (uint8_t)((crGain & 0x0700) >> 8), 2, 0);
  RADIOLIB_ASSERT(state);
  state = _mod->SPIsetRegValue(SI443X_REG_CLOCK_REC_TIMING_LOOP_GAIN_0, (uint8_t)(crGain & 0x00FF));
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t Si443x::directMode() {
  int16_t state = _mod->SPIsetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_2, SI443X_TX_DATA_SOURCE_GPIO, 5, 4);
  RADIOLIB_ASSERT(state);

  state = _mod->SPIsetRegValue(SI443X_REG_MODULATION_MODE_CONTROL_2, SI443X_MODULATION_NONE, 1, 0);
  return(state);
}

#endif
