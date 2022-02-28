#include "SX128x.h"
#if !defined(RADIOLIB_EXCLUDE_SX128X)

SX128x::SX128x(Module* mod) : PhysicalLayer(RADIOLIB_SX128X_FREQUENCY_STEP_SIZE, RADIOLIB_SX128X_MAX_PACKET_LENGTH) {
  _mod = mod;
}

Module* SX128x::getMod() {
  return(_mod);
}

int16_t SX128x::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength) {
  // set module properties
  _mod->init();
  _mod->pinMode(_mod->getIrq(), INPUT);
  _mod->pinMode(_mod->getGpio(), INPUT);
  RADIOLIB_DEBUG_PRINTLN(F("M\tSX128x"));

  // initialize LoRa modulation variables
  _bwKhz = bw;
  _sf = RADIOLIB_SX128X_LORA_SF_9;
  _cr = RADIOLIB_SX128X_LORA_CR_4_7;

  // initialize LoRa packet variables
  _preambleLengthLoRa = preambleLength;
  _headerType = RADIOLIB_SX128X_LORA_HEADER_EXPLICIT;
  _payloadLen = 0xFF;
  _crcLoRa = RADIOLIB_SX128X_LORA_CRC_ON;

  // reset the module and verify startup
  int16_t state = reset();
  RADIOLIB_ASSERT(state);

  // set mode to standby
  state = standby();
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = config(RADIOLIB_SX128X_PACKET_TYPE_LORA);
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

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX128x::beginGFSK(float freq, uint16_t br, float freqDev, int8_t power, uint16_t preambleLength) {
  // set module properties
  _mod->init();
  _mod->pinMode(_mod->getIrq(), INPUT);
  _mod->pinMode(_mod->getGpio(), INPUT);
  RADIOLIB_DEBUG_PRINTLN(F("M\tSX128x"));

  // initialize GFSK modulation variables
  _brKbps = br;
  _br = RADIOLIB_SX128X_BLE_GFSK_BR_0_800_BW_2_4;
  _modIndexReal = 1.0;
  _modIndex = RADIOLIB_SX128X_BLE_GFSK_MOD_IND_1_00;
  _shaping = RADIOLIB_SX128X_BLE_GFSK_BT_0_5;

  // initialize GFSK packet variables
  _preambleLengthGFSK = preambleLength;
  _syncWordLen = 2;
  _syncWordMatch = RADIOLIB_SX128X_GFSK_FLRC_SYNC_WORD_1;
  _crcGFSK = RADIOLIB_SX128X_GFSK_FLRC_CRC_2_BYTE;
  _whitening = RADIOLIB_SX128X_GFSK_BLE_WHITENING_ON;

  // reset the module and verify startup
  int16_t state = reset();
  RADIOLIB_ASSERT(state);

  // set mode to standby
  state = standby();
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = config(RADIOLIB_SX128X_PACKET_TYPE_GFSK);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  state = setDataShaping(RADIOLIB_SHAPING_0_5);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  uint8_t sync[] = { 0x12, 0xAD };
  state = setSyncWord(sync, 2);
  RADIOLIB_ASSERT(state);

  state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX128x::beginBLE(float freq, uint16_t br, float freqDev, int8_t power, uint8_t dataShaping) {
  // set module properties
  _mod->init();
  _mod->pinMode(_mod->getIrq(), INPUT);
  _mod->pinMode(_mod->getGpio(), INPUT);
  RADIOLIB_DEBUG_PRINTLN(F("M\tSX128x"));

  // initialize BLE modulation variables
  _brKbps = br;
  _br = RADIOLIB_SX128X_BLE_GFSK_BR_0_800_BW_2_4;
  _modIndexReal = 1.0;
  _modIndex = RADIOLIB_SX128X_BLE_GFSK_MOD_IND_1_00;
  _shaping = RADIOLIB_SX128X_BLE_GFSK_BT_0_5;

  // initialize BLE packet variables
  _crcGFSK = RADIOLIB_SX128X_BLE_CRC_3_BYTE;
  _whitening = RADIOLIB_SX128X_GFSK_BLE_WHITENING_ON;

  // reset the module and verify startup
  int16_t state = reset();
  RADIOLIB_ASSERT(state);

  // set mode to standby
  state = standby();
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = config(RADIOLIB_SX128X_PACKET_TYPE_BLE);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = setDataShaping(dataShaping);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX128x::beginFLRC(float freq, uint16_t br, uint8_t cr, int8_t power, uint16_t preambleLength, uint8_t dataShaping) {
  // set module properties
  _mod->init();
  _mod->pinMode(_mod->getIrq(), INPUT);
  _mod->pinMode(_mod->getGpio(), INPUT);
  RADIOLIB_DEBUG_PRINTLN(F("M\tSX128x"));

  // initialize FLRC modulation variables
  _brKbps = br;
  _br = RADIOLIB_SX128X_FLRC_BR_0_650_BW_0_6;
  _crFLRC = RADIOLIB_SX128X_FLRC_CR_3_4;
  _shaping = RADIOLIB_SX128X_FLRC_BT_0_5;

  // initialize FLRC packet variables
  _preambleLengthGFSK = preambleLength;
  _syncWordLen = 2;
  _syncWordMatch = RADIOLIB_SX128X_GFSK_FLRC_SYNC_WORD_1;
  _crcGFSK = RADIOLIB_SX128X_GFSK_FLRC_CRC_2_BYTE;
  _whitening = RADIOLIB_SX128X_GFSK_BLE_WHITENING_OFF;

  // reset the module and verify startup
  int16_t state = reset();
  RADIOLIB_ASSERT(state);

  // set mode to standby
  state = standby();
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = config(RADIOLIB_SX128X_PACKET_TYPE_FLRC);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setCodingRate(cr);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  state = setDataShaping(dataShaping);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  uint8_t sync[] = { 0x2D, 0x01, 0x4B, 0x1D};
  state = setSyncWord(sync, 4);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX128x::reset(bool verify) {
  // run the reset sequence - same as SX126x, as SX128x docs don't seem to mention this
  _mod->pinMode(_mod->getRst(), OUTPUT);
  _mod->digitalWrite(_mod->getRst(), LOW);
  _mod->delay(1);
  _mod->digitalWrite(_mod->getRst(), HIGH);

  // return immediately when verification is disabled
  if(!verify) {
    return(RADIOLIB_ERR_NONE);
  }

  // set mode to standby
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

int16_t SX128x::transmit(uint8_t* data, size_t len, uint8_t addr) {
  // check packet length
  if(len > RADIOLIB_SX128X_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // check active modem
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // calculate timeout (500% of expected time-on-air)
  uint32_t timeout = getTimeOnAir(len) * 5;

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

  // clear interrupt flags
  state = clearIrqStatus();
  RADIOLIB_ASSERT(state);

  // set mode to standby to disable transmitter
  state = standby();

  return(state);
}

int16_t SX128x::receive(uint8_t* data, size_t len) {
  // check active modem
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // calculate timeout (1000% of expected time-on-air)
  uint32_t timeout = getTimeOnAir(len) * 10;

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
      clearIrqStatus();
      standby();
      return(RADIOLIB_ERR_RX_TIMEOUT);
    }
  }

  // read the received data
  return(readData(data, len));
}

int16_t SX128x::transmitDirect(uint32_t frf) {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, HIGH);

  // user requested to start transmitting immediately (required for RTTY)
  int16_t state = RADIOLIB_ERR_NONE;
  if(frf != 0) {
    state = setRfFrequency(frf);
  }
  RADIOLIB_ASSERT(state);

  // start transmitting
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_TX_CONTINUOUS_WAVE, NULL, 0));
}

int16_t SX128x::receiveDirect() {
  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // SX128x is unable to output received data directly
  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX128x::scanChannel() {
  // check active modem
  if(getPacketType() != RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set DIO pin mapping
  state = setDioIrqParams(RADIOLIB_SX128X_IRQ_CAD_DETECTED | RADIOLIB_SX128X_IRQ_CAD_DONE, RADIOLIB_SX128X_IRQ_CAD_DETECTED | RADIOLIB_SX128X_IRQ_CAD_DONE);
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqStatus();
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // set mode to CAD
  state = setCad();
  RADIOLIB_ASSERT(state);

  // wait for channel activity detected or timeout
  while(!_mod->digitalRead(_mod->getIrq())) {
    _mod->yield();
  }

  // check CAD result
  uint16_t cadResult = getIrqStatus();
  if(cadResult & RADIOLIB_SX128X_IRQ_CAD_DETECTED) {
    // detected some LoRa activity
    clearIrqStatus();
    return(RADIOLIB_LORA_DETECTED);
  } else if(cadResult & RADIOLIB_SX128X_IRQ_CAD_DONE) {
    // channel is free
    clearIrqStatus();
    return(RADIOLIB_CHANNEL_FREE);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX128x::sleep(bool retainConfig) {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, LOW);

  uint8_t sleepConfig = RADIOLIB_SX128X_SLEEP_DATA_BUFFER_RETAIN | RADIOLIB_SX128X_SLEEP_DATA_RAM_RETAIN;
  if(!retainConfig) {
    sleepConfig = RADIOLIB_SX128X_SLEEP_DATA_BUFFER_FLUSH | RADIOLIB_SX128X_SLEEP_DATA_RAM_FLUSH;
  }
  int16_t state = SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_SLEEP, &sleepConfig, 1, false);

  // wait for SX128x to safely enter sleep mode
  _mod->delay(1);

  return(state);
}

int16_t SX128x::standby() {
  return(SX128x::standby(RADIOLIB_SX128X_STANDBY_RC));
}

int16_t SX128x::standby(uint8_t mode) {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, LOW);

  uint8_t data[] = { mode };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_STANDBY, data, 1));
}

