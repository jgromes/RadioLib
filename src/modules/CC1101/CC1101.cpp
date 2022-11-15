#include "CC1101.h"
#if !defined(RADIOLIB_EXCLUDE_CC1101)

CC1101::CC1101(Module* module) : PhysicalLayer(RADIOLIB_CC1101_FREQUENCY_STEP_SIZE, RADIOLIB_CC1101_MAX_PACKET_LENGTH) {
  _mod = module;
}

Module* CC1101::getMod() {
  return(_mod);
}

int16_t CC1101::begin(float freq, float br, float freqDev, float rxBw, int8_t power, uint8_t preambleLength) {
  // set module properties
  _mod->SPIreadCommand = RADIOLIB_CC1101_CMD_READ;
  _mod->SPIwriteCommand = RADIOLIB_CC1101_CMD_WRITE;
  _mod->init();
  _mod->pinMode(_mod->getIrq(), INPUT);

  // try to find the CC1101 chip
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    int16_t version = getChipVersion();
    if((version == RADIOLIB_CC1101_VERSION_CURRENT) || (version == RADIOLIB_CC1101_VERSION_LEGACY) || (version == RADIOLIB_CC1101_VERSION_CLONE)) {
      flagFound = true;
    } else {
      #if defined(RADIOLIB_DEBUG)
        RADIOLIB_DEBUG_PRINT(F("CC1101 not found! ("));
        RADIOLIB_DEBUG_PRINT(i + 1);
        RADIOLIB_DEBUG_PRINT(F(" of 10 tries) RADIOLIB_CC1101_REG_VERSION == "));

        char buffHex[7];
        sprintf(buffHex, "0x%04X", version);
        RADIOLIB_DEBUG_PRINT(buffHex);
        RADIOLIB_DEBUG_PRINT(F(", expected 0x0004/0x0014"));
        RADIOLIB_DEBUG_PRINTLN();
      #endif
      _mod->delay(10);
      i++;
    }
  }

  if(!flagFound) {
    RADIOLIB_DEBUG_PRINTLN(F("No CC1101 found!"));
    _mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  } else {
    RADIOLIB_DEBUG_PRINTLN(F("M\tCC1101"));
  }

  // configure settings not accessible by API
  int16_t state = config();
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  // configure bitrate
  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  // configure default RX bandwidth
  state = setRxBandwidth(rxBw);
  RADIOLIB_ASSERT(state);

  // configure default frequency deviation
  state = setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  // configure default TX output power
  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  // set default packet length mode
  state = variablePacketLengthMode();
  RADIOLIB_ASSERT(state);

  // configure default preamble length
  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // set default data shaping
  state = setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);

  // set default encoding
  state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  // set default sync word
  uint8_t sw[2] = RADIOLIB_CC1101_DEFAULT_SW;
  state = setSyncWord(sw[0], sw[1], 0, false);
  RADIOLIB_ASSERT(state);

  // flush FIFOs
  SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_RX);
  SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_TX);

  return(state);
}

int16_t CC1101::transmit(uint8_t* data, size_t len, uint8_t addr) {
  // calculate timeout (5ms + 500 % of expected time-on-air)
  uint32_t timeout = 5000000 + (uint32_t)((((float)(len * 8)) / (_br * 1000.0)) * 5000000.0);

  // start transmission
  int16_t state = startTransmit(data, len, addr);
  RADIOLIB_ASSERT(state);

  // wait for transmission start or timeout
  uint32_t start = _mod->micros();
  while(!_mod->digitalRead(_mod->getIrq())) {
    _mod->yield();

    if(_mod->micros() - start > timeout) {
      finishTransmit();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }

  // wait for transmission end or timeout
  start = _mod->micros();
  while(_mod->digitalRead(_mod->getIrq())) {
    _mod->yield();

    if(_mod->micros() - start > timeout) {
      finishTransmit();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }

  return(finishTransmit());
}

int16_t CC1101::receive(uint8_t* data, size_t len) {
  // calculate timeout (500 ms + 400 full max-length packets at current bit rate)
  uint32_t timeout = 500000 + (1.0/(_br*1000.0))*(RADIOLIB_CC1101_MAX_PACKET_LENGTH*400.0);

  // start reception
  int16_t state = startReceive();
  RADIOLIB_ASSERT(state);

  // wait for packet or timeout
  uint32_t start = _mod->micros();
  while(!_mod->digitalRead(_mod->getIrq())) {
    _mod->yield();

    if(_mod->micros() - start > timeout) {
      standby();
      SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_RX);
      return(RADIOLIB_ERR_RX_TIMEOUT);
    }
  }

  // read packet data
  return(readData(data, len));
}

int16_t CC1101::standby() {
  // set idle mode
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, LOW);
  return(RADIOLIB_ERR_NONE);
}

