#include "SX126x.h"

SX126x::SX126x(Module* mod) : PhysicalLayer(SX126X_CRYSTAL_FREQ, SX126X_DIV_EXPONENT) {
  _mod = mod;
}

int16_t SX126x::begin(float bw, uint8_t sf, uint8_t cr, uint16_t syncWord, uint16_t preambleLength) {
  // set module properties
  _mod->init(USE_SPI, INT_BOTH);
  pinMode(_mod->getRx(), INPUT);

  // BW in kHz and SF are required in order to calculate LDRO for setModulationParams
  _bwKhz = bw;
  _sf = sf;

  // initialize dummy configuration variables
  _bw = SX126X_LORA_BW_125_0;
  _cr = SX126X_LORA_CR_4_7;
  _ldro = 0x00;
  _crcType = SX126X_LORA_CRC_ON;
  _preambleLength = preambleLength;

  // get status and errors
  getStatus();
  getDeviceErrors();

  // set mode to standby
  standby();

  // configure settings not accessible by API
  config();

  // configure publicly accessible settings
  int16_t state = setSpreadingFactor(sf);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setBandwidth(bw);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setCodingRate(cr);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setSyncWord(syncWord);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setPreambleLength(preambleLength);
  if(state != ERR_NONE) {
    return(state);
  }

  return(state);
}

int16_t SX126x::transmit(uint8_t* data, size_t len, uint8_t addr) {
  // set mode to standby
  int16_t state = standby();

  // check packet length
  if(len >= 256) {
    return(ERR_PACKET_TOO_LONG);
  }

  uint32_t timeout = 0;

  // get currently active modem
  uint8_t modem = getPacketType();
  if(modem == SX126X_PACKET_TYPE_LORA) {
    // calculate timeout (150% of expected time-on-air)
    float symbolLength = (float)(uint32_t(1) << _sf) / (float)_bwKhz;
    float sfCoeff1 = 4.25;
    float sfCoeff2 = 8.0;
    if(_sf == 5 || _sf == 6) {
      sfCoeff1 = 6.25;
      sfCoeff2 = 0.0;
    }
    uint8_t sfDivisor = 4*_sf;
    if(symbolLength >= 16.0) {
      sfDivisor = 4*(_sf - 2);
    }
    float nSymbol = _preambleLength + sfCoeff1 + 8 + ceil(max(8.0 * len + (_crcType * 16.0) - 4.0 * _sf + sfCoeff2 + 20.0, 0.0) / sfDivisor) * (_cr + 4);
    timeout = (uint32_t)(symbolLength * nSymbol * 1500.0);

    // set packet length
    setPacketParams(_preambleLength, _crcType, len);

  } else if(modem == SX126X_PACKET_TYPE_GFSK) {

  } else {
    return(ERR_UNKNOWN);
  }

  DEBUG_PRINT(F("Timeout in "))
  DEBUG_PRINT(timeout);
  DEBUG_PRINTLN(F(" us"))

  // set DIO mapping
  setDioIrqParams(SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT, SX126X_IRQ_TX_DONE);

  // set buffer pointers
  setBufferBaseAddress();

  // write packet to buffer
  writeBuffer(data, len);

  // clear interrupt flags
  clearIrqStatus();

  // start transmission
  uint32_t timeoutValue = (uint32_t)((float)timeout / 15.625);
  setTx(timeoutValue);

  // wait for BUSY to go low (= PA ramp up done)
  while(digitalRead(_mod->getRx()));

  // wait for packet transmission or timeout
  uint32_t start = micros();
  while(!digitalRead(_mod->getInt0())) {
    if(micros() - start > timeout) {
      clearIrqStatus();
      return(ERR_TX_TIMEOUT);
    }
  }
  uint32_t elapsed = micros() - start;

  // update data rate
  _dataRate = (len*8.0)/((float)elapsed/1000000.0);

  // clear interrupt flags
  clearIrqStatus();

  return(ERR_NONE);
}

