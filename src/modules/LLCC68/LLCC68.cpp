#include "LLCC68.h"
#if !defined(RADIOLIB_EXCLUDE_SX126X)

LLCC68::LLCC68(Module* mod) : SX1262(mod) {

}

int16_t LLCC68::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  // execute common part
  int16_t state = SX126x::begin(cr, syncWord, preambleLength, tcxoVoltage, useRegulatorLDO);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setBandwidth(bw);
  RADIOLIB_ASSERT(state);

  state = setSpreadingFactor(sf);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t LLCC68::setBandwidth(float bw) {
  RADIOLIB_CHECK_RANGE(bw, 100.0, 510.0, RADIOLIB_ERR_INVALID_BANDWIDTH);
  return(SX1262::setBandwidth(bw));
}

int16_t LLCC68::setSpreadingFactor(uint8_t sf) {
  switch(SX126x::_bw) {
    case RADIOLIB_SX126X_LORA_BW_125_0:
      RADIOLIB_CHECK_RANGE(sf, 5, 9, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
      break;
    case RADIOLIB_SX126X_LORA_BW_250_0:
      RADIOLIB_CHECK_RANGE(sf, 5, 10, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
      break;
    case RADIOLIB_SX126X_LORA_BW_500_0:
      RADIOLIB_CHECK_RANGE(sf, 5, 11, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
      break;
    default:
      return(RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
  }

  return(SX1262::setSpreadingFactor(sf));
}

#endif
