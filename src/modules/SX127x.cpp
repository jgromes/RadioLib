#include "SX127x.h"

SX127x::SX127x(Module* mod) : PhysicalLayer(SX127X_CRYSTAL_FREQ, SX127X_DIV_EXPONENT, SX127X_MAX_PACKET_LENGTH) {
  _mod = mod;
  _packetLengthQueried = false;
}

int16_t SX127x::begin(uint8_t chipVersion, uint8_t syncWord, uint8_t currentLimit, uint16_t preambleLength) {
  // set module properties
  _mod->init(RADIOLIB_USE_SPI, RADIOLIB_INT_BOTH);

  // try to find the SX127x chip
  if(!SX127x::findChip(chipVersion)) {
    RADIOLIB_DEBUG_PRINTLN(F("No SX127x found!"));
    _mod->term();
    return(ERR_CHIP_NOT_FOUND);
  } else {
    RADIOLIB_DEBUG_PRINTLN(F("Found SX127x!"));
  }

  // check active modem
  int16_t state;
  if(getActiveModem() != SX127X_LORA) {
    // set LoRa mode
    state = setActiveModem(SX127X_LORA);
    if(state != ERR_NONE) {
      return(state);
    }
  }

  // set LoRa sync word
  state = SX127x::setSyncWord(syncWord);
  if(state != ERR_NONE) {
    return(state);
  }

  // set over current protection
  state = SX127x::setCurrentLimit(currentLimit);
  if(state != ERR_NONE) {
    return(state);
  }

  // set preamble length
  state = SX127x::setPreambleLength(preambleLength);

  // initalize internal variables
  _dataRate = 0.0;

  return(state);
}

int16_t SX127x::beginFSK(uint8_t chipVersion, float br, float freqDev, float rxBw, uint8_t currentLimit, uint16_t preambleLength, bool enableOOK) {
  // set module properties
  _mod->init(RADIOLIB_USE_SPI, RADIOLIB_INT_BOTH);

  // try to find the SX127x chip
  if(!SX127x::findChip(chipVersion)) {
    RADIOLIB_DEBUG_PRINTLN(F("No SX127x found!"));
    _mod->term();
    return(ERR_CHIP_NOT_FOUND);
  } else {
    RADIOLIB_DEBUG_PRINTLN(F("Found SX127x!"));
  }

  // check currently active modem
  int16_t state;
  if(getActiveModem() != SX127X_FSK_OOK) {
    // set FSK mode
    state = setActiveModem(SX127X_FSK_OOK);
    if(state != ERR_NONE) {
      return(state);
    }
  }

  // set bit rate
  state = SX127x::setBitRate(br);
  if(state != ERR_NONE) {
    return(state);
  }

  // set frequency deviation
  state = SX127x::setFrequencyDeviation(freqDev);
  if(state != ERR_NONE) {
    return(state);
  }

  // set receiver bandwidth
  state = SX127x::setRxBandwidth(rxBw);
  if(state != ERR_NONE) {
    return(state);
  }

  // set over current protection
  state = SX127x::setCurrentLimit(currentLimit);
  if(state != ERR_NONE) {
    return(state);
  }

  // set preamble length
  state = SX127x::setPreambleLength(preambleLength);
  if(state != ERR_NONE) {
    return(state);
  }

  // default sync word value 0x2D01 is the same as the default in LowPowerLab RFM69 library
  uint8_t syncWord[] = {0x2D, 0x01};
  state = setSyncWord(syncWord, 2);
  if(state != ERR_NONE) {
    return(state);
  }

  // disable address filtering
  state = disableAddressFiltering();
  if(state != ERR_NONE) {
    return(state);
  }

  // enable/disable OOK
  state = setOOK(enableOOK);
  if(state != ERR_NONE) {
    return(state);
  }

  // set default RSSI measurement config
  state = setRSSIConfig(2);
  if(state != ERR_NONE) {
    return(state);
  }

  // set default encoding
  state = setEncoding(0);

  return(state);
}

