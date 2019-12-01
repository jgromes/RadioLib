#include "SX126x.h"

SX126x::SX126x(Module* mod) : PhysicalLayer(SX126X_CRYSTAL_FREQ, SX126X_DIV_EXPONENT, SX126X_MAX_PACKET_LENGTH) {
  _mod = mod;
}

int16_t SX126x::begin(float bw, uint8_t sf, uint8_t cr, uint16_t syncWord, float currentLimit, uint16_t preambleLength, float tcxoVoltage) {
  // set module properties
  _mod->init(RADIOLIB_USE_SPI, RADIOLIB_INT_BOTH);

  // BW in kHz and SF are required in order to calculate LDRO for setModulationParams
  _bwKhz = bw;
  _sf = sf;

  // initialize configuration variables (will be overwritten during public settings configuration)
  _bw = SX126X_LORA_BW_125_0;
  _cr = SX126X_LORA_CR_4_7;
  _ldro = 0x00;
  _crcType = SX126X_LORA_CRC_ON;
  _preambleLength = preambleLength;

  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
    return(state);
  }

  // configure settings not accessible by API
  state = config(SX126X_PACKET_TYPE_LORA);
  if(state != ERR_NONE) {
    return(state);
  }

  // set TCXO control, if requested
  if(tcxoVoltage > 0.0) {
    state = setTCXO(tcxoVoltage);
    if(state != ERR_NONE) {
      return(state);
    }
  }

  // configure publicly accessible settings
  state = setSpreadingFactor(sf);
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

  state = setCurrentLimit(currentLimit);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setPreambleLength(preambleLength);
  if(state != ERR_NONE) {
    return(state);
  }

  // set publicly accessible settings that are not a part of begin method
  state = setDio2AsRfSwitch(false);

  return(state);
}

int16_t SX126x::beginFSK(float br, float freqDev, float rxBw, float currentLimit, uint16_t preambleLength, float dataShaping, float tcxoVoltage) {
  // set module properties
  _mod->init(RADIOLIB_USE_SPI, RADIOLIB_INT_BOTH);

  // initialize configuration variables (will be overwritten during public settings configuration)
  _br = 21333;                                  // 48.0 kbps
  _freqDev = 52428;                             // 50.0 kHz
  _rxBw = SX126X_GFSK_RX_BW_156_2;
  _rxBwKhz = 156.2;
  _pulseShape = SX126X_GFSK_FILTER_GAUSS_0_5;
  _crcTypeFSK = SX126X_GFSK_CRC_2_BYTE_INV;     // CCIT CRC configuration
  _preambleLengthFSK = preambleLength;
  _addrComp = SX126X_GFSK_ADDRESS_FILT_OFF;

  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
    return(state);
  }

  // configure settings not accessible by API
  state = config(SX126X_PACKET_TYPE_GFSK);
  if(state != ERR_NONE) {
    return(state);
  }

  // set TCXO control, if requested
  if(tcxoVoltage > 0.0) {
    state = setTCXO(tcxoVoltage);
    if(state != ERR_NONE) {
      return(state);
    }
  }

  // configure publicly accessible settings
  state = setBitRate(br);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setFrequencyDeviation(freqDev);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setRxBandwidth(rxBw);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setCurrentLimit(currentLimit);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setDataShaping(dataShaping);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setPreambleLength(preambleLength);
  if(state != ERR_NONE) {
    return(state);
  }

  // set publicly accessible settings that are not a part of begin method
  uint8_t sync[] = {0x2D, 0x01};
  state = setSyncWord(sync, 2);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setWhitening(true, 0x0100);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setDio2AsRfSwitch(false);

  return(state);
}

int16_t SX126x::transmit(uint8_t* data, size_t len, uint8_t addr) {
  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
    return(state);
  }

  // check packet length
  if(len > SX126X_MAX_PACKET_LENGTH) {
    return(ERR_PACKET_TOO_LONG);
  }

  uint32_t timeout = 0;

  // get currently active modem
  uint8_t modem = getPacketType();
  if(modem == SX126X_PACKET_TYPE_LORA) {
    // calculate timeout (150% of expected time-on-air)
    timeout = (float)getTimeOnAir(len) * 1.5;

  } else if(modem == SX126X_PACKET_TYPE_GFSK) {
    // calculate timeout (500% of expected time-on-air)
    timeout = (float)getTimeOnAir(len) * 5.0;

  } else {
    return(ERR_UNKNOWN);
  }

  RADIOLIB_DEBUG_PRINT(F("Timeout in "));
  RADIOLIB_DEBUG_PRINT(timeout);
  RADIOLIB_DEBUG_PRINTLN(F(" us"));

  // start transmission
  state = startTransmit(data, len, addr);
  if(state != ERR_NONE) {
    return(state);
  }

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
  state = clearIrqStatus();
  if(state != ERR_NONE) {
    return(state);
  }

  // set mode to standby to disable transmitter
  state = standby();

  return(state);
}

int16_t SX126x::receive(uint8_t* data, size_t len) {
  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
    return(state);
  }

  uint32_t timeout = 0;

  // get currently active modem
  uint8_t modem = getPacketType();
  if(modem == SX126X_PACKET_TYPE_LORA) {
    // calculate timeout (100 LoRa symbols, the default for SX127x series)
    float symbolLength = (float)(uint32_t(1) << _sf) / (float)_bwKhz;
    timeout = (uint32_t)(symbolLength * 100.0 * 1000.0);

  } else if(modem == SX126X_PACKET_TYPE_GFSK) {
    // calculate timeout (500 % of expected time-one-air)
    size_t maxLen = len;
    if(len == 0) {
      maxLen = 0xFF;
    }
    float brBps = ((float)(SX126X_CRYSTAL_FREQ) * 1000000.0 * 32.0) / (float)_br;
    timeout = (uint32_t)(((maxLen * 8.0) / brBps) * 1000000.0 * 5.0);

  } else {
    return(ERR_UNKNOWN);
  }

  RADIOLIB_DEBUG_PRINT(F("Timeout in "));
  RADIOLIB_DEBUG_PRINT(timeout);
  RADIOLIB_DEBUG_PRINTLN(F(" us"));

  // start reception
  uint32_t timeoutValue = (uint32_t)((float)timeout / 15.625);
  state = startReceive(timeoutValue);
  if(state != ERR_NONE) {
    return(state);
  }

  // wait for packet reception or timeout
  uint32_t start = micros();
  while(!digitalRead(_mod->getInt0())) {
    if(micros() - start > timeout) {
      fixImplicitTimeout();
      clearIrqStatus();
      return(ERR_RX_TIMEOUT);
    }
  }

  // timeout fix is recommended after any reception with active timeout
  state = fixImplicitTimeout();
  if(state != ERR_NONE) {
    return(state);
  }

  // read the received data
  return(readData(data, len));
}