int16_t CC1101::transmitDirect(uint32_t frf) {
  return transmitDirect(true, frf);
}

int16_t CC1101::transmitDirectAsync(uint32_t frf) {
  return transmitDirect(false, frf);
}

int16_t CC1101::transmitDirect(bool sync, uint32_t frf) {
  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, HIGH);

  // user requested to start transmitting immediately (required for RTTY)
  if(frf != 0) {
    SPIwriteRegister(RADIOLIB_CC1101_REG_FREQ2, (frf & 0xFF0000) >> 16);
    SPIwriteRegister(RADIOLIB_CC1101_REG_FREQ1, (frf & 0x00FF00) >> 8);
    SPIwriteRegister(RADIOLIB_CC1101_REG_FREQ0, frf & 0x0000FF);

    SPIsendCommand(RADIOLIB_CC1101_CMD_TX);
  }

  // activate direct mode
  int16_t state = directMode(sync);
  RADIOLIB_ASSERT(state);

  // start transmitting
  SPIsendCommand(RADIOLIB_CC1101_CMD_TX);
  return(state);
}

int16_t CC1101::receiveDirect() {
  return receiveDirect(true);
}

int16_t CC1101::receiveDirectAsync() {
  return receiveDirect(false);
}

int16_t CC1101::receiveDirect(bool sync) {
  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // activate direct mode
  int16_t state = directMode(sync);
  RADIOLIB_ASSERT(state);

  // start receiving
  SPIsendCommand(RADIOLIB_CC1101_CMD_RX);
  return(RADIOLIB_ERR_NONE);
}

int16_t CC1101::packetMode() {
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, RADIOLIB_CC1101_CRC_AUTOFLUSH_OFF | RADIOLIB_CC1101_APPEND_STATUS_ON | RADIOLIB_CC1101_ADR_CHK_NONE, 3, 0);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_WHITE_DATA_OFF | RADIOLIB_CC1101_PKT_FORMAT_NORMAL, 6, 4);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_CRC_ON | _packetLengthConfig, 2, 0);
  return(state);
}

void CC1101::setGdo0Action(void (*func)(void), RADIOLIB_INTERRUPT_STATUS dir) {
  _mod->attachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getIrq()), func, dir);
}

void CC1101::clearGdo0Action() {
  _mod->detachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getIrq()));
}

void CC1101::setGdo2Action(void (*func)(void), RADIOLIB_INTERRUPT_STATUS dir) {
  if(_mod->getGpio() != RADIOLIB_NC) {
    return;
  }
  _mod->pinMode(_mod->getGpio(), INPUT);
  _mod->attachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getGpio()), func, dir);
}

void CC1101::clearGdo2Action() {
  if(_mod->getGpio() != RADIOLIB_NC) {
    return;
  }
  _mod->detachInterrupt(RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(_mod->getGpio()));
}