int16_t SX126x::receive(uint8_t* data, size_t len) {
  // set mode to standby
  int16_t state = standby();

  uint32_t timeout = 0;

  // get currently active modem
  uint8_t modem = getPacketType();
  if(modem == SX126X_PACKET_TYPE_LORA) {
    // calculate timeout (100 LoRa symbols, the default for SX127x series)
    float symbolLength = (float)(uint32_t(1) << _sf) / (float)_bwKhz;
    timeout = (uint32_t)(symbolLength * 100.0 * 1000.0);

  } else if(modem == SX126X_PACKET_TYPE_GFSK) {

  } else {
    return(ERR_UNKNOWN);
  }

  DEBUG_PRINT(F("Timeout in "))
  DEBUG_PRINT(timeout);
  DEBUG_PRINTLN(F(" us"))

  // set DIO mapping
  setDioIrqParams(SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT, SX126X_IRQ_RX_DONE);

  // set buffer pointers
  setBufferBaseAddress();

  // clear interrupt flags
  clearIrqStatus();

  // start reception
  uint32_t timeoutValue = (uint32_t)((float)timeout / 15.625);
  setRx(timeoutValue);

  // wait for packet reception or timeout
  uint32_t start = micros();
  while(!digitalRead(_mod->getInt0())) {
    if(micros() - start > timeout) {
      clearIrqStatus();
      return(ERR_RX_TIMEOUT);
    }
  }

  // check integrity CRC
  uint16_t irq = getIrqStatus();
  if((irq & SX126X_IRQ_CRC_ERR) || (irq & SX126X_IRQ_HEADER_ERR)) {
    clearIrqStatus();
    return(ERR_CRC_MISMATCH);
  }

  // get packet length
  uint8_t rxBufStatus[2];
  SPIreadCommand(SX126X_CMD_GET_RX_BUFFER_STATUS, rxBufStatus, 2);
  size_t length = rxBufStatus[0];

  // read packet data
  if(len == 0) {
    // argument 'len' equal to zero indicates String call, which means dynamically allocated data array
    // dispose of the original and create a new one
    delete[] data;
    data = new uint8_t[length + 1];
  }
  readBuffer(data, length);

  // add terminating null
  data[length] = 0;

  // clear interrupt flags
  clearIrqStatus();

  return(ERR_NONE);
}

int16_t SX126x::transmitDirect(uint32_t frf) {
  // user requested to start transmitting immediately (required for RTTY)
  if(frf != 0) {
    setRfFrequency(frf);
  }

  // start transmitting
  uint8_t data[] = {SX126X_CMD_NOP};
  SPIwriteCommand(SX126X_CMD_SET_TX_CONTINUOUS_WAVE, data, 1);
  return(ERR_NONE);
}

int16_t SX126x::receiveDirect() {
  // SX126x is unable to ouput received data directly
  return(ERR_UNKNOWN);
}

int16_t SX126x::sleep() {
  uint8_t data[] = {SX126X_SLEEP_START_COLD | SX126X_SLEEP_RTC_OFF};
  SPIwriteCommand(SX126X_CMD_SET_SLEEP, data, 1);

  // wait for SX126x to safely enter sleep mode
  delayMicroseconds(500);

  return(ERR_NONE);
}

int16_t SX126x::standby(uint8_t mode) {
  uint8_t data[] = {mode};
  SPIwriteCommand(SX126X_CMD_SET_STANDBY, data, 1);
  return(ERR_NONE);
}