void SX128x::setDio1Action(void (*func)(void)) {
  _mod->attachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getIrq()), func, RISING);
}

void SX128x::clearDio1Action() {
  _mod->detachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getIrq()));
}

int16_t SX128x::startTransmit(uint8_t* data, size_t len, uint8_t addr) {
  // suppress unused variable warning
  (void)addr;

  // check packet length
  if(len > RADIOLIB_SX128X_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // set packet Length
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    state = setPacketParamsLoRa(_preambleLengthLoRa, _headerType, len, _crcLoRa);
  } else if((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC)) {
    state = setPacketParamsGFSK(_preambleLengthGFSK, _syncWordLen, _syncWordMatch, _crcGFSK, _whitening, len);
  } else if(modem == RADIOLIB_SX128X_PACKET_TYPE_BLE) {
    state = setPacketParamsBLE(_connectionState, _crcBLE, _bleTestPayload, _whitening);
  } else {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  RADIOLIB_ASSERT(state);

  // update output power
  state = setTxParams(_pwr);
  RADIOLIB_ASSERT(state);

  // set buffer pointers
  state = setBufferBaseAddress();
  RADIOLIB_ASSERT(state);

  // write packet to buffer
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_BLE) {
    // first 2 bytes of BLE payload are PDU header
    state = writeBuffer(data, len, 2);
    RADIOLIB_ASSERT(state);
  } else {
    state = writeBuffer(data, len);
    RADIOLIB_ASSERT(state);
  }

  // set DIO mapping
  state = setDioIrqParams(RADIOLIB_SX128X_IRQ_TX_DONE | RADIOLIB_SX128X_IRQ_RX_TX_TIMEOUT, RADIOLIB_SX128X_IRQ_TX_DONE);
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqStatus();
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, HIGH);

  // start transmission
  state = setTx(RADIOLIB_SX128X_TX_TIMEOUT_NONE);
  RADIOLIB_ASSERT(state);

  // wait for BUSY to go low (= PA ramp up done)
  while(_mod->digitalRead(_mod->getGpio())) {
    _mod->yield();
  }

  return(state);
}

