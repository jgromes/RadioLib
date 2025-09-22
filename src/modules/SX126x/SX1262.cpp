#include "SX1262.h"
#include <math.h>

#if !RADIOLIB_EXCLUDE_SX126X

SX1262::SX1262(Module* mod) : SX126x(mod) {
  chipType = RADIOLIB_SX1262_CHIP_TYPE;
}

int16_t SX1262::begin(const Configuration_t& config) {
  // execute common part
  int16_t state = SX126x::begin(
    config.codingRate, config.syncWord, config.preambleLength,
    config.tcxoVoltage, config.useRegulatorLDO);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setSpreadingFactor(config.spreadingFactor);
  RADIOLIB_ASSERT(state);

  state = setBandwidth(config.bandwidth);
  RADIOLIB_ASSERT(state);

  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

// deprecated
int16_t SX1262::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  // C++11 does not support designated initializers here, so we need to do this the old way
  Configuration_t config;
  config.frequency = freq;
  config.bandwidth = bw;
  config.spreadingFactor = sf;
  config.codingRate = cr;
  config.syncWord = syncWord;
  config.power = power;
  config.preambleLength = preambleLength;
  config.tcxoVoltage = tcxoVoltage;
  config.useRegulatorLDO = useRegulatorLDO;
  return(begin(config));
}

int16_t SX1262::beginFSK(const ConfigurationFSK_t& config) {
  // execute common part
  int16_t state = SX126x::beginFSK(
    config.bitRate, config.frequencyDeviation, config.receiverBandwidth,
    config.preambleLength, config.tcxoVoltage, config.useRegulatorLDO);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

// deprecated
int16_t SX1262::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  // C++11 does not support designated initializers here, so we need to do this the old way
  ConfigurationFSK_t config;
  config.frequency = freq;
  config.bitRate = br;
  config.frequencyDeviation = freqDev;
  config.receiverBandwidth = rxBw;
  config.power = power;
  config.preambleLength = preambleLength;
  config.tcxoVoltage = tcxoVoltage;
  config.useRegulatorLDO = useRegulatorLDO;
  return(beginFSK(config));
}

int16_t SX1262::beginLRFHSS(const ConfigurationLRFHSS_t& config) {
  // execute common part
  int16_t state = SX126x::beginLRFHSS(
    config.bandwidth, config.codingRate, config.narrowGrid,
    config.tcxoVoltage, config.useRegulatorLDO);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

// deprecated
int16_t SX1262::beginLRFHSS(float freq, uint8_t bw, uint8_t cr, bool narrowGrid, int8_t power, float tcxoVoltage, bool useRegulatorLDO) {
  // C++11 does not support designated initializers here, so we need to do this the old way
  ConfigurationLRFHSS_t config;
  config.frequency = freq;
  config.bandwidth = bw;
  config.codingRate = cr;
  config.narrowGrid = narrowGrid;
  config.power = power;
  config.tcxoVoltage = tcxoVoltage;
  config.useRegulatorLDO = useRegulatorLDO;
  return(beginLRFHSS(config));
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
  // check if power value is configurable
  int16_t state = checkOutputPower(power, NULL);
  RADIOLIB_ASSERT(state);

  // get current OCP configuration
  uint8_t ocp = 0;
  state = readRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1);
  RADIOLIB_ASSERT(state);

  // set PA config
  state = SX126x::setPaConfig(0x04, RADIOLIB_SX126X_PA_CONFIG_SX1262);
  RADIOLIB_ASSERT(state);

  // set output power with default 200us ramp
  state = SX126x::setTxParams(power, RADIOLIB_SX126X_PA_RAMP_200U);
  RADIOLIB_ASSERT(state);

  // restore OCP configuration
  return(writeRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1));
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
