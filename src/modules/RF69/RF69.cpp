#include "RF69.h"

RF69::RF69(Module* module) : PhysicalLayer(RF69_CRYSTAL_FREQ, RF69_DIV_EXPONENT, RF69_MAX_PACKET_LENGTH)  {
  _mod = module;
  _tempOffset = 0;

  _packetLengthQueried = false;
  _packetLengthConfig = RF69_PACKET_FORMAT_VARIABLE;

  _promiscuous = false;

  _syncWordLength = 2;
}

int16_t RF69::begin(float freq, float br, float rxBw, float freqDev, int8_t power) {
  // set module properties
  _mod->init(RADIOLIB_USE_SPI, RADIOLIB_INT_0);

  // try to find the RF69 chip
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    uint8_t version = _mod->SPIreadRegister(RF69_REG_VERSION);
    if(version == 0x24) {
      flagFound = true;
    } else {
      #ifdef RADIOLIB_DEBUG
        RADIOLIB_DEBUG_PRINT(F("RF69 not found! ("));
        RADIOLIB_DEBUG_PRINT(i + 1);
        RADIOLIB_DEBUG_PRINT(F(" of 10 tries) RF69_REG_VERSION == "));

        char buffHex[7];
        sprintf(buffHex, "0x%04X", version);
        RADIOLIB_DEBUG_PRINT(buffHex);
        RADIOLIB_DEBUG_PRINT(F(", expected 0x0024"));
        RADIOLIB_DEBUG_PRINTLN();
      #endif
      delay(1000);
      i++;
    }
  }

  if(!flagFound) {
    RADIOLIB_DEBUG_PRINTLN(F("No RF69 found!"));
    SPI.end();
    return(ERR_CHIP_NOT_FOUND);
  } else {
    RADIOLIB_DEBUG_PRINTLN(F("Found RF69! (match by RF69_REG_VERSION == 0x24)"));
  }

  // configure settings not accessible by API
  int16_t state = config();
  if(state != ERR_NONE) {
    return(state);
  }

  // configure publicly accessible settings
  state = setFrequency(freq);
  if(state != ERR_NONE) {
    return(state);
  }

  // configure bitrate
  _rxBw = 125.0;
  state = setBitRate(br);
  if(state != ERR_NONE) {
    return(state);
  }

  // configure default RX bandwidth
  state = setRxBandwidth(rxBw);
  if(state != ERR_NONE) {
    return(state);
  }

  // configure default frequency deviation
  state = setFrequencyDeviation(freqDev);
  if(state != ERR_NONE) {
    return(state);
  }

  // configure default TX output power
  state = setOutputPower(power);
  if(state != ERR_NONE) {
    return(state);
  }

  // set default packet length mode
  state = variablePacketLengthMode();
  if (state != ERR_NONE) {
    return(state);
  }

  // default sync word values 0x2D01 is the same as the default in LowPowerLab RFM69 library
  uint8_t syncWord[] = {0x2D, 0x01};
  state = setSyncWord(syncWord, sizeof(syncWord));
  if(state != ERR_NONE) {
    return(state);
  }

  return(ERR_NONE);
}

int16_t RF69::transmit(uint8_t* data, size_t len, uint8_t addr) {
  // start transmission
  int16_t state = startTransmit(data, len, addr);
  if(state != ERR_NONE) {
    return(state);
  }

  // wait for transmission end
  while(!digitalRead(_mod->getInt0()));

  // clear interrupt flags
  clearIRQFlags();

  return(ERR_NONE);
}

int16_t RF69::receive(uint8_t* data, size_t len) {
  // start reception
  int16_t state = startReceive(true);
  if(state != ERR_NONE) {
    return(state);
  }

  // wait for packet reception or timeout
  while(!digitalRead(_mod->getInt0())) {
    if(digitalRead(_mod->getInt1())) {
      clearIRQFlags();
      return(ERR_RX_TIMEOUT);
    }
  }

  // read packet data
  return(readData(data, len));
}

int16_t RF69::sleep() {
  // set module to sleep
  return(setMode(RF69_SLEEP));
}