int16_t CC1101::startTransmit(uint8_t* data, size_t len, uint8_t addr) {
  // check packet length
  if(len > RADIOLIB_CC1101_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // set mode to standby
  standby();

  // flush Tx FIFO
  SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_TX);

  // set GDO0 mapping
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG0, RADIOLIB_CC1101_GDOX_SYNC_WORD_SENT_OR_RECEIVED);
  RADIOLIB_ASSERT(state);

  // data put on FIFO.
  uint8_t dataSent = 0;

  // optionally write packet length
  if (_packetLengthConfig == RADIOLIB_CC1101_LENGTH_CONFIG_VARIABLE) {

    // enforce variable len limit.
    if (len > RADIOLIB_CC1101_MAX_PACKET_LENGTH - 1) {
      return (RADIOLIB_ERR_PACKET_TOO_LONG);
    }

    SPIwriteRegister(RADIOLIB_CC1101_REG_FIFO, len);
    dataSent += 1;
  }

  // check address filtering
  uint8_t filter = SPIgetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, 1, 0);
  if(filter != RADIOLIB_CC1101_ADR_CHK_NONE) {
    SPIwriteRegister(RADIOLIB_CC1101_REG_FIFO, addr);
    dataSent += 1;
  }

  // fill the FIFO.
  uint8_t initialWrite = min((uint8_t)len, (uint8_t)(RADIOLIB_CC1101_FIFO_SIZE - dataSent));
  SPIwriteRegisterBurst(RADIOLIB_CC1101_REG_FIFO, data, initialWrite);
  dataSent += initialWrite;

  // set RF switch (if present)
  _mod->setRfSwitchState(LOW, HIGH);

  // set mode to transmit
  SPIsendCommand(RADIOLIB_CC1101_CMD_TX);

  // keep feeding the FIFO until the packet is over.
  while (dataSent < len) {
    // get number of bytes in FIFO.
    uint8_t bytesInFIFO = SPIgetRegValue(RADIOLIB_CC1101_REG_TXBYTES, 6, 0);

    // if there's room then put other data.
    if (bytesInFIFO < RADIOLIB_CC1101_FIFO_SIZE) {
      uint8_t bytesToWrite = min((uint8_t)(RADIOLIB_CC1101_FIFO_SIZE - bytesInFIFO), (uint8_t)(len - dataSent));
      SPIwriteRegisterBurst(RADIOLIB_CC1101_REG_FIFO, &data[dataSent], bytesToWrite);
      dataSent += bytesToWrite;
    } else {
      // wait for radio to send some data.
      /*
        * Does this work for all rates? If 1 ms is longer than the 1ms delay
        * then the entire FIFO will be transmitted during that delay.
        *
        * TODO: test this on real hardware
      */
     delayMicroseconds(250);
    }
  }

  return (state);
}

int16_t CC1101::finishTransmit() {
  // set mode to standby to disable transmitter/RF switch
  int16_t state = standby();

  // flush Tx FIFO
  SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_TX);

  return(state);
}

int16_t CC1101::startReceive() {
  // set mode to standby
  standby();

  // flush Rx FIFO
  SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_RX);

  // set GDO0 mapping: Asserted when RX FIFO > 4 bytes.
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG0, RADIOLIB_CC1101_GDOX_RX_FIFO_FULL_OR_PKT_END);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_FIFOTHR, RADIOLIB_CC1101_FIFO_THR_TX_61_RX_4, 3, 0);
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  _mod->setRfSwitchState(HIGH, LOW);

  // set mode to receive
  SPIsendCommand(RADIOLIB_CC1101_CMD_RX);

  return(state);
}

int16_t CC1101::readData(uint8_t* data, size_t len) {
  // get packet length
  size_t length = getPacketLength();
  if((len != 0) && (len < length)) {
    // user requested less data than we got, only return what was requested
    length = len;
  }

  // check address filtering
  uint8_t filter = SPIgetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, 1, 0);
  if(filter != RADIOLIB_CC1101_ADR_CHK_NONE) {
    SPIreadRegister(RADIOLIB_CC1101_REG_FIFO);
  }

  uint8_t bytesInFIFO = SPIgetRegValue(RADIOLIB_CC1101_REG_RXBYTES, 6, 0);
  size_t readBytes = 0;
  uint32_t lastPop = millis();

  // keep reading from FIFO until we get all the packet.
  while (readBytes < length) {
    if (bytesInFIFO == 0) {
      if (millis() - lastPop > 5) {
        // readData was required to read a packet longer than the one received.
        RADIOLIB_DEBUG_PRINTLN(F("No data for more than 5mS. Stop here."));
        break;
      } else {
        delay(1);
        bytesInFIFO = SPIgetRegValue(RADIOLIB_CC1101_REG_RXBYTES, 6, 0);
        continue;
      }
    }

    // read the minimum between "remaining length" and bytesInFifo
    uint8_t bytesToRead = min((uint8_t)(length - readBytes), bytesInFIFO);
    SPIreadRegisterBurst(RADIOLIB_CC1101_REG_FIFO, bytesToRead, &(data[readBytes]));
    readBytes += bytesToRead;
    lastPop = millis();

    // Get how many bytes are left in FIFO.
    bytesInFIFO = SPIgetRegValue(RADIOLIB_CC1101_REG_RXBYTES, 6, 0);
  }

  // check if status bytes are enabled (default: RADIOLIB_CC1101_APPEND_STATUS_ON)
  bool isAppendStatus = SPIgetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, 2, 2) == RADIOLIB_CC1101_APPEND_STATUS_ON;

  // for some reason, we need this delay here to get the correct status bytes
  delay(3);

  // If status byte is enabled at least 2 bytes (2 status bytes + any following packet) will remain in FIFO.
  if (isAppendStatus) {
    // read RSSI byte
    _rawRSSI = SPIgetRegValue(RADIOLIB_CC1101_REG_FIFO);

    // read LQI and CRC byte
    uint8_t val = SPIgetRegValue(RADIOLIB_CC1101_REG_FIFO);
    _rawLQI = val & 0x7F;

    // check CRC
    if (_crcOn && (val & RADIOLIB_CC1101_CRC_OK) == RADIOLIB_CC1101_CRC_ERROR) {
      _packetLengthQueried = false;
      return (RADIOLIB_ERR_CRC_MISMATCH);
    }
  }

  // clear internal flag so getPacketLength can return the new packet length
  _packetLengthQueried = false;

  // Flush then standby according to RXOFF_MODE (default: RADIOLIB_CC1101_RXOFF_IDLE)
  if (SPIgetRegValue(RADIOLIB_CC1101_REG_MCSM1, 3, 2) == RADIOLIB_CC1101_RXOFF_IDLE) {

    // flush Rx FIFO
    SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_RX);

    // set mode to standby
    standby();
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t CC1101::setFrequency(float freq) {
  // check allowed frequency range
  if(!(((freq > 300.0) && (freq < 348.0)) ||
       ((freq > 387.0) && (freq < 464.0)) ||
       ((freq > 779.0) && (freq < 928.0)))) {
    return(RADIOLIB_ERR_INVALID_FREQUENCY);
  }

  // set mode to standby
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  //set carrier frequency
  uint32_t base = 1;
  uint32_t FRF = (freq * (base << 16)) / 26.0;
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_FREQ2, (FRF & 0xFF0000) >> 16, 7, 0);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_FREQ1, (FRF & 0x00FF00) >> 8, 7, 0);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_FREQ0, FRF & 0x0000FF, 7, 0);

  if(state == RADIOLIB_ERR_NONE) {
    _freq = freq;
  }

  // Update the TX power accordingly to new freq. (PA values depend on chosen freq)
  return(setOutputPower(_power));
}

