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

  // set PA config for optimal consumption as described in section 13-21 of the datasheet:
  // the final column of Table 13-21 suggests that the value passed in SetTxParams is actually scaled depending on the parameters of setPaConfig. However, testing suggests this isn't the case.
  if (power > 10) {
    state = SX126x::setPaConfig(0x04, SX126X_PA_CONFIG_SX1261, 0x00);
  }
  else {
    state = SX126x::setPaConfig(0x01, SX126X_PA_CONFIG_SX1261, 0x00);
  }
  if (state != ERR_NONE) {
    return(state);
  }
  // TODO investigate if better power efficiency can be achieved using undocumented even lower settings

  // set output power
  // TODO power ramp time configuration
  state = SX126x::setTxParams(power);
  if (state != ERR_NONE) {
    return state;
  }


  // restore OCP configuration
  return(writeRegister(SX126X_REG_OCP_CONFIGURATION, &ocp, 1));
}
