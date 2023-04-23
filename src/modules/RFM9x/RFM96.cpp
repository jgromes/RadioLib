#include "RFM96.h"
#if !defined(RADIOLIB_EXCLUDE_RFM9X)

RFM96::RFM96(Module* mod) : SX1278(mod) {

}

int16_t RFM96::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain) {
  // execute common part
  int16_t state = SX127x::begin(RADIOLIB_RFM9X_CHIP_VERSION_OFFICIAL, syncWord, preambleLength);
  if(state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
    // SX127X_REG_VERSION might be set 0x12
    state = SX127x::begin(RADIOLIB_RFM9X_CHIP_VERSION_UNOFFICIAL, syncWord, preambleLength);
    RADIOLIB_ASSERT(state);
  } else if(state != RADIOLIB_ERR_NONE) {
    // some other error
    return(state);
  }
  RADIOLIB_DEBUG_PRINTLN("M\tSX1278");
  RADIOLIB_DEBUG_PRINTLN("M\tRFM96");

  // configure publicly accessible settings
  state = setBandwidth(bw);
  RADIOLIB_ASSERT(state);

  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setSpreadingFactor(sf);
  RADIOLIB_ASSERT(state);

  state = setCodingRate(cr);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = setGain(gain);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t RFM96::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK) {
  // execute common part
  int16_t state = SX127x::beginFSK(RADIOLIB_RFM9X_CHIP_VERSION_OFFICIAL, freqDev, rxBw, preambleLength, enableOOK);
  if(state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
    // SX127X_REG_VERSION might be set 0x12
    state = SX127x::beginFSK(RADIOLIB_RFM9X_CHIP_VERSION_UNOFFICIAL, freqDev, rxBw, preambleLength, enableOOK);
    RADIOLIB_ASSERT(state);
  } else if(state != RADIOLIB_ERR_NONE) {
    // some other error
    return(state);
  }
  RADIOLIB_DEBUG_PRINTLN("M\tSX1278");
  RADIOLIB_DEBUG_PRINTLN("M\tRFM96");

  // configure settings not accessible by API
  state = configFSK();
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  if(enableOOK) {
    state = setDataShapingOOK(RADIOLIB_SHAPING_NONE);
    RADIOLIB_ASSERT(state);
  } else {
    state = setDataShaping(RADIOLIB_SHAPING_NONE);
    RADIOLIB_ASSERT(state);
  }

  return(state);
}

int16_t RFM96::setFrequency(float freq) {
  RADIOLIB_CHECK_RANGE(freq, 410.0, 525.0, RADIOLIB_ERR_INVALID_FREQUENCY);

  // set frequency and if successful, save the new setting
  int16_t state = SX127x::setFrequencyRaw(freq);
  if(state == RADIOLIB_ERR_NONE) {
    SX127x::frequency = freq;
  }
  return(state);
}

#endif