int16_t CC1101::setBitRate(float br) {
  RADIOLIB_CHECK_RANGE(br, 0.025, 600.0, RADIOLIB_ERR_INVALID_BIT_RATE);

  // set mode to standby
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  // calculate exponent and mantissa values
  uint8_t e = 0;
  uint8_t m = 0;
  getExpMant(br * 1000.0, 256, 28, 14, e, m);

  // set bit rate value
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG4, e, 3, 0);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG3, m);
  if(state == RADIOLIB_ERR_NONE) {
    _br = br;
  }
  return(state);
}

int16_t CC1101::setRxBandwidth(float rxBw) {
  RADIOLIB_CHECK_RANGE(rxBw, 58.0, 812.0, RADIOLIB_ERR_INVALID_RX_BANDWIDTH);

  // set mode to standby
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  // calculate exponent and mantissa values
  for(int8_t e = 3; e >= 0; e--) {
    for(int8_t m = 3; m >= 0; m --) {
      float point = (RADIOLIB_CC1101_CRYSTAL_FREQ * 1000000.0)/(8 * (m + 4) * ((uint32_t)1 << e));
      if(fabs((rxBw * 1000.0) - point) <= 1000) {
        // set Rx channel filter bandwidth
        uint16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG4, (e << 6) | (m << 4), 7, 4);

        if(state == RADIOLIB_ERR_NONE) {
          _rxBw = rxBw;
        }

        return(state);
      }

    }
  }

  return(RADIOLIB_ERR_INVALID_RX_BANDWIDTH);
}

int16_t CC1101::setFrequencyDeviation(float freqDev) {
  // set frequency deviation to lowest available setting (required for digimodes)
  float newFreqDev = freqDev;
  if(freqDev < 0.0) {
    newFreqDev = 1.587;
  }

  RADIOLIB_CHECK_RANGE(newFreqDev, 1.587, 380.8, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);

  // set mode to standby
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  // calculate exponent and mantissa values
  uint8_t e = 0;
  uint8_t m = 0;
  getExpMant(newFreqDev * 1000.0, 8, 17, 7, e, m);

  // set frequency deviation value
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_DEVIATN, (e << 4), 6, 4);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_DEVIATN, m, 2, 0);
  
    if(state == RADIOLIB_ERR_NONE) {
    _freqDev = freqDev;
  }


  
  return(state);
}

