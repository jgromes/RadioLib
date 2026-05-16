#include "Si4432.h"

#if !RADIOLIB_EXCLUDE_SI443X

Si4432::Si4432(Module* mod) : Si443x(mod) {

}

int16_t Si4432::begin(const Si443x::ConfigFSK_t& cfg) {
  // execute common part
  int16_t state = Si443x::begin(cfg.bitRate, cfg.frequencyDeviation, cfg.receiverBandwidth, cfg.preambleLength);
  RADIOLIB_ASSERT(state);
  RADIOLIB_DEBUG_BASIC_PRINTLN("M\tSi4432");

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  return(state);
}

int16_t Si4432::begin(float freq, float br, float freqDev, float rxBw, int8_t power, uint8_t preambleLen) {
  Si443x::ConfigFSK_t cfg;
  cfg.frequency = freq;
  cfg.bitRate = br;
  cfg.frequencyDeviation = freqDev;
  cfg.receiverBandwidth = rxBw;
  cfg.power = power;
  cfg.preambleLength = preambleLen;
  return(this->begin(cfg));
}

int16_t Si4432::setFrequency(float freq) {
  RADIOLIB_CHECK_RANGE(freq, 240.0f, 930.0f, RADIOLIB_ERR_INVALID_FREQUENCY);

  // set frequency
  return(Si443x::setFrequencyRaw(freq));
}

int16_t Si4432::setOutputPower(int8_t power) {
  RADIOLIB_CHECK_RANGE(power, -1, 20, RADIOLIB_ERR_INVALID_OUTPUT_POWER);

  // set output power
  Module* mod = this->getMod();
  return(mod->SPIsetRegValue(RADIOLIB_SI443X_REG_TX_POWER, (uint8_t)((power + 1) / 3), 2, 0));
}

#endif
