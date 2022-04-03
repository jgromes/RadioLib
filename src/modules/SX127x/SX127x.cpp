#include "SX127x.h"
#if !defined(RADIOLIB_EXCLUDE_SX127X)

SX127x::SX127x(Module* mod) : PhysicalLayer(RADIOLIB_SX127X_FREQUENCY_STEP_SIZE, RADIOLIB_SX127X_MAX_PACKET_LENGTH) {
  _mod = mod;
}

Module* SX127x::getMod() {
  return(_mod);
}

int16_t SX127x::begin(uint8_t chipVersion, uint8_t syncWord, uint16_t preambleLength) {
  // set module properties
  _mod->init();
  _mod->pinMode(_mod->getIrq(), INPUT);
  _mod->pinMode(_mod->getGpio(), INPUT);

  // try to find the SX127x chip
  if(!SX127x::findChip(chipVersion)) {
    RADIOLIB_DEBUG_PRINTLN(F("No SX127x found!"));
    _mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  RADIOLIB_DEBUG_PRINTLN(F("M\tSX127x"));

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = config();
  RADIOLIB_ASSERT(state);

  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    // set LoRa mode
    state = setActiveModem(RADIOLIB_SX127X_LORA);
    RADIOLIB_ASSERT(state);
  }

  // set LoRa sync word
  state = SX127x::setSyncWord(syncWord);
  RADIOLIB_ASSERT(state);

  // set over current protection
  state = SX127x::setCurrentLimit(60);
  RADIOLIB_ASSERT(state);

  // set preamble length
  state = SX127x::setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // initialize internal variables
  _dataRate = 0.0;

  return(state);
}

int16_t SX127x::beginFSK(uint8_t chipVersion, float br, float freqDev, float rxBw, uint16_t preambleLength, bool enableOOK) {
  // set module properties
  _mod->init();
  _mod->pinMode(_mod->getIrq(), INPUT);

  // try to find the SX127x chip
  if(!SX127x::findChip(chipVersion)) {
    RADIOLIB_DEBUG_PRINTLN(F("No SX127x found!"));
    _mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  RADIOLIB_DEBUG_PRINTLN(F("M\tSX127x"));

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check currently active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    // set FSK mode
    state = setActiveModem(RADIOLIB_SX127X_FSK_OOK);
    RADIOLIB_ASSERT(state);
  }

  // enable/disable OOK
  state = setOOK(enableOOK);
  RADIOLIB_ASSERT(state);

  // set bit rate
  state = SX127x::setBitRate(br);
  RADIOLIB_ASSERT(state);

  // set frequency deviation
  state = SX127x::setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  // set AFC bandwidth
  state = SX127x::setAFCBandwidth(rxBw);
  RADIOLIB_ASSERT(state);

  // set AFC&AGC trigger to RSSI (both in OOK and FSK)
  state = SX127x::setAFCAGCTrigger(RADIOLIB_SX127X_RX_TRIGGER_RSSI_INTERRUPT);
  RADIOLIB_ASSERT(state);

  // enable AFC
  state = SX127x::setAFC(false);
  RADIOLIB_ASSERT(state);

  // set receiver bandwidth
  state = SX127x::setRxBandwidth(rxBw);
  RADIOLIB_ASSERT(state);

  // set over current protection
  state = SX127x::setCurrentLimit(60);
  RADIOLIB_ASSERT(state);

  // set preamble length
  state = SX127x::setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // set default sync word
  uint8_t syncWord[] = {0x12, 0xAD};
  state = setSyncWord(syncWord, 2);
  RADIOLIB_ASSERT(state);

  // disable address filtering
  state = disableAddressFiltering();
  RADIOLIB_ASSERT(state);

  // set default RSSI measurement config
  state = setRSSIConfig(2);
  RADIOLIB_ASSERT(state);

  // set default encoding
  state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  // set default packet length mode
  state = variablePacketLengthMode();

  return(state);
}

int16_t SX127x::transmit(uint8_t* data, size_t len, uint8_t addr) {
  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  int16_t modem = getActiveModem();
  uint32_t start = 0;
  if(modem == RADIOLIB_SX127X_LORA) {
    // calculate timeout (150 % of expected time-one-air)
    float symbolLength = (float)(uint32_t(1) <<_sf) / (float)_bw;
    float de = 0;
    if(symbolLength >= 16.0) {
      de = 1;
    }
    float ih = (float)_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_MODEM_CONFIG_1, 0, 0);
    float crc = (float)(_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_MODEM_CONFIG_2, 2, 2) >> 2);
    float n_pre = (float)((_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_MSB) << 8) | _mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_LSB));
    float n_pay = 8.0 + max(ceil((8.0 * (float)len - 4.0 * (float)_sf + 28.0 + 16.0 * crc - 20.0 * ih)/(4.0 * (float)_sf - 8.0 * de)) * (float)_cr, 0.0);
    uint32_t timeout = ceil(symbolLength * (n_pre + n_pay + 4.25) * 1500.0);

    // start transmission
    state = startTransmit(data, len, addr);
    RADIOLIB_ASSERT(state);

    // wait for packet transmission or timeout
    start = _mod->micros();
    while(!_mod->digitalRead(_mod->getIrq())) {
      _mod->yield();
      if(_mod->micros() - start > timeout) {
        clearIRQFlags();
        return(RADIOLIB_ERR_TX_TIMEOUT);
      }
    }

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // calculate timeout (5ms + 500 % of expected time-on-air)
    uint32_t timeout = 5000000 + (uint32_t)((((float)(len * 8)) / (_br * 1000.0)) * 5000000.0);

    // start transmission
    state = startTransmit(data, len, addr);
    RADIOLIB_ASSERT(state);

    // wait for transmission end or timeout
    start = _mod->micros();
    while(!_mod->digitalRead(_mod->getIrq())) {
      _mod->yield();
      if(_mod->micros() - start > timeout) {
        clearIRQFlags();
        standby();
        return(RADIOLIB_ERR_TX_TIMEOUT);
      }
    }
  } else {
    return(RADIOLIB_ERR_UNKNOWN);
  }

  // update data rate
  uint32_t elapsed = _mod->micros() - start;
  _dataRate = (len*8.0)/((float)elapsed/1000000.0);

  // clear interrupt flags
  clearIRQFlags();

  // set mode to standby to disable transmitter
  return(standby());
}