int16_t SX126x::transmitDirect(uint32_t frf) {
  // user requested to start transmitting immediately (required for RTTY)
  int16_t state = ERR_NONE;
  if(frf != 0) {
    state = setRfFrequency(frf);
  }
  if(state != ERR_NONE) {
    return(state);
  }

  // start transmitting
  uint8_t data[] = {SX126X_CMD_NOP};
  return(SPIwriteCommand(SX126X_CMD_SET_TX_CONTINUOUS_WAVE, data, 1));
}

int16_t SX126x::receiveDirect() {
  // SX126x is unable to ouput received data directly
  return(ERR_UNKNOWN);
}

int16_t SX126x::scanChannel() {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
    return(state);
  }

  // set DIO pin mapping
  state = setDioIrqParams(SX126X_IRQ_CAD_DETECTED | SX126X_IRQ_CAD_DONE, SX126X_IRQ_CAD_DETECTED | SX126X_IRQ_CAD_DONE);
  if(state != ERR_NONE) {
    return(state);
  }

  // clear interrupt flags
  state = clearIrqStatus();
  if(state != ERR_NONE) {
    return(state);
  }

  // set mode to CAD
  state = setCad();
  if(state != ERR_NONE) {
    return(state);
  }

  // wait for channel activity detected or timeout
  while(!digitalRead(_mod->getInt0()));

  // check CAD result
  uint16_t cadResult = getIrqStatus();
  if(cadResult & SX126X_IRQ_CAD_DETECTED) {
    // detected some LoRa activity
    clearIrqStatus();
    return(LORA_DETECTED);
  } else if(cadResult & SX126X_IRQ_CAD_DONE) {
    // channel is free
    clearIrqStatus();
    return(CHANNEL_FREE);
  }

  return(ERR_UNKNOWN);
}

int16_t SX126x::sleep() {
  uint8_t data[] = {SX126X_SLEEP_START_COLD | SX126X_SLEEP_RTC_OFF};
  int16_t state = SPIwriteCommand(SX126X_CMD_SET_SLEEP, data, 1, false);

  // wait for SX126x to safely enter sleep mode
  delayMicroseconds(500);

  return(state);
}

int16_t SX126x::standby() {
  return(SX126x::standby(SX126X_STANDBY_RC));
}

int16_t SX126x::standby(uint8_t mode) {
  uint8_t data[] = {mode};
  return(SPIwriteCommand(SX126X_CMD_SET_STANDBY, data, 1));
}

void SX126x::setDio1Action(void (*func)(void)) {
  attachInterrupt(digitalPinToInterrupt(_mod->getInt0()), func, RISING);
}

int16_t SX126x::startTransmit(uint8_t* data, size_t len, uint8_t addr) {
  // suppress unused variable warning
  (void)addr;

  // check packet length
  if(len > SX126X_MAX_PACKET_LENGTH) {
    return(ERR_PACKET_TOO_LONG);
  }

  // maximum packet length is decreased by 1 when address filtering is active
  if((_addrComp != SX126X_GFSK_ADDRESS_FILT_OFF) && (len > SX126X_MAX_PACKET_LENGTH - 1)) {
    return(ERR_PACKET_TOO_LONG);
  }

  // set packet Length
  int16_t state = ERR_NONE;
  uint8_t modem = getPacketType();
  if(modem == SX126X_PACKET_TYPE_LORA) {
    state = setPacketParams(_preambleLength, _crcType, len);
  } else if(modem == SX126X_PACKET_TYPE_GFSK) {
    state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType, len);
  } else {
    return(ERR_UNKNOWN);
  }
  if(state != ERR_NONE) {
    return(state);
  }

  // set DIO mapping
  state = setDioIrqParams(SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT, SX126X_IRQ_TX_DONE);
  if(state != ERR_NONE) {
    return(state);
  }

  // set buffer pointers
  state = setBufferBaseAddress();
  if(state != ERR_NONE) {
    return(state);
  }

  // write packet to buffer
  state = writeBuffer(data, len);
  if(state != ERR_NONE) {
    return(state);
  }

  // clear interrupt flags
  state = clearIrqStatus();
  if(state != ERR_NONE) {
    return(state);
  }

  // fix sensitivity
  state = fixSensitivity();
  if(state != ERR_NONE) {
    return(state);
  }

  // start transmission
  state = setTx(SX126X_TX_TIMEOUT_NONE);
  if(state != ERR_NONE) {
    return(state);
  }

  // wait for BUSY to go low (= PA ramp up done)
  while(digitalRead(_mod->getInt1()));

  return(state);
}

int16_t SX126x::startReceive(uint32_t timeout) {
  // set DIO mapping
  int16_t state = setDioIrqParams(SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CRC_ERR | SX126X_IRQ_HEADER_ERR, SX126X_IRQ_RX_DONE);
  if(state != ERR_NONE) {
    return(state);
  }

  // set buffer pointers
  state = setBufferBaseAddress();
  if(state != ERR_NONE) {
    return(state);
  }

  // clear interrupt flags
  state = clearIrqStatus();
  if(state != ERR_NONE) {
    return(state);
  }

  // set mode to receive
  state = setRx(timeout);

  return(state);
}

