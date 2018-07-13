#include "RF69.h"

RF69::RF69(Module* module) {
  _mod = module;
}

uint8_t RF69::begin(float freq, float br, float rxBw, float freqDev, int8_t power) {
  // set module properties
  _mod->init(USE_SPI, INT_0);
  
  // try to find the RF69 chip
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    uint8_t version = _mod->SPIreadRegister(RF69_REG_VERSION);
    if(version == 0x24) {
      flagFound = true;
    } else {
      #ifdef KITELIB_DEBUG
        Serial.print("RF69 not found! (");
        Serial.print(i + 1);
        Serial.print(" of 10 tries) RF69_REG_VERSION == ");
        
        char buffHex[5];
        sprintf(buffHex, "0x%02X", version);
        Serial.print(buffHex);
        Serial.println();
      #endif
      delay(1000);
      i++;
    }
  }
  
  if(!flagFound) {
    DEBUG_PRINTLN_STR("No RF69 found!");
    SPI.end();
    return(ERR_CHIP_NOT_FOUND);
  } else {
    DEBUG_PRINTLN_STR("Found RF69! (match by RF69_REG_VERSION == 0x24)");
  }
  
  // configure settings not accessible by API
  uint8_t state = config();
  if(state != ERR_NONE) {
    return(state);
  }
  
  // configure publicly accessible settings
  state = setFrequency(freq);
  if(state != ERR_NONE) {
    return(state);
  }
  
  _rxBw = 125.0;
  state = setBitRate(br);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setRxBandwidth(rxBw);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setFrequencyDeviation(freqDev);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setOutputPower(power);
  if(state != ERR_NONE) {
    return(state);
  }
  
  return(ERR_NONE);
}

uint8_t RF69::transmit(uint8_t* data, size_t len) {
  // check packet length
  if(len >= 256) {
    return(ERR_PACKET_TOO_LONG);
  }
  
  // set mode to standby
  setMode(RF69_STANDBY);
  
  // set DIO pin mapping
  _mod->SPIsetRegValue(RF69_REG_DIO_MAPPING_1, RF69_DIO0_PACK_PACKET_SENT, 7, 6);
  
  // clear interrupt flags
  clearIRQFlags();
  
  // set packet length
  _mod->SPIwriteRegister(RF69_REG_FIFO, len);
  
  // write packet to FIFO
  _mod->SPIwriteRegisterBurst(RF69_REG_FIFO, data, len);
  
  // set mode to transmit
  setMode(RF69_TX);
  _mod->SPIsetRegValue(RF69_REG_TEST_PA1, RF69_PA1_20_DBM);
  _mod->SPIsetRegValue(RF69_REG_TEST_PA2, RF69_PA2_20_DBM);
  
  // wait for transmission end
  while(!_mod->getInt0State());
  
  // clear interrupt flags
  clearIRQFlags();
  
  return(ERR_NONE);
}

uint8_t RF69::transmit(const char* str) {
  return(RF69::transmit((uint8_t*)str, strlen(str)));
}

uint8_t RF69::transmit(String& str) {
  return(RF69::transmit(str.c_str()));
}

uint8_t RF69::receive(uint8_t* data, size_t len) {
  // set mode to standby
  setMode(RF69_STANDBY);
  
  // set DIO pin mapping
  _mod->SPIsetRegValue(RF69_REG_DIO_MAPPING_1, RF69_DIO0_PACK_PAYLOAD_READY | RF69_DIO1_PACK_TIMEOUT, 7, 4);
  
  // clear interrupt flags
  clearIRQFlags();
  
  // set mode to receive
  setMode(RF69_RX);
  _mod->SPIsetRegValue(RF69_REG_TEST_PA1, RF69_PA1_NORMAL);
  _mod->SPIsetRegValue(RF69_REG_TEST_PA2, RF69_PA2_NORMAL);
  
  // wait for packet reception or timeout
  while(!_mod->getInt0State()) {
    if(_mod->getInt1State()) {
      clearIRQFlags();
      return(ERR_RX_TIMEOUT);
    }
  }
  
  // get packet length
  size_t length = _mod->SPIreadRegister(RF69_REG_FIFO);
  
  // read packet data
  if(len == 0) {
    // argument len equal to zero indicates String call, which means dynamically allocated data array
    // dispose of the original and create a new one
    delete[] data;
    data = new uint8_t[length];
  }
  _mod->SPIreadRegisterBurst(RF69_REG_FIFO, length, data);
  
  // add terminating null
  if(len == 0) {
    data[length] = 0;
  }
  
  // clear interrupt flags
  clearIRQFlags();
  
  return(ERR_NONE);
}

uint8_t RF69::receive(String& str, size_t len) {
  // create temporary array to store received data
  char* data = new char[len];
  uint8_t state = RF69::receive((uint8_t*)data, len);
  
  // if packet was received successfully, copy data into String
  if(state == ERR_NONE) {
    str = String(data);
  }
  
  delete[] data;
  return(state);
}

uint8_t RF69::sleep() {
  // set module to sleep
  return(setMode(RF69_SLEEP));
}

uint8_t RF69::standby() {
  // set module to standby
  return(setMode(RF69_STANDBY));
}

