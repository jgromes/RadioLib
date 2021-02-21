#include "SX1278.h"
#if !defined(RADIOLIB_EXCLUDE_SX127X)

SX1278::SX1278(Module* mod) : SX127x(mod) {

}

int16_t SX1278::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain) {
  // execute common part
  int16_t state = SX127x::begin(SX1278_CHIP_VERSION, syncWord, preambleLength);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setBandwidth(bw);
  RADIOLIB_ASSERT(state);

  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setSpreadingFactor(sf);
  RADIOLIB_ASSERT(state);

  state = setCodingRate(cr);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = setGain(gain);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1278::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK) {
  // execute common part
  int16_t state = SX127x::beginFSK(SX1278_CHIP_VERSION, br, freqDev, rxBw, preambleLength, enableOOK);
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = configFSK();
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);

  return(state);
}

void SX1278::reset() {
  Module::pinMode(_mod->getRst(), OUTPUT);
  Module::digitalWrite(_mod->getRst(), LOW);
  Module::delay(1);
  Module::digitalWrite(_mod->getRst(), HIGH);
  Module::delay(5);
}

int16_t SX1278::setFrequency(float freq) {
  RADIOLIB_CHECK_RANGE(freq, 137.0, 525.0, ERR_INVALID_FREQUENCY);

  // SX1276/77/78 Errata fixes
  if(getActiveModem() == SX127X_LORA) {
    // sensitivity optimization for 500kHz bandwidth
    // see SX1276/77/78 Errata, section 2.1 for details
    if(abs(_bw - 500.0) <= 0.001) {
      if((freq >= 862.0) && (freq <= 1020.0)) {
        _mod->SPIwriteRegister(0x36, 0x02);
        _mod->SPIwriteRegister(0x3a, 0x64);
      } else if((freq >= 410.0) && (freq <= 525.0)) {
        _mod->SPIwriteRegister(0x36, 0x02);
        _mod->SPIwriteRegister(0x3a, 0x7F);
      }
    }

    // mitigation of receiver spurious response
    // see SX1276/77/78 Errata, section 2.3 for details
    if(abs(_bw - 7.8) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x48);
      _mod->SPIsetRegValue(0x30, 0x00);
      freq += 7.8;
    } else if(abs(_bw - 10.4) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x44);
      _mod->SPIsetRegValue(0x30, 0x00);
      freq += 10.4;
    } else if(abs(_bw - 15.6) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x44);
      _mod->SPIsetRegValue(0x30, 0x00);
      freq += 15.6;
    } else if(abs(_bw - 20.8) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x44);
      _mod->SPIsetRegValue(0x30, 0x00);
      freq += 20.8;
    } else if(abs(_bw - 31.25) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x44);
      _mod->SPIsetRegValue(0x30, 0x00);
      freq += 31.25;
    } else if(abs(_bw - 41.7) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x44);
      _mod->SPIsetRegValue(0x30, 0x00);
      freq += 41.7;
    } else if(abs(_bw - 62.5) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x40);
      _mod->SPIsetRegValue(0x30, 0x00);
    } else if(abs(_bw - 125.0) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x40);
      _mod->SPIsetRegValue(0x30, 0x00);
    } else if(abs(_bw - 250.0) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b0000000, 7, 7);
      _mod->SPIsetRegValue(0x2F, 0x40);
      _mod->SPIsetRegValue(0x30, 0x00);
    } else if(abs(_bw - 500.0) <= 0.001) {
      _mod->SPIsetRegValue(0x31, 0b1000000, 7, 7);
    }
  }

  // set frequency and if successful, save the new setting
  int16_t state = SX127x::setFrequencyRaw(freq);
  if(state == ERR_NONE) {
    SX127x::_freq = freq;
  }
  return(state);
}

