#include "LR1110.h"
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR11X0

LR1110::LR1110(Module* mod) : LR11x0(mod) {
  chipType = RADIOLIB_LR11X0_DEVICE_LR1110;
}

int16_t LR1110::begin(const ConfigLoRa_t& cfg) {
  // execute common part
  int16_t state = LR11x0::begin(cfg.bandwidth*1000.0f, cfg.spreadingFactor, cfg.codingRate, cfg.syncWord, cfg.preambleLength);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency*1000000.0f);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  return(state);
}

int16_t LR1110::beginGFSK(const ConfigFSK_t& cfg) {
  // execute common part
  int16_t state = LR11x0::beginGFSK(cfg.bitRate*1000.0f, cfg.frequencyDeviation*1000.0f, cfg.receiverBandwidth*1000.0f, cfg.preambleLength);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency*1000000.0f);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  return(state);
}

int16_t LR1110::beginLRFHSS(const ConfigLRFHSS_t& cfg) {
  // execute common part
  int16_t state = LR11x0::beginLRFHSS(cfg.bandwidth, cfg.bandwidth, cfg.narrowGrid);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency*1000000.0f);
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
  // check if power value is configurable
  int16_t state = this->checkOutputPower(power, NULL, forceHighPower);
  RADIOLIB_ASSERT(state);

  // determine whether to use HP or LP PA and check range accordingly
  bool useHp = forceHighPower || (power > 14);
  this->txMode = useHp ? LR11x0::MODE_TX_HP : LR11x0::MODE_TX;
  
  // TODO how and when to configure OCP?

  // update PA config and set output power - always use VBAT for high-power PA
  // the value returned by LRxxxx class is offset by 3 for LR11x0
  state = LR11x0::setOutputPower(power, (uint8_t)useHp, (uint8_t)useHp, 0x04, 0x07, roundRampTime(rampTimeUs) - 0x03);
  return(state);
}

int16_t LR1110::checkOutputPower(int8_t power, int8_t* clipped) {
  return(checkOutputPower(power, clipped, false));
}

int16_t LR1110::checkOutputPower(int8_t power, int8_t* clipped, bool forceHighPower) {
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