int16_t RF69::standby() {
  // set module to standby
  return(setMode(RF69_STANDBY));
}

int16_t RF69::transmitDirect(uint32_t frf) {
  // user requested to start transmitting immediately (required for RTTY)
  if(frf != 0) {
    _mod->SPIwriteRegister(RF69_REG_FRF_MSB, (frf & 0xFF0000) >> 16);
    _mod->SPIwriteRegister(RF69_REG_FRF_MID, (frf & 0x00FF00) >> 8);
    _mod->SPIwriteRegister(RF69_REG_FRF_LSB, frf & 0x0000FF);

    return(setMode(RF69_TX));
  }

  // activate direct mode
  int16_t state = directMode();
  if(state != ERR_NONE) {
    return(state);
  }

  // start transmitting
  return(setMode(RF69_TX));
}

int16_t RF69::receiveDirect() {
  // activate direct mode
  int16_t state = directMode();
  if(state != ERR_NONE) {
    return(state);
  }

  // start receiving
  return(setMode(RF69_RX));
}

int16_t RF69::directMode() {
  // set mode to standby
  int16_t state = setMode(RF69_STANDBY);
  if(state != ERR_NONE) {
    return(state);
  }

  // set DIO mapping
  state = _mod->SPIsetRegValue(RF69_REG_DIO_MAPPING_1, RF69_DIO1_CONT_DCLK | RF69_DIO2_CONT_DATA, 5, 2);

  // set continuous mode
  state |= _mod->SPIsetRegValue(RF69_REG_DATA_MODUL, RF69_CONTINUOUS_MODE_WITH_SYNC, 6, 5);
  return(state);
}

int16_t RF69::packetMode() {
  return(_mod->SPIsetRegValue(RF69_REG_DATA_MODUL, RF69_PACKET_MODE, 6, 5));
}

void RF69::setAESKey(uint8_t* key) {
  _mod->SPIwriteRegisterBurst(RF69_REG_AES_KEY_1, key, 16);
}

int16_t RF69::enableAES() {
  return(_mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_2, RF69_AES_ON, 0, 0));
}

int16_t RF69::disableAES() {
  return(_mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_2, RF69_AES_OFF, 0, 0));
}

int16_t RF69::startReceive(bool timeout) {
  // set mode to standby
  int16_t state = setMode(RF69_STANDBY);

  // set RX timeouts and DIO pin mapping
  if(timeout) {
    state = _mod->SPIsetRegValue(RF69_REG_DIO_MAPPING_1, RF69_DIO0_PACK_PAYLOAD_READY | RF69_DIO1_PACK_TIMEOUT, 7, 4);
    state |= _mod->SPIsetRegValue(RF69_REG_RX_TIMEOUT_1, RF69_TIMEOUT_RX_START);
    state |= _mod->SPIsetRegValue(RF69_REG_RX_TIMEOUT_2, RF69_TIMEOUT_RSSI_THRESH);
  } else {
    state = _mod->SPIsetRegValue(RF69_REG_DIO_MAPPING_1, RF69_DIO0_PACK_PAYLOAD_READY, 7, 6);
    state |= _mod->SPIsetRegValue(RF69_REG_RX_TIMEOUT_1, RF69_TIMEOUT_RX_START_OFF);
    state |= _mod->SPIsetRegValue(RF69_REG_RX_TIMEOUT_2, RF69_TIMEOUT_RSSI_THRESH_OFF);
  }
  if(state != ERR_NONE) {
    return(state);
  }

  // clear interrupt flags
  clearIRQFlags();

  // set mode to receive
  state = _mod->SPIsetRegValue(RF69_REG_TEST_PA1, RF69_PA1_NORMAL);
  state |= _mod->SPIsetRegValue(RF69_REG_TEST_PA2, RF69_PA2_NORMAL);
  state |= setMode(RF69_RX);

  return(state);
}

void RF69::setDio0Action(void (*func)(void)) {
  attachInterrupt(digitalPinToInterrupt(_mod->getInt0()), func, RISING);
}

void RF69::setDio1Action(void (*func)(void)) {
  attachInterrupt(digitalPinToInterrupt(_mod->getInt1()), func, RISING);
}

