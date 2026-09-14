#include "SX1279.h"
#if !RADIOLIB_EXCLUDE_SX127X

SX1279::SX1279(Module* mod) : SX1278(mod) {

}

int16_t SX1279::begin(const ConfigLoRa_t& cfg) {
  // execute common part
  const uint8_t versions[] = { RADIOLIB_SX1278_CHIP_VERSION, RADIOLIB_SX1278_CHIP_VERSION_ALT, RADIOLIB_SX1278_CHIP_VERSION_RFM9X };
  int16_t state = SX127x::begin(versions, 3, cfg.syncWord, cfg.preambleLength);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setBandwidth(cfg.bandwidth);
  RADIOLIB_ASSERT(state);

  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = setSpreadingFactor(cfg.spreadingFactor);
  RADIOLIB_ASSERT(state);

  state = setCodingRate(cfg.codingRate);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  RADIOLIB_ASSERT(state);

  state = setGain(this->gain);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  state = setCRC(true);
  return(state);
}

int16_t SX1279::beginFSK(const ConfigFSK_t& cfg) {
  // execute common part
  const uint8_t versions[] = { RADIOLIB_SX1278_CHIP_VERSION, RADIOLIB_SX1278_CHIP_VERSION_ALT, RADIOLIB_SX1278_CHIP_VERSION_RFM9X };
  int16_t state = SX127x::beginFSK(versions, 3, cfg.frequencyDeviation, cfg.receiverBandwidth, cfg.preambleLength);
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = configFSK();
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = setBitRate(cfg.bitRate);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  RADIOLIB_ASSERT(state);

  if(this->enableOOK) {
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

int16_t SX1279::setFrequency(uint32_t freq) {
  // NOTE: The datasheet specifies Band 2 as 410-480 MHz, but the hardware has been
  // verified to work down to ~395 MHz. The lower bound is set here to 395 MHz to
  // accommodate real-world use cases (e.g. TinyGS satellites, radiosondes) while
  // adding a small margin below the 400 MHz practical limit.
  if(!(((freq >= RADIOLIB_UNIT_MEGA(137)) && (freq <= RADIOLIB_UNIT_MEGA(160))) ||
       ((freq >= RADIOLIB_UNIT_MEGA(395)) && (freq <= RADIOLIB_UNIT_MEGA(480))) ||
       ((freq >= RADIOLIB_UNIT_MEGA(779)) && (freq <= RADIOLIB_UNIT_MEGA(960))))) {
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
      ConfigLoRa_t cfg;
      return(this->begin(cfg));
    } break;
    case(ModemType_t::RADIOLIB_MODEM_FSK): {
      ConfigFSK_t cfg;
      return(this->beginFSK(cfg));
    } break;
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
}

#endif