int16_t SX127x::transmit(uint8_t* data, size_t len, uint8_t addr) {
  // set mode to standby
  int16_t state = setMode(SX127X_STANDBY);

  int16_t modem = getActiveModem();
  uint32_t start = 0;
  if(modem == SX127X_LORA) {
    // calculate timeout (150 % of expected time-one-air)
    float symbolLength = (float)(uint32_t(1) <<_sf) / (float)_bw;
    float de = 0;
    if(symbolLength >= 16.0) {
      de = 1;
    }
    float ih = (float)_mod->SPIgetRegValue(SX127X_REG_MODEM_CONFIG_1, 0, 0);
    float crc = (float)(_mod->SPIgetRegValue(SX127X_REG_MODEM_CONFIG_2, 2, 2) >> 2);
    float n_pre = (float)((_mod->SPIgetRegValue(SX127X_REG_PREAMBLE_MSB) << 8) | _mod->SPIgetRegValue(SX127X_REG_PREAMBLE_LSB));
    float n_pay = 8.0 + max(ceil((8.0 * (float)len - 4.0 * (float)_sf + 28.0 + 16.0 * crc - 20.0 * ih)/(4.0 * (float)_sf - 8.0 * de)) * (float)_cr, 0.0);
    uint32_t timeout = ceil(symbolLength * (n_pre + n_pay + 4.25) * 1500.0);

    // start transmission
    state = startTransmit(data, len, addr);
    if(state != ERR_NONE) {
      return(state);
    }

    // wait for packet transmission or timeout
    start = micros();
    while(!digitalRead(_mod->getInt0())) {
      if(micros() - start > timeout) {
        clearIRQFlags();
        return(ERR_TX_TIMEOUT);
      }
    }

  } else if(modem == SX127X_FSK_OOK) {
    // calculate timeout (5ms + 500 % of expected time-on-air)
    uint32_t timeout = 5000000 + (uint32_t)((((float)(len * 8)) / (_br * 1000.0)) * 5000000.0);

    // start transmission
    state = startTransmit(data, len, addr);
    if(state != ERR_NONE) {
      return(state);
    }

    // wait for transmission end or timeout
    start = micros();
    while(!digitalRead(_mod->getInt0())) {
      if(micros() - start > timeout) {
        clearIRQFlags();
        standby();
        return(ERR_TX_TIMEOUT);
      }
    }
  } else {
    return(ERR_UNKNOWN);
  }

  // update data rate
  uint32_t elapsed = micros() - start;
  _dataRate = (len*8.0)/((float)elapsed/1000000.0);

  // clear interrupt flags
  clearIRQFlags();

  // set mode to standby to disable transmitter
  return(standby());
}

int16_t SX127x::receive(uint8_t* data, size_t len) {
  // set mode to standby
  int16_t state = setMode(SX127X_STANDBY);

  int16_t modem = getActiveModem();
  if(modem == SX127X_LORA) {
    // set mode to receive
    state = startReceive(len, SX127X_RXSINGLE);
    if(state != ERR_NONE) {
      return(state);
    }

    // wait for packet reception or timeout (100 LoRa symbols)
    while(!digitalRead(_mod->getInt0())) {
      if(digitalRead(_mod->getInt1())) {
        clearIRQFlags();
        return(ERR_RX_TIMEOUT);
      }
    }

  } else if(modem == SX127X_FSK_OOK) {
    // calculate timeout (500 % of expected time-one-air)
    uint32_t timeout = (uint32_t)((((float)(len * 8)) / (_br * 1000.0)) * 5000000.0);

    // set mode to receive
    state = startReceive(len, SX127X_RX);
    if(state != ERR_NONE) {
      return(state);
    }

    // wait for packet reception or timeout
    uint32_t start = micros();
    while(!digitalRead(_mod->getInt0())) {
      if(micros() - start > timeout) {
        clearIRQFlags();
        return(ERR_RX_TIMEOUT);
      }
    }
  }

  // read the received data
  state = readData(data, len);

  return(state);
}

int16_t SX127x::scanChannel() {
  // check active modem
  if(getActiveModem() != SX127X_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = setMode(SX127X_STANDBY);

  // set DIO pin mapping
  state |= _mod->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_CAD_DONE | SX127X_DIO1_CAD_DETECTED, 7, 4);

  // clear interrupt flags
  clearIRQFlags();

  // set mode to CAD
  state |= setMode(SX127X_CAD);
  if(state != ERR_NONE) {
    return(state);
  }

  // wait for channel activity detected or timeout
  while(!digitalRead(_mod->getInt0())) {
    if(digitalRead(_mod->getInt1())) {
      clearIRQFlags();
      return(PREAMBLE_DETECTED);
    }
  }

  // clear interrupt flags
  clearIRQFlags();

  return(CHANNEL_FREE);
}

int16_t SX127x::sleep() {
  // set mode to sleep
  return(setMode(SX127X_SLEEP));
}

