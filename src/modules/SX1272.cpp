#include "SX1272.h"

SX1272::SX1272(Module* mod) : SX127x(mod) {
  
}

uint8_t SX1272::begin(float freq, uint32_t bw, uint8_t sf, uint8_t cr, uint8_t syncWord, uint16_t addrEeprom) {
  uint8_t state = SX127x::begin(freq, bw, sf, cr, syncWord, addrEeprom);
  if(state != ERR_NONE) {
    return(state);
  }
  
  return(config(freq, bw, sf, cr, syncWord));
}

uint8_t SX1272::setBandwidth(uint32_t bw) {
  uint8_t state = SX1272::config(bw, _sf, _cr, _freq, _syncWord);
  if(state == ERR_NONE) {
    _bw = bw;
  }
  return(state);
}

uint8_t SX1272::setSpreadingFactor(uint8_t sf) {
  uint8_t state = SX1272::config(_bw, sf, _cr, _freq, _syncWord);
  if(state == ERR_NONE) {
    _sf = sf;
  }
  return(state);
}

uint8_t SX1272::setCodingRate(uint8_t cr) {
  uint8_t state = SX1272::config(_bw, _sf, cr, _freq, _syncWord);
  if(state == ERR_NONE) {
    _cr = cr;
  }
  return(state);
}

uint8_t SX1272::config(float freq, uint32_t bw, uint8_t sf, uint8_t cr, uint8_t syncWord) {
  uint8_t status = ERR_NONE;
  uint8_t newBandwidth, newSpreadingFactor, newCodingRate;
  
  // check the supplied BW, CR and SF values
  switch(bw) {
    case 125000:
      newBandwidth = SX1272_BW_125_00_KHZ;
      break;
    case 250000:
      newBandwidth = SX1272_BW_250_00_KHZ;
      break;
    case 500000:
      newBandwidth = SX1272_BW_500_00_KHZ;
      break;
    default:
      return(ERR_INVALID_BANDWIDTH);
  }
  
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
    case 10:
      newSpreadingFactor = SX127X_SF_10;
      break;
    case 11:
      newSpreadingFactor = SX127X_SF_11;
      break;
    case 12:
      newSpreadingFactor = SX127X_SF_12;
      break;
    default:
      return(ERR_INVALID_SPREADING_FACTOR);
  }
  
  switch(cr) {
    case 5:
      newCodingRate = SX1272_CR_4_5;
      break;
    case 6:
      newCodingRate = SX1272_CR_4_6;
      break;
    case 7:
      newCodingRate = SX1272_CR_4_7;
      break;
    case 8:
      newCodingRate = SX1272_CR_4_8;
      break;
    default:
      return(ERR_INVALID_CODING_RATE);
  }
  
  if((freq < 137.0) || (freq > 525.0)) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  // execute common part
  status = SX1272::configCommon(newBandwidth, newSpreadingFactor, newCodingRate, freq, syncWord);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // configuration successful, save the new settings
  _bw = bw;
  _sf = sf;
  _cr = cr;
  _freq = freq;
  
  return(ERR_NONE);
}

uint8_t SX1272::configCommon(uint8_t bw, uint8_t sf, uint8_t cr, float freq, uint8_t syncWord) {
  // configure common registers
  uint8_t status = SX127x::config(bw, sf, cr, freq, syncWord);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // output power configuration
  status = _mod->SPIsetRegValue(SX1272_REG_PA_DAC, SX127X_PA_BOOST_ON, 2, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // enable LNA gain setting by register
  status = _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, SX1272_AGC_AUTO_OFF, 2, 2);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set SF6 optimizations
  if(sf == SX127X_SF_6) {
    status = _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, bw | cr | SX1272_HEADER_IMPL_MODE | SX1272_RX_CRC_MODE_OFF, 7, 1);
  } else {
    status = _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, bw | cr | SX1272_HEADER_EXPL_MODE | SX1272_RX_CRC_MODE_ON,  7, 1);
  }
  if(status != ERR_NONE) {
    return(status);
  }
  
  // calculate symbol length and set low datarate optimization, if needed
  uint16_t base = 1;
  float symbolLength = (float)(base << _sf) / (float)_bw;
  if(symbolLength >= 0.016) {
    status = _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, SX1272_LOW_DATA_RATE_OPT_ON,  0, 0);
  } else {
    status = _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, SX1272_LOW_DATA_RATE_OPT_OFF,  0, 0);
  }
  if(status != ERR_NONE) {
    return(status);
  }
  
  return(status);
}
