#include "LR1110.h"
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR11X0

LR1110::LR1110(Module* mod) : LR11x0(mod) {
  chipType = RADIOLIB_LR11X0_DEVICE_LR1110;
  this->powerMin = RADIOLIB_LR1110_LP_POUT_MIN;
  this->powerMax = RADIOLIB_LR1110_HP_POUT_MAX;
  this->paSteps = this->powerMax - this->powerMin + 1;
}

int16_t LR1110::begin(const ConfigLoRa_t& cfg) {
  // execute common part
  int16_t state = LR11x0::begin(cfg.bandwidth, cfg.spreadingFactor, cfg.codingRate, cfg.syncWord, cfg.preambleLength);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  return(state);
}

int16_t LR1110::beginGFSK(const ConfigFSK_t& cfg) {
  // execute common part
  int16_t state = LR11x0::beginGFSK(cfg.bitRate, cfg.frequencyDeviation, cfg.receiverBandwidth, cfg.preambleLength);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  return(state);
}

int16_t LR1110::beginLRFHSS(const ConfigLRFHSS_t& cfg) {
  // execute common part
  int16_t state = LR11x0::beginLRFHSS(cfg.bandwidth, cfg.bandwidth, cfg.narrowGrid);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  return(state);
}

int16_t LR1110::setFrequency(uint32_t freq) {
  return(this->setFrequency(freq, false));
}

int16_t LR1110::setFrequency(uint32_t freq, bool skipCalibration, uint32_t band) {
  RADIOLIB_CHECK_RANGE(freq, RADIOLIB_UNIT_MEGA(150), RADIOLIB_UNIT_MEGA(960), RADIOLIB_ERR_INVALID_FREQUENCY);
  
  // check if we need to recalibrate image
  int16_t state;
  if(!skipCalibration && (RADIOLIB_ABS_UNSIGNED(freq, this->freqHz) >= RADIOLIB_UNIT_MEGA(RADIOLIB_LR11X0_CAL_IMG_FREQ_TRIG_MHZ))) {
    state = LR11x0::calibrateImageRejection(freq - band, freq + band);
    RADIOLIB_ASSERT(state);
  }

  // set frequency
  state = LR11x0::setRfFrequency(freq);
  RADIOLIB_ASSERT(state);
  this->freqHz = freq;
  return(state);
}

int16_t LR1110::setOutputPower(int8_t power) {
  return(this->setOutputPower(power, false));
}

int16_t LR1110::setOutputPower(int8_t power, bool forceHighPower, uint32_t rampTimeUs) {
  // apply offset for external PA
  // use the minimum possible output as LUT base
  int8_t pwr = power;
  int16_t state = this->applyOutputPowerOffset(RADIOLIB_LR1110_LP_POUT_MIN, &power, &pwr);
  RADIOLIB_ASSERT(state);

  // check the user did not request power output that is not possible
  const Module* mod = this->getMod();
  bool lp_supported = mod->findRfSwitchMode(LR11x0::MODE_TX);
  bool hp_supported = mod->findRfSwitchMode(LR11x0::MODE_TX_HP);

  // check whether any RF switch configuration exists at all - if standby does not exist, then consider the module switchless
  if(!mod->findRfSwitchMode(LR11x0::MODE_STBY)) {
    lp_supported = true;
    hp_supported = true;
  }

  if(((!lp_supported && (pwr < RADIOLIB_LR112X_HP_POUT_MIN)) || (!hp_supported && (pwr > RADIOLIB_LR112X_LP_POUT_MAX)) ||
    (!lp_supported && !hp_supported) || (forceHighPower && !hp_supported))) {
    // LP not supported but requested power is below HP low bound or
    // HP not supported but requested power is above LP high bound or
    // neither is supported or
    // user is forcing high power while it is not supported
    return(RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  }

  // determine which PA to use (the default is for LP)
  uint8_t paSel = 0;
  uint8_t paSupply = 0;
  this->txMode = LR11x0::MODE_TX;

  // if HP is supported, use it in the following cases:
  // 1. user forces it
  // 2. HP is the only option
  // 3. but power requested exceeds the LP range
  if(hp_supported && (forceHighPower || (!lp_supported) || (pwr > RADIOLIB_LR112X_LP_POUT_MAX))) {
    paSel = 1;
    paSupply = 1;
    this->txMode = LR11x0::MODE_TX_HP;
  }
  
  //! \TODO: [LR1110] how and when to configure OCP?

  // update PA config and set output power - always use VBAT for high-power PA
  // the value returned by LRxxxx class is offset by 3 for LR11x0
  state = LR11x0::setOutputPower(pwr, paSel, paSupply, 0x04, 0x07, roundRampTime(rampTimeUs) - 0x03);
  return(state);
}

int16_t LR1110::setModem(ModemType_t modem) {
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

#endif