int16_t SX128x::startReceive(uint16_t timeout) {
  // check active modem
  if(getPacketType() == RADIOLIB_SX128X_PACKET_TYPE_RANGING) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set DIO mapping
  int16_t state = setDioIrqParams(RADIOLIB_SX128X_IRQ_RX_DONE | RADIOLIB_SX128X_IRQ_RX_TX_TIMEOUT | RADIOLIB_SX128X_IRQ_CRC_ERROR | RADIOLIB_SX128X_IRQ_HEADER_ERROR, RADIOLIB_SX128X_IRQ_RX_DONE);
  RADIOLIB_ASSERT(state);

  // set buffer pointers
  state = setBufferBaseAddress();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqStatus();
  RADIOLIB_ASSERT(state);

  // set implicit mode and expected len if applicable
  if((_headerType == RADIOLIB_SX128X_LORA_HEADER_IMPLICIT) && (getPacketType() == RADIOLIB_SX128X_PACKET_TYPE_LORA)) {
    state = setPacketParamsLoRa(_preambleLengthLoRa, _headerType, _payloadLen, _crcLoRa);
    RADIOLIB_ASSERT(state);
  }

  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // set mode to receive
  state = setRx(timeout);

  return(state);
}

int16_t SX128x::readData(uint8_t* data, size_t len) {
  // check active modem
  if(getPacketType() == RADIOLIB_SX128X_PACKET_TYPE_RANGING) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check integrity CRC
  uint16_t irq = getIrqStatus();
  int16_t crcState = RADIOLIB_ERR_NONE;
  if((irq & RADIOLIB_SX128X_IRQ_CRC_ERROR) || (irq & RADIOLIB_SX128X_IRQ_HEADER_ERROR)) {
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

int16_t SX128x::setFrequency(float freq) {
  RADIOLIB_CHECK_RANGE(freq, 2400.0, 2500.0, RADIOLIB_ERR_INVALID_FREQUENCY);

  // calculate raw value
  uint32_t frf = (freq * (uint32_t(1) << RADIOLIB_SX128X_DIV_EXPONENT)) / RADIOLIB_SX128X_CRYSTAL_FREQ;
  return(setRfFrequency(frf));
}

int16_t SX128x::setBandwidth(float bw) {
  // check active modem
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    // check range for LoRa
    RADIOLIB_CHECK_RANGE(bw, 203.125, 1625.0, RADIOLIB_ERR_INVALID_BANDWIDTH);
  } else if(modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING) {
    // check range for ranging
    RADIOLIB_CHECK_RANGE(bw, 406.25, 1625.0, RADIOLIB_ERR_INVALID_BANDWIDTH);
  } else {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  if(fabs(bw - 203.125) <= 0.001) {
    _bw = RADIOLIB_SX128X_LORA_BW_203_125;
  } else if(fabs(bw - 406.25) <= 0.001) {
    _bw = RADIOLIB_SX128X_LORA_BW_406_25;
  } else if(fabs(bw - 812.5) <= 0.001) {
    _bw = RADIOLIB_SX128X_LORA_BW_812_50;
  } else if(fabs(bw - 1625.0) <= 0.001) {
    _bw = RADIOLIB_SX128X_LORA_BW_1625_00;
  } else {
    return(RADIOLIB_ERR_INVALID_BANDWIDTH);
  }

  // update modulation parameters
  _bwKhz = bw;
  return(setModulationParams(_sf, _bw, _cr));
}

int16_t SX128x::setSpreadingFactor(uint8_t sf) {
  // check active modem
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    // check range for LoRa
    RADIOLIB_CHECK_RANGE(sf, 5, 12, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
  } else if(modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING) {
    // check range for ranging
    RADIOLIB_CHECK_RANGE(sf, 5, 10, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
  } else {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update modulation parameters
  _sf = sf << 4;
  int16_t state = setModulationParams(_sf, _bw, _cr);
  RADIOLIB_ASSERT(state);

  // update mystery register in LoRa mode - SX1280 datasheet v3.0 section 13.4.1
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    uint8_t data = 0;
    if((_sf == RADIOLIB_SX128X_LORA_SF_5) || (_sf == RADIOLIB_SX128X_LORA_SF_6)) {
      data = 0x1E;
    } else if((_sf == RADIOLIB_SX128X_LORA_SF_7) || (_sf == RADIOLIB_SX128X_LORA_SF_8)) {
      data = 0x37;
    } else {
      data = 0x32;
    }
    state = SX128x::writeRegister(RADIOLIB_SX128X_REG_LORA_SF_CONFIG, &data, 1);
  }

  return(state);
}

int16_t SX128x::setCodingRate(uint8_t cr, bool longInterleaving) {
  // check active modem
  uint8_t modem = getPacketType();

  // LoRa/ranging
  if((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING)) {
    RADIOLIB_CHECK_RANGE(cr, 5, 8, RADIOLIB_ERR_INVALID_CODING_RATE);

    // update modulation parameters
    if(longInterleaving && (modem == RADIOLIB_SX128X_PACKET_TYPE_LORA)) {
      _cr = cr;
    } else {
      _cr = cr - 4;
    }
    return(setModulationParams(_sf, _bw, _cr));

  // FLRC
  } else if(modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC) {
    RADIOLIB_CHECK_RANGE(cr, 2, 4, RADIOLIB_ERR_INVALID_CODING_RATE);

    // update modulation parameters
    _crFLRC = (cr - 2) * 2;
    return(setModulationParams(_br, _crFLRC, _shaping));
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t SX128x::setOutputPower(int8_t power) {
  RADIOLIB_CHECK_RANGE(power, -18, 13, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  _pwr = power + 18;
  return(setTxParams(_pwr));
}

int16_t SX128x::setPreambleLength(uint32_t preambleLength) {
  uint8_t modem = getPacketType();
  if((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING)) {
    // LoRa or ranging
    RADIOLIB_CHECK_RANGE(preambleLength, 2, 491520, RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);

    // check preamble length is even - no point even trying odd numbers
    if(preambleLength % 2 != 0) {
      return(RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);
    }

    // calculate exponent and mantissa values (use the next longer preamble if there's no exact match)
    uint8_t e = 1;
    uint8_t m = 1;
    uint32_t len = 0;
    for(; e <= 15; e++) {
      for(; m <= 15; m++) {
        len = m * (uint32_t(1) << e);
        if(len >= preambleLength) {
          break;
        }
      }
      if(len >= preambleLength) {
        break;
      }
    }

    // update packet parameters
    _preambleLengthLoRa = (e << 4) | m;
    return(setPacketParamsLoRa(_preambleLengthLoRa, _headerType, _payloadLen, _crcLoRa));

  } else if((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC)) {
    // GFSK or FLRC
    RADIOLIB_CHECK_RANGE(preambleLength, 4, 32, RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);

    // check preamble length is multiple of 4
    if(preambleLength % 4 != 0) {
      return(RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);
    }

    // update packet parameters
    _preambleLengthGFSK = ((preambleLength / 4) - 1) << 4;
    return(setPacketParamsGFSK(_preambleLengthGFSK, _syncWordLen, _syncWordMatch, _crcGFSK, _whitening));
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t SX128x::setBitRate(uint16_t br) {
  // check active modem
  uint8_t modem = getPacketType();

  // GFSK/BLE
  if((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_BLE)) {
    if(br == 125) {
      _br = RADIOLIB_SX128X_BLE_GFSK_BR_0_125_BW_0_3;
    } else if(br == 250) {
      _br = RADIOLIB_SX128X_BLE_GFSK_BR_0_250_BW_0_6;
    } else if(br == 400) {
      _br = RADIOLIB_SX128X_BLE_GFSK_BR_0_400_BW_1_2;
    } else if(br == 500) {
      _br = RADIOLIB_SX128X_BLE_GFSK_BR_0_500_BW_1_2;
    } else if(br == 800) {
      _br = RADIOLIB_SX128X_BLE_GFSK_BR_0_800_BW_2_4;
    } else if(br == 1000) {
      _br = RADIOLIB_SX128X_BLE_GFSK_BR_1_000_BW_2_4;
    } else if(br == 1600) {
      _br = RADIOLIB_SX128X_BLE_GFSK_BR_1_600_BW_2_4;
    } else if(br == 2000) {
      _br = RADIOLIB_SX128X_BLE_GFSK_BR_2_000_BW_2_4;
    } else {
      return(RADIOLIB_ERR_INVALID_BIT_RATE);
    }

    // update modulation parameters
    _brKbps = br;
    return(setModulationParams(_br, _modIndex, _shaping));

  // FLRC
  } else if(modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC) {
    if(br == 260) {
      _br = RADIOLIB_SX128X_FLRC_BR_0_260_BW_0_3;
    } else if(br == 325) {
      _br = RADIOLIB_SX128X_FLRC_BR_0_325_BW_0_3;
    } else if(br == 520) {
      _br = RADIOLIB_SX128X_FLRC_BR_0_520_BW_0_6;
    } else if(br == 650) {
      _br = RADIOLIB_SX128X_FLRC_BR_0_650_BW_0_6;
    } else if(br == 1000) {
      _br = RADIOLIB_SX128X_FLRC_BR_1_000_BW_1_2;
    } else if(br == 1300) {
      _br = RADIOLIB_SX128X_FLRC_BR_1_300_BW_1_2;
    } else {
      return(RADIOLIB_ERR_INVALID_BIT_RATE);
    }

    // update modulation parameters
    _brKbps = br;
    return(setModulationParams(_br, _crFLRC, _shaping));

  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t SX128x::setFrequencyDeviation(float freqDev) {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_BLE))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set frequency deviation to lowest available setting (required for digimodes)
  float newFreqDev = freqDev;
  if(freqDev < 0.0) {
    newFreqDev = 62.5;
  }

  RADIOLIB_CHECK_RANGE(newFreqDev, 62.5, 1000.0, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);

  // override for the lowest possible frequency deviation - required for some PhysicalLayer protocols
  if(newFreqDev == 0.0) {
    _modIndex = RADIOLIB_SX128X_BLE_GFSK_MOD_IND_0_35;
    _br = RADIOLIB_SX128X_BLE_GFSK_BR_0_125_BW_0_3;
    return(setModulationParams(_br, _modIndex, _shaping));
  }

  // update modulation parameters
  uint8_t modIndex = (uint8_t)((8.0 * (newFreqDev / (float)_brKbps)) - 1.0);
  if(modIndex > RADIOLIB_SX128X_BLE_GFSK_MOD_IND_4_00) {
    return(RADIOLIB_ERR_INVALID_MODULATION_PARAMETERS);
  }

  // update modulation parameters
  _modIndex = modIndex;
  return(setModulationParams(_br, _modIndex, _shaping));
}

int16_t SX128x::setDataShaping(uint8_t sh) {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_BLE) || (modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set data shaping
  switch(sh) {
    case RADIOLIB_SHAPING_NONE:
      _shaping = RADIOLIB_SX128X_BLE_GFSK_BT_OFF;
      break;
    case RADIOLIB_SHAPING_0_5:
      _shaping = RADIOLIB_SX128X_BLE_GFSK_BT_0_5;
      break;
    case RADIOLIB_SHAPING_1_0:
      _shaping = RADIOLIB_SX128X_BLE_GFSK_BT_1_0;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_DATA_SHAPING);
  }

  // update modulation parameters
  if((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_BLE)) {
    return(setModulationParams(_br, _modIndex, _shaping));
  } else {
    return(setModulationParams(_br, _crFLRC, _shaping));
  }
}

int16_t SX128x::setSyncWord(uint8_t* syncWord, uint8_t len) {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  if(modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) {
    // GFSK can use up to 5 bytes as sync word
    if(len > 5) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }

    // calculate sync word length parameter value
    if(len > 0) {
      _syncWordLen = (len - 1)*2;
    }

  } else {
    // FLRC requires 32-bit sync word
    if(!((len == 0) || (len == 4))) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }

    // save sync word length parameter value
    _syncWordLen = len;
  }

  // reverse sync word byte order
  uint8_t syncWordBuff[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
  for(uint8_t i = 0; i < len; i++) {
    syncWordBuff[4 - i] = syncWord[i];
  }

  // update sync word
  int16_t state = SX128x::writeRegister(RADIOLIB_SX128X_REG_SYNC_WORD_1_BYTE_4, syncWordBuff, 5);
  RADIOLIB_ASSERT(state);

  // update packet parameters
  if(_syncWordLen == 0) {
    _syncWordMatch = RADIOLIB_SX128X_GFSK_FLRC_SYNC_WORD_OFF;
  } else {
    /// \todo add support for multiple sync words
    _syncWordMatch = RADIOLIB_SX128X_GFSK_FLRC_SYNC_WORD_1;
  }
  return(setPacketParamsGFSK(_preambleLengthGFSK, _syncWordLen, _syncWordMatch, _crcGFSK, _whitening));
}

int16_t SX128x::setSyncWord(uint8_t syncWord, uint8_t controlBits) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update register
  uint8_t data[2] = {(uint8_t)((syncWord & 0xF0) | ((controlBits & 0xF0) >> 4)), (uint8_t)(((syncWord & 0x0F) << 4) | (controlBits & 0x0F))};
  return(writeRegister(RADIOLIB_SX128X_REG_LORA_SYNC_WORD_MSB, data, 2));
}

int16_t SX128x::setCRC(uint8_t len, uint32_t initial, uint16_t polynomial) {
  // check active modem
  uint8_t modem = getPacketType();

  int16_t state;
  if((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC)) {
    // update packet parameters
    if(modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) {
      if(len > 2) {
        return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
      }
    } else {
      if(len > 3) {
        return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
      }
    }
    _crcGFSK = len << 4;
    state = setPacketParamsGFSK(_preambleLengthGFSK, _syncWordLen, _syncWordMatch, _crcGFSK, _whitening);
    RADIOLIB_ASSERT(state);

    // set initial CRC value
    uint8_t data[] = { (uint8_t)((initial >> 8) & 0xFF), (uint8_t)(initial & 0xFF) };
    state = writeRegister(RADIOLIB_SX128X_REG_CRC_INITIAL_MSB, data, 2);
    RADIOLIB_ASSERT(state);

    // set CRC polynomial
    data[0] = (uint8_t)((polynomial >> 8) & 0xFF);
    data[1] = (uint8_t)(polynomial & 0xFF);
    state = writeRegister(RADIOLIB_SX128X_REG_CRC_POLYNOMIAL_MSB, data, 2);
    return(state);

  } else if(modem == RADIOLIB_SX128X_PACKET_TYPE_BLE) {
    // update packet parameters
    if(len == 0) {
      _crcBLE = RADIOLIB_SX128X_BLE_CRC_OFF;
    } else if(len == 3) {
      _crcBLE = RADIOLIB_SX128X_BLE_CRC_3_BYTE;
    } else {
      return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
    }
    state = setPacketParamsBLE(_connectionState, _crcBLE, _bleTestPayload, _whitening);
    RADIOLIB_ASSERT(state);

    // set initial CRC value
    uint8_t data[] = { (uint8_t)((initial >> 16) & 0xFF), (uint8_t)((initial >> 8) & 0xFF), (uint8_t)(initial & 0xFF) };
    state = writeRegister(RADIOLIB_SX128X_REG_BLE_CRC_INITIAL_MSB, data, 3);
    return(state);

  } else if((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING)) {
    // update packet parameters
    if(len == 0) {
      _crcLoRa = RADIOLIB_SX128X_LORA_CRC_OFF;
    } else if(len == 2) {
      _crcLoRa = RADIOLIB_SX128X_LORA_CRC_ON;
    } else {
      return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
    }
    state = setPacketParamsLoRa(_preambleLengthLoRa, _headerType, _payloadLen, _crcLoRa);
    return(state);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX128x::setWhitening(bool enabled) {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_BLE))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update packet parameters
  if(enabled) {
    _whitening = RADIOLIB_SX128X_GFSK_BLE_WHITENING_ON;
  } else {
    _whitening = RADIOLIB_SX128X_GFSK_BLE_WHITENING_OFF;
  }

  if(modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) {
    return(setPacketParamsGFSK(_preambleLengthGFSK, _syncWordLen, _syncWordMatch, _crcGFSK, _whitening));
  }
  return(setPacketParamsBLE(_connectionState, _crcBLE, _bleTestPayload, _whitening));
}

int16_t SX128x::setAccessAddress(uint32_t addr) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX128X_PACKET_TYPE_BLE) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set the address
  uint8_t addrBuff[] = { (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF) };
  return(SX128x::writeRegister(RADIOLIB_SX128X_REG_ACCESS_ADDRESS_BYTE_3, addrBuff, 4));
}

int16_t SX128x::setHighSensitivityMode(bool hsm) {
  // read the current registers
  uint8_t RxGain = 0;
  int16_t state = readRegister(RADIOLIB_SX128X_REG_GAIN_MODE, &RxGain, 1);
  RADIOLIB_ASSERT(state);

  if(hsm) {
    RxGain |= 0xC0; // Set bits 6 and 7
  } else {
    RxGain &= ~0xC0; // Unset bits 6 and 7
  }

  // update all values
  state = writeRegister(RADIOLIB_SX128X_REG_GAIN_MODE, &RxGain, 1);
  return(state);
}

int16_t SX128x::setGainControl(uint8_t gain) {
  // read the current registers
  uint8_t ManualGainSetting = 0;
  int16_t state = readRegister(RADIOLIB_SX128X_REG_MANUAL_GAIN_CONTROL_ENABLE_2, &ManualGainSetting, 1);
  RADIOLIB_ASSERT(state);
  uint8_t LNAGainValue = 0;
  state = readRegister(RADIOLIB_SX128X_REG_MANUAL_GAIN_SETTING, &LNAGainValue, 1);
  RADIOLIB_ASSERT(state);
  uint8_t LNAGainControl = 0;
  state = readRegister(RADIOLIB_SX128X_REG_MANUAL_GAIN_CONTROL_ENABLE_1, &LNAGainControl, 1);
  RADIOLIB_ASSERT(state);

  // set the gain
  if (gain > 0 && gain < 14) {
    // Set manual gain
    ManualGainSetting &= ~0x01; // Set bit 0 to 0 (Enable Manual Gain Control)
    LNAGainValue &= 0xF0; // Bits 0, 1, 2 and 3 to 0
    LNAGainValue |= gain; // Set bits 0, 1, 2 and 3 to Manual Gain Setting (1-13)
    LNAGainControl |= 0x80; // Set bit 7 to 1 (Enable Manual Gain Control)
  } else {
    // Set automatic gain if 0 or out of range
    ManualGainSetting |= 0x01; // Set bit 0 to 1 (Enable Automatic Gain Control)
    LNAGainValue &= 0xF0; // Bits 0, 1, 2 and 3 to 0
    LNAGainValue |= 0x0A; // Set bits 0, 1, 2 and 3 to Manual Gain Setting (1-13)
    LNAGainControl &= ~0x80; // Set bit 7 to 0 (Enable Automatic Gain Control)
  }

  // update all values
  state = writeRegister(RADIOLIB_SX128X_REG_MANUAL_GAIN_CONTROL_ENABLE_2, &ManualGainSetting, 1);
  RADIOLIB_ASSERT(state);
  state = writeRegister(RADIOLIB_SX128X_REG_MANUAL_GAIN_SETTING, &LNAGainValue, 1);
  RADIOLIB_ASSERT(state);
  state = writeRegister(RADIOLIB_SX128X_REG_MANUAL_GAIN_CONTROL_ENABLE_1, &LNAGainControl, 1);
  return(state);
}

float SX128x::getRSSI() {
  // get packet status
  uint8_t packetStatus[5];
  SPIreadCommand(RADIOLIB_SX128X_CMD_GET_PACKET_STATUS, packetStatus, 5);

  // check active modem
  uint8_t modem = getPacketType();
  if((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING)) {
    // LoRa or ranging
    uint8_t rssiSync = packetStatus[0];
    float rssiMeasured = -1.0 * rssiSync/2.0;
    float snr = getSNR();
    if(snr <= 0.0) {
      return(rssiMeasured - snr);
    } else {
      return(rssiMeasured);
    }
  } else {
    // GFSK, BLE or FLRC
    uint8_t rssiSync = packetStatus[1];
    return(-1.0 * rssiSync/2.0);
  }
}

float SX128x::getSNR() {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING))) {
    return(0.0);
  }

  // get packet status
  uint8_t packetStatus[5];
  SPIreadCommand(RADIOLIB_SX128X_CMD_GET_PACKET_STATUS, packetStatus, 5);

  // calculate real SNR
  uint8_t snr = packetStatus[1];
  if(snr < 128) {
    return(snr/4.0);
  } else {
    return((snr - 256)/4.0);
  }
}