int16_t SX126x::readData(uint8_t* data, size_t len) {
  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
    return(state);
  }

  // check integrity CRC
  uint16_t irq = getIrqStatus();
  if((irq & SX126X_IRQ_CRC_ERR) || (irq & SX126X_IRQ_HEADER_ERR)) {
    clearIrqStatus();
    return(ERR_CRC_MISMATCH);
  }

  // get packet length
  size_t length = len;
  if(len == SX126X_MAX_PACKET_LENGTH) {
    length = getPacketLength();
  }

  // read packet data
  state = readBuffer(data, length);
  if(state != ERR_NONE) {
    return(state);
  }

  // clear interrupt flags
  state = clearIrqStatus();

  return(state);
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
  return(setModulationParams(_sf, _bw, _cr));
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
  return(setModulationParams(_sf, _bw, _cr));
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
  return(setModulationParams(_sf, _bw, _cr));
}

int16_t SX126x::setSyncWord(uint16_t syncWord) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // update register
  uint8_t data[2] = {(uint8_t)((syncWord >> 8) & 0xFF), (uint8_t)(syncWord & 0xFF)};
  return(writeRegister(SX126X_REG_LORA_SYNC_WORD_MSB, data, 2));
}

int16_t SX126x::setCurrentLimit(float currentLimit) {
  // calculate raw value
  uint8_t rawLimit = (uint8_t)(currentLimit / 2.5);

  // update register
  return(writeRegister(SX126X_REG_OCP_CONFIGURATION, &rawLimit, 1));
}

int16_t SX126x::setPreambleLength(uint16_t preambleLength) {
  uint8_t modem = getPacketType();
  if(modem == SX126X_PACKET_TYPE_LORA) {
    _preambleLength = preambleLength;
    return(setPacketParams(_preambleLength, _crcType));
  } else if(modem == SX126X_PACKET_TYPE_GFSK) {
    _preambleLengthFSK = preambleLength;
    return(setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType));
  }

  return(ERR_UNKNOWN);
}

int16_t SX126x::setFrequencyDeviation(float freqDev) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_GFSK) {
    return(ERR_WRONG_MODEM);
  }

  // check alowed frequency deviation values
  if(!(freqDev <= 200.0)) {
    return(ERR_INVALID_FREQUENCY_DEVIATION);
  }

  // calculate raw frequency deviation value
  uint32_t freqDevRaw = (uint32_t)(((freqDev * 1000.0) * (float)((uint32_t)(1) << 25)) / (SX126X_CRYSTAL_FREQ * 1000000.0));

  // check modulation parameters
  /*if(2 * freqDevRaw + _br > _rxBwKhz * 1000.0) {
    return(ERR_INVALID_MODULATION_PARAMETERS);
  }*/
  _freqDev = freqDevRaw;

  // update modulation parameters
  return(setModulationParamsFSK(_br, _pulseShape, _rxBw, _freqDev));
}

int16_t SX126x::setBitRate(float br) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_GFSK) {
    return(ERR_WRONG_MODEM);
  }

  // check alowed bit rate values
  if(!((br >= 0.6) && (br <= 300.0))) {
    return(ERR_INVALID_BIT_RATE);
  }

  // calculate raw bit rate value
  uint32_t brRaw = (uint32_t)((SX126X_CRYSTAL_FREQ * 1000000.0 * 32.0) / (br * 1000.0));

  // check modulation parameters
  /*if(2 * _freqDev + brRaw > _rxBwKhz * 1000.0) {
    return(ERR_INVALID_MODULATION_PARAMETERS);
  }*/
  _br = brRaw;

  // update modulation parameters
  return(setModulationParamsFSK(_br, _pulseShape, _rxBw, _freqDev));
}

int16_t SX126x::setRxBandwidth(float rxBw) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_GFSK) {
    return(ERR_WRONG_MODEM);
  }

  // check modulation parameters
  /*if(2 * _freqDev + _br > rxBw * 1000.0) {
    return(ERR_INVALID_MODULATION_PARAMETERS);
  }*/
  _rxBwKhz = rxBw;

  // check alowed receiver bandwidth values
  if(abs(rxBw - 4.8) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_4_8;
  } else if(abs(rxBw - 5.8) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_5_8;
  } else if(abs(rxBw - 7.3) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_7_3;
  } else if(abs(rxBw - 9.7) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_9_7;
  } else if(abs(rxBw - 11.7) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_11_7;
  } else if(abs(rxBw - 14.6) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_14_6;
  } else if(abs(rxBw - 19.5) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_19_5;
  } else if(abs(rxBw - 23.4) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_23_4;
  } else if(abs(rxBw - 29.3) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_29_3;
  } else if(abs(rxBw - 39.0) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_39_0;
  } else if(abs(rxBw - 46.9) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_46_9;
  } else if(abs(rxBw - 58.6) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_58_6;
  } else if(abs(rxBw - 78.2) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_78_2;
  } else if(abs(rxBw - 93.8) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_93_8;
  } else if(abs(rxBw - 117.3) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_117_3;
  } else if(abs(rxBw - 156.2) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_156_2;
  } else if(abs(rxBw - 187.2) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_187_2;
  } else if(abs(rxBw - 234.3) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_234_3;
  } else if(abs(rxBw - 312.0) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_312_0;
  } else if(abs(rxBw - 373.6) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_373_6;
  } else if(abs(rxBw - 467.0) <= 0.001) {
    _rxBw = SX126X_GFSK_RX_BW_467_0;
  } else {
    return(ERR_INVALID_RX_BANDWIDTH);
  }

  // update modulation parameters
  return(setModulationParamsFSK(_br, _pulseShape, _rxBw, _freqDev));
}

