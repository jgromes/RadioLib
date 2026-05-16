#include "SX1279.h"
#if !RADIOLIB_EXCLUDE_SX127X

SX1279::SX1279(Module* mod) : SX1278(mod) {

}

int16_t SX1279::begin(const SX127x::ConfigLoRa_t& config) {
  // execute common part
  const uint8_t versions[] = { RADIOLIB_SX1278_CHIP_VERSION, RADIOLIB_SX1278_CHIP_VERSION_ALT, RADIOLIB_SX1278_CHIP_VERSION_RFM9X };
  int16_t state = SX127x::begin(versions, 3, config.syncWord, config.preambleLength);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setBandwidth(config.bandwidth);
  RADIOLIB_ASSERT(state);

  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = setSpreadingFactor(config.spreadingFactor);
  RADIOLIB_ASSERT(state);

  state = setCodingRate(config.codingRate);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  RADIOLIB_ASSERT(state);

  state = setGain(config.gain);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  state = setCRC(true);
  return(state);
}

int16_t SX1279::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain) {
  SX127x::ConfigLoRa_t config;
  config.frequency = freq;
  config.bandwidth = bw;
  config.spreadingFactor = sf;
  config.codingRate = cr;
  config.syncWord = syncWord;
  config.power = power;
  config.preambleLength = preambleLength;
  config.gain = gain;
  return(begin(config));
}

int16_t SX1279::beginFSK(const SX127x::ConfigFSK_t& config) {
  // execute common part
  const uint8_t versions[] = { RADIOLIB_SX1278_CHIP_VERSION, RADIOLIB_SX1278_CHIP_VERSION_ALT, RADIOLIB_SX1278_CHIP_VERSION_RFM9X };
  int16_t state = SX127x::beginFSK(versions, 3, config.frequencyDeviation, 
    config.receiverBandwidth, config.preambleLength, config.enableOOK);
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = configFSK();
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = setBitRate(config.bitRate);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  RADIOLIB_ASSERT(state);

  if(config.enableOOK) {
    state = setDataShapingOOK(RADIOLIB_SHAPING_NONE);
    RADIOLIB_ASSERT(state);
  } else {
    state = setDataShaping(RADIOLIB_SHAPING_NONE);
    RADIOLIB_ASSERT(state);
  }

  // set publicly accessible settings that are not a part of begin method
  state = setCRC(true);
  return(state);
}

int16_t SX1279::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK) {
  SX127x::ConfigFSK_t config;
  config.frequency = freq;
  config.bitRate = br;
  config.frequencyDeviation = freqDev;
  config.receiverBandwidth = rxBw;
  config.power = power;
  config.preambleLength = preambleLength;
  config.enableOOK = enableOOK;
  return(beginFSK(config));
}

int16_t SX1279::setFrequency(float freq) {
  // NOTE: The datasheet specifies Band 2 as 410-480 MHz, but the hardware has been
  // verified to work down to ~395 MHz. The lower bound is set here to 395 MHz to
  // accommodate real-world use cases (e.g. TinyGS satellites, radiosondes) while
  // adding a small margin below the 400 MHz practical limit.
  if(!(((freq >= 137.0f) && (freq <= 160.0f)) ||
       ((freq >= 395.0f) && (freq <= 480.0f)) ||
       ((freq >= 779.0f) && (freq <= 960.0f)))) {
    return(RADIOLIB_ERR_INVALID_FREQUENCY);
  }

  // set frequency and if successful, save the new setting
  int16_t state = SX127x::setFrequencyRaw(freq);
  if(state == RADIOLIB_ERR_NONE) {
    SX127x::frequency = freq;
  }
  return(state);
}

int16_t SX1279::setModem(ModemType_t modem) {
  switch(modem) {
    case(ModemType_t::RADIOLIB_MODEM_LORA): {
      return(this->begin());
    } break;
    case(ModemType_t::RADIOLIB_MODEM_FSK): {
      return(this->beginFSK());
    } break;
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
}

#endif
