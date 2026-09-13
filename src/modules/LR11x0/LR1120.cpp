#include "LR1120.h"
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR11X0

LR1120::LR1120(Module* mod) : LR11x0(mod) {
  chipType = RADIOLIB_LR11X0_DEVICE_LR1120;
  this->updatePowerLimits(false);
}

int16_t LR1120::begin(const ConfigLoRa_t& cfg) {
  // execute common part
  int16_t state = LR11x0::begin(cfg.bandwidth, cfg.spreadingFactor, cfg.codingRate, cfg.syncWord, cfg.preambleLength, (cfg.frequency > 1000.0f));
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  return(state);
}

int16_t LR1120::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  ConfigLoRa_t cfg;
  cfg.frequency = freq;
  cfg.bandwidth = bw;
  cfg.spreadingFactor = sf;
  cfg.codingRate = cr;
  cfg.syncWord = syncWord;
  cfg.power = power;
  cfg.preambleLength = preambleLength;
  this->tcxoVoltage = tcxoVoltage;
  return(begin(cfg));
}

int16_t LR1120::beginGFSK(const ConfigFSK_t& cfg) {
  // execute common part
  int16_t state = LR11x0::beginGFSK(cfg.bitRate, cfg.frequencyDeviation, cfg.receiverBandwidth, cfg.preambleLength);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  return(state);
}

int16_t LR1120::beginGFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  ConfigFSK_t cfg;
  cfg.frequency = freq;
  cfg.bitRate = br;
  cfg.frequencyDeviation = freqDev;
  cfg.receiverBandwidth = rxBw;
  cfg.power = power;
  cfg.preambleLength = preambleLength;
  this->tcxoVoltage = tcxoVoltage;
  return(beginGFSK(cfg));
}

int16_t LR1120::beginLRFHSS(const ConfigLRFHSS_t& cfg) {
  // execute common part
  int16_t state = LR11x0::beginLRFHSS(cfg.bandwidth, cfg.bandwidth, cfg.narrowGrid);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  return(state);
}

int16_t LR1120::beginLRFHSS(float freq, uint8_t bw, uint8_t cr, bool narrowGrid, int8_t power, float tcxoVoltage) {
  ConfigLRFHSS_t cfg;
  cfg.frequency = freq;
  cfg.bandwidth = bw;
  cfg.codingRate = cr;
  cfg.narrowGrid = narrowGrid;
  cfg.power = power;
  this->tcxoVoltage = tcxoVoltage;
  return(beginLRFHSS(cfg));
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
  
  // power limits depend on the frequency band
  this->updatePowerLimits(this->highFreq);

  // apply workaround for GFSK
  return(workaroundGFSK());
}

int16_t LR1120::setOutputPower(int8_t power) {
  return(this->setOutputPower(power, false));
}

int16_t LR1120::setOutputPower(int8_t power, bool forceHighPower, uint32_t rampTimeUs) {
  // apply offset for external PA
  int8_t pwr = power;
  int8_t lutBase = this->highFreq ? -18 : -17;
  int16_t state = this->applyOutputPowerOffset(lutBase, &power, &pwr);
  RADIOLIB_ASSERT(state);

  // check if power value is configurable
  if(this->highFreq) {
    RADIOLIB_CHECK_RANGE(pwr, -18, 13, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  } else if(forceHighPower || (pwr > 14)) {
    RADIOLIB_CHECK_RANGE(pwr, -9, 22, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  } else {
    RADIOLIB_CHECK_RANGE(pwr, -17, 14, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  }

  // determine whether to use HP or LP PA and check range accordingly
  uint8_t paSel = 0;
  uint8_t paSupply = 0;
  this->txMode = LR11x0::MODE_TX;
  if(this->highFreq) {
    paSel = 2;
    this->txMode = LR11x0::MODE_TX_HF;
  } else if(forceHighPower || (pwr > 14)) {
    paSel = 1;
    paSupply = 1;
    this->txMode = LR11x0::MODE_TX_HP;
  }
  
  // TODO how and when to configure OCP?

  // update PA config and set output power - always use VBAT for high-power PA
  // the value returned by LRxxxx class is offset by 3 for LR11x0
  state = LR11x0::setOutputPower(pwr, paSel, paSupply, 0x04, 0x07, roundRampTime(rampTimeUs) - 0x03);
  return(state);
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

void LR1120::updatePowerLimits(bool highFreq) {
  if(highFreq) {
    this->powerMin = -18;
    this->powerMax = 13;
  } else {
    this->powerMin = -17;
    this->powerMax = 22;
  }
  this->paSteps = this->powerMax - this->powerMin + 1;
}

#endif
