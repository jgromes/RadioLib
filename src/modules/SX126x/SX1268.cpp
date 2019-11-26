#include "SX1268.h"

SX1268::SX1268(Module* mod) : SX126x(mod) {

}

int16_t SX1268::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint16_t syncWord, int8_t power, float currentLimit, uint16_t preambleLength, float tcxoVoltage) {
  // execute common part
  int16_t state = SX126x::begin(bw, sf, cr, syncWord, currentLimit, preambleLength, tcxoVoltage);
  if(state != ERR_NONE) {
    return(state);
  }

  // configure publicly accessible settings
  state = setFrequency(freq);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setOutputPower(power);
  if(state != ERR_NONE) {
    return(state);
  }

  state = SX126x::fixPaClamping();
  if (state != ERR_NONE) {
    return state;
  }

  return(state);
}
int16_t SX1268::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, float currentLimit, uint16_t preambleLength, float dataShaping, float tcxoVoltage) {
  // execute common part
  int16_t state = SX126x::beginFSK(br, freqDev, rxBw, currentLimit, preambleLength, dataShaping, tcxoVoltage);
  if(state != ERR_NONE) {
    return(state);
  }

  // configure publicly accessible settings
  state = setFrequency(freq);
  if(state != ERR_NONE) {
    return(state);
  }

  state = setOutputPower(power);
  if(state != ERR_NONE) {
    return(state);
  }

  state = SX126x::fixPaClamping();
  if (state != ERR_NONE) {
    return state;
  }

  return(state);
}

int16_t SX1268::setFrequency(float freq, bool calibrate) {
  // check frequency range
  if((freq < 410.0) || (freq > 810.0)) {
    return(ERR_INVALID_FREQUENCY);
  }

  int16_t state = ERR_NONE;

  // calibrate image
  if(calibrate) {
    uint8_t data[2];
    if(freq > 770.0) {
      data[0] = SX126X_CAL_IMG_779_MHZ_1;
      data[1] = SX126X_CAL_IMG_779_MHZ_2;
    } else if(freq > 460.0) {
      data[0] = SX126X_CAL_IMG_470_MHZ_1;
      data[1] = SX126X_CAL_IMG_470_MHZ_2;
    } else {
      data[0] = SX126X_CAL_IMG_430_MHZ_1;
      data[1] = SX126X_CAL_IMG_430_MHZ_2;
    }
    state = SX126x::calibrateImage(data);
    if(state != ERR_NONE) {
      return(state);
    }
  }

  // set frequency
  return(SX126x::setFrequencyRaw(freq));
}

int16_t SX1268::setOutputPower(int8_t power) {
  // check allowed power range
  if(!((power >= -9) && (power <= 22))) {
    return(ERR_INVALID_OUTPUT_POWER);
  }

  // get current OCP configuration
  uint8_t ocp = 0;
  int16_t state = readRegister(SX126X_REG_OCP_CONFIGURATION, &ocp, 1);
  if(state != ERR_NONE) {
    return(state);
  }

  // enable optimal PA - this changes the value of power.
  state = SX126x::setOptimalHiPowerPaConfig(&power);
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