int16_t SX1278::setBandwidth(float bw) {
  // check active modem
  if(getActiveModem() != SX127X_LORA) {
    return(ERR_WRONG_MODEM);
  }

  uint8_t newBandwidth;

  // check allowed bandwidth values
  if(abs(bw - 7.8) <= 0.001) {
    newBandwidth = SX1278_BW_7_80_KHZ;
  } else if(abs(bw - 10.4) <= 0.001) {
    newBandwidth = SX1278_BW_10_40_KHZ;
  } else if(abs(bw - 15.6) <= 0.001) {
    newBandwidth = SX1278_BW_15_60_KHZ;
  } else if(abs(bw - 20.8) <= 0.001) {
    newBandwidth = SX1278_BW_20_80_KHZ;
  } else if(abs(bw - 31.25) <= 0.001) {
    newBandwidth = SX1278_BW_31_25_KHZ;
  } else if(abs(bw - 41.7) <= 0.001) {
    newBandwidth = SX1278_BW_41_70_KHZ;
  } else if(abs(bw - 62.5) <= 0.001) {
    newBandwidth = SX1278_BW_62_50_KHZ;
  } else if(abs(bw - 125.0) <= 0.001) {
    newBandwidth = SX1278_BW_125_00_KHZ;
  } else if(abs(bw - 250.0) <= 0.001) {
    newBandwidth = SX1278_BW_250_00_KHZ;
  } else if(abs(bw - 500.0) <= 0.001) {
    newBandwidth = SX1278_BW_500_00_KHZ;
  } else {
    return(ERR_INVALID_BANDWIDTH);
  }

  // set bandwidth and if successful, save the new setting
  int16_t state = SX1278::setBandwidthRaw(newBandwidth);
  if(state == ERR_NONE) {
    SX127x::_bw = bw;

    // calculate symbol length and set low data rate optimization, if auto-configuration is enabled
    if(_ldroAuto) {
      float symbolLength = (float)(uint32_t(1) << SX127x::_sf) / (float)SX127x::_bw;
      RADIOLIB_DEBUG_PRINT("Symbol length: ");
      RADIOLIB_DEBUG_PRINT(symbolLength);
      RADIOLIB_DEBUG_PRINTLN(" ms");
      if(symbolLength >= 16.0) {
        state = _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_LOW_DATA_RATE_OPT_ON, 3, 3);
      } else {
        state = _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_LOW_DATA_RATE_OPT_OFF, 3, 3);
      }
    }
  }
  return(state);
}

int16_t SX1278::setSpreadingFactor(uint8_t sf) {
  // check active modem
  if(getActiveModem() != SX127X_LORA) {
    return(ERR_WRONG_MODEM);
  }

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
  int16_t state = SX1278::setSpreadingFactorRaw(newSpreadingFactor);
  if(state == ERR_NONE) {
    SX127x::_sf = sf;

    // calculate symbol length and set low data rate optimization, if auto-configuration is enabled
    if(_ldroAuto) {
      float symbolLength = (float)(uint32_t(1) << SX127x::_sf) / (float)SX127x::_bw;
      RADIOLIB_DEBUG_PRINT("Symbol length: ");
      RADIOLIB_DEBUG_PRINT(symbolLength);
      RADIOLIB_DEBUG_PRINTLN(" ms");
      if(symbolLength >= 16.0) {
        state = _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_LOW_DATA_RATE_OPT_ON, 3, 3);
      } else {
        state = _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_LOW_DATA_RATE_OPT_OFF, 3, 3);
      }
    }
  }
  return(state);
}

int16_t SX1278::setCodingRate(uint8_t cr) {
  // check active modem
  if(getActiveModem() != SX127X_LORA) {
    return(ERR_WRONG_MODEM);
  }

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
  int16_t state = SX1278::setCodingRateRaw(newCodingRate);
  if(state == ERR_NONE) {
    SX127x::_cr = cr;
  }
  return(state);
}

int16_t SX1278::setOutputPower(int8_t power) {
  // check allowed power range
  if(!(((power >= -3) && (power <= 17)) || (power == 20))) {
    return(ERR_INVALID_OUTPUT_POWER);
  }

  // set mode to standby
  int16_t state = SX127x::standby();

  // set output power
  if(power < 2) {
    // power is less than 2 dBm, enable PA on RFO
    state |= _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX127X_PA_SELECT_RFO, 7, 7);
    state |= _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX1278_LOW_POWER | (power + 3), 6, 0);
    state |= _mod->SPIsetRegValue(SX1278_REG_PA_DAC, SX127X_PA_BOOST_OFF, 2, 0);
  } else if(power <= 17) {
    // power is 2 - 17 dBm, enable PA1 + PA2 on PA_BOOST
    state |= _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX127X_PA_SELECT_BOOST, 7, 7);
    state |= _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX1278_MAX_POWER | (power - 2), 6, 0);
    state |= _mod->SPIsetRegValue(SX1278_REG_PA_DAC, SX127X_PA_BOOST_OFF, 2, 0);
  } else if(power == 20) {
    // power is 20 dBm, enable PA1 + PA2 on PA_BOOST and enable high power mode
    state |= _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX127X_PA_SELECT_BOOST, 7, 7);
    state |= _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX1278_MAX_POWER | (power - 5), 6, 0);
    state |= _mod->SPIsetRegValue(SX1278_REG_PA_DAC, SX127X_PA_BOOST_ON, 2, 0);
  }
  return(state);
}