int16_t CC1101::setOutputPower(int8_t power) {
  // round to the known frequency settings
  uint8_t f;
  if(_freq < 374.0) {
    // 315 MHz
    f = 0;
  } else if(_freq < 650.5) {
    // 434 MHz
    f = 1;
  } else if(_freq < 891.5) {
    // 868 MHz
    f = 2;
  } else {
    // 915 MHz
    f = 3;
  }

  // get raw power setting
  uint8_t paTable[8][4] = {{0x12, 0x12, 0x03, 0x03},
                           {0x0D, 0x0E, 0x0F, 0x0E},
                           {0x1C, 0x1D, 0x1E, 0x1E},
                           {0x34, 0x34, 0x27, 0x27},
                           {0x51, 0x60, 0x50, 0x8E},
                           {0x85, 0x84, 0x81, 0xCD},
                           {0xCB, 0xC8, 0xCB, 0xC7},
                           {0xC2, 0xC0, 0xC2, 0xC0}};

  uint8_t powerRaw;
  switch(power) {
    case -30:
      powerRaw = paTable[0][f];
      break;
    case -20:
      powerRaw = paTable[1][f];
      break;
    case -15:
      powerRaw = paTable[2][f];
      break;
    case -10:
      powerRaw = paTable[3][f];
      break;
    case 0:
      powerRaw = paTable[4][f];
      break;
    case 5:
      powerRaw = paTable[5][f];
      break;
    case 7:
      powerRaw = paTable[6][f];
      break;
    case 10:
      powerRaw = paTable[7][f];
      break;
    default:
      return(RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  }

  // store the value
  _power = power;

  if(_modulation == RADIOLIB_CC1101_MOD_FORMAT_ASK_OOK){
    // Amplitude modulation:
    // PA_TABLE[0] is the power to be used when transmitting a 0  (no power)
    // PA_TABLE[1] is the power to be used when transmitting a 1  (full power)

    uint8_t paValues[2] = {0x00, powerRaw};
    SPIwriteRegisterBurst(RADIOLIB_CC1101_REG_PATABLE, paValues, 2);
    return(RADIOLIB_ERR_NONE);

  } else {
    // Freq modulation:
    // PA_TABLE[0] is the power to be used when transmitting.
    return(SPIsetRegValue(RADIOLIB_CC1101_REG_PATABLE, powerRaw));
  }
}

int16_t CC1101::setSyncWord(uint8_t* syncWord, uint8_t len, uint8_t maxErrBits, bool requireCarrierSense) {
  if((maxErrBits > 1) || (len != 2)) {
    return(RADIOLIB_ERR_INVALID_SYNC_WORD);
  }

  // sync word must not contain value 0x00
  for(uint8_t i = 0; i < len; i++) {
    if(syncWord[i] == 0x00) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }
  }

  _syncWordLength = len;

  // enable sync word filtering
  int16_t state = enableSyncWordFiltering(maxErrBits, requireCarrierSense);
  RADIOLIB_ASSERT(state);

  // set sync word register
  state = SPIsetRegValue(RADIOLIB_CC1101_REG_SYNC1, syncWord[0]);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_SYNC0, syncWord[1]);

  return(state);
}

int16_t CC1101::setSyncWord(uint8_t syncH, uint8_t syncL, uint8_t maxErrBits, bool requireCarrierSense) {
  uint8_t syncWord[] = { syncH, syncL };
  return(setSyncWord(syncWord, sizeof(syncWord), maxErrBits, requireCarrierSense));
}

int16_t CC1101::setPreambleLength(uint8_t preambleLength) {
  // check allowed values
  uint8_t value;
  switch(preambleLength){
    case 16:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_2;
      break;
    case 24:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_3;
      break;
    case 32:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_4;
      break;
    case 48:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_6;
      break;
    case 64:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_8;
      break;
    case 96:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_12;
      break;
    case 128:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_16;
      break;
    case 192:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_24;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);
  }

  uint16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG1, value, 6, 4);
  if(state == RADIOLIB_ERR_NONE) {
    _preambleLen = preambleLength;
  }

  return(state);
}


int16_t CC1101::setNodeAddress(uint8_t nodeAddr, uint8_t numBroadcastAddrs) {
  RADIOLIB_CHECK_RANGE(numBroadcastAddrs, 1, 2, RADIOLIB_ERR_INVALID_NUM_BROAD_ADDRS);

  // enable address filtering
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, numBroadcastAddrs + 0x01, 1, 0);
  RADIOLIB_ASSERT(state);

  // set node address
  return(SPIsetRegValue(RADIOLIB_CC1101_REG_ADDR, nodeAddr));
}

