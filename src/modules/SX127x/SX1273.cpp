#include "SX1273.h"
#if !defined(RADIOLIB_EXCLUDE_SX127X)

SX1273::SX1273(Module* mod) : SX1272(mod) {

}

int16_t SX1273::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain) {
  // execute common part
  int16_t state = SX127x::begin(SX1272_CHIP_VERSION, syncWord, preambleLength);
  RADIOLIB_ASSERT(state);

  // mitigation of receiver spurious response
  // see SX1272/73 Errata, section 2.2 for details
  state = _mod->SPIsetRegValue(0x31, 0b10000000, 7, 7);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setBandwidth(bw);
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

int16_t SX1273::setSpreadingFactor(uint8_t sf) {
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
  int16_t state = setSpreadingFactorRaw(newSpreadingFactor);
  if(state == ERR_NONE) {
    SX127x::_sf = sf;
  }

  return(state);
}

#endif
