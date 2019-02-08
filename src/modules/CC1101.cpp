#include "CC1101.h"

CC1101::CC1101(Module* module) : PhysicalLayer(CC1101_CRYSTAL_FREQ, CC1101_DIV_EXPONENT) {
  _mod = module;
}

int16_t CC1101::begin(float freq, float br, float rxBw, float freqDev) {
  // set module properties
  _mod->SPIreadCommand = CC1101_CMD_READ;
  _mod->SPIwriteCommand = CC1101_CMD_WRITE;
  _mod->init(USE_SPI, INT_0);
  
  // try to find the CC1101 chip
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    uint8_t version = SPIreadRegister(CC1101_REG_VERSION);
    if(version == 0x14) {
      flagFound = true;
    } else {
      #ifdef RADIOLIB_DEBUG
        Serial.print(F("CC1101 not found! ("));
        Serial.print(i + 1);
        Serial.print(F(" of 10 tries) CC1101_REG_VERSION == "));
        
        char buffHex[7];
        sprintf(buffHex, "0x%04X", version);
        Serial.print(buffHex);
        Serial.print(F(", expected 0x0014"));
        Serial.println();
      #endif
      delay(1000);
      i++;
    }
  }
  
  if(!flagFound) {
    DEBUG_PRINTLN_STR("No CC1101 found!");
    SPI.end();
    return(ERR_CHIP_NOT_FOUND);
  } else {
    DEBUG_PRINTLN_STR("Found CC1101! (match by CC1101_REG_VERSION == 0x14)");
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
  
  return(state);
}

int16_t CC1101::transmit(String& str, uint8_t addr) {
  return(CC1101::transmit(str.c_str()));
}

int16_t CC1101::transmit(const char* str, uint8_t addr) {
  return(CC1101::transmit((uint8_t*)str, strlen(str)));
}

int16_t CC1101::transmit(uint8_t* data, size_t len, uint8_t addr) {
  // check packet length
  if(len > 255) {
    return(ERR_PACKET_TOO_LONG);
  }
  
  // set mode to standby
  standby();
  
  // set GDO0 mapping
  _mod->SPIsetRegValue(CC1101_REG_IOCFG0, CC1101_GDOX_SYNC_WORD_SENT_OR_RECEIVED);
  
  // write packet length
  _mod->SPIwriteRegister(CC1101_REG_FIFO, len);
  
  // write packet to FIFO
  _mod->SPIwriteRegisterBurst(CC1101_REG_FIFO | CC1101_CMD_BURST, data, len);
  
  // set mode to transmit
  SPIsendCommand(CC1101_CMD_TX);
  
  // wait for transmission start
  while(!digitalRead(_mod->getInt0()));
  
  // wait for transmission end
  while(digitalRead(_mod->getInt0()));
  
  // set mode to standby
  standby();
  
  // flush Tx FIFO
  SPIsendCommand(CC1101_CMD_FLUSH_TX);
  
  return(ERR_NONE);
}

int16_t CC1101::receive(String& str, size_t len) {
  // create temporary array to store received data
  char* data = new char[len + 1];
  int16_t state = CC1101::receive((uint8_t*)data, len);
  
  // if packet was received successfully, copy data into String
  if(state == ERR_NONE) {
    str = String(data);
  }
  
  delete[] data;
  return(state);
}

int16_t CC1101::receive(uint8_t* data, size_t len) {
  // TODO
  // set mode to standby
  
  // set GDO0 and GDO2 mapping
  
  return(ERR_NONE);
}

int16_t CC1101::standby() {
  SPIsendCommand(CC1101_CMD_IDLE);
  return(ERR_NONE);
}

int16_t CC1101::transmitDirect(uint32_t FRF) {
  // user requested to start transmitting immediately (required for RTTY)
  if(FRF != 0) {
    _mod->SPIwriteRegister(CC1101_REG_FREQ2, (FRF & 0xFF0000) >> 16);
    _mod->SPIwriteRegister(CC1101_REG_FREQ1, (FRF & 0x00FF00) >> 8);
    _mod->SPIwriteRegister(CC1101_REG_FREQ0, FRF & 0x0000FF);
  
    SPIsendCommand(CC1101_CMD_TX);
  }
  
  // activate direct mode
  int16_t state = directMode();
  if(state != ERR_NONE) {
    return(state);
  }
  
  // start transmitting
  SPIsendCommand(CC1101_CMD_TX);
  return(state);
}

int16_t CC1101::receiveDirect() {
  // activate direct mode
  int16_t state = directMode();
  if(state != ERR_NONE) {
    return(state);
  }
  
  // start receiving
  SPIsendCommand(CC1101_CMD_RX);
  return(ERR_NONE);
}

int16_t CC1101::setFrequency(float freq) {
  // check allowed frequency range
  if(!(((freq > 300.0) && (freq < 348.0)) ||
       ((freq > 387.0) && (freq < 464.0)) ||
       ((freq > 779.0) && (freq < 928.0)))) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  // set mode to standby
  SPIsendCommand(CC1101_CMD_IDLE);
  
  //set carrier frequency
  uint32_t base = 1;
  uint32_t FRF = (freq * (base << 16)) / 26.0;
  int16_t state = _mod->SPIsetRegValue(CC1101_REG_FREQ2, (FRF & 0xFF0000) >> 16, 7, 0);
  state |= _mod->SPIsetRegValue(CC1101_REG_FREQ1, (FRF & 0x00FF00) >> 8, 7, 0);
  state |= _mod->SPIsetRegValue(CC1101_REG_FREQ0, FRF & 0x0000FF, 7, 0);

  return(state);
}

