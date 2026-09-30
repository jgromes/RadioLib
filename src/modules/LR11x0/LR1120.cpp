#include "LR1120.h"
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR11X0

LR1120::LR1120(Module* mod) : LR11x0(mod) {
  chipType = RADIOLIB_LR11X0_DEVICE_LR1120;
  this->updatePowerLimits(false);
}

int16_t LR1120::begin(const ConfigLoRa_t& cfg) {
  // execute common part
  int16_t state = LR11x0::begin(cfg.bandwidth, cfg.spreadingFactor, cfg.codingRate, cfg.syncWord, cfg.preambleLength, (cfg.frequency > RADIOLIB_LR11X0_LF_CUTOFF_FREQ));
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  return(state);
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

int16_t LR1120::setFrequency(uint32_t freq) {
  return(this->setFrequency(freq, false));
}

int16_t LR1120::setFrequency(uint32_t freq, bool skipCalibration, uint32_t band) {
  #if RADIOLIB_CHECK_PARAMS
  if(!(((freq >= RADIOLIB_UNIT_MEGA(150)) && (freq <= RADIOLIB_UNIT_MEGA(960))) ||
    ((freq >= RADIOLIB_UNIT_MEGA(1900)) && (freq <= RADIOLIB_UNIT_MEGA(2200))) ||
    ((freq >= RADIOLIB_UNIT_MEGA(2400)) && (freq <= RADIOLIB_UNIT_MEGA(2500))))) {
      return(RADIOLIB_ERR_INVALID_FREQUENCY);
  }
  #endif

  // check if we need to recalibrate image
  int16_t state;
  if(!skipCalibration && (RADIOLIB_ABS(freq - this->freqHz) >= RADIOLIB_UNIT_MEGA(RADIOLIB_LR11X0_CAL_IMG_FREQ_TRIG_MHZ))) {
    state = LR11x0::calibrateImageRejection(freq - band, freq + band);
    RADIOLIB_ASSERT(state);
  }

  // set frequency
  state = LR11x0::setRfFrequency(freq);
  RADIOLIB_ASSERT(state);

  this->freqHz = freq;
  this->highFreq = (freq > RADIOLIB_LR11X0_LF_CUTOFF_FREQ);
  
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
  int8_t lutBase = this->highFreq ? RADIOLIB_LR112X_HF_POUT_MIN : RADIOLIB_LR112X_LP_POUT_MIN;
  int16_t state = this->applyOutputPowerOffset(lutBase, &power, &pwr);
  RADIOLIB_ASSERT(state);

  // check if power value is configurable
  bool useHp = forceHighPower || (pwr > RADIOLIB_LR112X_LP_POUT_MAX);
  if(this->highFreq) {
    RADIOLIB_CHECK_RANGE(pwr, RADIOLIB_LR112X_HF_POUT_MIN, RADIOLIB_LR112X_HF_POUT_MAX, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  } else if(useHp) {
    RADIOLIB_CHECK_RANGE(pwr, RADIOLIB_LR112X_HP_POUT_MIN, RADIOLIB_LR112X_HP_POUT_MAX, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  } else {
    RADIOLIB_CHECK_RANGE(pwr, RADIOLIB_LR112X_LP_POUT_MIN, RADIOLIB_LR112X_LP_POUT_MAX, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  }

  // determine whether to use HP or LP PA and check range accordingly
  uint8_t paSel = 0;
  uint8_t paSupply = 0;
  this->txMode = LR11x0::MODE_TX;
  if(this->highFreq) {
    paSel = 2;
    this->txMode = LR11x0::MODE_TX_HF;
  } else if(useHp) {
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
      ConfigLoRa_t cfg;
      return(this->begin(cfg));
    } break;
    case(ModemType_t::RADIOLIB_MODEM_FSK): {
      ConfigFSK_t cfg;
      return(this->beginGFSK(cfg));
    } break;
    case(ModemType_t::RADIOLIB_MODEM_LRFHSS): {
      ConfigLRFHSS_t cfg;
      return(this->beginLRFHSS(cfg));
    } break;
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
  return(RADIOLIB_ERR_WRONG_MODEM);
}

void LR1120::updatePowerLimits(bool highFreq) {
  if(highFreq) {
    this->powerMin = RADIOLIB_LR112X_HF_POUT_MIN;
    this->powerMax = RADIOLIB_LR112X_HF_POUT_MAX;
  } else {
    this->powerMin = RADIOLIB_LR112X_LP_POUT_MIN;
    this->powerMax = RADIOLIB_LR112X_HP_POUT_MAX;
  }
  this->paSteps = this->powerMax - this->powerMin + 1;
}

#endif
