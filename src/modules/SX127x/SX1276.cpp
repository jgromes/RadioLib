#include "SX1276.h"
#if !defined(RADIOLIB_EXCLUDE_SX127X)

SX1276::SX1276(Module* mod) : SX1278(mod) {

}

int16_t SX1276::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain) {
  // execute common part
  uint8_t versions[] = { RADIOLIB_SX1278_CHIP_VERSION, RADIOLIB_SX1278_CHIP_VERSION_ALT, RADIOLIB_SX1278_CHIP_VERSION_RFM9X };
  int16_t state = SX127x::begin(versions, 3, syncWord, preambleLength);
  RADIOLIB_ASSERT(state);

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

  // set publicly accessible settings that are not a part of begin method
  state = setCRC(true);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1276::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK) {
  // execute common part
  uint8_t versions[] = { RADIOLIB_SX1278_CHIP_VERSION, RADIOLIB_SX1278_CHIP_VERSION_ALT, RADIOLIB_SX1278_CHIP_VERSION_RFM9X };
  int16_t state = SX127x::beginFSK(versions, 3, freqDev, rxBw, preambleLength, enableOOK);
  RADIOLIB_ASSERT(state);

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

int16_t SX1276::setFrequency(float freq) {
  RADIOLIB_CHECK_RANGE(freq, 137.0, 1020.0, RADIOLIB_ERR_INVALID_FREQUENCY);

  // set frequency and if successful, save the new setting
  int16_t state = SX127x::setFrequencyRaw(freq);
  if(state == RADIOLIB_ERR_NONE) {
    SX127x::frequency = freq;
  }
  return(state);
}

#endif