int16_t CC1101::disableAddressFiltering() {
  // disable address filtering
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, RADIOLIB_CC1101_ADR_CHK_NONE, 1, 0);
  RADIOLIB_ASSERT(state);

  // set node address to default (0x00)
  return(SPIsetRegValue(RADIOLIB_CC1101_REG_ADDR, 0x00));
}


int16_t CC1101::setOOK(bool enableOOK) {
  // Change modulation
  if(enableOOK) {
    int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MOD_FORMAT_ASK_OOK, 6, 4);
    RADIOLIB_ASSERT(state);

    // PA_TABLE[0] is (by default) the power value used when transmitting a "0".
    // Set PA_TABLE[1] to be used when transmitting a "1".
    state = SPIsetRegValue(RADIOLIB_CC1101_REG_FREND0, 1, 2, 0);
    RADIOLIB_ASSERT(state);

    // update current modulation
    _modulation = RADIOLIB_CC1101_MOD_FORMAT_ASK_OOK;
  } else {
    int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MOD_FORMAT_2_FSK, 6, 4);
    RADIOLIB_ASSERT(state);

    // Reset FREND0 to default value.
    state = SPIsetRegValue(RADIOLIB_CC1101_REG_FREND0, 0, 2, 0);
    RADIOLIB_ASSERT(state);

    // update current modulation
    _modulation = RADIOLIB_CC1101_MOD_FORMAT_2_FSK;
  }

  // Update PA_TABLE values according to the new _modulation.
  return(setOutputPower(_power));
}

float CC1101::getRSSI() {
  float rssi;

  if (_directMode) {
    if(_rawRSSI >= 128) {
      rssi = (((float)_rawRSSI - 256.0)/2.0) - 74.0;
    } else {
      rssi = (((float)_rawRSSI)/2.0) - 74.0;
    }
  } else {
    uint8_t rawRssi = SPIreadRegister(RADIOLIB_CC1101_REG_RSSI);
    if (rawRssi >= 128)
    {
      rssi = ((rawRssi - 256) / 2) - 74;
    }
    else
    {
      rssi = (rawRssi / 2) - 74;
    }
  }
  return(rssi);
}

uint8_t CC1101::getLQI() const {
  return(_rawLQI);
}

size_t CC1101::getPacketLength(bool update) {
  if(!_packetLengthQueried && update) {
    if (_packetLengthConfig == RADIOLIB_CC1101_LENGTH_CONFIG_VARIABLE) {
      _packetLength = SPIreadRegister(RADIOLIB_CC1101_REG_FIFO);
    } else {
      _packetLength = SPIreadRegister(RADIOLIB_CC1101_REG_PKTLEN);
    }

    _packetLengthQueried = true;
  }

  return(_packetLength);
}

int16_t CC1101::fixedPacketLengthMode(uint8_t len) {
  return(setPacketMode(RADIOLIB_CC1101_LENGTH_CONFIG_FIXED, len));
}

int16_t CC1101::variablePacketLengthMode(uint8_t maxLen) {
  return(setPacketMode(RADIOLIB_CC1101_LENGTH_CONFIG_VARIABLE, maxLen));
}

int16_t CC1101::enableSyncWordFiltering(uint8_t maxErrBits, bool requireCarrierSense) {
  switch(maxErrBits){
    case 0:
      // in 16 bit sync word, expect all 16 bits
      return(SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, (requireCarrierSense ? RADIOLIB_CC1101_SYNC_MODE_16_16_THR : RADIOLIB_CC1101_SYNC_MODE_16_16), 2, 0));
    case 1:
      // in 16 bit sync word, expect at least 15 bits
      return(SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, (requireCarrierSense ? RADIOLIB_CC1101_SYNC_MODE_15_16_THR : RADIOLIB_CC1101_SYNC_MODE_15_16), 2, 0));
    default:
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
  }
}

int16_t CC1101::disableSyncWordFiltering(bool requireCarrierSense) {
  return(SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, (requireCarrierSense ? RADIOLIB_CC1101_SYNC_MODE_NONE_THR : RADIOLIB_CC1101_SYNC_MODE_NONE), 2, 0));
}

int16_t CC1101::setCrcFiltering(bool crcOn) {
  _crcOn = crcOn;

  if (crcOn == true) {
    return(SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_CRC_ON, 2, 2));
  } else {
    return(SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_CRC_OFF, 2, 2));
  }
}