int16_t RF69::startTransmit(uint8_t* data, size_t len, uint8_t addr) {
  // check packet length
  if(len > RF69_MAX_PACKET_LENGTH) {
    return(ERR_PACKET_TOO_LONG);
  }

  // set mode to standby
  int16_t state = setMode(RF69_STANDBY);

  // set DIO pin mapping
  state |= _mod->SPIsetRegValue(RF69_REG_DIO_MAPPING_1, RF69_DIO0_PACK_PACKET_SENT, 7, 6);
  if(state != ERR_NONE) {
    return(state);
  }

  // clear interrupt flags
  clearIRQFlags();

  // set packet length
  _mod->SPIwriteRegister(RF69_REG_FIFO, len);

  // optionally write packet length
  if (_packetLengthConfig == RF69_PACKET_FORMAT_VARIABLE) {
    _mod->SPIwriteRegister(RF69_REG_FIFO, len);
  }

  // check address filtering
  uint8_t filter = _mod->SPIgetRegValue(RF69_REG_PACKET_CONFIG_1, 2, 1);
  if((filter == RF69_ADDRESS_FILTERING_NODE) || (filter == RF69_ADDRESS_FILTERING_NODE_BROADCAST)) {
    _mod->SPIwriteRegister(RF69_REG_FIFO, addr);
  }

  // write packet to FIFO
  _mod->SPIwriteRegisterBurst(RF69_REG_FIFO, data, len);

  // set mode to transmit
  state = _mod->SPIsetRegValue(RF69_REG_TEST_PA1, RF69_PA1_20_DBM);
  state |= _mod->SPIsetRegValue(RF69_REG_TEST_PA2, RF69_PA2_20_DBM);
  state |= setMode(RF69_TX);

  return(state);
}

int16_t RF69::readData(uint8_t* data, size_t len) {
  // set mdoe to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
    return(state);
  }

  // get packet length
  size_t length = len;
  if(len == RF69_MAX_PACKET_LENGTH) {
    length = getPacketLength();
  }

  // check address filtering
  uint8_t filter = _mod->SPIgetRegValue(RF69_REG_PACKET_CONFIG_1, 2, 1);
  if((filter == RF69_ADDRESS_FILTERING_NODE) || (filter == RF69_ADDRESS_FILTERING_NODE_BROADCAST)) {
    _mod->SPIreadRegister(RF69_REG_FIFO);
  }

  // read packet data
  _mod->SPIreadRegisterBurst(RF69_REG_FIFO, length, data);

  // add terminating null
  data[length] = 0;

  // update RSSI
  lastPacketRSSI = -1.0 * (_mod->SPIgetRegValue(RF69_REG_RSSI_VALUE)/2.0);

  // clear internal flag so getPacketLength can return the new packet length
  _packetLengthQueried = false;

  // clear interrupt flags
  clearIRQFlags();

  return(ERR_NONE);
}

int16_t RF69::setFrequency(float freq) {
  // check allowed frequency range
  if(!(((freq > 290.0) && (freq < 340.0)) ||
       ((freq > 431.0) && (freq < 510.0)) ||
       ((freq > 862.0) && (freq < 1020.0)))) {
    return(ERR_INVALID_FREQUENCY);
  }

  // set mode to standby
  setMode(RF69_STANDBY);

  //set carrier frequency
  uint32_t FRF = (freq * (uint32_t(1) << RF69_DIV_EXPONENT)) / RF69_CRYSTAL_FREQ;
  int16_t state = _mod->SPIsetRegValue(RF69_REG_FRF_MSB, (FRF & 0xFF0000) >> 16, 7, 0);
  state |= _mod->SPIsetRegValue(RF69_REG_FRF_MID, (FRF & 0x00FF00) >> 8, 7, 0);
  state |= _mod->SPIsetRegValue(RF69_REG_FRF_LSB, FRF & 0x0000FF, 7, 0);

  return(state);
}

