#include "SX1261.h"
#if !RADIOLIB_EXCLUDE_SX126X

SX1261::SX1261(Module* mod): SX1262(mod) {
  chipType = RADIOLIB_SX1261_CHIP_TYPE;
}

int16_t SX1261::setOutputPower(int8_t power) {
  // check if power value is configurable
  int16_t state = checkOutputPower(power, NULL);
  RADIOLIB_ASSERT(state);

  // get current OCP configuration
  uint8_t ocp = 0;
  state = readRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1);
  RADIOLIB_ASSERT(state);

  // set PA config
  state = SX126x::setPaConfig(0x04, RADIOLIB_SX126X_PA_CONFIG_SX1261, 0x00);
  RADIOLIB_ASSERT(state);

  // set output power with default 200us ramp
  state = SX126x::setTxParams(power, RADIOLIB_SX126X_PA_RAMP_200U);
  RADIOLIB_ASSERT(state);

  // restore OCP configuration
  return(writeRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1));
}

int16_t SX1261::checkOutputPower(int8_t power, int8_t* clipped) {
  if(clipped) {
    *clipped = RADIOLIB_MAX(-17, RADIOLIB_MIN(14, power));
  }
  RADIOLIB_CHECK_RANGE(power, -17, 14, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  return(RADIOLIB_ERR_NONE);
}

#endif
