#include "SX1262.h"
#if !defined(RADIOLIB_EXCLUDE_SX126X)

SX1262::SX1262(Module* mod) : SX126x(mod) {

}

int16_t SX1262::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  // execute common part
  int16_t state = SX126x::begin(bw, sf, cr, syncWord, preambleLength, tcxoVoltage, useRegulatorLDO);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
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

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1262::setFrequency(float freq, bool calibrate) {
  RADIOLIB_CHECK_RANGE(freq, 150.0, 960.0, ERR_INVALID_FREQUENCY);

  // calibrate image
  if(calibrate) {
    uint8_t data[2];
    if(freq > 900.0) {
      data[0] = SX126X_CAL_IMG_902_MHZ_1;
      data[1] = SX126X_CAL_IMG_902_MHZ_2;
    } else if(freq > 850.0) {
      data[0] = SX126X_CAL_IMG_863_MHZ_1;
      data[1] = SX126X_CAL_IMG_863_MHZ_2;
    } else if(freq > 770.0) {
      data[0] = SX126X_CAL_IMG_779_MHZ_1;
      data[1] = SX126X_CAL_IMG_779_MHZ_2;
    } else if(freq > 460.0) {
      data[0] = SX126X_CAL_IMG_470_MHZ_1;
      data[1] = SX126X_CAL_IMG_470_MHZ_2;
    } else {
      data[0] = SX126X_CAL_IMG_430_MHZ_1;
      data[1] = SX126X_CAL_IMG_430_MHZ_2;
    }
    int16_t state = SX126x::calibrateImage(data);
    RADIOLIB_ASSERT(state);
  }

  // set frequency
  return(SX126x::setFrequencyRaw(freq));
}

int16_t SX1262::setOutputPower(int8_t power) {
  RADIOLIB_CHECK_RANGE(power, -17, 22, ERR_INVALID_OUTPUT_POWER);

  // get current OCP configuration
  uint8_t ocp = 0;
  int16_t state = readRegister(SX126X_REG_OCP_CONFIGURATION, &ocp, 1);
  RADIOLIB_ASSERT(state);

  // set PA config
  state = SX126x::setPaConfig(0x04, SX126X_PA_CONFIG_SX1262);
  RADIOLIB_ASSERT(state);

  // set output power
  /// \todo power ramp time configuration
  state = SX126x::setTxParams(power);
  RADIOLIB_ASSERT(state);

  // restore OCP configuration
  return(writeRegister(SX126X_REG_OCP_CONFIGURATION, &ocp, 1));
}

#endif
