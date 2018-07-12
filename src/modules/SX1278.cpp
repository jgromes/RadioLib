#include "SX1278.h"

SX1278::SX1278(Module* mod) : SX127x(mod) {
  
}

uint8_t SX1278::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power) {
  // execute common part
  uint8_t state = SX127x::begin(syncWord);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // configure settings not accessible by API
  state = config();
  if(state != ERR_NONE) {
    return(state);
  }
  
  // configure publicly accessible settings
  state = setFrequency(freq);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setBandwidth(bw);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setSpreadingFactor(sf);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setCodingRate(cr);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setOutputPower(power);
  if(state != ERR_NONE) {
    return(state);
  }
  
  return(ERR_NONE);
}

uint8_t SX1278::setFrequency(float freq) {
  // check frequency range
  if((freq < 137.0) || (freq > 525.0)) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  // set frequency and if successful, save the new setting
  uint8_t state = SX1278::setFrequencyRaw(freq);
  if(state == ERR_NONE) {
    SX127x::_freq = freq;
  }
  return(state);
}

uint8_t SX1278::setBandwidth(float bw) {
  uint8_t newBandwidth;
  
  // check alowed bandwidth values
  if(bw == 7.8) {
    newBandwidth = SX1278_BW_7_80_KHZ;
  } else if(bw == 10.4) {
    newBandwidth = SX1278_BW_10_40_KHZ;
  } else if(bw == 15.6) {
    newBandwidth = SX1278_BW_15_60_KHZ;
  } else if(bw == 20.8) {
    newBandwidth = SX1278_BW_20_80_KHZ;
  } else if(bw == 31.25) {
    newBandwidth = SX1278_BW_31_25_KHZ;
  } else if(bw == 62.5) {
    newBandwidth = SX1278_BW_62_50_KHZ;
  } else if(bw == 125.0) {
    newBandwidth = SX1278_BW_125_00_KHZ;
  } else if(bw == 250.0) {
    newBandwidth = SX1278_BW_250_00_KHZ;
  } else if(bw == 500.0) {
    newBandwidth = SX1278_BW_500_00_KHZ;
  } else {
    return(ERR_INVALID_BANDWIDTH);
  }
  
  // set bandwidth and if successful, save the new setting
  uint8_t state = SX1278::setBandwidthRaw(newBandwidth);
  if(state == ERR_NONE) {
    SX127x::_bw = bw;
  }
  
  return(state);
}

uint8_t SX1278::setSpreadingFactor(uint8_t sf) {
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
  
  // set spreading factor and if successful, save the new setting
  uint8_t state = SX1278::setSpreadingFactorRaw(newSpreadingFactor);
  if(state == ERR_NONE) {
    SX127x::_sf = sf;
  }
  
  return(state);
}

uint8_t SX1278::setCodingRate(uint8_t cr) {
  uint8_t newCodingRate;
  
  // check allowed coding rate values
  switch(cr) {
    case 5:
      newCodingRate = SX1278_CR_4_5;
      break;
    case 6:
      newCodingRate = SX1278_CR_4_6;
      break;
    case 7:
      newCodingRate = SX1278_CR_4_7;
      break;
    case 8:
      newCodingRate = SX1278_CR_4_8;
      break;
    default:
      return(ERR_INVALID_CODING_RATE);
  }
  
  // set coding rate and if successful, save the new setting
  uint8_t state = SX1278::setCodingRateRaw(newCodingRate);
  if(state == ERR_NONE) {
    SX127x::_cr = cr;
  }
  
  return(state);
}

uint8_t SX1278::setOutputPower(int8_t power) {
  if((power < -3) || (power > 20)) {
    return(ERR_INVALID_OUTPUT_POWER);
  }
  
  // set mode to standby
  SX127x::standby();
  
  uint8_t state;
  if(power < 13) {
    // power is less than 12 dBm, enable PA on RFO
    state = _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX127X_PA_SELECT_RFO, 7, 7);
    state |= _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX1278_LOW_POWER | (power + 3), 6, 0);
    state |= _mod->SPIsetRegValue(SX1278_REG_PA_DAC, SX127X_PA_BOOST_OFF, 2, 0);
  } else if((power >= 13) && (power < 18)) {
    // power is 13 - 17 dBm, enable PA1 + PA2 on PA_BOOST
    state = _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX127X_PA_SELECT_BOOST, 7, 7);
    state |= _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX1278_MAX_POWER | (power + 2), 6, 0);
    state |= _mod->SPIsetRegValue(SX1278_REG_PA_DAC, SX127X_PA_BOOST_OFF, 2, 0);
  } else if(power >= 18) {
    // power is 18 - 20 dBm, enable PA1 + PA2 on PA_BOOST and enable high power mode
    state = _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX127X_PA_SELECT_BOOST, 7, 7);
    state |= _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX1278_MAX_POWER | (power + 2), 6, 0);
    state |= _mod->SPIsetRegValue(SX1278_REG_PA_DAC, SX127X_PA_BOOST_ON, 2, 0);
  }
  
  // configuration successful, save new setting
  if(state == ERR_NONE) {
    _power = power;
  }
  
  return(state);
}

uint8_t SX1278::setBandwidthRaw(uint8_t newBandwidth) {
  // set mode to standby
  SX127x::standby();
  
  // write register
  return(_mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, newBandwidth, 7, 4));
}

uint8_t SX1278::setSpreadingFactorRaw(uint8_t newSpreadingFactor) {
  // set mode to standby
  SX127x::standby();
  
  // write registers
  uint8_t state = 0;
  if(newSpreadingFactor == SX127X_SF_6) {
    state |= _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, SX1278_HEADER_IMPL_MODE, 0, 0);
    state |= _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, SX127X_SF_6 | SX127X_TX_MODE_SINGLE | SX1278_RX_CRC_MODE_OFF, 7, 2);
    state |= _mod->SPIsetRegValue(SX127X_REG_DETECT_OPTIMIZE, SX127X_DETECT_OPTIMIZE_SF_6, 2, 0);
    state |= _mod->SPIsetRegValue(SX127X_REG_DETECTION_THRESHOLD, SX127X_DETECTION_THRESHOLD_SF_6);
  } else {
    state |= _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, SX1278_HEADER_EXPL_MODE, 0, 0);
    state |= _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, newSpreadingFactor | SX127X_TX_MODE_SINGLE | SX1278_RX_CRC_MODE_OFF, 7, 2);
    state |= _mod->SPIsetRegValue(SX127X_REG_DETECT_OPTIMIZE, SX127X_DETECT_OPTIMIZE_SF_7_12, 2, 0);
    state |= _mod->SPIsetRegValue(SX127X_REG_DETECTION_THRESHOLD, SX127X_DETECTION_THRESHOLD_SF_7_12);
  }
  
  return(state);
}

uint8_t SX1278::setCodingRateRaw(uint8_t newCodingRate) {
  // set mode to standby
  SX127x::standby();
  
  // write register
  return(_mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, newCodingRate, 3, 1));
}

uint8_t SX1278::config() {
  // configure common registers
  uint8_t state = SX127x::config();
  if(state != ERR_NONE) {
    return(state);
  }
  
  // enable LNA gain setting by register 
  state = _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_AGC_AUTO_OFF, 2, 2);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // calculate symbol length and set low datarate optimization, if needed
  uint16_t base = 1;
  float symbolLength = (float)(base << _sf) / (float)_bw;
  if(symbolLength >= 0.016) {
    state = _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_LOW_DATA_RATE_OPT_ON, 0, 0);
  } else {
    state = _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_LOW_DATA_RATE_OPT_OFF, 0, 0);
  }
  
  return(state);
}
