#include "Si4430.h"

#if !RADIOLIB_EXCLUDE_SI443X

Si4430::Si4430(Module* mod) : Si4432(mod) {

}

int16_t Si4430::begin(const Si443x::ConfigFSK_t& config) {
  // execute common part
  int16_t state = Si443x::begin(config.bitRate, config.frequencyDeviation, config.receiverBandwidth, config.preambleLength);
  RADIOLIB_ASSERT(state);
  RADIOLIB_DEBUG_BASIC_PRINTLN("M\tSi4432");

  // configure publicly accessible settings
  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  return(state);
}

int16_t Si4430::begin(float freq, float br, float freqDev, float rxBw, int8_t power, uint8_t preambleLen) {
  Si443x::ConfigFSK_t config;
  config.frequency = freq;
  config.bitRate = br;
  config.frequencyDeviation = freqDev;
  config.receiverBandwidth = rxBw;
  config.power = power;
  config.preambleLength = preambleLen;
  return(this->begin(config));
}

int16_t Si4430::setFrequency(float freq) {
  RADIOLIB_CHECK_RANGE(freq, 900.0f, 960.0f, RADIOLIB_ERR_INVALID_FREQUENCY);

  // set frequency
  return(Si443x::setFrequencyRaw(freq));
}

int16_t Si4430::setOutputPower(int8_t power) {
  RADIOLIB_CHECK_RANGE(power, -8, 13, RADIOLIB_ERR_INVALID_OUTPUT_POWER);

  // set output power
  Module* mod = this->getMod();
  return(mod->SPIsetRegValue(RADIOLIB_SI443X_REG_TX_POWER, (uint8_t)((power + 8) / 3), 2, 0));
}

#endif
