#include "SX1262.h"
#include <math.h>

#if !RADIOLIB_EXCLUDE_SX126X

// this is a lookup table for optimized PA configuration
// it was determined by testing in https://github.com/jgromes/RadioLib/issues/1628
// see also https://github.com/radiolib-org/power-tests
static const SX126x::paTableEntry_t paOptimizedTable[RADIOLIB_SX126X_PA_TABLE_LEN] = {
  { .paDutyCycle = 2, .hpMax = 2, .paVal = -5 },
  { .paDutyCycle = 2, .hpMax = 1, .paVal = 0 },
  { .paDutyCycle = 1, .hpMax = 1, .paVal = 3 },
  { .paDutyCycle = 1, .hpMax = 2, .paVal = 0 },
  { .paDutyCycle = 1, .hpMax = 1, .paVal = 6 },
  { .paDutyCycle = 1, .hpMax = 2, .paVal = 3 },
  { .paDutyCycle = 2, .hpMax = 2, .paVal = 2 },
  { .paDutyCycle = 4, .hpMax = 1, .paVal = 6 },
  { .paDutyCycle = 1, .hpMax = 1, .paVal = 11 },
  { .paDutyCycle = 2, .hpMax = 1, .paVal = 11 },
  { .paDutyCycle = 1, .hpMax = 1, .paVal = 14 },
  { .paDutyCycle = 2, .hpMax = 1, .paVal = 14 },
  { .paDutyCycle = 1, .hpMax = 1, .paVal = 20 },
  { .paDutyCycle = 1, .hpMax = 1, .paVal = 22 },
  { .paDutyCycle = 2, .hpMax = 2, .paVal = 11 },
  { .paDutyCycle = 3, .hpMax = 1, .paVal = 21 },
  { .paDutyCycle = 1, .hpMax = 2, .paVal = 17 },
  { .paDutyCycle = 4, .hpMax = 2, .paVal = 13 },
  { .paDutyCycle = 1, .hpMax = 2, .paVal = 20 },
  { .paDutyCycle = 1, .hpMax = 2, .paVal = 22 },
  { .paDutyCycle = 2, .hpMax = 2, .paVal = 21 },
  { .paDutyCycle = 3, .hpMax = 2, .paVal = 21 },
  { .paDutyCycle = 1, .hpMax = 4, .paVal = 19 },
  { .paDutyCycle = 1, .hpMax = 4, .paVal = 20 },
  { .paDutyCycle = 3, .hpMax = 3, .paVal = 20 },
  { .paDutyCycle = 2, .hpMax = 5, .paVal = 19 },
  { .paDutyCycle = 1, .hpMax = 6, .paVal = 22 },
  { .paDutyCycle = 2, .hpMax = 5, .paVal = 22 },
  { .paDutyCycle = 3, .hpMax = 5, .paVal = 22 },
  { .paDutyCycle = 3, .hpMax = 6, .paVal = 22 },
  { .paDutyCycle = 4, .hpMax = 6, .paVal = 22 },
  { .paDutyCycle = 4, .hpMax = 7, .paVal = 22 },
};

SX1262::SX1262(Module* mod) : SX126x(mod) {
  chipType = RADIOLIB_SX1262_CHIP_TYPE;
}

int16_t SX1262::begin(const ConfigLoRa_t& cfg) {
  int16_t state = SX126x::begin(cfg.codingRate, cfg.syncWord, cfg.preambleLength);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setSpreadingFactor(cfg.spreadingFactor);
  RADIOLIB_ASSERT(state);

  state = setBandwidth(cfg.bandwidth);
  RADIOLIB_ASSERT(state);

  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1262::beginFSK(const ConfigFSK_t& cfg) {
  // execute common part
  int16_t state = SX126x::beginFSK(cfg.bitRate, cfg.frequencyDeviation, cfg.receiverBandwidth, cfg.preambleLength);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1262::beginBPSK(const ConfigBPSK_t& cfg) {
  // execute common part
  int16_t state = SX126x::beginBPSK(cfg.bitRate);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1262::beginLRFHSS(const ConfigLRFHSS_t& cfg) {
  // execute common part
  int16_t state = SX126x::beginLRFHSS(cfg.bandwidth, cfg.codingRate, cfg.narrowGrid);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1262::setFrequency(uint32_t freq) {
  return(setFrequency(freq, false));
}

int16_t SX1262::setFrequency(uint32_t freq, bool skipCalibration) {
  RADIOLIB_CHECK_RANGE(freq, RADIOLIB_UNIT_MEGA(150), RADIOLIB_UNIT_MEGA(960), RADIOLIB_ERR_INVALID_FREQUENCY);

  // check if we need to recalibrate image
  if(!skipCalibration && (RADIOLIB_ABS(freq - this->freqHz) >= RADIOLIB_SX126X_CAL_IMG_FREQ_TRIG_MHZ)) {
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
  SX126x::paTableEntry_t* paTable = this->paOptTable ? this->paOptTable : const_cast<SX126x::paTableEntry_t*>(paOptimizedTable);
  int8_t paVal = optimize ? paTable[power + 9].paVal : power;
  uint8_t paDutyCycle = optimize ? paTable[power + 9].paDutyCycle : 0x04;
  uint8_t hpMax = optimize ? paTable[power + 9].hpMax : 0x07;
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
      ConfigLoRa_t cfg;
      return(this->begin(cfg));
    } break;
    case(ModemType_t::RADIOLIB_MODEM_FSK): {
      ConfigFSK_t cfg;
      return(this->beginFSK(cfg));
    } break;
    case(ModemType_t::RADIOLIB_MODEM_LRFHSS): {
      ConfigLRFHSS_t cfg;
      return(this->beginLRFHSS(cfg));
    } break;
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
}

#endif