int16_t SX126x::setBandwidth(float bw) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // check alowed bandwidth values
  if(abs(bw - 7.8) <= 0.001) {
    _bw = SX126X_LORA_BW_7_8;
  } else if(abs(bw - 10.4) <= 0.001) {
    _bw = SX126X_LORA_BW_10_4;
  } else if(abs(bw - 15.6) <= 0.001) {
    _bw = SX126X_LORA_BW_15_6;
  } else if(abs(bw - 20.8) <= 0.001) {
    _bw = SX126X_LORA_BW_20_8;
  } else if(abs(bw - 31.25) <= 0.001) {
    _bw = SX126X_LORA_BW_31_25;
  } else if(abs(bw - 41.7) <= 0.001) {
    _bw = SX126X_LORA_BW_41_7;
  } else if(abs(bw - 62.5) <= 0.001) {
    _bw = SX126X_LORA_BW_62_5;
  } else if(abs(bw - 125.0) <= 0.001) {
    _bw = SX126X_LORA_BW_125_0;
  } else if(abs(bw - 250.0) <= 0.001) {
    _bw = SX126X_LORA_BW_250_0;
  } else if(abs(bw - 500.0) <= 0.001) {
    _bw = SX126X_LORA_BW_500_0;
  } else {
    return(ERR_INVALID_BANDWIDTH);
  }

  // update modulation parameters
  _bwKhz = bw;
  setModulationParams(_sf, _bw, _cr);
  return(ERR_NONE);
}

int16_t SX126x::setSpreadingFactor(uint8_t sf) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // check allowed spreading factor values
  if(!((sf >= 5) && (sf <= 12))) {
    return(ERR_INVALID_SPREADING_FACTOR);
  }

  // update modulation parameters
  _sf = sf;
  setModulationParams(_sf, _bw, _cr);
  return(ERR_NONE);
}

int16_t SX126x::setCodingRate(uint8_t cr) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // check allowed spreading factor values
  if(!((cr >= 5) && (cr <= 8))) {
    return(ERR_INVALID_CODING_RATE);
  }

  // update modulation parameters
  _cr = cr - 4;
  setModulationParams(_sf, _bw, _cr);
  return(ERR_NONE);
}

int16_t SX126x::setSyncWord(uint16_t syncWord) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // update register
  uint8_t data[2] = {(uint8_t)((syncWord >> 8) & 0xFF), (uint8_t)(syncWord & 0xFF)};
  writeRegister(SX126X_REG_LORA_SYNC_WORD_MSB, data, 2);
  return(ERR_NONE);
}

int16_t SX126x::setCurrentLimit(float currentLimit) {
  // calculate raw value
  uint8_t rawLimit = (uint8_t)(currentLimit / 2.5);

  // update register
  writeRegister(SX126X_REG_OCP_CONFIGURATION, &rawLimit, 1);
  return(ERR_NONE);
}

int16_t SX126x::setPreambleLength(uint16_t preambleLength) {
  // update packet parameters
  _preambleLength = preambleLength;
  setPacketParams(_preambleLength, _crcType);
  return(ERR_NONE);
}

float SX126x::getDataRate() {
  return(_dataRate);
}

int16_t SX126x::setFrequencyDeviation(float freqDev) {

  return(ERR_NONE);
}

float SX126x::getRSSI() {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // get last packet RSSI from packet status
  uint32_t packetStatus = getPacketStatus();
  uint8_t rssiPkt = packetStatus & 0xFF;
  return(-1.0 * rssiPkt/2.0);
}

float SX126x::getSNR() {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // get last packet SNR from packet status
  uint32_t packetStatus = getPacketStatus();
  uint8_t snrPkt = (packetStatus >> 8) & 0xFF;
  return(snrPkt/4.0);
}

void SX126x::setTx(uint32_t timeout) {
  uint8_t data[3] = {(uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF)};
  SPIwriteCommand(SX126X_CMD_SET_TX, data, 3);
}

void SX126x::setRx(uint32_t timeout) {
  uint8_t data[3] = {(uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF)};
  SPIwriteCommand(SX126X_CMD_SET_RX, data, 3);
}

void SX126x::setCad() {
  SPIwriteCommand(SX126X_CMD_SET_CAD, NULL, 0);
}