int16_t RF69::setBitRate(float br) {
  // check allowed bitrate
  if((br < 1.2) || (br > 300.0)) {
    return(ERR_INVALID_BIT_RATE);
  }

  // check bitrate-bandwidth ratio
  if(!(br < 2000 * _rxBw)) {
    return(ERR_INVALID_BIT_RATE_BW_RATIO);
  }

  // set mode to standby
  setMode(RF69_STANDBY);

  // set bit rate
  uint16_t bitRate = 32000 / br;
  int16_t state = _mod->SPIsetRegValue(RF69_REG_BITRATE_MSB, (bitRate & 0xFF00) >> 8, 7, 0);
  state |= _mod->SPIsetRegValue(RF69_REG_BITRATE_LSB, bitRate & 0x00FF, 7, 0);
  if(state == ERR_NONE) {
    RF69::_br = br;
  }
  return(state);
}

int16_t RF69::setRxBandwidth(float rxBw) {
  // check bitrate-bandwidth ratio
  if(!(_br < 2000 * rxBw)) {
    return(ERR_INVALID_BIT_RATE_BW_RATIO);
  }

  // check allowed bandwidth values
  uint8_t bwMant, bwExp;
  if(rxBw == 2.6) {
    bwMant = RF69_RX_BW_MANT_24;
    bwExp = 7;
  } else if(rxBw == 3.1) {
    bwMant = RF69_RX_BW_MANT_20;
    bwExp = 7;
  } else if(rxBw == 3.9) {
    bwMant = RF69_RX_BW_MANT_16;
    bwExp = 7;
  } else if(rxBw == 5.2) {
    bwMant = RF69_RX_BW_MANT_24;
    bwExp = 6;
  } else if(rxBw == 6.3) {
    bwMant = RF69_RX_BW_MANT_20;
    bwExp = 6;
  } else if(rxBw == 7.8) {
    bwMant = RF69_RX_BW_MANT_16;
    bwExp = 6;
  } else if(rxBw == 10.4) {
    bwMant = RF69_RX_BW_MANT_24;
    bwExp = 5;
  } else if(rxBw == 12.5) {
    bwMant = RF69_RX_BW_MANT_20;
    bwExp = 5;
  } else if(rxBw == 15.6) {
    bwMant = RF69_RX_BW_MANT_16;
    bwExp = 5;
  } else if(rxBw == 20.8) {
    bwMant = RF69_RX_BW_MANT_24;
    bwExp = 4;
  } else if(rxBw == 25.0) {
    bwMant = RF69_RX_BW_MANT_20;
    bwExp = 4;
  } else if(rxBw == 31.3) {
    bwMant = RF69_RX_BW_MANT_16;
    bwExp = 4;
  } else if(rxBw == 41.7) {
    bwMant = RF69_RX_BW_MANT_24;
    bwExp = 3;
  } else if(rxBw == 50.0) {
    bwMant = RF69_RX_BW_MANT_20;
    bwExp = 3;
  } else if(rxBw == 62.5) {
    bwMant = RF69_RX_BW_MANT_16;
    bwExp = 3;
  } else if(rxBw == 83.3) {
    bwMant = RF69_RX_BW_MANT_24;
    bwExp = 2;
  } else if(rxBw == 100.0) {
    bwMant = RF69_RX_BW_MANT_20;
    bwExp = 2;
  } else if(rxBw == 125.0) {
    bwMant = RF69_RX_BW_MANT_16;
    bwExp = 2;
  } else if(rxBw == 166.7) {
    bwMant = RF69_RX_BW_MANT_24;
    bwExp = 1;
  } else if(rxBw == 200.0) {
    bwMant = RF69_RX_BW_MANT_20;
    bwExp = 1;
  } else if(rxBw == 250.0) {
    bwMant = RF69_RX_BW_MANT_16;
    bwExp = 1;
  } else if(rxBw == 333.3) {
    bwMant = RF69_RX_BW_MANT_24;
    bwExp = 0;
  } else if(rxBw == 400.0) {
    bwMant = RF69_RX_BW_MANT_20;
    bwExp = 0;
  } else if(rxBw == 500.0) {
    bwMant = RF69_RX_BW_MANT_16;
    bwExp = 0;
  } else {
    return(ERR_INVALID_RX_BANDWIDTH);
  }

  // set mode to standby
  setMode(RF69_STANDBY);

  // set Rx bandwidth
  int16_t state = _mod->SPIsetRegValue(RF69_REG_RX_BW, RF69_DCC_FREQ | bwMant | bwExp, 7, 0);
  if(state == ERR_NONE) {
    RF69::_rxBw = rxBw;
  }
  return(state);
}