int16_t SX127x::standby() {
  // set mode to standby
  return(setMode(SX127X_STANDBY));
}

int16_t SX127x::transmitDirect(uint32_t FRF) {
  // check modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // user requested to start transmitting immediately (required for RTTY)
  if(FRF != 0) {
    _mod->SPIwriteRegister(SX127X_REG_FRF_MSB, (FRF & 0xFF0000) >> 16);
    _mod->SPIwriteRegister(SX127X_REG_FRF_MID, (FRF & 0x00FF00) >> 8);
    _mod->SPIwriteRegister(SX127X_REG_FRF_LSB, FRF & 0x0000FF);

    return(setMode(SX127X_TX));
  }

  // activate direct mode
  int16_t state = directMode();
  if(state != ERR_NONE) {
    return(state);
  }

  // start transmitting
  return(setMode(SX127X_TX));
}

int16_t SX127x::receiveDirect() {
  // check modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // activate direct mode
  int16_t state = directMode();
  if(state != ERR_NONE) {
    return(state);
  }

  // start receiving
  return(setMode(SX127X_RX));
}

int16_t SX127x::directMode() {
  // set mode to standby
  int16_t state = setMode(SX127X_STANDBY);
  if(state != ERR_NONE) {
    return(state);
  }

  // set DIO mapping
  state = _mod->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO1_CONT_DCLK | SX127X_DIO2_CONT_DATA, 5, 2);

  // set continuous mode
  state |= _mod->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_2, SX127X_DATA_MODE_CONTINUOUS, 6, 6);
  return(state);
}

int16_t SX127x::packetMode() {
  // check modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  return(_mod->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_2, SX127X_DATA_MODE_PACKET, 6, 6));
}

int16_t SX127x::startReceive(uint8_t len, uint8_t mode) {
  // set mode to standby
  int16_t state = setMode(SX127X_STANDBY);

  int16_t modem = getActiveModem();
  if(modem == SX127X_LORA) {
    // set DIO pin mapping
    state |= _mod->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_RX_DONE | SX127X_DIO1_RX_TIMEOUT, 7, 4);

    // set expected packet length for SF6
    if(_sf == 6) {
      state |= _mod->SPIsetRegValue(SX127X_REG_PAYLOAD_LENGTH, len);
    }

    // clear interrupt flags
    clearIRQFlags();

    // set FIFO pointers
    state |= _mod->SPIsetRegValue(SX127X_REG_FIFO_RX_BASE_ADDR, SX127X_FIFO_RX_BASE_ADDR_MAX);
    state |= _mod->SPIsetRegValue(SX127X_REG_FIFO_ADDR_PTR, SX127X_FIFO_RX_BASE_ADDR_MAX);
    if(state != ERR_NONE) {
      return(state);
    }

  } else if(modem == SX127X_FSK_OOK) {
    // set DIO pin mapping
    state |= _mod->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_PACK_PAYLOAD_READY, 7, 6);

    // clear interrupt flags
    clearIRQFlags();

    // FSK modem does not distinguish between Rx single and continuous
    if(mode == SX127X_RXCONTINUOUS) {
      return(setMode(SX127X_RX));
    }
  }

  // set mode to receive
  return(setMode(mode));
}

void SX127x::setDio0Action(void (*func)(void)) {
  attachInterrupt(digitalPinToInterrupt(_mod->getInt0()), func, RISING);
}

void SX127x::setDio1Action(void (*func)(void)) {
  attachInterrupt(digitalPinToInterrupt(_mod->getInt1()), func, RISING);
}