void SX126x::setPaConfig(uint8_t paDutyCycle, uint8_t deviceSel, uint8_t hpMax, uint8_t paLut) {
  uint8_t data[4] = {paDutyCycle, hpMax, deviceSel, paLut};
  SPIwriteCommand(SX126X_CMD_SET_PA_CONFIG, data, 4);
}

void SX126x::writeRegister(uint16_t addr, uint8_t* data, uint8_t numBytes) {
  uint8_t* dat = new uint8_t[2 + numBytes];
  dat[0] = (uint8_t)((addr >> 8) & 0xFF);
  dat[1] = (uint8_t)(addr & 0xFF);
  memcpy(dat + 2, data, numBytes);
  SPIwriteCommand(SX126X_CMD_WRITE_REGISTER, dat, 2 + numBytes);
  delete[] dat;
}

void SX126x::writeBuffer(uint8_t* data, uint8_t numBytes, uint8_t offset) {
  uint8_t* dat = new uint8_t[1 + numBytes];
  dat[0] = offset;
  memcpy(dat + 1, data, numBytes);
  SPIwriteCommand(SX126X_CMD_WRITE_BUFFER, dat, 1 + numBytes);
  delete[] dat;
}

void SX126x::readBuffer(uint8_t* data, uint8_t numBytes) {
  // offset will be always set to 0 (one extra NOP is sent)
  uint8_t* dat = new uint8_t[1 + numBytes];
  dat[0] = SX126X_CMD_NOP;
  memcpy(dat + 1, data, numBytes);
  SPIreadCommand(SX126X_CMD_READ_BUFFER, dat, numBytes);
  memcpy(data, dat + 1, numBytes);
  delete[] dat;
}

void SX126x::setDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask) {
  uint8_t data[8] = {(uint8_t)((irqMask >> 8) & 0xFF), (uint8_t)(irqMask & 0xFF),
                     (uint8_t)((dio1Mask >> 8) & 0xFF), (uint8_t)(dio1Mask & 0xFF),
                     (uint8_t)((dio2Mask >> 8) & 0xFF), (uint8_t)(dio2Mask & 0xFF),
                     (uint8_t)((dio3Mask >> 8) & 0xFF), (uint8_t)(dio3Mask & 0xFF)};
  SPIwriteCommand(SX126X_CMD_SET_DIO_IRQ_PARAMS, data, 8);
}

uint16_t SX126x::getIrqStatus() {
  uint8_t data[2];
  SPIreadCommand(SX126X_CMD_GET_IRQ_STATUS, data, 2);
  return(((uint16_t)(data[1]) << 8) | data[0]);
}

void SX126x::clearIrqStatus(uint16_t clearIrqParams) {
  uint8_t data[2] = {(uint8_t)((clearIrqParams >> 8) & 0xFF), (uint8_t)(clearIrqParams & 0xFF)};
  SPIwriteCommand(SX126X_CMD_CLEAR_IRQ_STATUS, data, 2);
}

void SX126x::setRfFrequency(uint32_t frf) {
  uint8_t data[4] = {(uint8_t)((frf >> 24) & 0xFF), (uint8_t)((frf >> 16) & 0xFF), (uint8_t)((frf >> 8) & 0xFF), (uint8_t)(frf & 0xFF)};
  SPIwriteCommand(SX126X_CMD_SET_RF_FREQUENCY, data, 4);
}

uint8_t SX126x::getPacketType() {
  uint8_t data[1];
  SPIreadCommand(SX126X_CMD_GET_PACKET_TYPE, data, 1);
  return(data[0]);
}

void SX126x::setTxParams(uint8_t power, uint8_t rampTime) {
  uint8_t data[2] = {power, rampTime};
  SPIwriteCommand(SX126X_CMD_SET_TX_PARAMS, data, 2);
}