int16_t SX1278::setGain(uint8_t gain) {
  // check active modem
  if(getActiveModem() != SX127X_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // check allowed range
  if(gain > 6) {
    return(ERR_INVALID_GAIN);
  }

  // set mode to standby
  int16_t state = SX127x::standby();

  // set gain
  if(gain == 0) {
    // gain set to 0, enable AGC loop
    state |= _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_AGC_AUTO_ON, 2, 2);
  } else {
    state |= _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_AGC_AUTO_OFF, 2, 2);
    state |= _mod->SPIsetRegValue(SX127X_REG_LNA, (gain << 5) | SX127X_LNA_BOOST_ON);
  }
  return(state);
}

int16_t SX1278::setDataShaping(uint8_t sh) {
  // check active modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // check modulation
  if(SX127x::_ook) {
    return(ERR_INVALID_MODULATION);
  }

  // set mode to standby
  int16_t state = SX127x::standby();
  RADIOLIB_ASSERT(state);

  // set data shaping
  switch(sh) {
    case RADIOLIB_SHAPING_NONE:
      return(_mod->SPIsetRegValue(SX127X_REG_PA_RAMP, SX1278_NO_SHAPING, 6, 5));
    case RADIOLIB_SHAPING_0_3:
      return(_mod->SPIsetRegValue(SX127X_REG_PA_RAMP, SX1278_FSK_GAUSSIAN_0_3, 6, 5));
    case RADIOLIB_SHAPING_0_5:
      return(_mod->SPIsetRegValue(SX127X_REG_PA_RAMP, SX1278_FSK_GAUSSIAN_0_5, 6, 5));
    case RADIOLIB_SHAPING_1_0:
      return(_mod->SPIsetRegValue(SX127X_REG_PA_RAMP, SX1278_FSK_GAUSSIAN_1_0, 6, 5));
    default:
      return(ERR_INVALID_DATA_SHAPING);
  }
}

int16_t SX1278::setDataShapingOOK(uint8_t sh) {
  // check active modem
  if(getActiveModem() != SX127X_FSK_OOK) {
    return(ERR_WRONG_MODEM);
  }

  // check modulation
  if(!SX127x::_ook) {
    return(ERR_INVALID_MODULATION);
  }

  // set mode to standby
  int16_t state = SX127x::standby();

  // set data shaping
  switch(sh) {
    case 0:
      state |= _mod->SPIsetRegValue(SX127X_REG_PA_RAMP, SX1278_NO_SHAPING, 6, 5);
      break;
    case 1:
      state |= _mod->SPIsetRegValue(SX127X_REG_PA_RAMP, SX1278_OOK_FILTER_BR, 6, 5);
      break;
    case 2:
      state |= _mod->SPIsetRegValue(SX127X_REG_PA_RAMP, SX1278_OOK_FILTER_2BR, 6, 5);
      break;
    default:
      return(ERR_INVALID_DATA_SHAPING);
  }

  return(state);
}

float SX1278::getRSSI() {
  if(getActiveModem() == SX127X_LORA) {
    // for LoRa, get RSSI of the last packet
    float lastPacketRSSI;

    // RSSI calculation uses different constant for low-frequency and high-frequency ports
    if(_freq < 868.0) {
      lastPacketRSSI = -164 + _mod->SPIgetRegValue(SX127X_REG_PKT_RSSI_VALUE);
    } else {
      lastPacketRSSI = -157 + _mod->SPIgetRegValue(SX127X_REG_PKT_RSSI_VALUE);
    }

    // spread-spectrum modulation signal can be received below noise floor
    // check last packet SNR and if it's less than 0, add it to reported RSSI to get the correct value
    float lastPacketSNR = SX127x::getSNR();
    if(lastPacketSNR < 0.0) {
      lastPacketRSSI += lastPacketSNR;
    }

    return(lastPacketRSSI);

  } else {
    // enable listen mode
    startReceive();

    // read the value for FSK
    float rssi = (float)_mod->SPIgetRegValue(SX127X_REG_RSSI_VALUE_FSK) / -2.0;

    // set mode back to standby
    standby();

    // return the value
    return(rssi);
  }
}