float SX128x::getFrequencyError() {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING))) {
    return(0.0);
  }

  // read the raw frequency error register values
  uint8_t efeRaw[3] = {0};
  int16_t state = readRegister(RADIOLIB_SX128X_REG_FEI_MSB, &efeRaw[0], 1);
  RADIOLIB_ASSERT(state);
  state = readRegister(RADIOLIB_SX128X_REG_FEI_MID, &efeRaw[1], 1);
  RADIOLIB_ASSERT(state);
  state = readRegister(RADIOLIB_SX128X_REG_FEI_LSB, &efeRaw[2], 1);
  RADIOLIB_ASSERT(state);
  uint32_t efe = ((uint32_t) efeRaw[0] << 16) | ((uint32_t) efeRaw[1] << 8) | efeRaw[2];
  efe &= 0x0FFFFF;

  float error = 0;

  // check the first bit
  if (efe & 0x80000) {
    // frequency error is negative
    efe |= (uint32_t) 0xFFF00000;
    efe = ~efe + 1;
    error = 1.55 * (float) efe / (1600.0 / (float) _bwKhz) * -1.0;
  } else {
    error = 1.55 * (float) efe / (1600.0 / (float) _bwKhz);
  }

  return(error);
}

size_t SX128x::getPacketLength(bool update) {
  (void)update;
  uint8_t rxBufStatus[2] = {0, 0};
  SPIreadCommand(RADIOLIB_SX128X_CMD_GET_RX_BUFFER_STATUS, rxBufStatus, 2);
  return((size_t)rxBufStatus[0]);
}

