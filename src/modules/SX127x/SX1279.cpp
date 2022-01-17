#include "SX1279.h"
#if !defined(RADIOLIB_EXCLUDE_SX127X)

SX1279::SX1279(Module* mod) : SX1278(mod) {

}

int16_t SX1279::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain) {
  // execute common part
  int16_t state = SX127x::begin(RADIOLIB_SX1278_CHIP_VERSION, syncWord, preambleLength);
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

int16_t SX1279::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK) {
  // execute common part
  int16_t state = SX127x::beginFSK(RADIOLIB_SX1278_CHIP_VERSION, br, freqDev, rxBw, preambleLength, enableOOK);
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = configFSK();
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
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

int16_t SX1279::setFrequency(float freq) {
  RADIOLIB_CHECK_RANGE(freq, 137.0, 960.0, RADIOLIB_ERR_INVALID_FREQUENCY);

  // set frequency and if successful, save the new setting
  int16_t state = SX127x::setFrequencyRaw(freq);
  if(state == RADIOLIB_ERR_NONE) {
    SX127x::_freq = freq;
  }
  return(state);
}

int16_t SX1279::setFHSSHoppingPeriod(uint8_t freqHoppingPeriod) {
  return(_mod->SPIsetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD, freqHoppingPeriod));
}

uint8_t SX1279::getFHSSHoppingPeriod(void) {
  return(_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD)); 
}

uint8_t SX1279::getFHSSChannel(void) {
  return(_mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_CHANNEL, 5, 0)); 
}

void SX1279::clearFHSSInt(void) {
  int16_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    _mod->SPIwriteRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS, getIRQFlags() | RADIOLIB_SX127X_CLEAR_IRQ_FLAG_FHSS_CHANGE_CHANNEL);
  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    return; //These are not the interrupts you are looking for
  }  
}

#endif