int16_t CC1101::setPromiscuousMode(bool promiscuous) {
  int16_t state = RADIOLIB_ERR_NONE;

  if (_promiscuous == promiscuous) {
    return(state);
  }

  if (promiscuous == true) {
    // disable preamble and sync word filtering and insertion
    state = disableSyncWordFiltering();
    RADIOLIB_ASSERT(state);

    // disable CRC filtering
    state = setCrcFiltering(false);
  } else {
    // enable preamble and sync word filtering and insertion
    state = enableSyncWordFiltering();
    RADIOLIB_ASSERT(state);

    // enable CRC filtering
    state = setCrcFiltering(true);
  }

  _promiscuous = promiscuous;

  return(state);
}

bool CC1101::getPromiscuousMode() {
  return (_promiscuous);
}

int16_t CC1101::setDataShaping(uint8_t sh) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set data shaping
  switch(sh) {
    case RADIOLIB_SHAPING_NONE:
      state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MOD_FORMAT_2_FSK, 6, 4);
      break;
    case RADIOLIB_SHAPING_0_5:
      state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MOD_FORMAT_GFSK, 6, 4);
      break;
    default:
      return(RADIOLIB_ERR_INVALID_DATA_SHAPING);
  }
  return(state);
}

int16_t CC1101::setEncoding(uint8_t encoding) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set encoding
  switch(encoding) {
    case RADIOLIB_ENCODING_NRZ:
      state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MANCHESTER_EN_OFF, 3, 3);
      RADIOLIB_ASSERT(state);
      return(SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_WHITE_DATA_OFF, 6, 6));
    case RADIOLIB_ENCODING_MANCHESTER:
      state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MANCHESTER_EN_ON, 3, 3);
      RADIOLIB_ASSERT(state);
      return(SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_WHITE_DATA_OFF, 6, 6));
    case RADIOLIB_ENCODING_WHITENING:
      state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MANCHESTER_EN_OFF, 3, 3);
      RADIOLIB_ASSERT(state);
      return(SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_WHITE_DATA_ON, 6, 6));
    default:
      return(RADIOLIB_ERR_INVALID_ENCODING);
  }
}

void CC1101::setRfSwitchPins(RADIOLIB_PIN_TYPE rxEn, RADIOLIB_PIN_TYPE txEn) {
  _mod->setRfSwitchPins(rxEn, txEn);
}

uint8_t CC1101::randomByte() {
  // set mode to Rx
  SPIsendCommand(RADIOLIB_CC1101_CMD_RX);
  RADIOLIB_DEBUG_PRINTLN("random");

  // wait a bit for the RSSI reading to stabilise
  _mod->delay(10);

  // read RSSI value 8 times, always keep just the least significant bit
  uint8_t randByte = 0x00;
  for(uint8_t i = 0; i < 8; i++) {
    randByte |= ((SPIreadRegister(RADIOLIB_CC1101_REG_RSSI) & 0x01) << i);
  }

  // set mode to standby
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  return(randByte);
}

int16_t CC1101::getChipVersion() {
  return(SPIgetRegValue(RADIOLIB_CC1101_REG_VERSION));
}

#if !defined(RADIOLIB_EXCLUDE_DIRECT_RECEIVE)
void CC1101::setDirectAction(void (*func)(void)) {
  setGdo0Action(func);
}

void CC1101::readBit(RADIOLIB_PIN_TYPE pin) {
  updateDirectBuffer((uint8_t)digitalRead(pin));
}
#endif

int16_t CC1101::setDIOMapping(RADIOLIB_PIN_TYPE pin, uint8_t value) {
  if (pin > 2)
    return RADIOLIB_ERR_INVALID_DIO_PIN;

  return(SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG0 - pin, value));
}

int16_t CC1101::config() {
  // Reset the radio. Registers may be dirty from previous usage.
  SPIsendCommand(RADIOLIB_CC1101_CMD_RESET);

  // Wait a ridiculous amount of time to be sure radio is ready.
  _mod->delay(150);

  // enable automatic frequency synthesizer calibration
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_MCSM0, RADIOLIB_CC1101_FS_AUTOCAL_IDLE_TO_RXTX, 5, 4);
  RADIOLIB_ASSERT(state);

  // set packet mode
  state = packetMode();

  return(state);
}