int16_t RF69::setFrequencyDeviation(float freqDev) {
  // check frequency deviation range
  if(!((freqDev + _br/2 <= 500))) {
    return(ERR_INVALID_FREQUENCY_DEVIATION);
  }

  // set mode to standby
  setMode(RF69_STANDBY);

  // set frequency deviation from carrier frequency
  uint32_t base = 1;
  uint32_t fdev = (freqDev * (base << 19)) / 32000;
  int16_t state = _mod->SPIsetRegValue(RF69_REG_FDEV_MSB, (fdev & 0xFF00) >> 8, 5, 0);
  state |= _mod->SPIsetRegValue(RF69_REG_FDEV_LSB, fdev & 0x00FF, 7, 0);

  return(state);
}

int16_t RF69::setOutputPower(int8_t power) {
  // check output power range
  if((power < -18) || (power > 17)) {
    return(ERR_INVALID_OUTPUT_POWER);
  }

  // set mode to standby
  setMode(RF69_STANDBY);

  // set output power
  int16_t state;
  if(power > 13) {
    // requested output power is higher than 13 dBm, enable PA2 + PA1 on PA_BOOST
    state = _mod->SPIsetRegValue(RF69_REG_PA_LEVEL, RF69_PA0_OFF | RF69_PA1_ON | RF69_PA2_ON | (power + 14), 7, 0);
  } else {
    // requested output power is lower than 13 dBm, enable PA0 on RFIO
    state = _mod->SPIsetRegValue(RF69_REG_PA_LEVEL, RF69_PA0_ON | RF69_PA1_OFF | RF69_PA2_OFF | (power + 18), 7, 0);
  }

  return(state);
}

int16_t RF69::setSyncWord(uint8_t* syncWord, size_t len, uint8_t maxErrBits) {
  // check constraints
  if((maxErrBits > 7) || (len > 8)) {
    return(ERR_INVALID_SYNC_WORD);
  }

  // sync word must not contain value 0x00
  for(uint8_t i = 0; i < len; i++) {
    if(syncWord[i] == 0x00) {
      return(ERR_INVALID_SYNC_WORD);
    }
  }

  _syncWordLength = len;

  int16_t state = enableSyncWordFiltering(maxErrBits);
  if (state != ERR_NONE) {
    return(state);
  }

  // set sync word register
  _mod->SPIwriteRegisterBurst(RF69_REG_SYNC_VALUE_1, syncWord, len);
  return(ERR_NONE);
}

int16_t RF69::setNodeAddress(uint8_t nodeAddr) {
  // enable address filtering (node only)
  int16_t state = _mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_1, RF69_ADDRESS_FILTERING_NODE, 2, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // set node address
  return(_mod->SPIsetRegValue(RF69_REG_NODE_ADRS, nodeAddr));
}

int16_t RF69::setBroadcastAddress(uint8_t broadAddr) {
  // enable address filtering (node + broadcast)
  int16_t state = _mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_1, RF69_ADDRESS_FILTERING_NODE_BROADCAST, 2, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // set broadcast address
  return(_mod->SPIsetRegValue(RF69_REG_BROADCAST_ADRS, broadAddr));
}

int16_t RF69::disableAddressFiltering() {
  // disable address filtering
  int16_t state = _mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_1, RF69_ADDRESS_FILTERING_OFF, 2, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // set node address to default (0x00)
  state = _mod->SPIsetRegValue(RF69_REG_NODE_ADRS, 0x00);
  if(state != ERR_NONE) {
    return(state);
  }

  // set broadcast address to default (0x00)
  return(_mod->SPIsetRegValue(RF69_REG_BROADCAST_ADRS, 0x00));
}

void RF69::setAmbientTemperature(int16_t tempAmbient) {
  _tempOffset = getTemperature() -  tempAmbient;
}