int16_t SX127x::receive(uint8_t* data, size_t len) {
  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  int16_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    // set mode to receive
    state = startReceive(len, RADIOLIB_SX127X_RXSINGLE);
    RADIOLIB_ASSERT(state);

    // wait for packet reception or timeout (100 LoRa symbols)
    while(!_mod->digitalRead(_mod->getIrq())) {
      _mod->yield();
      if(_mod->digitalRead(_mod->getGpio())) {
        clearIRQFlags();
        return(RADIOLIB_ERR_RX_TIMEOUT);
      }
    }

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // calculate timeout (500 % of expected time-one-air)
    uint32_t timeout = (uint32_t)((((float)(len * 8)) / (_br * 1000.0)) * 5000000.0);

    // set mode to receive
    state = startReceive(len, RADIOLIB_SX127X_RX);
    RADIOLIB_ASSERT(state);

    // wait for packet reception or timeout
    uint32_t start = _mod->micros();
    while(!_mod->digitalRead(_mod->getIrq())) {
      _mod->yield();
      if(_mod->micros() - start > timeout) {
        clearIRQFlags();
        return(RADIOLIB_ERR_RX_TIMEOUT);
      }
    }
  }

  // read the received data
  state = readData(data, len);

  return(state);
}

int16_t SX127x::scanChannel() {
  // start CAD
  int16_t state = startChannelScan();
  RADIOLIB_ASSERT(state);

  // wait for channel activity detected or timeout
  while(!_mod->digitalRead(_mod->getIrq())) {
    _mod->yield();
    if(_mod->digitalRead(_mod->getGpio())) {
      clearIRQFlags();
      return(RADIOLIB_PREAMBLE_DETECTED);
    }
  }

  // clear interrupt flags
  clearIRQFlags();

  return(RADIOLIB_CHANNEL_FREE);
}

int16_t SX127x::sleep() {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, LOW);

  // set mode to sleep
  return(setMode(RADIOLIB_SX127X_SLEEP));
}

int16_t SX127x::standby() {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, LOW);

  // set mode to standby
  return(setMode(RADIOLIB_SX127X_STANDBY));
}

int16_t SX127x::transmitDirect(uint32_t frf) {
  // check modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, HIGH);

  // user requested to start transmitting immediately (required for RTTY)
  if(frf != 0) {
    _mod->SPIwriteRegister(RADIOLIB_SX127X_REG_FRF_MSB, (frf & 0xFF0000) >> 16);
    _mod->SPIwriteRegister(RADIOLIB_SX127X_REG_FRF_MID, (frf & 0x00FF00) >> 8);
    _mod->SPIwriteRegister(RADIOLIB_SX127X_REG_FRF_LSB, frf & 0x0000FF);

    return(setMode(RADIOLIB_SX127X_TX));
  }

  // activate direct mode
  int16_t state = directMode();
  RADIOLIB_ASSERT(state);

  // apply fixes to errata
  RADIOLIB_ERRATA_SX127X(false);

  // start transmitting
  return(setMode(RADIOLIB_SX127X_TX));
}

int16_t SX127x::receiveDirect() {
  // check modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // activate direct mode
  int16_t state = directMode();
  RADIOLIB_ASSERT(state);

  // apply fixes to errata
  RADIOLIB_ERRATA_SX127X(true);

  // start receiving
  return(setMode(RADIOLIB_SX127X_RX));
}

int16_t SX127x::directMode() {
  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // set DIO mapping
  state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO1_CONT_DCLK | RADIOLIB_SX127X_DIO2_CONT_DATA, 5, 2);
  RADIOLIB_ASSERT(state);

  // enable receiver startup without preamble or RSSI
  state = SX127x::setAFCAGCTrigger(RADIOLIB_SX127X_RX_TRIGGER_NONE);
  RADIOLIB_ASSERT(state);

  // set continuous mode
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_2, RADIOLIB_SX127X_DATA_MODE_CONTINUOUS, 6, 6));
}

int16_t SX127x::packetMode() {
  // check modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_2, RADIOLIB_SX127X_DATA_MODE_PACKET, 6, 6));
}