uint32_t SX128x::getTimeOnAir(size_t len) {
  // check active modem
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    // calculate number of symbols
    float N_symbol = 0;
    uint8_t sf = _sf >> 4;
    if(_cr <= RADIOLIB_SX128X_LORA_CR_4_8) {
      // legacy coding rate - nice and simple

      // get SF coefficients
      float coeff1 = 0;
      int16_t coeff2 = 0;
      int16_t coeff3 = 0;
      if(sf < 7) {
        // SF5, SF6
        coeff1 = 6.25;
        coeff2 = 4*sf;
        coeff3 = 4*sf;
      } else if(sf < 11) {
        // SF7. SF8, SF9, SF10
        coeff1 = 4.25;
        coeff2 = 4*sf + 8;
        coeff3 = 4*sf;
      } else {
        // SF11, SF12
        coeff1 = 4.25;
        coeff2 = 4*sf + 8;
        coeff3 = 4*(sf - 2);
      }

      // get CRC length
      int16_t N_bitCRC = 16;
      if(_crcLoRa == RADIOLIB_SX128X_LORA_CRC_OFF) {
        N_bitCRC = 0;
      }

      // get header length
      int16_t N_symbolHeader = 20;
      if(_headerType == RADIOLIB_SX128X_LORA_HEADER_IMPLICIT) {
        N_symbolHeader = 0;
      }

      // calculate number of LoRa preamble symbols
      uint32_t N_symbolPreamble = (_preambleLengthLoRa & 0x0F) * (uint32_t(1) << ((_preambleLengthLoRa & 0xF0) >> 4));

      // calculate the number of symbols
      N_symbol = (float)N_symbolPreamble + coeff1 + 8.0 + ceil(max((int16_t)(8 * len + N_bitCRC - coeff2 + N_symbolHeader), (int16_t)0) / (float)coeff3) * (float)(_cr + 4);

    } else {
      // long interleaving - abandon hope all ye who enter here
      /// \todo implement this mess - SX1280 datasheet v3.0 section 7.4.4.2

    }

    // get time-on-air in us
    return(((uint32_t(1) << sf) / _bwKhz) * N_symbol * 1000.0);

  } else {
    return(((uint32_t)len * 8 * 1000) / _brKbps);
  }

}

