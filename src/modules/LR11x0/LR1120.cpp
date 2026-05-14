#include "LR1120.h"
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR11X0

LR1120::LR1120(Module* mod) : LR11x0(mod) {
  chipType = RADIOLIB_LR11X0_DEVICE_LR1120;
}

int16_t LR1120::begin(const LR11x0::ConfigLoRa_t& config) {
  // execute common part
  int16_t state = LR11x0::begin(config.bandwidth, config.spreadingFactor, 
    config.codingRate, config.syncWord, config.preambleLength, config.tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  return(state);
}

int16_t LR1120::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  LR11x0::ConfigLoRa_t config;
  config.frequency = freq;
  config.bandwidth = bw;
  config.spreadingFactor = sf;
  config.codingRate = cr;
  config.syncWord = syncWord;
  config.power = power;
  config.preambleLength = preambleLength;
  config.tcxoVoltage = tcxoVoltage;
  return(begin(config));
}

int16_t LR1120::beginGFSK(const LR11x0::ConfigGFSK_t& config) {
  // execute common part
  int16_t state = LR11x0::beginGFSK(config.bitRate, config.frequencyDeviation, 
    config.receiverBandwidth, config.preambleLength, config.tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  return(state);
}

int16_t LR1120::beginGFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  LR11x0::ConfigGFSK_t config;
  config.frequency = freq;
  config.bitRate = br;
  config.frequencyDeviation = freqDev;
  config.receiverBandwidth = rxBw;
  config.power = power;
  config.preambleLength = preambleLength;
  config.tcxoVoltage = tcxoVoltage;
  return(beginGFSK(config));
}

int16_t LR1120::beginLRFHSS(const LR11x0::ConfigLRFHSS_t& config) {
  // execute common part
  int16_t state = LR11x0::beginLRFHSS(config.bandwidth, config.bandwidth, 
    config.narrowGrid, config.tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  return(state);
}

int16_t LR1120::beginLRFHSS(float freq, uint8_t bw, uint8_t cr, bool narrowGrid, int8_t power, float tcxoVoltage) {
  LR11x0::ConfigLRFHSS_t config;
  config.frequency = freq;
  config.bandwidth = bw;
  config.codingRate = cr;
  config.narrowGrid = narrowGrid;
  config.power = power;
  config.tcxoVoltage = tcxoVoltage;
  return(beginLRFHSS(config));
}

int16_t LR1120::setFrequency(float freq) {
  return(this->setFrequency(freq, false));
}

int16_t LR1120::setFrequency(float freq, bool skipCalibration, float band) {
  #if RADIOLIB_CHECK_PARAMS
  if(!(((freq >= 150.0f) && (freq <= 960.0f)) ||
    ((freq >= 1900.0f) && (freq <= 2200.0f)) ||
    ((freq >= 2400.0f) && (freq <= 2500.0f)))) {
      return(RADIOLIB_ERR_INVALID_FREQUENCY);
  }
  #endif

  // check if we need to recalibrate image
  int16_t state;
  if(!skipCalibration && (fabsf(freq - this->freqMHz) >= RADIOLIB_LR11X0_CAL_IMG_FREQ_TRIG_MHZ)) {
    state = LR11x0::calibrateImageRejection(freq - band, freq + band);
    RADIOLIB_ASSERT(state);
  }

  // set frequency
  state = LR11x0::setRfFrequency((uint32_t)(freq*1000000.0f));
  RADIOLIB_ASSERT(state);
  this->freqMHz = freq;
  this->highFreq = (freq > 1000.0f);

  // apply workaround for GFSK
  return(workaroundGFSK());
}

int16_t LR1120::setOutputPower(int8_t power) {
  return(this->setOutputPower(power, false));
}

int16_t LR1120::setOutputPower(int8_t power, bool forceHighPower, uint32_t rampTimeUs) {
  // check if power value is configurable
  int16_t state = this->checkOutputPower(power, NULL, forceHighPower);
  RADIOLIB_ASSERT(state);

  // determine whether to use HP or LP PA and check range accordingly
  uint8_t paSel = 0;
  uint8_t paSupply = 0;
  this->txMode = LR11x0::MODE_TX;
  if(this->highFreq) {
    paSel = 2;
    this->txMode = LR11x0::MODE_TX_HF;
  } else if(forceHighPower || (power > 14)) {
    paSel = 1;
    paSupply = 1;
    this->txMode = LR11x0::MODE_TX_HP;
  }
  
  // TODO how and when to configure OCP?

  // update PA config and set output power - always use VBAT for high-power PA
  // the value returned by LRxxxx class is offset by 3 for LR11x0
  state = LR11x0::setOutputPower(power, paSel, paSupply, 0x04, 0x07, roundRampTime(rampTimeUs) - 0x03);
  return(state);
}

int16_t LR1120::checkOutputPower(int8_t power, int8_t* clipped) {
  return(checkOutputPower(power, clipped, false));
}

int16_t LR1120::checkOutputPower(int8_t power, int8_t* clipped, bool forceHighPower) {
  if(this->highFreq) {
    if(clipped) {
      *clipped = RADIOLIB_MAX(-18, RADIOLIB_MIN(13, power));
    }
    RADIOLIB_CHECK_RANGE(power, -18, 13, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
    return(RADIOLIB_ERR_NONE);
  }

  if(forceHighPower || (power > 14)) {
    if(clipped) {
      *clipped = RADIOLIB_MAX(-9, RADIOLIB_MIN(22, power));
    }
    RADIOLIB_CHECK_RANGE(power, -9, 22, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  
  } else {
    if(clipped) {
      *clipped = RADIOLIB_MAX(-17, RADIOLIB_MIN(14, power));
    }
    RADIOLIB_CHECK_RANGE(power, -17, 14, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  
  }
  return(RADIOLIB_ERR_NONE);
}

int16_t LR1120::setModem(ModemType_t modem) {
  switch(modem) {
    case(ModemType_t::RADIOLIB_MODEM_LORA): {
      return(this->begin());
    } break;
    case(ModemType_t::RADIOLIB_MODEM_FSK): {
      return(this->beginGFSK());
    } break;
    case(ModemType_t::RADIOLIB_MODEM_LRFHSS): {
      return(this->beginLRFHSS());
    } break;
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
  return(RADIOLIB_ERR_WRONG_MODEM);
}

#endif