int16_t SX127x::startReceive(uint8_t len, uint8_t mode) {
  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  int16_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    // set DIO pin mapping
    if(_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD) > RADIOLIB_SX127X_HOP_PERIOD_OFF) {
      state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO0_RX_DONE | RADIOLIB_SX127X_DIO1_FHSS_CHANGE_CHANNEL, 7, 4);
    }
    else {
      state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO0_RX_DONE | RADIOLIB_SX127X_DIO1_RX_TIMEOUT, 7, 4);
    }

    // set expected packet length for SF6
    if(_sf == 6) {
      state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PAYLOAD_LENGTH, len);
    }

    // apply fixes to errata
    RADIOLIB_ERRATA_SX127X(true);

    // clear interrupt flags
    clearIRQFlags();

    // set FIFO pointers
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_RX_BASE_ADDR, RADIOLIB_SX127X_FIFO_RX_BASE_ADDR_MAX);
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_ADDR_PTR, RADIOLIB_SX127X_FIFO_RX_BASE_ADDR_MAX);
    RADIOLIB_ASSERT(state);

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // set DIO pin mapping
    state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO0_PACK_PAYLOAD_READY, 7, 6);
    RADIOLIB_ASSERT(state);

    // clear interrupt flags
    clearIRQFlags();

    // FSK modem does not distinguish between Rx single and continuous
    if(mode == RADIOLIB_SX127X_RXCONTINUOUS) {
      return(setMode(RADIOLIB_SX127X_RX));
    }
  }

  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // set mode to receive
  return(setMode(mode));
}

void SX127x::setDio0Action(void (*func)(void)) {
  _mod->attachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getIrq()), func, RISING);
}

void SX127x::clearDio0Action() {
  _mod->detachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getIrq()));
}

void SX127x::setDio1Action(void (*func)(void)) {
  if(_mod->getGpio() == RADIOLIB_NC) {
    return;
  }
  _mod->attachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getGpio()), func, RISING);
}

void SX127x::clearDio1Action() {
  if(_mod->getGpio() == RADIOLIB_NC) {
    return;
  }
  _mod->detachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getGpio()));
}

int16_t SX127x::startTransmit(uint8_t* data, size_t len, uint8_t addr) {
  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);

  int16_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    // check packet length
    if(len > RADIOLIB_SX127X_MAX_PACKET_LENGTH) {
      return(RADIOLIB_ERR_PACKET_TOO_LONG);
    }

    // set DIO mapping
    if(_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD) > RADIOLIB_SX127X_HOP_PERIOD_OFF) {
      _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO0_TX_DONE | RADIOLIB_SX127X_DIO1_FHSS_CHANGE_CHANNEL, 7, 4);
    }
    else {
      _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO0_TX_DONE, 7, 6);
    }

    // apply fixes to errata
    RADIOLIB_ERRATA_SX127X(false);

    // clear interrupt flags
    clearIRQFlags();

    // set packet length
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PAYLOAD_LENGTH, len);

    // set FIFO pointers
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_TX_BASE_ADDR, RADIOLIB_SX127X_FIFO_TX_BASE_ADDR_MAX);
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_ADDR_PTR, RADIOLIB_SX127X_FIFO_TX_BASE_ADDR_MAX);

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // check packet length
    if(len >= RADIOLIB_SX127X_MAX_PACKET_LENGTH_FSK) {
      return(RADIOLIB_ERR_PACKET_TOO_LONG);
    }

    // set DIO mapping
    _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO0_PACK_PACKET_SENT, 7, 6);

    // clear interrupt flags
    clearIRQFlags();

    // set packet length
    if (_packetLengthConfig == RADIOLIB_SX127X_PACKET_VARIABLE) {
      _mod->SPIwriteRegister(RADIOLIB_SX127X_REG_FIFO, len);
    }

    // check address filtering
    uint8_t filter = _mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, 2, 1);
    if((filter == RADIOLIB_SX127X_ADDRESS_FILTERING_NODE) || (filter == RADIOLIB_SX127X_ADDRESS_FILTERING_NODE_BROADCAST)) {
      _mod->SPIwriteRegister(RADIOLIB_SX127X_REG_FIFO, addr);
    }
  }

  // write packet to FIFO
  _mod->SPIwriteRegisterBurst(RADIOLIB_SX127X_REG_FIFO, data, len);

  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, HIGH);

  // start transmission
  state |= setMode(RADIOLIB_SX127X_TX);
  RADIOLIB_ASSERT(state);

  return(RADIOLIB_ERR_NONE);
}

