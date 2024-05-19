#include "LR1110.h"
#if !RADIOLIB_EXCLUDE_LR11X0

LR1110::LR1110(Module* mod) : LR11x0(mod) {
  chipType = RADIOLIB_LR11X0_DEVICE_LR1110;
}

int16_t LR1110::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::begin(bw, sf, cr, syncWord, power, preambleLength, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  return(state);
}

int16_t LR1110::beginGFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::beginGFSK(br, freqDev, rxBw, power, preambleLength, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  return(state);
}

int16_t LR1110::beginLRFHSS(float freq, uint8_t bw, uint8_t cr, int8_t power, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::beginLRFHSS(bw, cr, power, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  return(state);
}

int16_t LR1110::setFrequency(float freq) {
  return(this->setFrequency(freq, true));
}

int16_t LR1110::setFrequency(float freq, bool calibrate, float band) {
  RADIOLIB_CHECK_RANGE(freq, 150.0, 960.0, RADIOLIB_ERR_INVALID_FREQUENCY);

  // calibrate image rejection
  if(calibrate) {
    int16_t state = LR11x0::calibImage(freq - band, freq + band);
    RADIOLIB_ASSERT(state);
  }

  // set frequency
  return(LR11x0::setRfFrequency((uint32_t)(freq*1000000.0f)));
}

#endif