int16_t CC1101::setBitRate(float br) {
  // check allowed bit rate range
  if(!((br >= 0.025) && (br <= 600.0))) {
    return(ERR_INVALID_BIT_RATE);
  }
  
  // set mode to standby
  SPIsendCommand(CC1101_CMD_IDLE);
  
  // calculate exponent and mantisa values
  uint8_t e = 0;
  uint8_t m = 0;
  getExpMant(br * 1000.0, 256, 28, 14, e, m);
  
  // set bit rate value
  int16_t state = _mod->SPIsetRegValue(CC1101_REG_MDMCFG4, e, 3, 0);
  state |= _mod->SPIsetRegValue(CC1101_REG_MDMCFG3, m);
  return(state);
}

int16_t CC1101::setRxBandwidth(float rxBw) {
  // check allowed bandwidth range
  if(!((rxBw >= 58) && (rxBw <= 812))) {
    return(ERR_INVALID_RX_BANDWIDTH);
  }
  
  // set mode to standby
  SPIsendCommand(CC1101_CMD_IDLE);
  
  // calculate exponent and mantisa values
  for(int8_t e = 3; e >= 0; e--) {
    for(int8_t m = 3; m >= 0; m --) {
      float point = (CC1101_CRYSTAL_FREQ * 1000000.0)/(8 * (m + 4) * ((uint32_t)1 << e));
      if(abs((rxBw * 1000.0) - point) <= 0.001) {
        // set Rx channel filter bandwidth
        return(_mod->SPIsetRegValue(CC1101_REG_MDMCFG4, (e << 6) | (m << 4), 7, 4));
      }
    }
  }
  
  return(ERR_UNKNOWN);
}

int16_t CC1101::setFrequencyDeviation(float freqDev) {
  // set frequency deviation to lowest available setting (required for RTTY)
  if(freqDev == 0.0) {
    int16_t state = _mod->SPIsetRegValue(CC1101_REG_DEVIATN, 0, 6, 4);
    state |= _mod->SPIsetRegValue(CC1101_REG_DEVIATN, 0, 2, 0);
    return(state);
  }

  // check allowed frequency deviation range
  if(!((freqDev >= 1.587) && (freqDev <= 380.8))) {
    return(ERR_INVALID_FREQUENCY_DEVIATION);
  }
  
  // set mode to standby
  SPIsendCommand(CC1101_CMD_IDLE);
  
  // calculate exponent and mantisa values
  uint8_t e = 0;
  uint8_t m = 0;
  getExpMant(freqDev * 1000.0, 8, 17, 7, e, m);
  
  // set frequency deviation value
  int16_t state = _mod->SPIsetRegValue(CC1101_REG_DEVIATN, (e << 4), 6, 4);
  state |= _mod->SPIsetRegValue(CC1101_REG_DEVIATN, m, 2, 0);
  return(state);
}

int16_t CC1101::setSyncWord(uint8_t syncH, uint8_t syncL) {
  // set sync word
  int16_t state = _mod->SPIsetRegValue(CC1101_REG_SYNC1, syncH);
  state |= _mod->SPIsetRegValue(CC1101_REG_SYNC0, syncL);
  return(state);
}

int16_t CC1101::config() {
  // enable automatic frequency synthesizer calibration 
  int16_t state = _mod->SPIsetRegValue(CC1101_REG_MCSM0, CC1101_FS_AUTOCAL_IDLE_TO_RXTX, 5, 4);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // set output power
  _mod->SPIwriteRegister(CC1101_REG_PATABLE, 0x60);
  
  return(state);
}

int16_t CC1101::directMode() {
  // set mode to standby
  SPIsendCommand(CC1101_CMD_IDLE);
  
  // set GDO0 and GDO2 mapping
  int16_t state = _mod->SPIsetRegValue(CC1101_REG_IOCFG0, CC1101_GDOX_SERIAL_CLOCK , 5, 0);
  state |= _mod->SPIsetRegValue(CC1101_REG_IOCFG2, CC1101_GDOX_SERIAL_DATA_SYNC , 5, 0);
  
  // set continuous mode
  state |= _mod->SPIsetRegValue(CC1101_REG_PKTCTRL0, CC1101_PKT_FORMAT_SYNCHRONOUS, 5, 4);
  return(state);
}

void CC1101::getExpMant(float target, uint16_t mantOffset, uint8_t divExp, uint8_t expMax, uint8_t& exp, uint8_t& mant) {
  // get table origin point (exp = 0, mant = 0)
  float origin = (mantOffset * CC1101_CRYSTAL_FREQ * 1000000.0)/((uint32_t)1 << divExp);
  
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

void CC1101::SPIwriteRegisterBurst(uint8_t reg, uint8_t* data, size_t len) {
  _mod->SPIwriteRegisterBurst(reg | CC1101_CMD_BURST, data, len);
}

int16_t CC1101::SPIgetRegValue(uint8_t reg, uint8_t msb, uint8_t lsb) {
  // status registers require special command
  if(reg > CC1101_REG_TEST0) {
    reg |= CC1101_CMD_ACCESS_STATUS_REG;
  }
  
  return(_mod->SPIgetRegValue(reg, msb, lsb));
}

uint8_t CC1101::SPIreadRegister(uint8_t reg) {
  // status registers require special command
  if(reg > CC1101_REG_TEST0) {
    reg |= CC1101_CMD_ACCESS_STATUS_REG;
  }

  return(_mod->SPIreadRegister(reg));
}

void CC1101::SPIsendCommand(uint8_t cmd) {
  digitalWrite(_mod->getCs(), LOW);
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(cmd);
  SPI.endTransaction();
  digitalWrite(_mod->getCs(), HIGH);
}
