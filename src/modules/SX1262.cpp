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

int16_t SX1262::setFrequency(float freq) {
  // check frequency range
  if((freq < 150.0) || (freq > 960.0)) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  // set frequency
  return(SX126x::setFrequencyRaw(freq));
}

int16_t SX1262::setOutputPower(int8_t power) {
  // check allowed power range
  if(!((power >= -17) && (power <= 22))) {
    return(ERR_INVALID_OUTPUT_POWER);
  }
  
  // enable high power PA for output power higher than 14 dBm
  if(power > 14) {
    SX126x::setPaConfig(0x04, SX126X_PA_CONFIG_SX1262);
  } else {
    SX126x::setPaConfig(0x04, SX126X_PA_CONFIG_SX1261);
  }
  
  // set output power
  // TODO power ramp time configuration
  SX126x::setTxParams(power);
  return(ERR_NONE);
}
    