int16_t SX128x::implicitHeader(size_t len) {
  return(setHeaderType(RADIOLIB_SX128X_LORA_HEADER_IMPLICIT, len));
}

int16_t SX128x::explicitHeader() {
  return(setHeaderType(RADIOLIB_SX128X_LORA_HEADER_EXPLICIT));
}

int16_t SX128x::setEncoding(uint8_t encoding) {
  return(setWhitening(encoding));
}

void SX128x::setRfSwitchPins(RADIOLIB_PIN_TYPE rxEn, RADIOLIB_PIN_TYPE txEn) {
  _mod->setRfSwitchPins(rxEn, txEn);
}

uint8_t SX128x::randomByte() {
  // it's unclear whether SX128x can measure RSSI while not receiving a packet
  // this method is implemented only for PhysicalLayer compatibility
  return(0);
}

void SX128x::setDirectAction(void (*func)(void)) {
  // SX128x is unable to perform direct mode reception
  // this method is implemented only for PhysicalLayer compatibility
  (void)func;
}

void SX128x::readBit(RADIOLIB_PIN_TYPE pin) {
  // SX128x is unable to perform direct mode reception
  // this method is implemented only for PhysicalLayer compatibility
  (void)pin;
}

uint8_t SX128x::getStatus() {
  uint8_t data = 0;
  SPIreadCommand(RADIOLIB_SX128X_CMD_GET_STATUS, &data, 1);
  return(data);
}