int16_t SX1278::setCRC(bool enableCRC) {
  if(getActiveModem() == SX127X_LORA) {
    // set LoRa CRC
    SX127x::_crcEnabled = enableCRC;
    if(enableCRC) {
      return(_mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, SX1278_RX_CRC_MODE_ON, 2, 2));
    } else {
      return(_mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, SX1278_RX_CRC_MODE_OFF, 2, 2));
    }
  } else {
    // set FSK CRC
    if(enableCRC) {
      return(_mod->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_1, SX127X_CRC_ON, 4, 4));
    } else {
      return(_mod->SPIsetRegValue(SX127X_REG_PACKET_CONFIG_1, SX127X_CRC_OFF, 4, 4));
    }
  }
}

int16_t SX1278::forceLDRO(bool enable) {
  if(getActiveModem() != SX127X_LORA) {
    return(ERR_WRONG_MODEM);
  }

  _ldroAuto = false;
  if(enable) {
    return(_mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_LOW_DATA_RATE_OPT_ON, 3, 3));
  } else {
    return(_mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_3, SX1278_LOW_DATA_RATE_OPT_OFF, 3, 3));
  }
}

int16_t SX1278::autoLDRO() {
  if(getActiveModem() != SX127X_LORA) {
    return(ERR_WRONG_MODEM);
  }

  _ldroAuto = true;
  return(ERR_NONE);
}

int16_t SX1278::implicitHeader(size_t len) {
  return(setHeaderType(SX1278_HEADER_IMPL_MODE, len));
}

int16_t SX1278::explicitHeader() {
  return(setHeaderType(SX1278_HEADER_EXPL_MODE));
}

int16_t SX1278::setBandwidthRaw(uint8_t newBandwidth) {
  // set mode to standby
  int16_t state = SX127x::standby();

  // write register
  state |= _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, newBandwidth, 7, 4);
  return(state);
}

int16_t SX1278::setSpreadingFactorRaw(uint8_t newSpreadingFactor) {
  // set mode to standby
  int16_t state = SX127x::standby();

  // write registers
  if(newSpreadingFactor == SX127X_SF_6) {
    state |= _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, SX1278_HEADER_IMPL_MODE, 0, 0);
    state |= _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, SX127X_SF_6 | SX127X_TX_MODE_SINGLE | (SX127x::_crcEnabled ? SX1278_RX_CRC_MODE_ON : SX1278_RX_CRC_MODE_OFF), 7, 2);
    state |= _mod->SPIsetRegValue(SX127X_REG_DETECT_OPTIMIZE, SX127X_DETECT_OPTIMIZE_SF_6, 2, 0);
    state |= _mod->SPIsetRegValue(SX127X_REG_DETECTION_THRESHOLD, SX127X_DETECTION_THRESHOLD_SF_6);
  } else {
    state |= _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, SX1278_HEADER_EXPL_MODE, 0, 0);
    state |= _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, newSpreadingFactor | SX127X_TX_MODE_SINGLE | (SX127x::_crcEnabled ? SX1278_RX_CRC_MODE_ON : SX1278_RX_CRC_MODE_OFF), 7, 2);
    state |= _mod->SPIsetRegValue(SX127X_REG_DETECT_OPTIMIZE, SX127X_DETECT_OPTIMIZE_SF_7_12, 2, 0);
    state |= _mod->SPIsetRegValue(SX127X_REG_DETECTION_THRESHOLD, SX127X_DETECTION_THRESHOLD_SF_7_12);
  }
  return(state);
}

int16_t SX1278::setCodingRateRaw(uint8_t newCodingRate) {
  // set mode to standby
  int16_t state = SX127x::standby();

  // write register
  state |= _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, newCodingRate, 3, 1);
  return(state);
}

int16_t SX1278::setHeaderType(uint8_t headerType, size_t len) {
  // check active modem
  if(getActiveModem() != SX127X_LORA) {
    return(ERR_WRONG_MODEM);
  }

  // set requested packet mode
  int16_t state = _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_1, headerType, 0, 0);
  RADIOLIB_ASSERT(state);

  // set length to register
  state = _mod->SPIsetRegValue(SX127X_REG_PAYLOAD_LENGTH, len);
  RADIOLIB_ASSERT(state);

  // update cached value
  _packetLength = len;

  return(state);
}

int16_t SX1278::configFSK() {
  // configure common registers
  int16_t state = SX127x::configFSK();
  RADIOLIB_ASSERT(state);

  // set fast PLL hop
  state = _mod->SPIsetRegValue(SX1278_REG_PLL_HOP, SX127X_FAST_HOP_ON, 7, 7);
  return(state);
}

#endif