int16_t SX127x::startTransmit(uint8_t* data, size_t len, uint8_t addr) {
  // set mode to standby
  int16_t state = setMode(SX127X_STANDBY);

  int16_t modem = getActiveModem();
  if(modem == SX127X_LORA) {
    // check packet length
    if(len >= 256) {
      return(ERR_PACKET_TOO_LONG);
    }

    // set DIO mapping
    _mod->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_TX_DONE, 7, 6);

    // clear interrupt flags
    clearIRQFlags();

    // set packet length
    state |= _mod->SPIsetRegValue(SX127X_REG_PAYLOAD_LENGTH, len);

    // set FIFO pointers
    state |= _mod->SPIsetRegValue(SX127X_REG_FIFO_TX_BASE_ADDR, SX127X_FIFO_TX_BASE_ADDR_MAX);
    state |= _mod->SPIsetRegValue(SX127X_REG_FIFO_ADDR_PTR, SX127X_FIFO_TX_BASE_ADDR_MAX);

    // write packet to FIFO
    _mod->SPIwriteRegisterBurst(SX127X_REG_FIFO, data, len);

    // start transmission
    state |= setMode(SX127X_TX);
    if(state != ERR_NONE) {
      return(state);
    }

    return(ERR_NONE);

  } else if(modem == SX127X_FSK_OOK) {
    // check packet length
    if(len >= 64) {
      return(ERR_PACKET_TOO_LONG);
    }

    // set DIO mapping
    _mod->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_PACK_PACKET_SENT, 7, 6);

    // clear interrupt flags
    clearIRQFlags();

    // set packet length
    _mod->SPIwriteRegister(SX127X_REG_FIFO, len);

    // check address filtering
    uint8_t filter = _mod->SPIgetRegValue(SX127X_REG_PACKET_CONFIG_1, 2, 1);
    if((filter == SX127X_ADDRESS_FILTERING_NODE) || (filter == SX127X_ADDRESS_FILTERING_NODE_BROADCAST)) {
      _mod->SPIwriteRegister(SX127X_REG_FIFO, addr);
    }

    // write packet to FIFO
    _mod->SPIwriteRegisterBurst(SX127X_REG_FIFO, data, len);

    // start transmission
    state |= setMode(SX127X_TX);
    if(state != ERR_NONE) {
      return(state);
    }

    return(ERR_NONE);
  }

  return(ERR_UNKNOWN);
}

int16_t SX127x::readData(uint8_t* data, size_t len) {
  int16_t modem = getActiveModem();
  size_t length = len;

  // put module to standby
  standby();

  if(modem == SX127X_LORA) {
    // len set to maximum indicates unknown packet length, read the number of actually received bytes
    if(len == SX127X_MAX_PACKET_LENGTH) {
      length = getPacketLength();
    }

    // check integrity CRC
    if(_mod->SPIgetRegValue(SX127X_REG_IRQ_FLAGS, 5, 5) == SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR) {
      // clear interrupt flags
      clearIRQFlags();

      return(ERR_CRC_MISMATCH);
    }

  } else if(modem == SX127X_FSK_OOK) {
    // read packet length (always required in FSK)
    length = getPacketLength();

    // check address filtering
    uint8_t filter = _mod->SPIgetRegValue(SX127X_REG_PACKET_CONFIG_1, 2, 1);
    if((filter == SX127X_ADDRESS_FILTERING_NODE) || (filter == SX127X_ADDRESS_FILTERING_NODE_BROADCAST)) {
      _mod->SPIreadRegister(SX127X_REG_FIFO);
    }
  }

  // read packet data
  _mod->SPIreadRegisterBurst(SX127X_REG_FIFO, length, data);

  // dump bytes that weren't requested
  size_t packetLength = getPacketLength();
  if(packetLength > length) {
    clearFIFO(packetLength - length);
  }

  // clear internal flag so getPacketLength can return the new packet length
  _packetLengthQueried = false;

  // clear interrupt flags
  clearIRQFlags();

  return(ERR_NONE);
}