int16_t SX127x::readData(uint8_t* data, size_t len) {
  int16_t modem = getActiveModem();

  // put module to standby
  standby();

  // get packet length
  size_t length = getPacketLength();
  size_t dumpLen = 0;
  if((len != 0) && (len < length)) {
    // user requested less data than we got, only return what was requested
    dumpLen = length - len;
    length = len;
  }

  if(modem == RADIOLIB_SX127X_LORA) {
    // check packet header integrity
    if(_crcEnabled && (_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_CHANNEL, 6, 6)) == 0) {
      // CRC is disabled according to packet header and enabled according to user
      // most likely damaged packet header
      clearIRQFlags();
      return(RADIOLIB_ERR_LORA_HEADER_DAMAGED);
    }

    // check payload CRC
    if(_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_IRQ_FLAGS, 5, 5) == RADIOLIB_SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR) {
      // clear interrupt flags
      clearIRQFlags();
      return(RADIOLIB_ERR_CRC_MISMATCH);
    }

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // check address filtering
    uint8_t filter = _mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, 2, 1);
    if((filter == RADIOLIB_SX127X_ADDRESS_FILTERING_NODE) || (filter == RADIOLIB_SX127X_ADDRESS_FILTERING_NODE_BROADCAST)) {
      _mod->SPIreadRegister(RADIOLIB_SX127X_REG_FIFO);
    }
  }

  // read packet data
  _mod->SPIreadRegisterBurst(RADIOLIB_SX127X_REG_FIFO, length, data);

  // dump the bytes that weren't requested
  if(dumpLen != 0) {
    clearFIFO(dumpLen);
  }

  // clear internal flag so getPacketLength can return the new packet length
  _packetLengthQueried = false;

  // clear interrupt flags
  clearIRQFlags();

  return(RADIOLIB_ERR_NONE);
}

int16_t SX127x::startChannelScan() {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // set DIO pin mapping
  state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO0_CAD_DONE | RADIOLIB_SX127X_DIO1_CAD_DETECTED, 7, 4);
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  clearIRQFlags();

  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // set mode to CAD
  state = setMode(RADIOLIB_SX127X_CAD);
  return(state);
}

int16_t SX127x::setSyncWord(uint8_t syncWord) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  setMode(RADIOLIB_SX127X_STANDBY);

  // write register
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_SYNC_WORD, syncWord));
}

int16_t SX127x::setCurrentLimit(uint8_t currentLimit) {
  // check allowed range
  if(!(((currentLimit >= 45) && (currentLimit <= 240)) || (currentLimit == 0))) {
    return(RADIOLIB_ERR_INVALID_CURRENT_LIMIT);
  }

  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);

  // set OCP limit
  uint8_t raw;
  if(currentLimit == 0) {
    // limit set to 0, disable OCP
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OCP, RADIOLIB_SX127X_OCP_OFF, 5, 5);
  } else if(currentLimit <= 120) {
    raw = (currentLimit - 45) / 5;
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OCP, RADIOLIB_SX127X_OCP_ON | raw, 5, 0);
  } else if(currentLimit <= 240) {
    raw = (currentLimit + 30) / 10;
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OCP, RADIOLIB_SX127X_OCP_ON | raw, 5, 0);
  }
  return(state);
}

int16_t SX127x::setPreambleLength(uint16_t preambleLength) {
  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // check active modem
  uint8_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    // check allowed range
    if(preambleLength < 6) {
      return(RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);
    }

    // set preamble length
    state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_MSB, (uint8_t)((preambleLength >> 8) & 0xFF));
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_LSB, (uint8_t)(preambleLength & 0xFF));
    return(state);

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // set preamble length (in bytes)
    uint16_t numBytes = preambleLength / 8;
    state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_MSB_FSK, (uint8_t)((numBytes >> 8) & 0xFF));
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_LSB_FSK, (uint8_t)(numBytes & 0xFF));
    return(state);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

float SX127x::getFrequencyError(bool autoCorrect) {
  int16_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    // get raw frequency error
    uint32_t raw = (uint32_t)_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_FEI_MSB, 3, 0) << 16;
    raw |= (uint16_t)_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_FEI_MID) << 8;
    raw |= _mod->SPIgetRegValue(RADIOLIB_SX127X_REG_FEI_LSB);

    uint32_t base = (uint32_t)2 << 23;
    float error;

    // check the first bit
    if(raw & 0x80000) {
      // frequency error is negative
      raw |= (uint32_t)0xFFF00000;
      raw = ~raw + 1;
      error = (((float)raw * (float)base)/32000000.0) * (_bw/500.0) * -1.0;
    } else {
      error = (((float)raw * (float)base)/32000000.0) * (_bw/500.0);
    }

    if(autoCorrect) {
      // adjust LoRa modem data rate
      float ppmOffset = 0.95 * (error/32.0);
      _mod->SPIwriteRegister(0x27, (uint8_t)ppmOffset);
    }

    return(error);

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // get raw frequency error
    uint16_t raw = (uint16_t)_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_FEI_MSB_FSK) << 8;
    raw |= _mod->SPIgetRegValue(RADIOLIB_SX127X_REG_FEI_LSB_FSK);

    uint32_t base = 1;
    float error;

    // check the first bit
    if(raw & 0x8000) {
      // frequency error is negative
      raw |= (uint32_t)0xFFF00000;
      raw = ~raw + 1;
      error = (float)raw * (32000000.0 / (float)(base << 19)) * -1.0;
    } else {
      error = (float)raw * (32000000.0 / (float)(base << 19));
    }

    return(error);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

float SX127x::getAFCError()
{
  // check active modem
  int16_t modem = getActiveModem();
  if(modem != RADIOLIB_SX127X_FSK_OOK) {
    return 0;
  }

  // get raw frequency error
  int16_t raw = (uint16_t)_mod->SPIreadRegister(RADIOLIB_SX127X_REG_AFC_MSB) << 8;
  raw |= _mod->SPIreadRegister(RADIOLIB_SX127X_REG_AFC_LSB);

  uint32_t base = 1;
  return raw * (32000000.0 / (float)(base << 19));
}