int16_t SX126x::setDataShaping(float sh) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_GFSK) {
    return(ERR_WRONG_MODEM);
  }

  // check allowed values
  sh *= 10.0;
  if(abs(sh - 0.0) <= 0.001) {
    _pulseShape = SX126X_GFSK_FILTER_NONE;
  } else if(abs(sh - 3.0) <= 0.001) {
    _pulseShape = SX126X_GFSK_FILTER_GAUSS_0_3;
  } else if(abs(sh - 5.0) <= 0.001) {
    _pulseShape = SX126X_GFSK_FILTER_GAUSS_0_5;
  } else if(abs(sh - 7.0) <= 0.001) {
    _pulseShape = SX126X_GFSK_FILTER_GAUSS_0_7;
  } else if(abs(sh - 10.0) <= 0.001) {
    _pulseShape = SX126X_GFSK_FILTER_GAUSS_1;
  } else {
    return(ERR_INVALID_DATA_SHAPING);
  }

  // update modulation parameters
  return(setModulationParamsFSK(_br, _pulseShape, _rxBw, _freqDev));
}

int16_t SX126x::setSyncWord(uint8_t* syncWord, uint8_t len) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_GFSK) {
    return(ERR_WRONG_MODEM);
  }

  // check sync word Length
  if(len > 8) {
    return(ERR_INVALID_SYNC_WORD);
  }

  // write sync word
  int16_t state = writeRegister(SX126X_REG_SYNC_WORD_0, syncWord, len);
  if(state != ERR_NONE) {
    return(state);
  }

  // update packet parameters
  _syncWordLength = len * 8;
  state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);

  return(state);
}

int16_t SX126x::setSyncBits(uint8_t *syncWord, uint8_t bitsLen) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_GFSK) {
    return(ERR_WRONG_MODEM);
  }

  // check sync word Length
  if(bitsLen > 0x40) {
    return(ERR_INVALID_SYNC_WORD);
  }

  uint8_t bytesLen = bitsLen / 8;
  if ((bitsLen % 8) != 0) {
    bytesLen++;
  }

  // write sync word
  int16_t state = writeRegister(SX126X_REG_SYNC_WORD_0, syncWord, bytesLen);
  if(state != ERR_NONE) {
    return(state);
  }

  // update packet parameters
  _syncWordLength = bitsLen;
  state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);

  return(state);
}

int16_t SX126x::setNodeAddress(uint8_t nodeAddr) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_GFSK) {
    return(ERR_WRONG_MODEM);
  }

  // enable address filtering (node only)
  _addrComp = SX126X_GFSK_ADDRESS_FILT_NODE;
  int16_t state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);
  if(state != ERR_NONE) {
    return(state);
  }

  // set node address
  state = writeRegister(SX126X_REG_NODE_ADDRESS, &nodeAddr, 1);

  return(state);
}

int16_t SX126x::setBroadcastAddress(uint8_t broadAddr) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_GFSK) {
    return(ERR_WRONG_MODEM);
  }

  // enable address filtering (node and broadcast)
  _addrComp = SX126X_GFSK_ADDRESS_FILT_NODE_BROADCAST;
  int16_t state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);
  if(state != ERR_NONE) {
    return(state);
  }

  // set broadcast address
  state = writeRegister(SX126X_REG_BROADCAST_ADDRESS, &broadAddr, 1);

  return(state);
}

int16_t SX126x::disableAddressFiltering() {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_GFSK) {
    return(ERR_WRONG_MODEM);
  }

  // disable address filtering
  _addrComp = SX126X_GFSK_ADDRESS_FILT_OFF;
  return(setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening));
}

int16_t SX126x::setCRC(uint8_t len, uint16_t initial, uint16_t polynomial, bool inverted) {
  // check active modem
  uint8_t modem = getPacketType();

  if(modem == SX126X_PACKET_TYPE_GFSK) {
    // update packet parameters
    switch(len) {
      case 0:
        _crcTypeFSK = SX126X_GFSK_CRC_OFF;
        break;
      case 1:
        if(inverted) {
          _crcTypeFSK = SX126X_GFSK_CRC_1_BYTE_INV;
        } else {
          _crcTypeFSK = SX126X_GFSK_CRC_1_BYTE;
        }
        break;
      case 2:
        if(inverted) {
          _crcTypeFSK = SX126X_GFSK_CRC_2_BYTE_INV;
        } else {
          _crcTypeFSK = SX126X_GFSK_CRC_2_BYTE;
        }
        break;
      default:
        return(ERR_INVALID_CRC_CONFIGURATION);
    }

    int16_t state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);
    if(state != ERR_NONE) {
      return(state);
    }

    // write initial CRC value
    uint8_t data[2] = {(uint8_t)((initial >> 8) & 0xFF), (uint8_t)(initial & 0xFF)};
    state = writeRegister(SX126X_REG_CRC_INITIAL_MSB, data, 2);
    if(state != ERR_NONE) {
      return(state);
    }

    // write CRC polynomial value
    data[0] = (uint8_t)((polynomial >> 8) & 0xFF);
    data[1] = (uint8_t)(polynomial & 0xFF);
    state = writeRegister(SX126X_REG_CRC_POLYNOMIAL_MSB, data, 2);

    return(state);

  } else if(modem == SX126X_PACKET_TYPE_LORA) {
    // LoRa CRC doesn't allow to set CRC polynomial, inital value, or inversion

    // update packet parameters
    if(len) {
      _crcType = SX126X_LORA_CRC_ON;
    } else {
      _crcType = SX126X_LORA_CRC_OFF;
    }

    return(setPacketParams(_preambleLength, _crcType));
  }

  return(ERR_UNKNOWN);
}