int16_t SX128x::writeRegister(uint16_t addr, uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[] = { RADIOLIB_SX128X_CMD_WRITE_REGISTER, (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF) };
  return(SPIwriteCommand(cmd, 3, data, numBytes));
}

int16_t SX128x::readRegister(uint16_t addr, uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[] = { RADIOLIB_SX128X_CMD_READ_REGISTER, (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF) };
  return(SX128x::SPItransfer(cmd, 3, false, NULL, data, numBytes, true));
}

int16_t SX128x::writeBuffer(uint8_t* data, uint8_t numBytes, uint8_t offset) {
  uint8_t cmd[] = { RADIOLIB_SX128X_CMD_WRITE_BUFFER, offset };
  return(SPIwriteCommand(cmd, 2, data, numBytes));
}

int16_t SX128x::readBuffer(uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[] = { RADIOLIB_SX128X_CMD_READ_BUFFER, RADIOLIB_SX128X_CMD_NOP };
  return(SPIreadCommand(cmd, 2, data, numBytes));
}

int16_t SX128x::setTx(uint16_t periodBaseCount, uint8_t periodBase) {
  uint8_t data[] = { periodBase, (uint8_t)((periodBaseCount >> 8) & 0xFF), (uint8_t)(periodBaseCount & 0xFF) };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_TX, data, 3));
}

int16_t SX128x::setRx(uint16_t periodBaseCount, uint8_t periodBase) {
  uint8_t data[] = { periodBase, (uint8_t)((periodBaseCount >> 8) & 0xFF), (uint8_t)(periodBaseCount & 0xFF) };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_RX, data, 3));
}

int16_t SX128x::setCad() {
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_CAD, NULL, 0));
}

uint8_t SX128x::getPacketType() {
  uint8_t data = 0xFF;
  SPIreadCommand(RADIOLIB_SX128X_CMD_GET_PACKET_TYPE, &data, 1);
  return(data);
}

int16_t SX128x::setRfFrequency(uint32_t frf) {
  uint8_t data[] = { (uint8_t)((frf >> 16) & 0xFF), (uint8_t)((frf >> 8) & 0xFF), (uint8_t)(frf & 0xFF) };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_RF_FREQUENCY, data, 3));
}

int16_t SX128x::setTxParams(uint8_t power, uint8_t rampTime) {
  uint8_t data[] = { power, rampTime };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_TX_PARAMS, data, 2));
}

int16_t SX128x::setBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress) {
  uint8_t data[] = { txBaseAddress, rxBaseAddress };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_BUFFER_BASE_ADDRESS, data, 2));
}

int16_t SX128x::setModulationParams(uint8_t modParam1, uint8_t modParam2, uint8_t modParam3) {
  uint8_t data[] = { modParam1, modParam2, modParam3 };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_MODULATION_PARAMS, data, 3));
}

int16_t SX128x::setPacketParamsGFSK(uint8_t preambleLen, uint8_t syncWordLen, uint8_t syncWordMatch, uint8_t crcLen, uint8_t whitening, uint8_t payloadLen, uint8_t headerType) {
  uint8_t data[] = { preambleLen, syncWordLen, syncWordMatch, headerType, payloadLen, crcLen, whitening };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_PACKET_PARAMS, data, 7));
}

int16_t SX128x::setPacketParamsBLE(uint8_t connState, uint8_t crcLen, uint8_t bleTestPayload, uint8_t whitening) {
  uint8_t data[] = { connState, crcLen, bleTestPayload, whitening, 0x00, 0x00, 0x00 };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_PACKET_PARAMS, data, 7));
}

int16_t SX128x::setPacketParamsLoRa(uint8_t preambleLen, uint8_t headerType, uint8_t payloadLen, uint8_t crc, uint8_t invertIQ) {
  uint8_t data[] = { preambleLen, headerType, payloadLen, crc, invertIQ, 0x00, 0x00 };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_PACKET_PARAMS, data, 7));
}

int16_t SX128x::setDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask) {
  uint8_t data[] = { (uint8_t)((irqMask >> 8) & 0xFF), (uint8_t)(irqMask & 0xFF),
                     (uint8_t)((dio1Mask >> 8) & 0xFF), (uint8_t)(dio1Mask & 0xFF),
                     (uint8_t)((dio2Mask >> 8) & 0xFF), (uint8_t)(dio2Mask & 0xFF),
                     (uint8_t)((dio3Mask >> 8) & 0xFF), (uint8_t)(dio3Mask & 0xFF) };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_DIO_IRQ_PARAMS, data, 8));
}