float SX127x::getSNR() {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    return(0);
  }

  // get SNR value
  int8_t rawSNR = (int8_t)_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PKT_SNR_VALUE);
  return(rawSNR / 4.0);
}

float SX127x::getDataRate() const {
  return(_dataRate);
}

int16_t SX127x::setBitRate(float br) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check allowed bit rate
  if(_ook) {
    RADIOLIB_CHECK_RANGE(br, 1.2, 32.768, RADIOLIB_ERR_INVALID_BIT_RATE);
  } else {
    RADIOLIB_CHECK_RANGE(br, 1.2, 300.0, RADIOLIB_ERR_INVALID_BIT_RATE);
  }

  // set mode to STANDBY
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // set bit rate
  uint16_t bitRate = (RADIOLIB_SX127X_CRYSTAL_FREQ * 1000.0) / br;
  state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_BITRATE_MSB, (bitRate & 0xFF00) >> 8, 7, 0);
  state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_BITRATE_LSB, bitRate & 0x00FF, 7, 0);

  /// \todo fractional part of bit rate setting (not in OOK)
  if(state == RADIOLIB_ERR_NONE) {
    SX127x::_br = br;
  }
  return(state);
}

int16_t SX127x::setFrequencyDeviation(float freqDev) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set frequency deviation to lowest available setting (required for digimodes)
  float newFreqDev = freqDev;
  if(freqDev < 0.0) {
    newFreqDev = 0.6;
  }

  // check frequency deviation range
  if(!((newFreqDev + _br/2.0 <= 250.0) && (freqDev <= 200.0))) {
    return(RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);
  }

  // set mode to STANDBY
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // set allowed frequency deviation
  uint32_t base = 1;
  uint32_t FDEV = (newFreqDev * (base << 19)) / 32000;
  state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FDEV_MSB, (FDEV & 0xFF00) >> 8, 5, 0);
  state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FDEV_LSB, FDEV & 0x00FF, 7, 0);
  return(state);
}

uint8_t SX127x::calculateBWManExp(float bandwidth)
{
  for(uint8_t e = 7; e >= 1; e--) {
    for(int8_t m = 2; m >= 0; m--) {
      float point = (RADIOLIB_SX127X_CRYSTAL_FREQ * 1000000.0)/(((4 * m) + 16) * ((uint32_t)1 << (e + 2)));
      if(fabs(bandwidth - ((point / 1000.0) + 0.05)) <= 0.5) {
        return((m << 3) | e);
      }
    }
  }
  return 0;
}

int16_t SX127x::setRxBandwidth(float rxBw) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(rxBw, 2.6, 250.0, RADIOLIB_ERR_INVALID_RX_BANDWIDTH);

  // set mode to STANDBY
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // set Rx bandwidth
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_BW, calculateBWManExp(rxBw), 4, 0));
}

int16_t SX127x::setAFCBandwidth(float rxBw){
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK){
      return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(rxBw, 2.6, 250.0, RADIOLIB_ERR_INVALID_RX_BANDWIDTH);

  // set mode to STANDBY
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // set AFC bandwidth
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_AFC_BW, calculateBWManExp(rxBw), 4, 0));
}

int16_t SX127x::setAFC(bool isEnabled){
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  //set AFC auto on/off
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_CONFIG, isEnabled ? RADIOLIB_SX127X_AFC_AUTO_ON : RADIOLIB_SX127X_AFC_AUTO_OFF, 4, 4));
}

int16_t SX127x::setAFCAGCTrigger(uint8_t trigger){
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  //set AFC&AGC trigger
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_CONFIG, trigger, 2, 0));
}

int16_t SX127x::setSyncWord(uint8_t* syncWord, size_t len) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(len, 1, 8, RADIOLIB_ERR_INVALID_SYNC_WORD);

  // sync word must not contain value 0x00
  for(size_t i = 0; i < len; i++) {
    if(syncWord[i] == 0x00) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }
  }

  // enable sync word recognition
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_SYNC_CONFIG, RADIOLIB_SX127X_SYNC_ON, 4, 4);
  state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_SYNC_CONFIG, len - 1, 2, 0);
  RADIOLIB_ASSERT(state);

  // set sync word
  _mod->SPIwriteRegisterBurst(RADIOLIB_SX127X_REG_SYNC_VALUE_1, syncWord, len);
  return(RADIOLIB_ERR_NONE);
}

int16_t SX127x::setNodeAddress(uint8_t nodeAddr) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // enable address filtering (node only)
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_ADDRESS_FILTERING_NODE, 2, 1);
  RADIOLIB_ASSERT(state);

  // set node address
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_NODE_ADRS, nodeAddr));
}

int16_t SX127x::setBroadcastAddress(uint8_t broadAddr) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // enable address filtering (node + broadcast)
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_ADDRESS_FILTERING_NODE_BROADCAST, 2, 1);
  RADIOLIB_ASSERT(state);

  // set broadcast address
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_BROADCAST_ADRS, broadAddr));
}

int16_t SX127x::disableAddressFiltering() {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // disable address filtering
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_ADDRESS_FILTERING_OFF, 2, 1);
  RADIOLIB_ASSERT(state);

  // set node address to default (0x00)
  state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_NODE_ADRS, 0x00);
  RADIOLIB_ASSERT(state);

  // set broadcast address to default (0x00)
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_BROADCAST_ADRS, 0x00));
}