void SX126x::setModulationParams(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro) {
  // calculate symbol length and enable low data rate optimization, if needed
  if(ldro == 0xFF) {
    float symbolLength = (float)(uint32_t(1) << _sf) / (float)_bwKhz;
    if(symbolLength >= 16.0) {
      _ldro = SX126X_LORA_LOW_DATA_RATE_OPTIMIZE_ON;
    } else {
      _ldro = SX126X_LORA_LOW_DATA_RATE_OPTIMIZE_OFF;
    }
  } else {
    _ldro = ldro;
  }

  uint8_t data[4] = {sf, bw, cr, _ldro};
  SPIwriteCommand(SX126X_CMD_SET_MODULATION_PARAMS, data, 4);
}

void SX126x::setPacketParams(uint16_t preambleLength, uint8_t crcType, uint8_t payloadLength, uint8_t headerType, uint8_t invertIQ) {
  uint8_t data[6] = {(uint8_t)((preambleLength >> 8) & 0xFF), (uint8_t)(preambleLength & 0xFF), headerType, payloadLength, crcType, invertIQ};
  SPIwriteCommand(SX126X_CMD_SET_PACKET_PARAMS, data, 6);
}

void SX126x::setBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress) {
  uint8_t data[2] = {txBaseAddress, rxBaseAddress};
  SPIwriteCommand(SX126X_CMD_SET_BUFFER_BASE_ADDRESS, data, 2);
}

uint8_t SX126x::getStatus() {
  uint8_t data[1];
  SPIreadCommand(SX126X_CMD_GET_STATUS, data, 1);
  return(data[0]);
}

uint32_t SX126x::getPacketStatus() {
  uint8_t data[3];
  SPIreadCommand(SX126X_CMD_GET_PACKET_STATUS, data, 3);
  return((((uint32_t)data[2]) << 16) | (((uint32_t)data[1]) << 8) | (uint32_t)data[0]);
}

uint16_t SX126x::getDeviceErrors() {
  uint8_t data[2];
  SPIreadCommand(SX126X_CMD_GET_DEVICE_ERRORS, data, 2);
  uint16_t opError = (((uint16_t)data[0] & 0xFF) << 8) & ((uint16_t)data[1]);
  return(opError);
}

void SX126x::clearDeviceErrors() {
  uint8_t data[1] = {SX126X_CMD_NOP};
  SPIwriteCommand(SX126X_CMD_CLEAR_DEVICE_ERRORS, data, 1);
}

int16_t SX126x::setFrequencyRaw(float freq, bool calibrate) {
  // calibrate image
  if(calibrate) {
    uint8_t data[2];
    if(freq > 900.0) {
      data[0] = SX126X_CAL_IMG_902_MHZ_1;
      data[1] = SX126X_CAL_IMG_902_MHZ_2;
    } else if(freq > 850.0) {
      data[0] = SX126X_CAL_IMG_863_MHZ_1;
      data[1] = SX126X_CAL_IMG_863_MHZ_2;
    } else if(freq > 770.0) {
      data[0] = SX126X_CAL_IMG_779_MHZ_1;
      data[1] = SX126X_CAL_IMG_779_MHZ_2;
    } else if(freq > 460.0) {
      data[0] = SX126X_CAL_IMG_470_MHZ_1;
      data[1] = SX126X_CAL_IMG_470_MHZ_2;
    } else {
      data[0] = SX126X_CAL_IMG_430_MHZ_1;
      data[1] = SX126X_CAL_IMG_430_MHZ_2;
    }
    SPIwriteCommand(SX126X_CMD_CALIBRATE_IMAGE, data, 2);
  }

  // calculate raw value
  uint32_t frf = (freq * (uint32_t(1) << SX126X_DIV_EXPONENT)) / SX126X_CRYSTAL_FREQ;
  setRfFrequency(frf);
  return(ERR_NONE);
}