int16_t SX127x::setSyncWord(uint8_t syncWord) {
  // check active modem
  if(getActiveModem() != SX127X_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // set mode to standby
  setMode(SX127X_STANDBY);

  // write register
  return(_mod->SPIsetRegValue(SX127X_REG_SYNC_WORD, syncWord));
}

int16_t SX127x::setCurrentLimit(uint8_t currentLimit) {
  // check allowed range
  if(!(((currentLimit >= 45) && (currentLimit <= 240)) || (currentLimit == 0))) {
    return(ERR_INVALID_CURRENT_LIMIT);
  }

  // set mode to standby
  int16_t state = setMode(SX127X_STANDBY);

  // set OCP limit
  uint8_t raw;
  if(currentLimit == 0) {
    // limit set to 0, disable OCP
    state |= _mod->SPIsetRegValue(SX127X_REG_OCP, SX127X_OCP_OFF, 5, 5);
  } else if(currentLimit <= 120) {
    raw = (currentLimit - 45) / 5;
    state |= _mod->SPIsetRegValue(SX127X_REG_OCP, SX127X_OCP_ON | raw, 5, 0);
  } else if(currentLimit <= 240) {
    raw = (currentLimit + 30) / 10;
    state |= _mod->SPIsetRegValue(SX127X_REG_OCP, SX127X_OCP_ON | raw, 5, 0);
  }
  return(state);
}

int16_t SX127x::setPreambleLength(uint16_t preambleLength) {
  // set mode to standby
  int16_t state = setMode(SX127X_STANDBY);
  if(state != ERR_NONE) {
    return(state);
  }

  // check active modem
  uint8_t modem = getActiveModem();
  if(modem == SX127X_LORA) {
    // check allowed range
    if(preambleLength < 6) {
      return(ERR_INVALID_PREAMBLE_LENGTH);
    }

    // set preamble length
    state = _mod->SPIsetRegValue(SX127X_REG_PREAMBLE_MSB, (uint8_t)((preambleLength >> 8) & 0xFF));
    state |= _mod->SPIsetRegValue(SX127X_REG_PREAMBLE_LSB, (uint8_t)(preambleLength & 0xFF));
    return(state);

  } else if(modem == SX127X_FSK_OOK) {
    // set preamble length
    state = _mod->SPIsetRegValue(SX127X_REG_PREAMBLE_MSB_FSK, (uint8_t)((preambleLength >> 8) & 0xFF));
    state |= _mod->SPIsetRegValue(SX127X_REG_PREAMBLE_LSB_FSK, (uint8_t)(preambleLength & 0xFF));
    return(state);
  }

  return(ERR_UNKNOWN);
}

float SX127x::getFrequencyError(bool autoCorrect) {
  int16_t modem = getActiveModem();
  if(modem == SX127X_LORA) {
    // get raw frequency error
    uint32_t raw = (uint32_t)_mod->SPIgetRegValue(SX127X_REG_FEI_MSB, 3, 0) << 16;
    raw |= (uint16_t)_mod->SPIgetRegValue(SX127X_REG_FEI_MID) << 8;
    raw |= _mod->SPIgetRegValue(SX127X_REG_FEI_LSB);

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

  } else if(modem == SX127X_FSK_OOK) {
    // get raw frequency error
    uint16_t raw = (uint16_t)_mod->SPIgetRegValue(SX127X_REG_FEI_MSB_FSK) << 8;
    raw |= _mod->SPIgetRegValue(SX127X_REG_FEI_LSB_FSK);

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

  return(ERR_UNKNOWN);
}

float SX127x::getSNR() {
  // check active modem
  if(getActiveModem() != SX127X_LORA) {
    return(0);
  }

  // get SNR value
  int8_t rawSNR = (int8_t)_mod->SPIgetRegValue(SX127X_REG_PKT_SNR_VALUE);
  return(rawSNR / 4.0);
}

float SX127x::getDataRate() {
  return(_dataRate);
}

int16_t SX127x::setBitRate(float br) {
  // check active modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // check allowed bit rate
  if(_ook) {
    if((br < 1.2) || (br > 32.768)) {
      return(ERR_INVALID_BIT_RATE);
    }
  } else {
    if((br < 1.2) || (br > 300.0)) {
      return(ERR_INVALID_BIT_RATE);
    }
  }

  // set mode to STANDBY
  int16_t state = setMode(SX127X_STANDBY);
  if(state != ERR_NONE) {
    return(state);
  }

  // set bit rate
  uint16_t bitRate = (SX127X_CRYSTAL_FREQ * 1000.0) / br;
  state = _mod->SPIsetRegValue(SX127X_REG_BITRATE_MSB, (bitRate & 0xFF00) >> 8, 7, 0);
  state |= _mod->SPIsetRegValue(SX127X_REG_BITRATE_LSB, bitRate & 0x00FF, 7, 0);

  // TODO fractional part of bit rate setting (not in OOK)
  if(state == ERR_NONE) {
    SX127x::_br = br;
  }
  return(state);
}

int16_t SX127x::setFrequencyDeviation(float freqDev) {
  // check active modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // check frequency deviation range
  if(!((freqDev + _br/2.0 <= 250.0) && (freqDev <= 200.0))) {
    return(ERR_INVALID_FREQUENCY_DEVIATION);
  }

  // set mode to STANDBY
  int16_t state = setMode(SX127X_STANDBY);
  if(state != ERR_NONE) {
    return(state);
  }

  // set allowed frequency deviation
  uint32_t base = 1;
  uint32_t FDEV = (freqDev * (base << 19)) / 32000;
  state = _mod->SPIsetRegValue(SX127X_REG_FDEV_MSB, (FDEV & 0xFF00) >> 8, 5, 0);
  state |= _mod->SPIsetRegValue(SX127X_REG_FDEV_LSB, FDEV & 0x00FF, 7, 0);
  return(state);
}

int16_t SX127x::setRxBandwidth(float rxBw) {
  // check active modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // check allowed bandwidth values
  if(!((rxBw >= 2.6) && (rxBw <= 250.0))) {
    return(ERR_INVALID_RX_BANDWIDTH);
  }

  // set mode to STANDBY
  int16_t state = setMode(SX127X_STANDBY);
  if(state != ERR_NONE) {
    return(state);
  }

  // calculate exponent and mantissa values
  for(uint8_t e = 7; e >= 1; e--) {
    for(int8_t m = 2; m >= 0; m--) {
      float point = (SX127X_CRYSTAL_FREQ * 1000000.0)/(((4 * m) + 16) * ((uint32_t)1 << (e + 2)));
      if(abs(rxBw - ((point / 1000.0) + 0.05)) <= 0.5) {
        // set Rx bandwidth during AFC
        state = _mod->SPIsetRegValue(SX127X_REG_AFC_BW, (m << 3) | e, 4, 0);
        if(state != ERR_NONE) {
          return(state);
        }

        // set Rx bandwidth
        state = _mod->SPIsetRegValue(SX127X_REG_RX_BW, (m << 3) | e, 4, 0);
        if(state == ERR_NONE) {
          SX127x::_rxBw = rxBw;
        }

        return(state);
      }
    }
  }
  return(ERR_UNKNOWN);
}

int16_t SX127x::setSyncWord(uint8_t* syncWord, size_t len) {
  // check active modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // check constraints
  if((len > 8) || (len < 1)) {
    return(ERR_INVALID_SYNC_WORD);
  }

  // sync word must not contain value 0x00
  for(uint8_t i = 0; i < len; i++) {
    if(syncWord[i] == 0x00) {
      return(ERR_INVALID_SYNC_WORD);
    }
  }

  // enable sync word recognition
  int16_t state = _mod->SPIsetRegValue(SX127X_REG_SYNC_CONFIG, SX127X_SYNC_ON, 4, 4);
  state |= _mod->SPIsetRegValue(SX127X_REG_SYNC_CONFIG, len - 1, 2, 0);
  if(state != ERR_NONE) {
    return(state);
  }

  // set sync word
  _mod->SPIwriteRegisterBurst(SX127X_REG_SYNC_VALUE_1, syncWord, len);
  return(ERR_NONE);
}

int16_t SX127x::setNodeAddress(uint8_t nodeAddr) {
  // check active modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // enable address filtering (node only)
  int16_t state = _mod->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_1, SX127X_ADDRESS_FILTERING_NODE, 2, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // set node address
  return(_mod->SPIsetRegValue(SX127X_REG_NODE_ADRS, nodeAddr));
}

int16_t SX127x::setBroadcastAddress(uint8_t broadAddr) {
  // check active modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // enable address filtering (node + broadcast)
  int16_t state = _mod->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_1, SX127X_ADDRESS_FILTERING_NODE_BROADCAST, 2, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // set broadcast address
  return(_mod->SPIsetRegValue(SX127X_REG_BROADCAST_ADRS, broadAddr));
}

int16_t SX127x::disableAddressFiltering() {
  // check active modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // disable address filtering
  int16_t state = _mod->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_1, SX127X_ADDRESS_FILTERING_OFF, 2, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // set node address to default (0x00)
  state = _mod->SPIsetRegValue(SX127X_REG_NODE_ADRS, 0x00);
  if(state != ERR_NONE) {
    return(state);
  }

  // set broadcast address to default (0x00)
  return(_mod->SPIsetRegValue(SX127X_REG_BROADCAST_ADRS, 0x00));
}

int16_t SX127x::setOOK(bool enableOOK) {
  // check active modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // set OOK and if successful, save the new setting
  int16_t state = ERR_NONE;
  if(enableOOK) {
    state = _mod->SPIsetRegValue(SX127X_REG_OP_MODE, SX127X_MODULATION_OOK, 6, 5, 5);
  } else {
    state = _mod->SPIsetRegValue(SX127X_REG_OP_MODE, SX127X_MODULATION_FSK, 6, 5, 5);
  }
  if(state == ERR_NONE) {
    _ook = enableOOK;
  }

  return(state);
}

int16_t SX127x::setFrequencyRaw(float newFreq) {
  // set mode to standby
  int16_t state = setMode(SX127X_STANDBY);

  // calculate register values
  uint32_t FRF = (newFreq * (uint32_t(1) << SX127X_DIV_EXPONENT)) / SX127X_CRYSTAL_FREQ;

  // write registers
  state |= _mod->SPIsetRegValue(SX127X_REG_FRF_MSB, (FRF & 0xFF0000) >> 16);
  state |= _mod->SPIsetRegValue(SX127X_REG_FRF_MID, (FRF & 0x00FF00) >> 8);
  state |= _mod->SPIsetRegValue(SX127X_REG_FRF_LSB, FRF & 0x0000FF);
  return(state);
}

size_t SX127x::getPacketLength(bool update) {
  int16_t modem = getActiveModem();

  if(modem == SX127X_LORA) {
    if(_sf != 6) {
      // get packet length for SF7 - SF12
      return(_mod->SPIreadRegister(SX127X_REG_RX_NB_BYTES));

    } else {
      // return the maximum value for SF6
      return(SX127X_MAX_PACKET_LENGTH);
    }

  } else if(modem == SX127X_FSK_OOK) {
    // get packet length
    if(!_packetLengthQueried && update) {
      _packetLength = _mod->SPIreadRegister(SX127X_REG_FIFO);
      _packetLengthQueried = true;
    }
  }

  return(_packetLength);
}

int16_t SX127x::setRSSIConfig(uint8_t smoothingSamples, int8_t offset) {
  // check active modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
    return(state);
  }

  // check provided values
  if(!(smoothingSamples <= 7)) {
    return(ERR_INVALID_NUM_SAMPLES);
  }

  if(!((offset >= -16) && (offset <= 15))) {
    return(ERR_INVALID_RSSI_OFFSET);
  }

  // set new register values
  state = _mod->SPIsetRegValue(SX127X_REG_RSSI_CONFIG, offset, 7, 3);
  state |= _mod->SPIsetRegValue(SX127X_REG_RSSI_CONFIG, smoothingSamples, 2, 0);
  return(state);
}

int16_t SX127x::setEncoding(uint8_t encoding) {
  // check active modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // set encoding
  switch(encoding) {
    case 0:
      return(_mod->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_1, SX127X_DC_FREE_NONE, 6, 5));
    case 1:
      return(_mod->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_1, SX127X_DC_FREE_MANCHESTER, 6, 5));
    case 2:
      return(_mod->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_1, SX127X_DC_FREE_WHITENING, 6, 5));
    default:
      return(ERR_INVALID_ENCODING);
  }
}

