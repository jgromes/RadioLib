#include "LR1120.h"
#if !RADIOLIB_EXCLUDE_LR11X0

LR1120::LR1120(Module* mod) : LR11x0(mod) {
  chipType = RADIOLIB_LR11X0_DEVICE_LR1120;
}

int16_t LR1120::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::begin(bw, sf, cr, syncWord, power, preambleLength, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  return(state);
}

int16_t LR1120::beginGFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::beginGFSK(br, freqDev, rxBw, power, preambleLength, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  return(state);
}

int16_t LR1120::beginLRFHSS(float freq, uint8_t bw, uint8_t cr, int8_t power, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::beginLRFHSS(bw, cr, power, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
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
  if(calibrate) {
    int16_t state = LR11x0::calibImage(freq - band, freq + band);
    RADIOLIB_ASSERT(state);
  }

  // set frequency
  return(LR11x0::setRfFrequency((uint32_t)(freq*1000000.0f)));
}

#endif