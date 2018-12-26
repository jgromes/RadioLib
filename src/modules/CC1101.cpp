#include "CC1101.h"

CC1101::CC1101(Module* module) : PhysicalLayer(CC1101_CRYSTAL_FREQ, CC1101_DIV_EXPONENT) {
  _mod = module;
}

int16_t CC1101::begin(float freq, float br, uint16_t rxBw, float freqDev) {
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
      #ifdef KITELIB_DEBUG
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

int16_t CC1101::transmit(String& str, uint8_t addr = 0) {
  return(CC1101::transmit(str.c_str()));
}

int16_t CC1101::transmit(const char* str, uint8_t addr = 0) {
  return(CC1101::transmit((uint8_t*)str, strlen(str)));
}

int16_t CC1101::transmit(uint8_t* data, size_t len, uint8_t addr = 0) {
  // TODO
  // check packet length
  
  // set GDO0 and GDO2 mapping

  // set mode to standby
  
  return(ERR_NONE);
}

int16_t CC1101::receive(uint8_t* data, size_t len) {
  // TODO

  return(ERR_NONE);
}

int16_t CC1101::standby() {
  SPIsendCommand(CC1101_CMD_IDLE);
  return(ERR_NONE);
}

int16_t CC1101::transmitDirect(uint32_t FRF = 0) {
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
  // TODO

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
  for(uint8_t e = 14; e > 0; e++) {
    float intervalStart = 406250.00/(float)(1 << (14 - e));
    if((br * 1000.0) > intervalStart) {
      float stepSize = intervalStart/256.0;
      uint8_t m = (uint8_t)(((br * 1000.0) - intervalStart) / stepSize);
      
      // set bit rate value
      int16_t state = _mod->SPIsetRegValue(CC1101_REG_MDMCFG4, e, 3, 0);
      state |= _mod->SPIsetRegValue(CC1101_REG_MDMCFG3, m);
      return(state);
    }
	}
  
  return(ERR_UNKNOWN);
}

int16_t CC1101::setRxBandwidth(uint16_t rxBw) {
  // check allowed bandwidth range
  uint8_t bwMant, bwExp;
  switch(rxBw) {
    case 58:
      bwMant = 3;
      bwExp = 3;
      break;
    case 68:
      bwMant = 2;
      bwExp = 3;
      break;
    case 81:
      bwMant = 1;
      bwExp = 3;
      break;
    case 102:
      bwMant = 0;
      bwExp = 3;
      break;
    case 116:
      bwMant = 3;
      bwExp = 2;
      break;
    case 135:
      bwMant = 2;
      bwExp = 2;
      break;
    case 162:
      bwMant = 1;
      bwExp = 2;
      break;
    case 203:
      bwMant = 0;
      bwExp = 2;
      break;
    case 232:
      bwMant = 3;
      bwExp = 1;
      break;
    case 270:
      bwMant = 2;
      bwExp = 1;
      break;
    case 325:
      bwMant = 1;
      bwExp = 1;
      break;
    case 406:
      bwMant = 0;
      bwExp = 1;
      break;
    case 464:
      bwMant = 3;
      bwExp = 0;
      break;
    case 541:
      bwMant = 2;
      bwExp = 0;
      break;
    case 650:
      bwMant = 1;
      bwExp = 0;
      break;
    case 812:
      bwMant = 0;
      bwExp = 0;
      break;
    default:
      return(ERR_INVALID_RX_BANDWIDTH);
  }
  
  // set mode to standby
  SPIsendCommand(CC1101_CMD_IDLE);
  
  // set Rx channel filter bandwidth
  return(_mod->SPIsetRegValue(CC1101_REG_MDMCFG4, (bwExp << 6) | (bwMant << 4), 7, 4));
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

int16_t CC1101::config() {
  // enable autmatic frequency synthesizer calibration 
  int16_t state = _mod->SPIsetRegValue(CC1101_REG_MCSM0, CC1101_FS_AUTOCAL_IDLE_TO_RXTX, 5, 4);

  return(state);
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
  for(uint8_t e = 7; e > 0; e++) {
    float intervalStart = 203125.00/(float)(1 << (7 - e));
    if((freqDev * 1000.0) > intervalStart) {
      float stepSize = intervalStart/8.0;
      uint8_t m = (uint8_t)(((freqDev * 1000.0) - intervalStart) / stepSize);
      
      // set frequency deviation value
      int16_t state = _mod->SPIsetRegValue(CC1101_REG_DEVIATN, (e << 4), 6, 4);
      state |= _mod->SPIsetRegValue(CC1101_REG_DEVIATN, m, 2, 0);
      return(state);
    }
	}
  
  return(ERR_UNKNOWN);
}

int16_t CC1101::SPIgetRegValue(uint8_t reg, uint8_t msb, uint8_t lsb) {
  return(_mod->SPIgetRegValue(reg | CC1101_CMD_ACCESS_STATUS_REG, msb, lsb));
}

uint8_t CC1101::SPIreadRegister(uint8_t reg) {
  return(_mod->SPIreadRegister(reg | CC1101_CMD_ACCESS_STATUS_REG));
}

void CC1101::SPIsendCommand(uint8_t cmd) {
  digitalWrite(_mod->cs(), LOW);
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(cmd);
  SPI.endTransaction();
  digitalWrite(_mod->cs(), HIGH);
}