int16_t SX127x::setOokThresholdType(uint8_t type) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OOK_PEAK, type, 4, 3, 5));
}

int16_t SX127x::setOokFixedOrFloorThreshold(uint8_t value) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OOK_FIX, value, 7, 0, 5));
}

int16_t SX127x::setOokPeakThresholdDecrement(uint8_t value) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OOK_AVG, value, 7, 5, 5));
}

int16_t SX127x::setOokPeakThresholdStep(uint8_t value) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OOK_PEAK, value, 2, 0, 5));
}

int16_t SX127x::enableBitSync() {
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OOK_PEAK, RADIOLIB_SX127X_BIT_SYNC_ON, 5, 5, 5));
}

int16_t SX127x::disableBitSync() {
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OOK_PEAK, RADIOLIB_SX127X_BIT_SYNC_OFF, 5, 5, 5));
}

int16_t SX127x::setOOK(bool enableOOK) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set OOK and if successful, save the new setting
  int16_t state = RADIOLIB_ERR_NONE;
  if(enableOOK) {
    state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, RADIOLIB_SX127X_MODULATION_OOK, 6, 5, 5);
    state |= SX127x::setAFCAGCTrigger(RADIOLIB_SX127X_RX_TRIGGER_RSSI_INTERRUPT);
  } else {
    state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, RADIOLIB_SX127X_MODULATION_FSK, 6, 5, 5);
    state |= SX127x::setAFCAGCTrigger(RADIOLIB_SX127X_RX_TRIGGER_BOTH);
  }
  if(state == RADIOLIB_ERR_NONE) {
    _ook = enableOOK;
  }

  return(state);
}

int16_t SX127x::setFrequencyRaw(float newFreq) {
  int16_t state = RADIOLIB_ERR_NONE;

  // set mode to standby if not FHSS
  if(_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD) == RADIOLIB_SX127X_HOP_PERIOD_OFF) {
    state = setMode(RADIOLIB_SX127X_STANDBY);
  }

  // calculate register values
  uint32_t FRF = (newFreq * (uint32_t(1) << RADIOLIB_SX127X_DIV_EXPONENT)) / RADIOLIB_SX127X_CRYSTAL_FREQ;

  // write registers
  state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FRF_MSB, (FRF & 0xFF0000) >> 16);
  state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FRF_MID, (FRF & 0x00FF00) >> 8);
  state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FRF_LSB, FRF & 0x0000FF);
  return(state);
}

size_t SX127x::getPacketLength(bool update) {
  int16_t modem = getActiveModem();

  if(modem == RADIOLIB_SX127X_LORA) {
    if(_sf != 6) {
      // get packet length for SF7 - SF12
      return(_mod->SPIreadRegister(RADIOLIB_SX127X_REG_RX_NB_BYTES));

    } else {
      // return the cached value for SF6
      return(_packetLength);
    }

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // get packet length
    if(!_packetLengthQueried && update) {
      if (_packetLengthConfig == RADIOLIB_SX127X_PACKET_VARIABLE) {
        _packetLength = _mod->SPIreadRegister(RADIOLIB_SX127X_REG_FIFO);
      } else {
        _packetLength = _mod->SPIreadRegister(RADIOLIB_SX127X_REG_PAYLOAD_LENGTH_FSK);
      }
      _packetLengthQueried = true;
    }
  }

  return(_packetLength);
}

int16_t SX127x::fixedPacketLengthMode(uint8_t len) {
  return(SX127x::setPacketMode(RADIOLIB_SX127X_PACKET_FIXED, len));
}

int16_t SX127x::variablePacketLengthMode(uint8_t maxLen) {
  return(SX127x::setPacketMode(RADIOLIB_SX127X_PACKET_VARIABLE, maxLen));
}

int16_t SX127x::setRSSIConfig(uint8_t smoothingSamples, int8_t offset) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check provided values
  if(!(smoothingSamples <= 7)) {
    return(RADIOLIB_ERR_INVALID_NUM_SAMPLES);
  }

  RADIOLIB_CHECK_RANGE(offset, -16, 15, RADIOLIB_ERR_INVALID_RSSI_OFFSET);

  // set new register values
  state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RSSI_CONFIG, offset, 7, 3);
  state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RSSI_CONFIG, smoothingSamples, 2, 0);
  return(state);
}

int16_t SX127x::setEncoding(uint8_t encoding) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set encoding
  switch(encoding) {
    case RADIOLIB_ENCODING_NRZ:
      return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_DC_FREE_NONE, 6, 5));
    case RADIOLIB_ENCODING_MANCHESTER:
      return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_DC_FREE_MANCHESTER, 6, 5));
    case RADIOLIB_ENCODING_WHITENING:
      return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_DC_FREE_WHITENING, 6, 5));
    default:
      return(RADIOLIB_ERR_INVALID_ENCODING);
  }
}

uint16_t SX127x::getIRQFlags() {
  // check active modem
  if(getActiveModem() == RADIOLIB_SX127X_LORA) {
    // LoRa, just 8-bit value
    return((uint16_t)_mod->SPIreadRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS));

  } else {
    // FSK, the IRQ flags are 16 bits in total
    uint16_t flags = ((uint16_t)_mod->SPIreadRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS_2)) << 8;
    flags |= (uint16_t)_mod->SPIreadRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS_1);
    return(flags);
  }

}