int16_t SX127x::config() {
  // turn off frequency hopping
  int16_t state = _mod->SPIsetRegValue(SX127X_REG_HOP_PERIOD, SX127X_HOP_PERIOD_OFF);
  return(state);
}

int16_t SX127x::configFSK() {
  // set RSSI threshold
  int16_t state = _mod->SPIsetRegValue(SX127X_REG_RSSI_THRESH, SX127X_RSSI_THRESHOLD);
  if(state != ERR_NONE) {
    return(state);
  }

  // reset FIFO flag
  _mod->SPIwriteRegister(SX127X_REG_IRQ_FLAGS_2, SX127X_FLAG_FIFO_OVERRUN);

  // set packet configuration
  state = _mod->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_1, SX127X_PACKET_VARIABLE | SX127X_DC_FREE_WHITENING | SX127X_CRC_ON | SX127X_CRC_AUTOCLEAR_ON | SX127X_ADDRESS_FILTERING_OFF | SX127X_CRC_WHITENING_TYPE_CCITT, 7, 0);
  state |= _mod->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_2, SX127X_DATA_MODE_PACKET | SX127X_IO_HOME_OFF, 6, 5);
  if(state != ERR_NONE) {
    return(state);
  }

  // set preamble polarity
  state =_mod->SPIsetRegValue(SX127X_REG_SYNC_CONFIG, SX127X_PREAMBLE_POLARITY_55, 5, 5);
  if(state != ERR_NONE) {
    return(state);
  }

  // set FIFO threshold
  state = _mod->SPIsetRegValue(SX127X_REG_FIFO_THRESH, SX127X_TX_START_FIFO_NOT_EMPTY, 7, 7);
  state |= _mod->SPIsetRegValue(SX127X_REG_FIFO_THRESH, SX127X_FIFO_THRESH, 5, 0);
  if(state != ERR_NONE) {
    return(state);
  }

  // disable Rx timeouts
  state = _mod->SPIsetRegValue(SX127X_REG_RX_TIMEOUT_1, SX127X_TIMEOUT_RX_RSSI_OFF);
  state |= _mod->SPIsetRegValue(SX127X_REG_RX_TIMEOUT_2, SX127X_TIMEOUT_RX_PREAMBLE_OFF);
  state |= _mod->SPIsetRegValue(SX127X_REG_RX_TIMEOUT_3, SX127X_TIMEOUT_SIGNAL_SYNC_OFF);
  if(state != ERR_NONE) {
    return(state);
  }

  // enable preamble detector and set preamble length
  state = _mod->SPIsetRegValue(SX127X_REG_PREAMBLE_DETECT, SX127X_PREAMBLE_DETECTOR_ON | SX127X_PREAMBLE_DETECTOR_2_BYTE | SX127X_PREAMBLE_DETECTOR_TOL);
  state |= _mod->SPIsetRegValue(SX127X_REG_PREAMBLE_MSB_FSK, SX127X_PREAMBLE_SIZE_MSB);
  state |= _mod->SPIsetRegValue(SX127X_REG_PREAMBLE_LSB_FSK, SX127X_PREAMBLE_SIZE_LSB);
  if(state != ERR_NONE) {
    return(state);
  }

  return(state);
}

