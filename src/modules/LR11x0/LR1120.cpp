#include "LR1120.h"
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR11X0

LR1120::LR1120(Module* mod) : LR11x0(mod) {
  chipType = RADIOLIB_LR11X0_DEVICE_LR1120;
}

int16_t LR1120::begin(const ConfigLoRa_t& cfg) {
  // execute common part
  int16_t state = LR11x0::begin(cfg.bandwidth*1000.0f, cfg.spreadingFactor, cfg.codingRate, cfg.syncWord, cfg.preambleLength, (cfg.frequency > 1000.0f));
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency*1000000.0f);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  return(state);
}

int16_t LR1120::beginGFSK(const ConfigFSK_t& cfg) {
  // execute common part
  int16_t state = LR11x0::beginGFSK(cfg.bitRate, cfg.frequencyDeviation, cfg.receiverBandwidth, cfg.preambleLength);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency*1000000.0f);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  return(state);
}

int16_t LR1120::beginLRFHSS(const ConfigLRFHSS_t& cfg) {
  // execute common part
  int16_t state = LR11x0::beginLRFHSS(cfg.bandwidth, cfg.bandwidth, cfg.narrowGrid);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency*1000000.0f);
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
