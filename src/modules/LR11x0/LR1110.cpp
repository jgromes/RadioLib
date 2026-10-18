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
  if(!skipCalibration && (RADIOLIB_ABS(freq - this->freqHz) >= RADIOLIB_UNIT_MEGA(RADIOLIB_LR11X0_CAL_IMG_FREQ_TRIG_MHZ))) {
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
  int8_t pwr = power;
  int16_t state = this->applyOutputPowerOffset(RADIOLIB_LR1110_LP_POUT_MIN, &power, &pwr);
  RADIOLIB_ASSERT(state);

  // determine whether to use HP or LP PA and check range accordingly
  bool useHp = forceHighPower || (pwr > RADIOLIB_LR1110_LP_POUT_MAX);
  if(useHp) {
    RADIOLIB_CHECK_RANGE(pwr, RADIOLIB_LR1110_HP_POUT_MIN, RADIOLIB_LR1110_HP_POUT_MAX, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  } else {
    RADIOLIB_CHECK_RANGE(pwr, RADIOLIB_LR1110_LP_POUT_MIN, RADIOLIB_LR1110_LP_POUT_MAX, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  }
  
  // TODO how and when to configure OCP?

  // update PA config and set output power - always use VBAT for high-power PA
  // the value returned by LRxxxx class is offset by 3 for LR11x0
  this->txMode = useHp ? LR11x0::MODE_TX_HP : LR11x0::MODE_TX;
  state = LR11x0::setOutputPower(pwr, (uint8_t)useHp, (uint8_t)useHp, 0x04, 0x07, roundRampTime(rampTimeUs) - 0x03);
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