bool SX127x::findChip(uint8_t ver) {
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    uint8_t version = _mod->SPIreadRegister(SX127X_REG_VERSION);
    if(version == ver) {
      flagFound = true;
    } else {
      #ifdef RADIOLIB_DEBUG
        RADIOLIB_DEBUG_PRINT(F("SX127x not found! ("));
        RADIOLIB_DEBUG_PRINT(i + 1);
        RADIOLIB_DEBUG_PRINT(F(" of 10 tries) SX127X_REG_VERSION == "));

        char buffHex[5];
        sprintf(buffHex, "0x%02X", version);
        RADIOLIB_DEBUG_PRINT(buffHex);
        RADIOLIB_DEBUG_PRINT(F(", expected 0x00"));
        RADIOLIB_DEBUG_PRINTLN(ver, HEX);
      #endif
      delay(1000);
      i++;
    }
  }

  return(flagFound);
}

int16_t SX127x::setMode(uint8_t mode) {
  return(_mod->SPIsetRegValue(SX127X_REG_OP_MODE, mode, 2, 0, 5));
}

int16_t SX127x::getActiveModem() {
  return(_mod->SPIgetRegValue(SX127X_REG_OP_MODE, 7, 7));
}

int16_t SX127x::setActiveModem(uint8_t modem) {
  // set mode to SLEEP
  int16_t state = setMode(SX127X_SLEEP);

  // set modem
  state |= _mod->SPIsetRegValue(SX127X_REG_OP_MODE, modem, 7, 7, 5);

  // set mode to STANDBY
  state |= setMode(SX127X_STANDBY);
  return(state);
}

