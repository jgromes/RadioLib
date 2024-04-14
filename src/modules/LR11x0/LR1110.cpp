#include "LR1110.h"
#if !RADIOLIB_EXCLUDE_LR11X0

LR1110::LR1110(Module* mod) : LR11x0(mod) {
  chipType = RADIOLIB_LR11X0_HW_LR1110;
}

int16_t LR1110::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::begin(bw, sf, cr, syncWord, preambleLength, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  return(state);
}

int16_t LR1110::beginGFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::beginGFSK(br, freqDev, rxBw, preambleLength, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
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

int16_t LR1110::setOutputPower(int8_t power, bool forceHighPower) {
  // determine whether to use HP or LP PA and check range accordingly
  bool useHp = forceHighPower || (power > 14);
  if(useHp) {
    RADIOLIB_CHECK_RANGE(power, -9, 22, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
    useHp = true;
  
  } else {
    RADIOLIB_CHECK_RANGE(power, -17, 14, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
    useHp = false;
  
  }

  // TODO how and when to configure OCP?

  // update PA config - always use VBAT for high-power PA
  int16_t state = LR11x0::setPaConfig((uint8_t)useHp, (uint8_t)useHp, 0x04, 0x07);
  RADIOLIB_ASSERT(state);

  // set output power
  state = LR11x0::setTxParams(power, RADIOLIB_LR11X0_PA_RAMP_48U);
  return(state);
}

#endif