uint16_t SX128x::getIrqStatus() {
  uint8_t data[] = { 0x00, 0x00 };
  SPIreadCommand(RADIOLIB_SX128X_CMD_GET_IRQ_STATUS, data, 2);
  return(((uint16_t)(data[0]) << 8) | data[1]);
}

int16_t SX128x::clearIrqStatus(uint16_t clearIrqParams) {
  uint8_t data[] = { (uint8_t)((clearIrqParams >> 8) & 0xFF), (uint8_t)(clearIrqParams & 0xFF) };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_CLEAR_IRQ_STATUS, data, 2));
}

int16_t SX128x::setRangingRole(uint8_t role) {
  uint8_t data[] = { role };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_RANGING_ROLE, data, 1));
}

int16_t SX128x::setPacketType(uint8_t type) {
  uint8_t data[] = { type };
  return(SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_PACKET_TYPE, data, 1));
}

int16_t SX128x::setHeaderType(uint8_t headerType, size_t len) {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update packet parameters
  _headerType = headerType;
  _payloadLen = len;
  return(setPacketParamsLoRa(_preambleLengthLoRa, _headerType, _payloadLen, _crcLoRa));
}

int16_t SX128x::config(uint8_t modem) {
  // reset buffer base address
  int16_t state = setBufferBaseAddress();
  RADIOLIB_ASSERT(state);

  // set modem
  uint8_t data[1];
  data[0] = modem;
  state = SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_PACKET_TYPE, data, 1);
  RADIOLIB_ASSERT(state);

  // set CAD parameters
  data[0] = RADIOLIB_SX128X_CAD_ON_8_SYMB;
  state = SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_CAD_PARAMS, data, 1);
  RADIOLIB_ASSERT(state);

  // set regulator mode to DC-DC
  data[0] = RADIOLIB_SX128X_REGULATOR_DC_DC;
  state = SPIwriteCommand(RADIOLIB_SX128X_CMD_SET_REGULATOR_MODE, data, 1);
  RADIOLIB_ASSERT(state);

  return(RADIOLIB_ERR_NONE);
}

int16_t SX128x::SPIwriteCommand(uint8_t* cmd, uint8_t cmdLen, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  return(SX128x::SPItransfer(cmd, cmdLen, true, data, NULL, numBytes, waitForBusy));
}

int16_t SX128x::SPIwriteCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  return(SX128x::SPItransfer(&cmd, 1, true, data, NULL, numBytes, waitForBusy));
}

int16_t SX128x::SPIreadCommand(uint8_t* cmd, uint8_t cmdLen, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  return(SX128x::SPItransfer(cmd, cmdLen, false, NULL, data, numBytes, waitForBusy));
}

int16_t SX128x::SPIreadCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  return(SX128x::SPItransfer(&cmd, 1, false, NULL, data, numBytes, waitForBusy));
}

int16_t SX128x::SPItransfer(uint8_t* cmd, uint8_t cmdLen, bool write, uint8_t* dataOut, uint8_t* dataIn, uint8_t numBytes, bool waitForBusy, uint32_t timeout) {
  #if defined(RADIOLIB_VERBOSE)
    uint8_t debugBuff[256];
  #endif

  // ensure BUSY is low (state machine ready)
  uint32_t start = _mod->millis();
  while(_mod->digitalRead(_mod->getGpio())) {
    _mod->yield();
    if(_mod->millis() - start >= timeout) {
      _mod->digitalWrite(_mod->getCs(), HIGH);
      return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
    }
  }

  // pull NSS low
  _mod->digitalWrite(_mod->getCs(), LOW);

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
      if(((in & 0b00011100) == RADIOLIB_SX128X_STATUS_CMD_TIMEOUT) ||
         ((in & 0b00011100) == RADIOLIB_SX128X_STATUS_CMD_ERROR) ||
         ((in & 0b00011100) == RADIOLIB_SX128X_STATUS_CMD_FAILED)) {
        status = in & 0b00011100;
        break;
      } else if(in == 0x00 || in == 0xFF) {
        status = RADIOLIB_SX128X_STATUS_SPI_FAILED;
        break;
      }
    }

  } else {
    // skip the first byte for read-type commands (status-only)
    uint8_t in = _mod->SPItransfer(RADIOLIB_SX128X_CMD_NOP);
    #if defined(RADIOLIB_VERBOSE)
      debugBuff[0] = in;
    #endif

    // check status
    if(((in & 0b00011100) == RADIOLIB_SX128X_STATUS_CMD_TIMEOUT) ||
       ((in & 0b00011100) == RADIOLIB_SX128X_STATUS_CMD_ERROR) ||
       ((in & 0b00011100) == RADIOLIB_SX128X_STATUS_CMD_FAILED)) {
      status = in & 0b00011100;
    } else if(in == 0x00 || in == 0xFF) {
      status = RADIOLIB_SX128X_STATUS_SPI_FAILED;
    } else {
      for(uint8_t n = 0; n < numBytes; n++) {
        dataIn[n] = _mod->SPItransfer(RADIOLIB_SX128X_CMD_NOP);
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
        status = RADIOLIB_SX128X_STATUS_CMD_TIMEOUT;
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
      RADIOLIB_VERBOSE_PRINT(RADIOLIB_SX128X_CMD_NOP, HEX);
      RADIOLIB_VERBOSE_PRINT('\t');
      RADIOLIB_VERBOSE_PRINT(debugBuff[0], HEX);
      RADIOLIB_VERBOSE_PRINT('\t')

      for(uint8_t n = 0; n < numBytes; n++) {
        RADIOLIB_VERBOSE_PRINT(RADIOLIB_SX128X_CMD_NOP, HEX);
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
    case RADIOLIB_SX128X_STATUS_CMD_TIMEOUT:
      return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
    case RADIOLIB_SX128X_STATUS_CMD_ERROR:
      return(RADIOLIB_ERR_SPI_CMD_INVALID);
    case RADIOLIB_SX128X_STATUS_CMD_FAILED:
      return(RADIOLIB_ERR_SPI_CMD_FAILED);
    case RADIOLIB_SX128X_STATUS_SPI_FAILED:
      return(RADIOLIB_ERR_CHIP_NOT_FOUND);
    default:
      return(RADIOLIB_ERR_NONE);
  }
}

#endif
