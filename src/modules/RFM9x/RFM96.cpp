#include "RFM96.h"

RFM96::RFM96(Module* mod) : SX1278(mod) {

}

int16_t RFM96::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint8_t currentLimit, uint16_t preambleLength, uint8_t gain) {
  // execute common part
  int16_t state = SX127x::begin(RFM9X_CHIP_VERSION, syncWord, currentLimit, preambleLength);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // configure settings not accessible by API
  state = config();
  if(state != ERR_NONE) {
    return(state);
  }
  
  // configure publicly accessible settings
  state = setFrequency(freq);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setBandwidth(bw);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setSpreadingFactor(sf);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setCodingRate(cr);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setOutputPower(power);
  if(state != ERR_NONE) {
    return(state);
  }
  
  state = setGain(gain);
  if(state != ERR_NONE) {
    return(state);
  }
  
  return(state);
}

int16_t RFM96::setFrequency(float freq) {
  // check frequency range
  if((freq < 433.0) || (freq > 470.0)) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  // set frequency
  return(SX127x::setFrequencyRaw(freq));
}