int16_t RF69::getTemperature() {
  // set mode to STANDBY
  setMode(RF69_STANDBY);

  // start temperature measurement
  _mod->SPIsetRegValue(RF69_REG_TEMP_1, RF69_TEMP_MEAS_START, 3, 3);

  // wait until measurement is finished
  while(_mod->SPIgetRegValue(RF69_REG_TEMP_1, 2, 2) == RF69_TEMP_MEAS_RUNNING) {
    // check every 10 us
    delay(10);
  }
  int8_t rawTemp = _mod->SPIgetRegValue(RF69_REG_TEMP_2);

  return(0 - (rawTemp + _tempOffset));
}

size_t RF69::getPacketLength(bool update) {
  if(!_packetLengthQueried && update) {
    if (_packetLengthConfig == RF69_PACKET_FORMAT_VARIABLE) {
      _packetLength = _mod->SPIreadRegister(RF69_REG_FIFO);
    } else {
      _packetLength = _mod->SPIreadRegister(RF69_REG_PAYLOAD_LENGTH);
    }
    _packetLengthQueried = true;
  }

  return(_packetLength);
}

int16_t RF69::fixedPacketLengthMode(uint8_t len) {
  return(setPacketMode(RF69_PACKET_FORMAT_FIXED, len));
}

int16_t RF69::variablePacketLengthMode(uint8_t maxLen) {
  return(setPacketMode(RF69_PACKET_FORMAT_VARIABLE, maxLen));
}

int16_t RF69::enableSyncWordFiltering(uint8_t maxErrBits) {
  // enable sync word recognition
  int16_t state = _mod->SPIsetRegValue(RF69_REG_SYNC_CONFIG, RF69_SYNC_ON | RF69_FIFO_FILL_CONDITION_SYNC | (_syncWordLength - 1) << 3 | maxErrBits, 7, 0);
  if(state != ERR_NONE) {
    return(state);
  }

  return(state);
}

int16_t RF69::disableSyncWordFiltering() {
  // disable preamble detection and generation
  int16_t state = _mod->SPIsetRegValue(RF69_REG_PREAMBLE_LSB, 0, 7, 0);
  state |= _mod->SPIsetRegValue(RF69_REG_PREAMBLE_MSB, 0, 7, 0);
  if (state != ERR_NONE) {
    return(state);
  }

  // disable sync word detection and generation
  state = _mod->SPIsetRegValue(RF69_REG_SYNC_CONFIG, RF69_SYNC_OFF | RF69_FIFO_FILL_CONDITION, 7, 6);
  if (state != ERR_NONE) {
    return(state);
  }

  return(state);
}

int16_t RF69::setCrcFiltering(bool crcOn) {
  if (crcOn == true) {
    return(_mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_1, RF69_CRC_ON, 4, 4));
  } else {
    return(_mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_1, RF69_CRC_OFF, 4, 4));
  }
}

int16_t RF69::setPromiscuousMode(bool promiscuous) {
  int16_t state = ERR_NONE;

  if (_promiscuous == promiscuous) {
    return(state);
  }

  if (promiscuous == true) {
    // disable preamble and sync word filtering and insertion
    state = disableSyncWordFiltering();
    if (state != ERR_NONE) {
      return(state);
    }

    // disable CRC filtering
    state = setCrcFiltering(false);
  } else {
    // enable preamble and sync word filtering and insertion
    state = enableSyncWordFiltering();
    if (state != ERR_NONE) {
      return(state);
    }

    // enable CRC filtering
    state = setCrcFiltering(true);
  }

  return(state);
}