uint8_t SX127x::getModemStatus() {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    return(0x00);
  }

  // read the register
  return(_mod->SPIreadRegister(RADIOLIB_SX127X_REG_MODEM_STAT));
}

void SX127x::setRfSwitchPins(RADIOLIB_PIN_TYPE rxEn, RADIOLIB_PIN_TYPE txEn) {
  _mod->setRfSwitchPins(rxEn, txEn);
}

uint8_t SX127x::randomByte() {
  // check active modem
  uint8_t rssiValueReg = RADIOLIB_SX127X_REG_RSSI_WIDEBAND;
  if(getActiveModem() == RADIOLIB_SX127X_FSK_OOK) {
    rssiValueReg = RADIOLIB_SX127X_REG_RSSI_VALUE_FSK;
  }

  // set mode to Rx
  setMode(RADIOLIB_SX127X_RX);

  // wait a bit for the RSSI reading to stabilise
  _mod->delay(10);

  // read RSSI value 8 times, always keep just the least significant bit
  uint8_t randByte = 0x00;
  for(uint8_t i = 0; i < 8; i++) {
    randByte |= ((_mod->SPIreadRegister(rssiValueReg) & 0x01) << i);
  }

  // set mode to standby
  setMode(RADIOLIB_SX127X_STANDBY);

  return(randByte);
}

int16_t SX127x::getChipVersion() {
  return(_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_VERSION));
}

int8_t SX127x::getTempRaw() {
  int8_t temp = 0;
  uint8_t previousOpMode;
  uint8_t ival;

  // save current Op Mode
  previousOpMode = _mod->SPIgetRegValue(RADIOLIB_SX127X_REG_OP_MODE);

  // check if we need to step out of LoRa mode first
  if ((previousOpMode & RADIOLIB_SX127X_LORA) == RADIOLIB_SX127X_LORA) {
    _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, (RADIOLIB_SX127X_LORA | RADIOLIB_SX127X_SLEEP));
  }

  // put device in FSK sleep
  _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, (RADIOLIB_SX127X_FSK_OOK | RADIOLIB_SX127X_SLEEP));

  // put device in FSK RxSynth
  _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, (RADIOLIB_SX127X_FSK_OOK | RADIOLIB_SX127X_FSRX));

  // enable temperature reading
  _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_IMAGE_CAL, RADIOLIB_SX127X_TEMP_MONITOR_ON, 0, 0);

  // wait
  _mod->delayMicroseconds(200);

  // disable temperature reading
  _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_IMAGE_CAL, RADIOLIB_SX127X_TEMP_MONITOR_OFF, 0, 0);

  // put device in FSK sleep
  _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, (RADIOLIB_SX127X_FSK_OOK | RADIOLIB_SX127X_SLEEP));

  // read temperature
  ival = _mod->SPIgetRegValue(RADIOLIB_SX127X_REG_TEMP);

  // convert very raw value
  if ((ival & 0x80) == 0x80) {
    temp = 255 - ival;
  } else {
    temp = -1 * ival;
  }

  // check if we need to step back into LoRa mode
  if ((previousOpMode & RADIOLIB_SX127X_LORA) == RADIOLIB_SX127X_LORA) {
    _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, (RADIOLIB_SX127X_LORA | RADIOLIB_SX127X_SLEEP));
  }

  // reload previous Op Mode
  _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, previousOpMode);

  return(temp);
}

int16_t SX127x::config() {
  // turn off frequency hopping
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD, RADIOLIB_SX127X_HOP_PERIOD_OFF);
  return(state);
}

int16_t SX127x::configFSK() {
  // set RSSI threshold
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RSSI_THRESH, RADIOLIB_SX127X_RSSI_THRESHOLD);
  RADIOLIB_ASSERT(state);

  // reset FIFO flag
  _mod->SPIwriteRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS_2, RADIOLIB_SX127X_FLAG_FIFO_OVERRUN);

  // set packet configuration
  state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_PACKET_VARIABLE | RADIOLIB_SX127X_DC_FREE_NONE | RADIOLIB_SX127X_CRC_ON | RADIOLIB_SX127X_CRC_AUTOCLEAR_ON | RADIOLIB_SX127X_ADDRESS_FILTERING_OFF | RADIOLIB_SX127X_CRC_WHITENING_TYPE_CCITT, 7, 0);
  state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_2, RADIOLIB_SX127X_DATA_MODE_PACKET | RADIOLIB_SX127X_IO_HOME_OFF, 6, 5);
  RADIOLIB_ASSERT(state);

  // set preamble polarity
  state =_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_SYNC_CONFIG, RADIOLIB_SX127X_PREAMBLE_POLARITY_55, 5, 5);
  RADIOLIB_ASSERT(state);

  // set FIFO threshold
  state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_THRESH, RADIOLIB_SX127X_TX_START_FIFO_NOT_EMPTY, 7, 7);
  state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_THRESH, RADIOLIB_SX127X_FIFO_THRESH, 5, 0);
  RADIOLIB_ASSERT(state);

  // disable Rx timeouts
  state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_TIMEOUT_1, RADIOLIB_SX127X_TIMEOUT_RX_RSSI_OFF);
  state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_TIMEOUT_2, RADIOLIB_SX127X_TIMEOUT_RX_PREAMBLE_OFF);
  state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_TIMEOUT_3, RADIOLIB_SX127X_TIMEOUT_SIGNAL_SYNC_OFF);
  RADIOLIB_ASSERT(state);

  // enable preamble detector
  state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_DETECT, RADIOLIB_SX127X_PREAMBLE_DETECTOR_ON | RADIOLIB_SX127X_PREAMBLE_DETECTOR_2_BYTE | RADIOLIB_SX127X_PREAMBLE_DETECTOR_TOL);

  return(state);
}