int16_t SX126x::setWhitening(bool enabled, uint16_t initial) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_GFSK) {
    return(ERR_WRONG_MODEM);
  }

  int16_t state = ERR_NONE;
  if(enabled != true) {
    // disable whitening
    _whitening = SX126X_GFSK_WHITENING_OFF;

    state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);
    if(state != ERR_NONE) {
      return(state);
    }
  } else {
    // enable whitening
    _whitening = SX126X_GFSK_WHITENING_ON;

    // write initial whitening value
    // as per note on pg. 65 of datasheet v1.2: "The user should not change the value of the 7 MSB's of this register"
    uint8_t data[2];
    // first read the actual value and mask 7 MSB which we can not change
    // if different value is written in 7 MSB, the Rx won't even work (tested on HW)
    state = readRegister(SX126X_REG_WHITENING_INITIAL_MSB, data, 1);
    if(state != ERR_NONE) {
      return(state);
    }
    data[0] = (data[0] & 0xFE) | (uint8_t)((initial >> 8) & 0x01);
    data[1] = (uint8_t)(initial & 0xFF);
    state = writeRegister(SX126X_REG_WHITENING_INITIAL_MSB, data, 2);

    if(state != ERR_NONE) {
      return(state);
    }

    state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, _packetType);
    if(state != ERR_NONE) {
      return(state);
    }
  }
  return(state);
}

float SX126x::getDataRate() {
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
  if(getPacketType() != SX126X_PACKET_TYPE_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // get last packet SNR from packet status
  uint32_t packetStatus = getPacketStatus();
  uint8_t snrPkt = (packetStatus >> 8) & 0xFF;
  return(snrPkt/4.0);
}

size_t SX126x::getPacketLength(bool update) {
  (void)update;
  uint8_t rxBufStatus[2] = {0, 0};
  SPIreadCommand(SX126X_CMD_GET_RX_BUFFER_STATUS, rxBufStatus, 2);
  return((size_t)rxBufStatus[0]);
}

int16_t SX126x::fixedPacketLengthMode(uint8_t len) {
  return(setPacketMode(SX126X_GFSK_PACKET_FIXED, len));
}

int16_t SX126x::variablePacketLengthMode(uint8_t maxLen) {
  return(setPacketMode(SX126X_GFSK_PACKET_VARIABLE, maxLen));
}

uint32_t SX126x::getTimeOnAir(size_t len) {
  if(getPacketType() == SX126X_PACKET_TYPE_LORA) {
    float symbolLength = (float)((uint32_t)(1) << _sf) / (float)_bwKhz;
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
    return((uint32_t)(symbolLength * nSymbol * 1000.0));
  } else {
    float brBps = ((float)(SX126X_CRYSTAL_FREQ) * 1000000.0 * 32.0) / (float)_br;
    return((uint32_t)(((len * 8.0) / brBps) * 1000000.0));
  }
}

int16_t SX126x::setTCXO(float voltage, uint32_t delay) {
  // set mode to standby
  standby();

  // check SX126X_XOSC_START_ERR flag and clear it
  if(getDeviceErrors() & SX126X_XOSC_START_ERR) {
    clearDeviceErrors();
  }

  // check alowed voltage values
  uint8_t data[4];
  if(abs(voltage - 1.6) <= 0.001) {
    data[0] = SX126X_DIO3_OUTPUT_1_6;
  } else if(abs(voltage - 1.7) <= 0.001) {
    data[0] = SX126X_DIO3_OUTPUT_1_7;
  } else if(abs(voltage - 1.8) <= 0.001) {
    data[0] = SX126X_DIO3_OUTPUT_1_8;
  } else if(abs(voltage - 2.2) <= 0.001) {
    data[0] = SX126X_DIO3_OUTPUT_2_2;
  } else if(abs(voltage - 2.4) <= 0.001) {
    data[0] = SX126X_DIO3_OUTPUT_2_4;
  } else if(abs(voltage - 2.7) <= 0.001) {
    data[0] = SX126X_DIO3_OUTPUT_2_7;
  } else if(abs(voltage - 3.0) <= 0.001) {
    data[0] = SX126X_DIO3_OUTPUT_3_0;
  } else if(abs(voltage - 3.3) <= 0.001) {
    data[0] = SX126X_DIO3_OUTPUT_3_3;
  } else {
    return(ERR_INVALID_TCXO_VOLTAGE);
  }

  // calculate delay
  uint32_t delayValue = (float)delay / 15.625;
  data[1] = (uint8_t)((delayValue >> 16) & 0xFF);
  data[2] = (uint8_t)((delayValue >> 8) & 0xFF);
  data[3] = (uint8_t)(delayValue & 0xFF);

  // enable TCXO control on DIO3
  return(SPIwriteCommand(SX126X_CMD_SET_DIO3_AS_TCXO_CTRL, data, 4));
}

int16_t SX126x::setDio2AsRfSwitch(bool enable) {
  uint8_t data = 0;
  if(enable) {
    data = SX126X_DIO2_AS_RF_SWITCH;
  } else {
    data = SX126X_DIO2_AS_IRQ;
  }
  return(SPIwriteCommand(SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL, &data, 1));
}

int16_t SX126x::setTx(uint32_t timeout) {
  uint8_t data[3] = {(uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF)};
  return(SPIwriteCommand(SX126X_CMD_SET_TX, data, 3));
}

int16_t SX126x::setRx(uint32_t timeout) {
  uint8_t data[3] = {(uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF)};
  return(SPIwriteCommand(SX126X_CMD_SET_RX, data, 3));
}

int16_t SX126x::setCad() {
  return(SPIwriteCommand(SX126X_CMD_SET_CAD, NULL, 0));
}

int16_t SX126x::setPaConfig(uint8_t paDutyCycle, uint8_t deviceSel, uint8_t hpMax, uint8_t paLut) {
  uint8_t data[4] = {paDutyCycle, hpMax, deviceSel, paLut};
  return(SPIwriteCommand(SX126X_CMD_SET_PA_CONFIG, data, 4));
}

int16_t SX126x::writeRegister(uint16_t addr, uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[] = { SX126X_CMD_WRITE_REGISTER, (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF) };
  int16_t state = SPIwriteCommand(cmd, 3, data, numBytes);
  return(state);
}

int16_t SX126x::readRegister(uint16_t addr, uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[] = {SX126X_CMD_READ_REGISTER, (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF)};
  return(SX126x::SPItransfer(cmd, 3, false, NULL, data, numBytes, true));
}

int16_t SX126x::writeBuffer(uint8_t* data, uint8_t numBytes, uint8_t offset) {
  uint8_t cmd[] = { SX126X_CMD_WRITE_BUFFER, offset };
  int16_t state = SPIwriteCommand(cmd, 2, data, numBytes);

  return(state);
}

int16_t SX126x::readBuffer(uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[] = { SX126X_CMD_READ_BUFFER, SX126X_CMD_NOP };
  int16_t state = SPIreadCommand(cmd, 2, data, numBytes);
  return(state);
}

int16_t SX126x::setDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask) {
  uint8_t data[8] = {(uint8_t)((irqMask >> 8) & 0xFF), (uint8_t)(irqMask & 0xFF),
                     (uint8_t)((dio1Mask >> 8) & 0xFF), (uint8_t)(dio1Mask & 0xFF),
                     (uint8_t)((dio2Mask >> 8) & 0xFF), (uint8_t)(dio2Mask & 0xFF),
                     (uint8_t)((dio3Mask >> 8) & 0xFF), (uint8_t)(dio3Mask & 0xFF)};
  return(SPIwriteCommand(SX126X_CMD_SET_DIO_IRQ_PARAMS, data, 8));
}

