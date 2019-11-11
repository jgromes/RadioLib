#include "SX1262.h"

SX1262::SX1262(Module* mod) : SX126x(mod) {

}

int16_t SX1262::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint16_t syncWord, int8_t power, float currentLimit, uint16_t preambleLength) {
  // execute common part
  int16_t state = SX126x::begin(bw, sf, cr, syncWord, currentLimit, preambleLength);
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

  return(state);
}

int16_t SX1262::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, float currentLimit, uint16_t preambleLength, float dataShaping) {
  // execute common part
  int16_t state = SX126x::beginFSK(br, freqDev, rxBw, currentLimit, preambleLength, dataShaping);
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

  return(state);
}

int16_t SX1262::setFrequency(float freq, bool calibrate) {
  // check frequency range
  if((freq < 150.0) || (freq > 960.0)) {
    return(ERR_INVALID_FREQUENCY);
  }

  int16_t state = ERR_NONE;

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
    state = SX126x::calibrateImage(data);
    if(state != ERR_NONE) {
      return(state);
    }
  }

  // set frequency
  return(SX126x::setFrequencyRaw(freq));
}

int16_t SX1262::setOutputPower(int8_t power) {
  // check allowed power range
  if (!((power >= -17) && (power <= 22))) {
    return(ERR_INVALID_OUTPUT_POWER);
  }

  // get current OCP configuration
  uint8_t ocp = 0;
  int16_t state = readRegister(SX126X_REG_OCP_CONFIGURATION, &ocp, 1);
  if (state != ERR_NONE) {
    return(state);
  }

  int8_t scaledPower;
  // set PA config for optimal consumption as described in section 13-21 of the datasheet:
  // the final column of Table 13-21 suggests that the value passed in SetTxParams 
  // is actually scaled depending on the parameters of setPaConfig.
  // Testing confirms this is approximately right
  if (power >= 21) {
    state = SX126x::setPaConfig(0x04, SX126X_PA_CONFIG_SX1262, SX126X_PA_CONFIG_HP_MAX/*0x07*/);
    scaledPower = power;
  }
  else if (power >= 18) {
    state = SX126x::setPaConfig(0x03, SX126X_PA_CONFIG_SX1262, 0x05);
    scaledPower = power + 2;
  }
  else if (power >= 15) {
    state = SX126x::setPaConfig(0x02, SX126X_PA_CONFIG_SX1262, 0x03);
    scaledPower = power + 5;
  }
  else {
    state = SX126x::setPaConfig(0x02, SX126X_PA_CONFIG_SX1262, 0x02);
    scaledPower = power + 8;
  }
  if (state != ERR_NONE) {
    return(state);
  }
  // TODO investigate if better power efficiency can be achieved using undocumented even lower settings

  // note that we set SX126X_PA_CONFIG_SX1262 for all power levels - setting SX126X_PA_CONFIG_SX1261 causes no output (on the nameless module I have).

  // set output power
  // TODO power ramp time configuration
  state = SX126x::setTxParams(scaledPower);
  if (state != ERR_NONE) {
    return state;
  }


  // restore OCP configuration
  return(writeRegister(SX126X_REG_OCP_CONFIGURATION, &ocp, 1));
}