int16_t SX127x::setPacketMode(uint8_t mode, uint8_t len) {
  // check packet length
  if (len > RADIOLIB_SX127X_MAX_PACKET_LENGTH_FSK) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set to fixed packet length
  int16_t state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, mode, 7, 7);
  RADIOLIB_ASSERT(state);

  // set length to register
  state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PAYLOAD_LENGTH_FSK, len);
  RADIOLIB_ASSERT(state);

  // update cached value
  _packetLengthConfig = mode;
  return(state);
}

bool SX127x::findChip(uint8_t ver) {
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    // reset the module
    reset();

    // check version register
    int16_t version = getChipVersion();
    if(version == ver) {
      flagFound = true;
    } else {
      #if defined(RADIOLIB_DEBUG)
        RADIOLIB_DEBUG_PRINT(F("SX127x not found! ("));
        RADIOLIB_DEBUG_PRINT(i + 1);
        RADIOLIB_DEBUG_PRINT(F(" of 10 tries) RADIOLIB_SX127X_REG_VERSION == "));

        char buffHex[12];
        sprintf(buffHex, "0x%04X", version);
        RADIOLIB_DEBUG_PRINT(buffHex);
        RADIOLIB_DEBUG_PRINT(F(", expected 0x00"));
        RADIOLIB_DEBUG_PRINTLN(ver, HEX);
      #endif
      _mod->delay(10);
      i++;
    }
  }

  return(flagFound);
}

int16_t SX127x::setMode(uint8_t mode) {
  uint8_t checkMask = 0xFF;
  if((getActiveModem() == RADIOLIB_SX127X_FSK_OOK) && (mode == RADIOLIB_SX127X_RX)) {
    // disable checking of RX bit in FSK RX mode, as it sometimes seem to fail (#276)
    checkMask = 0xFE;
  }
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, mode, 2, 0, 5, checkMask));
}

int16_t SX127x::getActiveModem() {
  return(_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_OP_MODE, 7, 7));
}

int16_t SX127x::setActiveModem(uint8_t modem) {
  // set mode to SLEEP
  int16_t state = setMode(RADIOLIB_SX127X_SLEEP);

  // set modem
  state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, modem, 7, 7, 5);

  // set mode to STANDBY
  state |= setMode(RADIOLIB_SX127X_STANDBY);
  return(state);
}

void SX127x::clearIRQFlags() {
  int16_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    _mod->SPIwriteRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS, 0b11111111);
  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    _mod->SPIwriteRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS_1, 0b11111111);
    _mod->SPIwriteRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS_2, 0b11111111);
  }
}

void SX127x::clearFIFO(size_t count) {
  while(count) {
    _mod->SPIreadRegister(RADIOLIB_SX127X_REG_FIFO);
    count--;
  }
}

int16_t SX127x::invertIQ(bool invertIQ) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  int16_t state;
  if(invertIQ) {
    state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_INVERT_IQ, RADIOLIB_SX127X_INVERT_IQ_RXPATH_ON, 6, 6);
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_INVERT_IQ, RADIOLIB_SX127X_INVERT_IQ_TXPATH_ON, 0, 0);
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_INVERT_IQ2, RADIOLIB_SX127X_IQ2_ENABLE);
  } else {
    state = _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_INVERT_IQ, RADIOLIB_SX127X_INVERT_IQ_RXPATH_OFF, 6, 6);
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_INVERT_IQ, RADIOLIB_SX127X_INVERT_IQ_TXPATH_OFF, 0, 0);
    state |= _mod->SPIsetRegValue(RADIOLIB_SX127X_REG_INVERT_IQ2, RADIOLIB_SX127X_IQ2_DISABLE);
  }

  return(state);
}

void SX127x::setDirectAction(void (*func)(void)) {
  setDio1Action(func);
}

void SX127x::readBit(RADIOLIB_PIN_TYPE pin) {
  updateDirectBuffer((uint8_t)digitalRead(pin));
}

int16_t SX127x::setFHSSHoppingPeriod(uint8_t freqHoppingPeriod) {
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD, freqHoppingPeriod));
}

uint8_t SX127x::getFHSSHoppingPeriod(void) {
  return(_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD));
}

uint8_t SX127x::getFHSSChannel(void) {
  return(_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_CHANNEL, 5, 0));
}

void SX127x::clearFHSSInt(void) {
  int16_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    _mod->SPIwriteRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS, getIRQFlags() | RADIOLIB_SX127X_CLEAR_IRQ_FLAG_FHSS_CHANGE_CHANNEL);
  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    return; //These are not the interrupts you are looking for
  }
}

#endif
