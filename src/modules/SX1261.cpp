#include "SX1261.h"

// note that this is untested (lacking the hardware) and based purely on the datasheet.
int16_t SX1261::setOutputPower(int8_t power) {  
  // check allowed power range
  if (!((power >= -17) && (power <= 14))) {
    return(ERR_INVALID_OUTPUT_POWER);
  }

  // get current OCP configuration
  uint8_t ocp = 0;
  int16_t state = readRegister(SX126X_REG_OCP_CONFIGURATION, &ocp, 1);
  if (state != ERR_NONE) {
    return(state);
  }

  state = setOptimalLowPowerPaConfig(&power);

  // set output power
  // TODO power ramp time configuration
  if (state == ERR_NONE) {
    state = SX126x::setTxParams(power);
  }

  // restore OCP configuration
  int16_t state2 = writeRegister(SX126X_REG_OCP_CONFIGURATION, &ocp, 1);

  if (state != ERR_NONE) {
    return state;
  } else {
    return state2;
  }
}

int16_t SX1261::setOptimalLowPowerPaConfig(int8_t* inOutPower)
{
  int16_t state;
  if (*inOutPower > 10) {
    state = SX126x::setPaConfig(0x04, SX126X_PA_CONFIG_SX1261, 0x00);
  }
  else {
    state = SX126x::setPaConfig(0x01, SX126X_PA_CONFIG_SX1261, 0x00);
    *inOutPower -= 4;
  }
  return state;
}