uint16_t SX126x::getIrqStatus() {
  uint8_t data[2] = {0, 0};;
  SPIreadCommand(SX126X_CMD_GET_IRQ_STATUS, data, 2);
  return(((uint16_t)(data[0]) << 8) | data[1]);
}

int16_t SX126x::clearIrqStatus(uint16_t clearIrqParams) {
  uint8_t data[2] = {(uint8_t)((clearIrqParams >> 8) & 0xFF), (uint8_t)(clearIrqParams & 0xFF)};
  return(SPIwriteCommand(SX126X_CMD_CLEAR_IRQ_STATUS, data, 2));
}

int16_t SX126x::setRfFrequency(uint32_t frf) {
  uint8_t data[4] = {(uint8_t)((frf >> 24) & 0xFF), (uint8_t)((frf >> 16) & 0xFF), (uint8_t)((frf >> 8) & 0xFF), (uint8_t)(frf & 0xFF)};
  return(SPIwriteCommand(SX126X_CMD_SET_RF_FREQUENCY, data, 4));
}

int16_t SX126x::calibrateImage(uint8_t* data) {
  return(SPIwriteCommand(SX126X_CMD_CALIBRATE_IMAGE, data, 2));
}

uint8_t SX126x::getPacketType() {
  uint8_t data = 0xFF;
  SPIreadCommand(SX126X_CMD_GET_PACKET_TYPE, &data, 1);
  return(data);
}

int16_t SX126x::setTxParams(uint8_t power, uint8_t rampTime) {
  uint8_t data[2] = {power, rampTime};
  return(SPIwriteCommand(SX126X_CMD_SET_TX_PARAMS, data, 2));
}

int16_t SX126x::setOptimalHiPowerPaConfig(int8_t * inOutPower) {
  // set PA config for optimal consumption as described in section 13-21 of SX1268 datasheet v1.1
  // the final column of Table 13-21 suggests that the value passed in SetTxParams
  // is actually scaled depending on the parameters of setPaConfig
  int16_t state;
  if (*inOutPower >= 21) {
    state = SX126x::setPaConfig(0x04, SX126X_PA_CONFIG_SX1262_8, SX126X_PA_CONFIG_HP_MAX/*0x07*/);
  }
  else if (*inOutPower >= 18) {
    state = SX126x::setPaConfig(0x03, SX126X_PA_CONFIG_SX1262_8, 0x05);
    // datasheet instructs request 22 dBm for 20 dBm actual output power
    *inOutPower += 2;
  } else if (*inOutPower >= 15) {
    state = SX126x::setPaConfig(0x02, SX126X_PA_CONFIG_SX1262_8, 0x03);
    // datasheet instructs request 22 dBm for 17 dBm actual output power
    *inOutPower += 5;
  } else {
    state = SX126x::setPaConfig(0x02, SX126X_PA_CONFIG_SX1262_8, 0x02);
    // datasheet instructs request 22 dBm for 14 dBm actual output power.
    *inOutPower += 8;
  }
  return state;
}

int16_t SX126x::setPacketMode(uint8_t mode, uint8_t len) {
  // check active modem
  if(getPacketType() != SX126X_PACKET_TYPE_GFSK) {
    return(ERR_WRONG_MODEM);
  }

  // set requested packet mode
  int16_t state = setPacketParamsFSK(_preambleLengthFSK, _crcTypeFSK, _syncWordLength, _addrComp, _whitening, mode, len);
  if(state != ERR_NONE) {
    return(state);
  }

  // update cached value
  _packetType = mode;
  return(state);
}

int16_t SX126x::setModulationParams(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro) {
  // calculate symbol length and enable low data rate optimization, if needed
  if(ldro == 0xFF) {
    float symbolLength = (float)(uint32_t(1) << _sf) / (float)_bwKhz;
    RADIOLIB_DEBUG_PRINT("Symbol length: ");
    RADIOLIB_DEBUG_PRINT(symbolLength);
    RADIOLIB_DEBUG_PRINTLN(" ms");
    if(symbolLength >= 16.0) {
      _ldro = SX126X_LORA_LOW_DATA_RATE_OPTIMIZE_ON;
    } else {
      _ldro = SX126X_LORA_LOW_DATA_RATE_OPTIMIZE_OFF;
    }
  } else {
    _ldro = ldro;
  }

  uint8_t data[4] = {sf, bw, cr, _ldro};
  return(SPIwriteCommand(SX126X_CMD_SET_MODULATION_PARAMS, data, 4));
}

int16_t SX126x::setModulationParamsFSK(uint32_t br, uint8_t pulseShape, uint8_t rxBw, uint32_t freqDev) {
  uint8_t data[8] = {(uint8_t)((br >> 16) & 0xFF), (uint8_t)((br >> 8) & 0xFF), (uint8_t)(br & 0xFF),
                     pulseShape, rxBw,
                     (uint8_t)((freqDev >> 16) & 0xFF), (uint8_t)((freqDev >> 8) & 0xFF), (uint8_t)(freqDev & 0xFF)};
  return(SPIwriteCommand(SX126X_CMD_SET_MODULATION_PARAMS, data, 8));
}

