#include "LLCC68.h"
#if !RADIOLIB_EXCLUDE_SX126X

LLCC68::LLCC68(Module* mod) : SX1262(mod) {
  chipType = RADIOLIB_LLCC68_CHIP_TYPE;
}

int16_t LLCC68::begin(const ConfigLoRa_t& cfg) {
  int16_t state = SX126x::begin(cfg.codingRate, cfg.syncWord, cfg.preambleLength);
  if(state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
    // bit of a hack, but some LLCC68 chips report as "SX1261", try that
    // for full discussion, see https://github.com/jgromes/RadioLib/issues/1329
    chipType = RADIOLIB_SX1261_CHIP_TYPE;
    state = SX126x::begin(cfg.codingRate, cfg.syncWord, cfg.preambleLength);
    RADIOLIB_DEBUG_PRINTLN("LLCC68 version string not found, using SX1261 instead");
  }
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setSpreadingFactor(cfg.spreadingFactor);
  RADIOLIB_ASSERT(state);

  state = setBandwidth(cfg.bandwidth);
  RADIOLIB_ASSERT(state);

  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t LLCC68::beginFSK(const ConfigFSK_t& cfg) {
  // execute common part
  int16_t state = SX126x::beginFSK(cfg.bitRate, cfg.frequencyDeviation, cfg.receiverBandwidth, cfg.preambleLength);
  if(state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
    // bit of a hack, but some LLCC68 chips report as "SX1261", try that
    // for full discussion, see https://github.com/jgromes/RadioLib/issues/1329
    chipType = RADIOLIB_SX1261_CHIP_TYPE;
    state = SX126x::beginFSK(cfg.bitRate, cfg.frequencyDeviation, cfg.receiverBandwidth, cfg.preambleLength);
    RADIOLIB_DEBUG_PRINTLN("LLCC68 version string not found, using SX1261 instead");
  }
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t LLCC68::beginBPSK(const ConfigBPSK_t& cfg) {
  // execute common part
  int16_t state = SX126x::beginBPSK(cfg.bitRate);
  if(state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
    // bit of a hack, but some LLCC68 chips report as "SX1261", try that
    // for full discussion, see https://github.com/jgromes/RadioLib/issues/1329
    chipType = RADIOLIB_SX1261_CHIP_TYPE;
    state = SX126x::beginBPSK(cfg.bitRate);
    RADIOLIB_DEBUG_PRINTLN("LLCC68 version string not found, using SX1261 instead");
  }
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t LLCC68::beginLRFHSS(const ConfigLRFHSS_t& cfg) {
  // execute common part
  int16_t state = SX126x::beginLRFHSS(cfg.bandwidth, cfg.codingRate, cfg.narrowGrid);
  if(state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
    // bit of a hack, but some LLCC68 chips report as "SX1261", try that
    // for full discussion, see https://github.com/jgromes/RadioLib/issues/1329
    chipType = RADIOLIB_SX1261_CHIP_TYPE;
    state = SX126x::beginLRFHSS(cfg.bandwidth, cfg.codingRate, cfg.narrowGrid);
    RADIOLIB_DEBUG_PRINTLN("LLCC68 version string not found, using SX1261 instead");
  }
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(cfg.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(cfg.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t LLCC68::setBandwidth(uint32_t bw) {
  RADIOLIB_CHECK_RANGE(bw, RADIOLIB_UNIT_KILO(125), RADIOLIB_UNIT_KILO(500), RADIOLIB_ERR_INVALID_BANDWIDTH);
  return(SX1262::setBandwidth(bw));
}

int16_t LLCC68::setSpreadingFactor(uint8_t sf) {
  switch(SX126x::bandwidth) {
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

int16_t LLCC68::setDataRate(DataRate_t dr, ModemType_t modem) {
  // get the current modem
  ModemType_t currentModem;
  int16_t state = this->getModem(&currentModem);
  RADIOLIB_ASSERT(state);

  // switch over if the requested modem is different
  if(modem != RADIOLIB_MODEM_NONE && modem != currentModem) {
    state = this->standby();
    RADIOLIB_ASSERT(state);
    state = this->setModem(modem);
    RADIOLIB_ASSERT(state);
  }
  
  if(modem == RADIOLIB_MODEM_NONE) {
    modem = currentModem;
  }

  // select interpretation based on modem
  if(modem == RADIOLIB_MODEM_FSK) {
    // set the bit rate
    state = this->setBitRate(dr.fsk.bitRate);
    RADIOLIB_ASSERT(state);

    // set the frequency deviation
    state = this->setFrequencyDeviation(dr.fsk.freqDev);

  } else if(modem == RADIOLIB_MODEM_LORA) {
    // set the spreading factor
    state = this->setSpreadingFactor(dr.lora.spreadingFactor);
    RADIOLIB_ASSERT(state);

    // set the bandwidth
    state = this->setBandwidth(dr.lora.bandwidth);
    RADIOLIB_ASSERT(state);

    // set the coding rate
    state = this->setCodingRate(dr.lora.codingRate);
  }

  return(state);
}

int16_t LLCC68::checkDataRate(DataRate_t dr, ModemType_t modem) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;

  // retrieve modem if not supplied
  if(modem == RADIOLIB_MODEM_NONE) {
    state = this->getModem(&modem);
    RADIOLIB_ASSERT(state);
  }

  // select interpretation based on modem
  if(modem == RADIOLIB_MODEM_FSK) {
    RADIOLIB_CHECK_RANGE(dr.fsk.bitRate, 600, RADIOLIB_UNIT_KILO(300), RADIOLIB_ERR_INVALID_BIT_RATE);
    RADIOLIB_CHECK_RANGE(dr.fsk.freqDev, 600, RADIOLIB_UNIT_KILO(200), RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);
    return(RADIOLIB_ERR_NONE);

  } else if(modem == RADIOLIB_MODEM_LORA) {
    RADIOLIB_CHECK_RANGE(dr.lora.bandwidth, RADIOLIB_UNIT_KILO(125), RADIOLIB_UNIT_KILO(500), RADIOLIB_ERR_INVALID_BANDWIDTH);
    RADIOLIB_CHECK_RANGE(dr.lora.codingRate, 4, 8, RADIOLIB_ERR_INVALID_CODING_RATE);
    switch(dr.lora.bandwidth)  {
      case(RADIOLIB_UNIT_KILO(125)):
        RADIOLIB_CHECK_RANGE(dr.lora.spreadingFactor, 5, 9, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
        break;
      case(RADIOLIB_UNIT_KILO(250)):
        RADIOLIB_CHECK_RANGE(dr.lora.spreadingFactor, 5, 10, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
        break;
      case(RADIOLIB_UNIT_KILO(500)):
        RADIOLIB_CHECK_RANGE(dr.lora.spreadingFactor, 5, 11, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
        break;
      default:
        return(RADIOLIB_ERR_INVALID_BANDWIDTH);
    }
    return(RADIOLIB_ERR_NONE);
  }

  return(state);
}

int16_t LLCC68::setModem(ModemType_t modem) {
  switch(modem) {
    case(ModemType_t::RADIOLIB_MODEM_LORA): {
      ConfigLoRa_t cfg;
      return(this->begin(cfg));
    } break;
    case(ModemType_t::RADIOLIB_MODEM_FSK): {
      ConfigFSK_t cfg;
      return(this->beginFSK(cfg));
    } break;
    case(ModemType_t::RADIOLIB_MODEM_LRFHSS): {
      ConfigLRFHSS_t cfg;
      return(this->beginLRFHSS(cfg));
    } break;
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
}

#endif
