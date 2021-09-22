#include "SX1277.h"
#if !defined(RADIOLIB_EXCLUDE_SX127X)

SX1277::SX1277(Module* mod) : SX1278(mod) {

}

int16_t SX1277::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain) {
  // execute common part
  int16_t state = SX127x::begin(SX1278_CHIP_VERSION, syncWord, preambleLength);
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

  return(state);
}

int16_t SX1277::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK) {
  // execute common part
  int16_t state = SX127x::beginFSK(SX1278_CHIP_VERSION, br, freqDev, rxBw, preambleLength, enableOOK);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX1277::setFrequency(float freq) {
  RADIOLIB_CHECK_RANGE(freq, 137.0, 1020.0, ERR_INVALID_FREQUENCY);

  // set frequency and if successful, save the new setting
  int16_t state = SX127x::setFrequencyRaw(freq);
  if(state == ERR_NONE) {
    SX127x::_freq = freq;
  }
  return(state);
}

int16_t SX1277::setSpreadingFactor(uint8_t sf) {
  uint8_t newSpreadingFactor;

  // check allowed spreading factor values
  switch(sf) {
    case 6:
      newSpreadingFactor = SX127X_SF_6;
      break;
    case 7:
      newSpreadingFactor = SX127X_SF_7;
      break;
    case 8:
      newSpreadingFactor = SX127X_SF_8;
      break;
    case 9:
      newSpreadingFactor = SX127X_SF_9;
      break;
    default:
      return(ERR_INVALID_SPREADING_FACTOR);
  }

  // set spreading factor and if successful, save the new setting
  int16_t state = SX1278::setSpreadingFactorRaw(newSpreadingFactor);
  if(state == ERR_NONE) {
    SX127x::_sf = sf;
  }

  return(state);
}

#endif