int16_t CC1101::directMode(bool sync) {
  // set mode to standby
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  int16_t state = 0;
  _directMode = sync;
  if (sync) {
    // set GDO0 and GDO2 mapping
  	state |= SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG0, RADIOLIB_CC1101_GDOX_SERIAL_CLOCK , 5, 0);
  	state |= SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG2, RADIOLIB_CC1101_GDOX_SERIAL_DATA_SYNC , 5, 0);

  	// set continuous mode
  	state |= SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_PKT_FORMAT_SYNCHRONOUS, 5, 4);
  }
  else {
    // set GDO0 mapping
    state |= SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG0, RADIOLIB_CC1101_GDOX_SERIAL_DATA_ASYNC , 5, 0);

    // set asynchronous continuous mode
    state |= SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_PKT_FORMAT_ASYNCHRONOUS, 5, 4);
  }

  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_LENGTH_CONFIG_INFINITE, 1, 0);
  return(state);
}

void CC1101::getExpMant(float target, uint16_t mantOffset, uint8_t divExp, uint8_t expMax, uint8_t& exp, uint8_t& mant) {
  // get table origin point (exp = 0, mant = 0)
  float origin = (mantOffset * RADIOLIB_CC1101_CRYSTAL_FREQ * 1000000.0)/((uint32_t)1 << divExp);

  // iterate over possible exponent values
  for(int8_t e = expMax; e >= 0; e--) {
    // get table column start value (exp = e, mant = 0);
	  float intervalStart = ((uint32_t)1 << e) * origin;

    // check if target value is in this column
	  if(target >= intervalStart) {
      // save exponent value
      exp = e;

      // calculate size of step between table rows
	    float stepSize = intervalStart/(float)mantOffset;

      // get target point position (exp = e, mant = m)
	    mant = ((target - intervalStart) / stepSize);

      // we only need the first match, terminate
	    return;
	  }
	}
}

int16_t CC1101::setPacketMode(uint8_t mode, uint16_t len) {
  // check length
  if (len > RADIOLIB_CC1101_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // set PKTCTRL0.LENGTH_CONFIG
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, mode, 1, 0);
  RADIOLIB_ASSERT(state);

  // set length to register
  state = SPIsetRegValue(RADIOLIB_CC1101_REG_PKTLEN, len);
  RADIOLIB_ASSERT(state);

  // update the cached value
  _packetLength = len;
  _packetLengthConfig = mode;
  return(state);
}

int16_t CC1101::SPIgetRegValue(uint8_t reg, uint8_t msb, uint8_t lsb) {
  // status registers require special command
  if(reg > RADIOLIB_CC1101_REG_TEST0) {
    reg |= RADIOLIB_CC1101_CMD_ACCESS_STATUS_REG;
  }

  return(_mod->SPIgetRegValue(reg, msb, lsb));
}

int16_t CC1101::SPIsetRegValue(uint8_t reg, uint8_t value, uint8_t msb, uint8_t lsb, uint8_t checkInterval) {
  // status registers require special command
  if(reg > RADIOLIB_CC1101_REG_TEST0) {
    reg |= RADIOLIB_CC1101_CMD_ACCESS_STATUS_REG;
  }

  return(_mod->SPIsetRegValue(reg, value, msb, lsb, checkInterval));
}

void CC1101::SPIreadRegisterBurst(uint8_t reg, uint8_t numBytes, uint8_t* inBytes) {
  _mod->SPIreadRegisterBurst(reg | RADIOLIB_CC1101_CMD_BURST, numBytes, inBytes);
}

uint8_t CC1101::SPIreadRegister(uint8_t reg) {
  // status registers require special command
  if(reg > RADIOLIB_CC1101_REG_TEST0) {
    reg |= RADIOLIB_CC1101_CMD_ACCESS_STATUS_REG;
  }

  return(_mod->SPIreadRegister(reg));
}

void CC1101::SPIwriteRegister(uint8_t reg, uint8_t data) {
  // status registers require special command
  if(reg > RADIOLIB_CC1101_REG_TEST0) {
    reg |= RADIOLIB_CC1101_CMD_ACCESS_STATUS_REG;
  }

  return(_mod->SPIwriteRegister(reg, data));
}

void CC1101::SPIwriteRegisterBurst(uint8_t reg, uint8_t* data, size_t len) {
  _mod->SPIwriteRegisterBurst(reg | RADIOLIB_CC1101_CMD_BURST, data, len);
}

void CC1101::SPIsendCommand(uint8_t cmd) {
  // pull NSS low
  _mod->digitalWrite(_mod->getCs(), LOW);

  // start transfer
  _mod->SPIbeginTransaction();

  // send the command byte
  _mod->SPItransfer(cmd);

  // stop transfer
  _mod->SPIendTransaction();
  _mod->digitalWrite(_mod->getCs(), HIGH);
}

#endif