int16_t SX126x::config() {
  // set DIO2 as IRQ
  uint8_t* data = new uint8_t[1];
  data[0] = SX126X_DIO2_AS_IRQ;
  SPIwriteCommand(SX126X_DIO2_AS_RF_SWITCH, data, 1);

  // set regulator mode
  data[0] = SX126X_REGULATOR_DC_DC;
  SPIwriteCommand(SX126X_CMD_SET_REGULATOR_MODE, data, 1);

  // reset buffer base address
  setBufferBaseAddress();

  // set LoRa mode
  data[0] = SX126X_PACKET_TYPE_LORA;
  SPIwriteCommand(SX126X_CMD_SET_PACKET_TYPE, data, 1);

  // set Rx/Tx fallback mode to STDBY_RC
  data[0] = SX126X_RX_TX_FALLBACK_MODE_STDBY_RC;
  SPIwriteCommand(SX126X_CMD_SET_RX_TX_FALLBACK_MODE, data, 1);

  // set CAD parameters
  delete[] data;
  data = new uint8_t[7];
  data[0] = SX126X_CAD_ON_8_SYMB;
  data[1] = _sf + 13;
  data[2] = 10;
  data[3] = SX126X_CAD_GOTO_STDBY;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = 0x00;
  SPIwriteCommand(SX126X_CMD_SET_CAD_PARAMS, data, 7);

  // clear IRQ
  clearIrqStatus();
  setDioIrqParams(SX126X_IRQ_NONE, SX126X_IRQ_NONE);

  delete[] data;

  return(ERR_NONE);
}

void SX126x::SPIwriteCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  SX126x::SPItransfer(cmd, true, data, NULL, numBytes, waitForBusy);
}

void SX126x::SPIreadCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  SX126x::SPItransfer(cmd, false, NULL, data, numBytes, waitForBusy);
}

void SX126x::SPItransfer(uint8_t cmd, bool write, uint8_t* dataOut, uint8_t* dataIn, uint8_t numBytes, bool waitForBusy) {
  // get pointer to used SPI interface
  SPIClass* spi = _mod->getSpi();

  // ensure BUSY is low (state meachine ready)
  // TODO timeout
  while(digitalRead(_mod->getRx()));

  // start transfer
  digitalWrite(_mod->getCs(), LOW);
  spi->beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

  // send command byte
  spi->transfer(cmd);
  DEBUG_PRINT(cmd, HEX);
  DEBUG_PRINT('\t');

  // send/receive all bytes
  if(write) {
    for(uint8_t n = 0; n < numBytes; n++) {
      uint8_t in = spi->transfer(dataOut[n]);
      DEBUG_PRINT(dataOut[n], HEX);
      DEBUG_PRINT('\t');
      DEBUG_PRINT(in, HEX);
      DEBUG_PRINT('\t');
    }
    DEBUG_PRINTLN();
  } else {
    // skip the first byte for read-type commands (status-only)
    uint8_t in = spi->transfer(SX126X_CMD_NOP);
    DEBUG_PRINT(SX126X_CMD_NOP, HEX);
    DEBUG_PRINT('\t');
    DEBUG_PRINT(in, HEX);
    DEBUG_PRINT('\t')
    for(uint8_t n = 0; n < numBytes; n++) {
      dataIn[n] = spi->transfer(SX126X_CMD_NOP);
      DEBUG_PRINT(SX126X_CMD_NOP, HEX);
      DEBUG_PRINT('\t');
      DEBUG_PRINT(dataIn[n], HEX);
      DEBUG_PRINT('\t');
    }
    DEBUG_PRINTLN();
  }

  // stop transfer
  spi->endTransaction();
  digitalWrite(_mod->getCs(), HIGH);

  // wait for BUSY to go high and then low
  // TODO timeout
  if(waitForBusy) {
    delayMicroseconds(1);
    while(digitalRead(_mod->getRx()));
  }
}
