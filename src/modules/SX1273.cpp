#include "SX1273.h"

SX1273::SX1273(Module* mod) : SX1272(mod) {
  
}

uint8_t SX1273::config(uint32_t bw, uint8_t sf, uint8_t cr, float freq, uint8_t syncWord) {
  uint8_t status = ERR_NONE;
  uint8_t newBandwidth, newSpreadingFactor, newCodingRate;
  
  // check the supplied BW, CR and SF values
  switch(bw) {
    case 125000:
      newBandwidth = SX1272_BW_125_00_KHZ;
      break;
    case 250000:
      newBandwidth = SX1272_BW_250_00_KHZ;
      break;
    case 500000:
      newBandwidth = SX1272_BW_500_00_KHZ;
      break;
    default:
      return(ERR_INVALID_BANDWIDTH);
  }
  
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
  
  switch(cr) {
    case 5:
      newCodingRate = SX1272_CR_4_5;
      break;
    case 6:
      newCodingRate = SX1272_CR_4_6;
      break;
    case 7:
      newCodingRate = SX1272_CR_4_7;
      break;
    case 8:
      newCodingRate = SX1272_CR_4_8;
      break;
    default:
      return(ERR_INVALID_CODING_RATE);
  }
  
  if((freq < 860.0) || (freq > 1020.0)) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  // execute common part
  status = configCommon(newBandwidth, newSpreadingFactor, newCodingRate, freq, syncWord);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // configuration successful, save the new settings
  _bw = bw;
  _sf = sf;
  _cr = cr;
  
  return(ERR_NONE);
}