int16_t SX126x::setPacketParams(uint16_t preambleLength, uint8_t crcType, uint8_t payloadLength, uint8_t headerType, uint8_t invertIQ) {
  fixInvertedIQ(invertIQ);
  uint8_t data[6] = {(uint8_t)((preambleLength >> 8) & 0xFF), (uint8_t)(preambleLength & 0xFF), headerType, payloadLength, crcType, invertIQ};
  return(SPIwriteCommand(SX126X_CMD_SET_PACKET_PARAMS, data, 6));
}

int16_t SX126x::setPacketParamsFSK(uint16_t preambleLength, uint8_t crcType, uint8_t syncWordLength, uint8_t addrComp, uint8_t whitening, uint8_t packetType, uint8_t payloadLength, uint8_t preambleDetectorLength) {
  uint8_t data[9] = {(uint8_t)((preambleLength >> 8) & 0xFF), (uint8_t)(preambleLength & 0xFF),
                     preambleDetectorLength, syncWordLength, addrComp,
                     packetType, payloadLength, crcType, whitening};
  return(SPIwriteCommand(SX126X_CMD_SET_PACKET_PARAMS, data, 9));
}

int16_t SX126x::setBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress) {
  uint8_t data[2] = {txBaseAddress, rxBaseAddress};
  return(SPIwriteCommand(SX126X_CMD_SET_BUFFER_BASE_ADDRESS, data, 2));
}

uint8_t SX126x::getStatus() {
  uint8_t data = 0;
  SPIreadCommand(SX126X_CMD_GET_STATUS, &data, 1);
  return(data);
}

uint32_t SX126x::getPacketStatus() {
  uint8_t data[3] = {0, 0, 0};
  SPIreadCommand(SX126X_CMD_GET_PACKET_STATUS, data, 3);
  return((((uint32_t)data[0]) << 16) | (((uint32_t)data[1]) << 8) | (uint32_t)data[2]);
}

uint16_t SX126x::getDeviceErrors() {
  uint8_t data[2] = {0, 0};
  SPIreadCommand(SX126X_CMD_GET_DEVICE_ERRORS, data, 2);
  uint16_t opError = (((uint16_t)data[0] & 0xFF) << 8) & ((uint16_t)data[1]);
  return(opError);
}

int16_t SX126x::clearDeviceErrors() {
  uint8_t data[2] = {SX126X_CMD_NOP, SX126X_CMD_NOP};
  return(SPIwriteCommand(SX126X_CMD_CLEAR_DEVICE_ERRORS, data, 2));
}

int16_t SX126x::setFrequencyRaw(float freq) {
  // calculate raw value
  uint32_t frf = (freq * (uint32_t(1) << SX126X_DIV_EXPONENT)) / SX126X_CRYSTAL_FREQ;
  return(setRfFrequency(frf));
}