void SX127x::clearIRQFlags() {
  int16_t modem = getActiveModem();
  if(modem == SX127X_LORA) {
    _mod->SPIwriteRegister(SX127X_REG_IRQ_FLAGS, 0b11111111);
  } else if(modem == SX127X_FSK_OOK) {
    _mod->SPIwriteRegister(SX127X_REG_IRQ_FLAGS_1, 0b11111111);
    _mod->SPIwriteRegister(SX127X_REG_IRQ_FLAGS_2, 0b11111111);
  }
}

void SX127x::clearFIFO(size_t count) {
  while(count) {
    _mod->SPIreadRegister(SX127X_REG_FIFO);
    count--;
  }
}

#ifdef RADIOLIB_DEBUG
void SX127x::regDump() {
  RADIOLIB_DEBUG_PRINTLN();
  RADIOLIB_DEBUG_PRINTLN(F("ADDR\tVALUE"));
  for(uint16_t addr = 0x01; addr <= 0x70; addr++) {
    if(addr <= 0x0F) {
      RADIOLIB_DEBUG_PRINT(F("0x0"));
    } else {
      RADIOLIB_DEBUG_PRINT(F("0x"));
    }
    RADIOLIB_DEBUG_PRINT(addr, HEX);
    RADIOLIB_DEBUG_PRINT('\t');
    uint8_t val = _mod->SPIreadRegister(addr);
    if(val <= 0x0F) {
      RADIOLIB_DEBUG_PRINT(F("0x0"));
    } else {
      RADIOLIB_DEBUG_PRINT(F("0x"));
    }
    RADIOLIB_DEBUG_PRINTLN(val, HEX);

    delay(50);
  }
}
#endif
