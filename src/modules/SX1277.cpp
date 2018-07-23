#include "SX1277.h"

SX1277::SX1277(Module* mod) : SX1278(mod) {
  
}

int16_t SX1277::setFrequency(float freq) {
  // check frequency range
  if((freq < 137.0) || (freq > 1020.0)) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  // sensitivity optimization for 500kHz bandwidth
  // see SX1276/77/78 Errata, section 2.1 for details
  if(_bw == 500.0) {
    if((freq >= 862.0) && (freq <= 1020.0)) {
      _mod->SPIwriteRegister(0x36, 0x02);
      _mod->SPIwriteRegister(0x3a, 0x64);
    } else if((freq >= 410.0) && (freq <= 525.0)) {
      _mod->SPIwriteRegister(0x36, 0x03);
      _mod->SPIwriteRegister(0x3a, 0x65);
    }
  }
  
  // mitigation of receiver spurious response
  // see SX1276/77/78 Errata, section 2.3 for details
  if(_bw == 7.8) {
    _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
    _mod->SPIsetRegValue(0x2F, 0x48);
    _mod->SPIsetRegValue(0x30, 0x00);
    freq += 7.8;
  } else if(_bw == 10.4) {
    _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
    _mod->SPIsetRegValue(0x2F, 0x44);
    _mod->SPIsetRegValue(0x30, 0x00);
    freq += 10.4;
  } else if(_bw == 15.6) {
    _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
    _mod->SPIsetRegValue(0x2F, 0x44);
    _mod->SPIsetRegValue(0x30, 0x00);
    freq += 15.6;
  } else if(_bw == 20.8) {
    _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
    _mod->SPIsetRegValue(0x2F, 0x44);
    _mod->SPIsetRegValue(0x30, 0x00);
    freq += 20.8;
  } else if(_bw == 31.25) {
    _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
    _mod->SPIsetRegValue(0x2F, 0x44);
    _mod->SPIsetRegValue(0x30, 0x00);
    freq += 31.25;
  } else if(_bw == 41.7) {
    _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
    _mod->SPIsetRegValue(0x2F, 0x44);
    _mod->SPIsetRegValue(0x30, 0x00);
    freq += 41.7;
  } else if(_bw == 62.5) {
    _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
    _mod->SPIsetRegValue(0x2F, 0x40);
    _mod->SPIsetRegValue(0x30, 0x00);
  } else if(_bw == 125.0) {
    _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
    _mod->SPIsetRegValue(0x2F, 0x40);
    _mod->SPIsetRegValue(0x30, 0x00);
  } else if(_bw == 250.0) {
    _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
    _mod->SPIsetRegValue(0x2F, 0x40);
    _mod->SPIsetRegValue(0x30, 0x00);
  } else if(_bw == 500.0) {
    _mod->SPIsetRegValue(0x31, 0b1000000, 7, 7);
  }
  
  // set frequency and if successful, save the new setting
  return(SX127x::setFrequencyRaw(freq));
}

int16_t SX1277::setSpreadingFactor(uint8_t sf) {
  uint8_t newSpreadingFactor;
  
  // check allowed spreading factor values
  switch(sf) {
    case 6:
      newSpreadingFactor = SX127X_SF_6;
      break;
    case 7:
      newSpreadingFactor = SX127X_SF_7;
      break;
    case 8:
      newSpreadingFactor = SX127X_SF_8;
      break;
    case 9:
      newSpreadingFactor = SX127X_SF_9;
      break;
    default:
      return(ERR_INVALID_SPREADING_FACTOR);
  }
  
  // set spreading factor and if successful, save the new setting
  int16_t state = SX1278::setSpreadingFactorRaw(newSpreadingFactor);
  if(state == ERR_NONE) {
    SX127x::_sf = sf;
  }
  
  return(state);
}
