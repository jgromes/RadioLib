#include "LR1120.h"
#if !RADIOLIB_EXCLUDE_LR11X0

LR1120::LR1120(Module* mod) : LR11x0(mod) {
  chipType = RADIOLIB_LR11X0_DEVICE_LR1120;
}

int16_t LR1120::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::begin(bw, sf, cr, syncWord, preambleLength, tcxoVoltage, freq > 1000.0);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  return(state);
}

int16_t LR1120::beginGFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::beginGFSK(br, freqDev, rxBw, preambleLength, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  return(state);
}

int16_t LR1120::beginLRFHSS(float freq, uint8_t bw, uint8_t cr, int8_t power, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::beginLRFHSS(bw, cr, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  return(state);
}

int16_t LR1120::setFrequency(float freq) {
  return(this->setFrequency(freq, true));
}

int16_t LR1120::setFrequency(float freq, bool calibrate, float band) {
  if(!(((freq >= 150.0) && (freq <= 960.0)) ||
    ((freq >= 1900.0) && (freq <= 2200.0)) ||
    ((freq >= 2400.0) && (freq <= 2500.0)))) {
      return(RADIOLIB_ERR_INVALID_FREQUENCY);
  }

  // calibrate image rejection
  int16_t state;
  if(calibrate) {
    state = LR11x0::calibImage(freq - band, freq + band);
    RADIOLIB_ASSERT(state);
  }

  // set frequency
  state = LR11x0::setRfFrequency((uint32_t)(freq*1000000.0f));
  RADIOLIB_ASSERT(state);
  this->highFreq = (freq > 1000.0);
  return(RADIOLIB_ERR_NONE);
}

int16_t LR1120::setOutputPower(int8_t power) {
  return(this->setOutputPower(power, false));
}

int16_t LR1120::setOutputPower(int8_t power, bool forceHighPower) {
  // check if power value is configurable
  int16_t state = this->checkOutputPower(power, NULL, forceHighPower);
  RADIOLIB_ASSERT(state);

  // determine whether to use HP or LP PA and check range accordingly
  uint8_t paSel = 0;
  uint8_t paSupply = 0;
  if(this->highFreq) {
    paSel = 2;
  } else if(forceHighPower || (power > 14)) {
    paSel = 1;
    paSupply = 1;
  }
  
  // TODO how and when to configure OCP?

  // update PA config - always use VBAT for high-power PA
  state = setPaConfig(paSel, paSupply, 0x04, 0x07);
  RADIOLIB_ASSERT(state);

  // set output power
  state = setTxParams(power, RADIOLIB_LR11X0_PA_RAMP_48U);
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

#endif