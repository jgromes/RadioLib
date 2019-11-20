#include "SX1261.h"

SX1261::SX1261(Module* mod) 
  : SX1262(mod) {

}

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
  if (state != ERR_NONE) {
    return(state);
  }

  // set output power
  // TODO power ramp time configuration
  state = SX126x::setTxParams(power);
  if (state != ERR_NONE) {
    return(state);
  }

  // restore OCP configuration
  return writeRegister(SX126X_REG_OCP_CONFIGURATION, &ocp, 1);
}

int16_t SX1261::setOptimalLowPowerPaConfig(int8_t* inOutPower)
{
  int16_t state;
  if (*inOutPower > 10) {
    state = SX126x::setPaConfig(0x04, SX126X_PA_CONFIG_SX1261, 0x00);
  }
  else {
    state = SX126x::setPaConfig(0x01, SX126X_PA_CONFIG_SX1261, 0x00);
    // changing the PaConfig means output power is now scaled so we get 3 dB less than requested.
    // see datasheet table 13-21 and comments in setOptimalHiPowerPaConfig.
    *inOutPower -= 3;
  }
  return state;
}
