#include "SX1262.h"
#include <math.h>

#if !RADIOLIB_EXCLUDE_SX126X

SX1262::SX1262(Module* mod) : SX126x(mod) {
  chipType = RADIOLIB_SX1262_CHIP_TYPE;
}

int16_t SX1262::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  // execute common part
  int16_t state = SX126x::begin(cr, syncWord, preambleLength, tcxoVoltage, useRegulatorLDO);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setSpreadingFactor(sf);
  RADIOLIB_ASSERT(state);

  state = setBandwidth(bw);
  RADIOLIB_ASSERT(state);

  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1262::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  // execute common part
  int16_t state = SX126x::beginFSK(br, freqDev, rxBw, preambleLength, tcxoVoltage, useRegulatorLDO);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1262::beginBPSK(float freq, float br, int8_t power, float tcxoVoltage, bool useRegulatorLDO) {
  // execute common part
  int16_t state = SX126x::beginBPSK(br, tcxoVoltage, useRegulatorLDO);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1262::beginLRFHSS(float freq, uint8_t bw, uint8_t cr, bool narrowGrid, int8_t power, float tcxoVoltage, bool useRegulatorLDO) {
  // execute common part
  int16_t state = SX126x::beginLRFHSS(bw, cr, narrowGrid, tcxoVoltage, useRegulatorLDO);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1262::setFrequency(float freq) {
  return(setFrequency(freq, false));
}

int16_t SX1262::setFrequency(float freq, bool skipCalibration) {
  RADIOLIB_CHECK_RANGE(freq, 150.0f, 960.0f, RADIOLIB_ERR_INVALID_FREQUENCY);

  // check if we need to recalibrate image
  if(!skipCalibration && (fabsf(freq - this->freqMHz) >= RADIOLIB_SX126X_CAL_IMG_FREQ_TRIG_MHZ)) {
    int16_t state = this->calibrateImage(freq);
    RADIOLIB_ASSERT(state);
  }

  // set frequency
  return(SX126x::setFrequencyRaw(freq));
}

int16_t SX1262::setOutputPower(int8_t power) {
  return(setOutputPower(power, true));
}

int16_t SX1262::setOutputPower(int8_t power, bool optimize) {
  // check if power value is configurable
  int16_t state = checkOutputPower(power, NULL);
  RADIOLIB_ASSERT(state);

  // set PA config
  int8_t paVal = optimize ? this->paOptTable[power + 9].paVal : power;
  uint8_t paDutyCycle = optimize ? this->paOptTable[power + 9].paDutyCycle : 0x04;
  uint8_t hpMax = optimize ? this->paOptTable[power + 9].hpMax : 0x07;
  return(SX126x::setOutputPower(paVal, paDutyCycle, hpMax, RADIOLIB_SX126X_PA_CONFIG_SX1262));
}

int16_t SX1262::checkOutputPower(int8_t power, int8_t* clipped) {
  if(clipped) {
    *clipped = RADIOLIB_MAX(-9, RADIOLIB_MIN(22, power));
  }
  RADIOLIB_CHECK_RANGE(power, -9, 22, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  return(RADIOLIB_ERR_NONE);
}

int16_t SX1262::setModem(ModemType_t modem) {
  switch(modem) {
    case(ModemType_t::RADIOLIB_MODEM_LORA): {
      return(this->begin());
    } break;
    case(ModemType_t::RADIOLIB_MODEM_FSK): {
      return(this->beginFSK());
    } break;
    case(ModemType_t::RADIOLIB_MODEM_LRFHSS): {
      return(this->beginLRFHSS());
    } break;
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
}

#endif