int16_t SX126x::fixSensitivity() {
  // fix receiver sensitivity for 500 kHz LoRa
  // see SX1262/SX1268 datasheet, chapter 15 Known Limitations, section 15.1 for details

  // read current sensitivity configuration
  uint8_t sensitivityConfig = 0;
  int16_t state = readRegister(SX126X_REG_SENSITIVITY_CONFIG, &sensitivityConfig, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // fix the value for LoRa with 500 kHz bandwidth
  if((getPacketType() == SX126X_PACKET_TYPE_LORA) && (abs(_bwKhz - 500.0) <= 0.001)) {
    sensitivityConfig &= 0xFB;
  } else {
    sensitivityConfig |= 0x04;
  }
  return(writeRegister(SX126X_REG_SENSITIVITY_CONFIG, &sensitivityConfig, 1));
}

int16_t SX126x::fixPaClamping() {
  // fixes overly eager PA clamping
  // see SX1262/SX1268 datasheet, chapter 15 Known Limitations, section 15.2 for details

  // read current clamping configuration
  uint8_t clampConfig = 0;
  int16_t state = readRegister(SX126X_REG_TX_CLAMP_CONFIG, &clampConfig, 1);
  if (state != ERR_NONE) {
    return state;
  }

  // update with the new value
  clampConfig |= 0x1E;
  return(writeRegister(SX126X_REG_TX_CLAMP_CONFIG, &clampConfig, 1));
}

int16_t SX126x::fixImplicitTimeout() {
  // fixes timeout in implicit header mode
  // see SX1262/SX1268 datasheet, chapter 15 Known Limitations, section 15.3 for details

  // stop RTC counter
  uint8_t rtcStop = 0x00;
  int16_t state = writeRegister(SX126X_REG_RTC_STOP, &rtcStop, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // read currently active event
  uint8_t rtcEvent = 0;
  state = readRegister(SX126X_REG_RTC_EVENT, &rtcEvent, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // clear events
  rtcEvent |= 0x02;
  return(writeRegister(SX126X_REG_RTC_EVENT, &rtcEvent, 1));
}

int16_t SX126x::fixInvertedIQ(uint8_t iqConfig) {
  // fixes IQ configuration for inverted IQ
  // see SX1262/SX1268 datasheet, chapter 15 Known Limitations, section 15.4 for details

  // read current IQ configuration
  uint8_t iqConfigCurrent = 0;
  int16_t state = readRegister(SX126X_REG_IQ_CONFIG, &iqConfigCurrent, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // set correct IQ configuration
  if(iqConfig == SX126X_LORA_IQ_STANDARD) {
    iqConfigCurrent &= 0xFB;
  } else {
    iqConfigCurrent |= 0x04;
  }

  // update with the new value
  return(writeRegister(SX126X_REG_IQ_CONFIG, &iqConfigCurrent, 1));
}

int16_t SX126x::config(uint8_t modem) {
  // set regulator mode
  uint8_t data[7];
  data[0] = SX126X_REGULATOR_DC_DC;
  int16_t state = SPIwriteCommand(SX126X_CMD_SET_REGULATOR_MODE, data, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // reset buffer base address
  state = setBufferBaseAddress();
  if(state != ERR_NONE) {
    return(state);
  }

  // set modem
  data[0] = modem;
  state = SPIwriteCommand(SX126X_CMD_SET_PACKET_TYPE, data, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // set Rx/Tx fallback mode to STDBY_RC
  data[0] = SX126X_RX_TX_FALLBACK_MODE_STDBY_RC;
  state = SPIwriteCommand(SX126X_CMD_SET_RX_TX_FALLBACK_MODE, data, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // set CAD parameters
  data[0] = SX126X_CAD_ON_8_SYMB;
  data[1] = _sf + 13;
  data[2] = 10;
  data[3] = SX126X_CAD_GOTO_STDBY;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = 0x00;
  state = SPIwriteCommand(SX126X_CMD_SET_CAD_PARAMS, data, 7);
  if(state != ERR_NONE) {
    return(state);
  }

  // clear IRQ
  state = clearIrqStatus();
  state |= setDioIrqParams(SX126X_IRQ_NONE, SX126X_IRQ_NONE);
  if(state != ERR_NONE) {
    return(state);
  }

  // calibrate all blocks
  data[0] = SX126X_CALIBRATE_ALL;
  state = SPIwriteCommand(SX126X_CMD_CALIBRATE, data, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // wait for calibration completion
  delay(5);
  while(digitalRead(_mod->getInt1()));

  return(ERR_NONE);
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
  // get pointer to used SPI interface and the settings
  SPIClass* spi = _mod->getSpi();
  SPISettings spiSettings = _mod->getSpiSettings();

  #ifdef RADIOLIB_VERBOSE
    uint8_t debugBuff[256];
  #endif

  // pull NSS low
  digitalWrite(_mod->getCs(), LOW);

  // ensure BUSY is low (state meachine ready)
  RADIOLIB_VERBOSE_PRINTLN(F("Wait for BUSY ... "));
  uint32_t start = millis();
  while(digitalRead(_mod->getInt1())) {
    if(millis() - start >= timeout) {
      return(ERR_SPI_CMD_TIMEOUT);
    }
  }

  // start transfer
  spi->beginTransaction(spiSettings);

  // send command byte(s)
  for(uint8_t n = 0; n < cmdLen; n++) {
    spi->transfer(cmd[n]);
  }

  // variable to save error during SPI transfer
  uint8_t status = 0;

  // send/receive all bytes
  if(write) {
    for(uint8_t n = 0; n < numBytes; n++) {
      // send byte
      uint8_t in = spi->transfer(dataOut[n]);
      #ifdef RADIOLIB_VERBOSE
        debugBuff[n] = in;
      #endif

      // check status
      if(((in & 0b00001110) == SX126X_STATUS_CMD_TIMEOUT) ||
         ((in & 0b00001110) == SX126X_STATUS_CMD_INVALID) ||
         ((in & 0b00001110) == SX126X_STATUS_CMD_FAILED)) {
        status = in & 0b00001110;
        break;
      } else if(in == 0x00 || in == 0xFF) {
        status = SX126X_STATUS_SPI_FAILED;
        break;
      }
    }

  } else {
    // skip the first byte for read-type commands (status-only)
    uint8_t in = spi->transfer(SX126X_CMD_NOP);
    #ifdef RADIOLIB_VERBOSE
      debugBuff[0] = in;
    #endif

    // check status
    if(((in & 0b00001110) == SX126X_STATUS_CMD_TIMEOUT) ||
       ((in & 0b00001110) == SX126X_STATUS_CMD_INVALID) ||
       ((in & 0b00001110) == SX126X_STATUS_CMD_FAILED)) {
      status = in & 0b00001110;
    } else if(in == 0x00 || in == 0xFF) {
      status = SX126X_STATUS_SPI_FAILED;
    } else {
      for(uint8_t n = 0; n < numBytes; n++) {
        dataIn[n] = spi->transfer(SX126X_CMD_NOP);
      }
    }
  }

  // stop transfer
  spi->endTransaction();
  digitalWrite(_mod->getCs(), HIGH);

  // wait for BUSY to go high and then low
  if(waitForBusy) {
    delayMicroseconds(1);
    start = millis();
    while(digitalRead(_mod->getInt1())) {
        if(millis() - start >= timeout) {
          status = SX126X_STATUS_CMD_TIMEOUT;
          break;
        }
    }
  }

  // print debug output
  #ifdef RADIOLIB_VERBOSE
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
      RADIOLIB_VERBOSE_PRINT(SX126X_CMD_NOP, HEX);
      RADIOLIB_VERBOSE_PRINT('\t');
      RADIOLIB_VERBOSE_PRINT(debugBuff[0], HEX);
      RADIOLIB_VERBOSE_PRINT('\t')

      for(uint8_t n = 0; n < numBytes; n++) {
        RADIOLIB_VERBOSE_PRINT(SX126X_CMD_NOP, HEX);
        RADIOLIB_VERBOSE_PRINT('\t');
        RADIOLIB_VERBOSE_PRINT(dataIn[n], HEX);
        RADIOLIB_VERBOSE_PRINT('\t');
      }
      RADIOLIB_VERBOSE_PRINTLN();
    }
    RADIOLIB_VERBOSE_PRINTLN();
  #endif

  // parse status
  switch(status) {
    case SX126X_STATUS_CMD_TIMEOUT:
      return(ERR_SPI_CMD_TIMEOUT);
    case SX126X_STATUS_CMD_INVALID:
      return(ERR_SPI_CMD_INVALID);
    case SX126X_STATUS_CMD_FAILED:
      return(ERR_SPI_CMD_FAILED);
    case SX126X_STATUS_SPI_FAILED:
      return(ERR_CHIP_NOT_FOUND);
    default:
      return(ERR_NONE);
  }
}
