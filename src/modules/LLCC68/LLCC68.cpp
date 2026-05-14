#include "LLCC68.h"
#if !RADIOLIB_EXCLUDE_SX126X

LLCC68::LLCC68(Module* mod) : SX1262(mod) {
  chipType = RADIOLIB_LLCC68_CHIP_TYPE;
  this->XTAL = true;
}

int16_t LLCC68::begin(const SX126x::ConfigLoRa_t& config) {
  int16_t state = SX126x::begin(
    config.codingRate, config.syncWord, config.preambleLength,
    config.tcxoVoltage, config.useRegulatorLDO);
  if(state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
    // bit of a hack, but some LLCC68 chips report as "SX1261", try that
    // for full discussion, see https://github.com/jgromes/RadioLib/issues/1329
    chipType = RADIOLIB_SX1261_CHIP_TYPE;
    state = SX126x::begin(
      config.codingRate, config.syncWord, config.preambleLength,
      config.tcxoVoltage, config.useRegulatorLDO);
    RADIOLIB_DEBUG_PRINTLN("LLCC68 version string not found, using SX1261 instead");
  }
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setSpreadingFactor(config.spreadingFactor);
  RADIOLIB_ASSERT(state);

  state = setBandwidth(config.bandwidth);
  RADIOLIB_ASSERT(state);

  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t LLCC68::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  SX126x::ConfigLoRa_t config;
  config.frequency = freq;
  config.bandwidth = bw;
  config.spreadingFactor = sf;
  config.codingRate = cr;
  config.syncWord = syncWord;
  config.power = power;
  config.preambleLength = preambleLength;
  config.tcxoVoltage = tcxoVoltage;
  config.useRegulatorLDO = useRegulatorLDO;
  return(begin(config));
}

int16_t LLCC68::beginFSK(const SX126x::ConfigFSK_t& config) {
  // execute common part
  int16_t state = SX126x::beginFSK(
    config.bitRate, config.frequencyDeviation, config.receiverBandwidth, 
    config.preambleLength, config.tcxoVoltage, config.useRegulatorLDO);
  if(state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
    // bit of a hack, but some LLCC68 chips report as "SX1261", try that
    // for full discussion, see https://github.com/jgromes/RadioLib/issues/1329
    chipType = RADIOLIB_SX1261_CHIP_TYPE;
    state = SX126x::beginFSK(
      config.bitRate, config.frequencyDeviation, config.receiverBandwidth, 
      config.preambleLength, config.tcxoVoltage, config.useRegulatorLDO);
    RADIOLIB_DEBUG_PRINTLN("LLCC68 version string not found, using SX1261 instead");
  }
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t LLCC68::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  SX126x::ConfigFSK_t config;
  config.frequency = freq;
  config.bitRate = br;
  config.frequencyDeviation = freqDev;
  config.receiverBandwidth = rxBw;
  config.power = power;
  config.preambleLength = preambleLength;
  config.tcxoVoltage = tcxoVoltage;
  config.useRegulatorLDO = useRegulatorLDO;
  return(beginFSK(config));
}

int16_t LLCC68::beginBPSK(const SX126x::ConfigBPSK_t& config) {
  // execute common part
  int16_t state = SX126x::beginBPSK(config.bitRate, config.tcxoVoltage, config.useRegulatorLDO);
  if(state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
    // bit of a hack, but some LLCC68 chips report as "SX1261", try that
    // for full discussion, see https://github.com/jgromes/RadioLib/issues/1329
    chipType = RADIOLIB_SX1261_CHIP_TYPE;
    state = SX126x::beginBPSK(config.bitRate, config.tcxoVoltage, config.useRegulatorLDO);
    RADIOLIB_DEBUG_PRINTLN("LLCC68 version string not found, using SX1261 instead");
  }
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t LLCC68::beginBPSK(float freq, float br, int8_t power, float tcxoVoltage, bool useRegulatorLDO) {
  SX126x::ConfigBPSK_t config;
  config.frequency = freq;
  config.bitRate = br;
  config.power = power;
  config.tcxoVoltage = tcxoVoltage;
  config.useRegulatorLDO = useRegulatorLDO;
  return(beginBPSK(config));
}

int16_t LLCC68::beginLRFHSS(const SX126x::ConfigLRFHSS_t& config) {
  // execute common part
  int16_t state = SX126x::beginLRFHSS(config.bandwidth, config.codingRate, 
    config.narrowGrid, config.tcxoVoltage, config.useRegulatorLDO);
  if(state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
    // bit of a hack, but some LLCC68 chips report as "SX1261", try that
    // for full discussion, see https://github.com/jgromes/RadioLib/issues/1329
    chipType = RADIOLIB_SX1261_CHIP_TYPE;
    state = SX126x::beginLRFHSS(config.bandwidth, config.codingRate, 
      config.narrowGrid, config.tcxoVoltage, config.useRegulatorLDO);
    RADIOLIB_DEBUG_PRINTLN("LLCC68 version string not found, using SX1261 instead");
  }
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(config.frequency);
  RADIOLIB_ASSERT(state);

  state = SX126x::fixPaClamping();
  RADIOLIB_ASSERT(state);

  state = setOutputPower(config.power);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t LLCC68::beginLRFHSS(float freq, uint8_t bw, uint8_t cr, bool narrowGrid, int8_t power, float tcxoVoltage, bool useRegulatorLDO) {
  SX126x::ConfigLRFHSS_t config;
  config.frequency = freq;
  config.bandwidth = bw;
  config.codingRate = cr;
  config.narrowGrid = narrowGrid;
  config.power = power;
  config.tcxoVoltage = tcxoVoltage;
  config.useRegulatorLDO = useRegulatorLDO;
  return(beginLRFHSS(config));
}

int16_t LLCC68::setBandwidth(float bw) {
  RADIOLIB_CHECK_RANGE(bw, 100.0f, 510.0f, RADIOLIB_ERR_INVALID_BANDWIDTH);
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
    RADIOLIB_CHECK_RANGE(dr.fsk.bitRate, 0.6f, 300.0f, RADIOLIB_ERR_INVALID_BIT_RATE);
    RADIOLIB_CHECK_RANGE(dr.fsk.freqDev, 0.6f, 200.0f, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);
    return(RADIOLIB_ERR_NONE);

  } else if(modem == RADIOLIB_MODEM_LORA) {
    RADIOLIB_CHECK_RANGE(dr.lora.bandwidth, 100.0f, 510.0f, RADIOLIB_ERR_INVALID_BANDWIDTH);
    RADIOLIB_CHECK_RANGE(dr.lora.codingRate, 4, 8, RADIOLIB_ERR_INVALID_CODING_RATE);
    uint8_t bw_div2 = dr.lora.bandwidth / 2 + 0.01f;
    switch (bw_div2)  {
      case 62: // 125.0:
        RADIOLIB_CHECK_RANGE(dr.lora.spreadingFactor, 5, 9, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
        break;
      case 125: // 250.0
        RADIOLIB_CHECK_RANGE(dr.lora.spreadingFactor, 5, 10, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
        break;
      case 250: // 500.0
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
      return(this->begin());
    } break;
    case(ModemType_t::RADIOLIB_MODEM_FSK): {
      return(this->beginFSK());
    } break;
    case(ModemType_t::RADIOLIB_MODEM_LRFHSS): {
      return(this->beginLRFHSS());
    } break;
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
}

#endif