int16_t RF69::config() {
  int16_t state = ERR_NONE;

  // set mode to STANDBY
  state = setMode(RF69_STANDBY);
  if(state != ERR_NONE) {
    return(state);
  }

  // set operation modes
  state = _mod->SPIsetRegValue(RF69_REG_OP_MODE, RF69_SEQUENCER_ON | RF69_LISTEN_OFF, 7, 6);
  if(state != ERR_NONE) {
    return(state);
  }

  // enable over-current protection
  state = _mod->SPIsetRegValue(RF69_REG_OCP, RF69_OCP_ON, 4, 4);
  if(state != ERR_NONE) {
    return(state);
  }

  // set data mode, modulation type and shaping
  state = _mod->SPIsetRegValue(RF69_REG_DATA_MODUL, RF69_PACKET_MODE | RF69_FSK, 6, 3);
  state |= _mod->SPIsetRegValue(RF69_REG_DATA_MODUL, RF69_FSK_GAUSSIAN_0_3, 1, 0);
  if(state != ERR_NONE) {
    return(state);
  }

  // set RSSI threshold
  state = _mod->SPIsetRegValue(RF69_REG_RSSI_THRESH, RF69_RSSI_THRESHOLD, 7, 0);
  if(state != ERR_NONE) {
    return(state);
  }

  // reset FIFO flag
  _mod->SPIwriteRegister(RF69_REG_IRQ_FLAGS_2, RF69_IRQ_FIFO_OVERRUN);

  // disable ClkOut on DIO5
  state = _mod->SPIsetRegValue(RF69_REG_DIO_MAPPING_2, RF69_CLK_OUT_OFF, 2, 0);
  if(state != ERR_NONE) {
    return(state);
  }

  // set packet configuration and disable encryption
  state = _mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_1, RF69_PACKET_FORMAT_VARIABLE | RF69_DC_FREE_NONE | RF69_CRC_ON | RF69_CRC_AUTOCLEAR_ON | RF69_ADDRESS_FILTERING_OFF, 7, 1);
  state |= _mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_2, RF69_INTER_PACKET_RX_DELAY, 7, 4);
  state |= _mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_2, RF69_AUTO_RX_RESTART_ON | RF69_AES_OFF, 1, 0);
  if(state != ERR_NONE) {
    return(state);
  }

  // set payload length
  state = _mod->SPIsetRegValue(RF69_REG_PAYLOAD_LENGTH, RF69_PAYLOAD_LENGTH, 7, 0);
  if(state != ERR_NONE) {
    return(state);
  }

  // set FIFO threshold
  state = _mod->SPIsetRegValue(RF69_REG_FIFO_THRESH, RF69_TX_START_CONDITION_FIFO_NOT_EMPTY | RF69_FIFO_THRESHOLD, 7, 0);
  if(state != ERR_NONE) {
    return(state);
  }

  // set Rx timeouts
  state = _mod->SPIsetRegValue(RF69_REG_RX_TIMEOUT_1, RF69_TIMEOUT_RX_START, 7, 0);
  state = _mod->SPIsetRegValue(RF69_REG_RX_TIMEOUT_2, RF69_TIMEOUT_RSSI_THRESH, 7, 0);
  if(state != ERR_NONE) {
    return(state);
  }

  // enable improved fading margin
  state = _mod->SPIsetRegValue(RF69_REG_TEST_DAGC, RF69_CONTINUOUS_DAGC_LOW_BETA_OFF, 7, 0);
  if(state != ERR_NONE) {
    return(state);
  }

  return(ERR_NONE);
}

int16_t RF69::setPacketMode(uint8_t mode, uint8_t len) {
  // check length
  if (len > RF69_MAX_PACKET_LENGTH) {
    return(ERR_PACKET_TOO_LONG);
  }

  // set to fixed packet length
  int16_t state = _mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_1, mode, 7, 7);
  if (state != ERR_NONE) {
    return(state);
  }

  // set length to register
  state = _mod->SPIsetRegValue(RF69_REG_PAYLOAD_LENGTH, len);
  if (state != ERR_NONE) {
    return(state);
  }

  // update the cached value
  _packetLengthConfig = mode;
  return(state);
}

int16_t RF69::setMode(uint8_t mode) {
  return(_mod->SPIsetRegValue(RF69_REG_OP_MODE, mode, 4, 2));
}

void RF69::clearIRQFlags() {
  _mod->SPIwriteRegister(RF69_REG_IRQ_FLAGS_1, 0b11111111);
  _mod->SPIwriteRegister(RF69_REG_IRQ_FLAGS_2, 0b11111111);
}
