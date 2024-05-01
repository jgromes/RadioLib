#include "SX1268.h"
#if !RADIOLIB_EXCLUDE_SX126X

SX1268::SX1268(Module* mod) : SX126x(mod) {
  chipType = RADIOLIB_SX1268_CHIP_TYPE;
}

int16_t SX1268::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  // execute common part
  int16_t state = SX126x::begin(cr, syncWord, preambleLength, tcxoVoltage, useRegulatorLDO);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setSpreadingFactor(sf);
  RADIOLIB_ASSERT(state);

  state = setBandwidth(bw);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1268::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
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

int16_t SX1268::setFrequency(float freq) {
  return(setFrequency(freq, true));
}

/// \todo integers only (all modules - frequency, data rate, bandwidth etc.)
int16_t SX1268::setFrequency(float freq, bool calibrate) {
  RADIOLIB_CHECK_RANGE(freq, 410.0, 810.0, RADIOLIB_ERR_INVALID_FREQUENCY);

  // calibrate image rejection
  if(calibrate) {
    uint8_t data[2] = { 0, 0 };

    // try to match the frequency ranges
    int freqBand = (int)freq;
    if((freqBand >= 779) && (freqBand <= 787)) {
      data[0] = RADIOLIB_SX126X_CAL_IMG_779_MHZ_1;
      data[1] = RADIOLIB_SX126X_CAL_IMG_779_MHZ_2;
    } else if((freqBand >= 470) && (freqBand <= 510)) {
      data[0] = RADIOLIB_SX126X_CAL_IMG_470_MHZ_1;
      data[1] = RADIOLIB_SX126X_CAL_IMG_470_MHZ_2;
    } else if((freqBand >= 430) && (freqBand <= 440)) {
      data[0] = RADIOLIB_SX126X_CAL_IMG_430_MHZ_1;
      data[1] = RADIOLIB_SX126X_CAL_IMG_430_MHZ_2;
    }

    int16_t state;
    if(data[0]) {
      // matched with predefined ranges, do the calibration
      state = SX126x::calibrateImage(data);
    
    } else {
      // if nothing matched, try custom calibration - the may or may not work
      RADIOLIB_DEBUG_BASIC_PRINTLN("Failed to match predefined frequency range, trying custom");
      state = SX126x::calibrateImageRejection(freq - 4.0f, freq + 4.0f);
    
    }
    
    RADIOLIB_ASSERT(state);

  }

  // set frequency
  return(SX126x::setFrequencyRaw(freq));
}

int16_t SX1268::setOutputPower(int8_t power) {
  // check if power value is configurable
  int16_t state = checkOutputPower(power, NULL);
  RADIOLIB_ASSERT(state);

  // get current OCP configuration
  uint8_t ocp = 0;
  state = readRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1);
  RADIOLIB_ASSERT(state);

  // set PA config
  state = SX126x::setPaConfig(0x04, RADIOLIB_SX126X_PA_CONFIG_SX1268);
  RADIOLIB_ASSERT(state);

  // set output power with default 200us ramp
  state = SX126x::setTxParams(power, RADIOLIB_SX126X_PA_RAMP_200U);
  RADIOLIB_ASSERT(state);

  // restore OCP configuration
  return(writeRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1));
}

int16_t SX1268::checkOutputPower(int8_t power, int8_t* clipped) {
  if(clipped) {
    *clipped = RADIOLIB_MAX(-9, RADIOLIB_MIN(22, power));
  }
  RADIOLIB_CHECK_RANGE(power, -9, 22, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  return(RADIOLIB_ERR_NONE);
}

#endif