uint8_t RF69::setFrequency(float freq) {
  // check allowed frequency range
  if(!((freq > 290.0) && (freq < 340.0) ||
       (freq > 431.0) && (freq < 510.0) ||
       (freq > 862.0) && (freq < 1020.0))) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  // set mode to standby
  setMode(RF69_STANDBY);
  
  //set carrier frequency
  uint32_t base = 1;
  uint32_t FRF = (freq * (base << 19)) / 32.0;
  uint8_t state = _mod->SPIsetRegValue(RF69_REG_FRF_MSB, (FRF & 0xFF0000) >> 16, 7, 0);
  state |= _mod->SPIsetRegValue(RF69_REG_FRF_MID, (FRF & 0x00FF00) >> 8, 7, 0);
  state |= _mod->SPIsetRegValue(RF69_REG_FRF_LSB, FRF & 0x0000FF, 7, 0);
  if(state == ERR_NONE) {
    RF69::_freq = freq;
  }
  return(state);
}

uint8_t RF69::setBitRate(float br) {
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
  uint8_t state = _mod->SPIsetRegValue(RF69_REG_BITRATE_MSB, (bitRate & 0xFF00) >> 8, 7, 0);
  state |= _mod->SPIsetRegValue(RF69_REG_BITRATE_LSB, bitRate & 0x00FF, 7, 0);
  if(state == ERR_NONE) {
    RF69::_br = br;
  }
  return(state);
}

uint8_t RF69::setRxBandwidth(float rxBw) {
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
  uint8_t state = _mod->SPIsetRegValue(RF69_REG_RX_BW, RF69_DCC_FREQ | bwMant | bwExp, 7, 0);
  if(state == ERR_NONE) {
    RF69::_rxBw = rxBw;
  }
  return(state);
}

uint8_t RF69::setFrequencyDeviation(float freqDev) {
  // check frequency deviation range
  if(!((freqDev + _br/2 <= 500) && (freqDev >= 0.6))) {
    return(ERR_INVALID_FREQUENCY_DEVIATION);
  }
  
  // set mode to standby
  setMode(RF69_STANDBY);
  
  //set allowed frequency deviation
  uint32_t base = 1;
  uint32_t FDEV = (freqDev * (base << 19)) / 32000;
  uint8_t state = _mod->SPIsetRegValue(RF69_REG_FDEV_MSB, (FDEV & 0xFF00) >> 8, 5, 0);
  state |= _mod->SPIsetRegValue(RF69_REG_FDEV_LSB, FDEV & 0x00FF, 7, 0);
  if(state == ERR_NONE) {
    RF69::_freqDev = freqDev;
  }
  return(state);
}

uint8_t RF69::setOutputPower(int8_t power) {
  // check output power range
  if((power < -18) || (power > 17)) {
    return(ERR_INVALID_OUTPUT_POWER);
  }
  
  // set mode to standby
  setMode(RF69_STANDBY);
  
  // set output power
  uint8_t state;
  if(power > 13) {
    // requested output power is higher than 13 dBm, enable PA2 + PA1 on PA_BOOST
    state = _mod->SPIsetRegValue(RF69_REG_PA_LEVEL, RF69_PA0_OFF | RF69_PA1_ON | RF69_PA2_ON | power + 14, 7, 0);
  } else {
    // requested output power is lower than 13 dBm, enable PA0 on RFIO
    state = _mod->SPIsetRegValue(RF69_REG_PA_LEVEL, RF69_PA0_ON | RF69_PA1_OFF | RF69_PA2_OFF | power + 18, 7, 0);
  }
  if(state == ERR_NONE) {
    RF69::_power = _power;
  }
  return(state);
}

uint8_t RF69::config() {
  uint8_t state = ERR_NONE;

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
  
  // reset FIFO flags
  state = _mod->SPIsetRegValue(RF69_REG_IRQ_FLAGS_2, RF69_IRQ_FIFO_OVERRUN, 4, 4);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // disable ClkOut on DIO5
  state = _mod->SPIsetRegValue(RF69_REG_DIO_MAPPING_2, RF69_CLK_OUT_OFF, 2, 0);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // set synchronization
  state = _mod->SPIsetRegValue(RF69_REG_SYNC_CONFIG, RF69_SYNC_ON | RF69_FIFO_FILL_CONDITION_SYNC | RF69_SYNC_SIZE | RF69_SYNC_TOL, 7, 0);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // set sync word
  state = _mod->SPIsetRegValue(RF69_REG_SYNC_VALUE_1, 0x2D, 7, 0);
  state |= _mod->SPIsetRegValue(RF69_REG_SYNC_VALUE_2, 100, 7, 0);
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

uint8_t RF69::setMode(uint8_t mode) {
  _mod->SPIsetRegValue(RF69_REG_OP_MODE, mode, 4, 2);
  return(ERR_NONE);
}

void RF69::clearIRQFlags() {
  _mod->SPIwriteRegister(RF69_REG_IRQ_FLAGS_1, 0b11111111);
  _mod->SPIwriteRegister(RF69_REG_IRQ_FLAGS_2, 0b11111111);
}
