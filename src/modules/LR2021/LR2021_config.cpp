#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::setBandwidth(float bw) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
    
  // convert to int to avoid bunch of math
  int bw_div2 = bw / 2 + 0.01f;

  // check allowed bandwidth values
  switch (bw_div2)  {
    case 15:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_31;
      break;
    case 20:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_41;
      break;
    case 31:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_62;
      break;
    case 41:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_83;
      break;
    case 50:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_101;
      break;
    case 101:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_203;
      break;
    case 62:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_125;
      break;
    case 125:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_250;
      break;
    case 203:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_406;
      break;
    case 250:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_500;
      break;
    case 406:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_812;
      break;
    case 500:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_1000;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_BANDWIDTH);
  }

  // update modulation parameters
  this->bandwidthKhz = bw;
  return(setLoRaModulationParams(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t LR2021::setSpreadingFactor(uint8_t sf, bool legacy) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(sf, 5, 12, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);

  //! \TODO: [LR2021] enable SF6 legacy mode
  if(legacy && (sf == 6)) {
    //this->mod->SPIsetRegValue(RADIOLIB_LR11X0_REG_SF6_SX127X_COMPAT, RADIOLIB_LR11X0_SF6_SX127X, 18, 18);
  }

  // update modulation parameters
  this->spreadingFactor = sf;
  return(setLoRaModulationParams(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t LR2021::setCodingRate(uint8_t cr, bool longInterleave) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(cr, 4, 8, RADIOLIB_ERR_INVALID_CODING_RATE);

  if(longInterleave) {
    switch(cr) {
      case 4:
        this->codingRate = 0;
        break;
      case 5:
      case 6:
        this->codingRate = cr;
        break;
      case 8: 
        this->codingRate = cr - 1;
        break;
      default:
        return(RADIOLIB_ERR_INVALID_CODING_RATE);
    }
  
  } else {
    this->codingRate = cr - 4;
  
  }

  // update modulation parameters
  return(setLoRaModulationParams(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t LR2021::setSyncWord(uint8_t syncWord) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  
  return(setLoRaSyncword(syncWord));
}

int16_t LR2021::setPreambleLength(size_t preambleLength) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    this->preambleLengthLoRa = preambleLength;
    return(setLoRaPacketParams(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, (uint8_t)this->invertIQEnabled));
  }

  //! \TODO: [LR2021] implement other modems

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR2021::setTCXO(float voltage, uint32_t delay) {
  // check if TCXO is enabled at all
  if(this->XTAL) {
    return(RADIOLIB_ERR_INVALID_TCXO_VOLTAGE);
  }

  // set mode to standby
  standby();

  // check oscillator startup error flag and clear it
  //! \TODO: [LR2021] legacy from LR11x0, chech if this really works on LR2021
  uint16_t errors = 0;
  int16_t state = getErrors(&errors);
  RADIOLIB_ASSERT(state);
  if(errors & RADIOLIB_LR2021_HF_XOSC_START_ERR) {
    clearErrors();
  }

  // check 0 V disable
  if(fabsf(voltage - 0.0f) <= 0.001f) {
    setTcxoMode(0, 0);
    return(reset());
  }

  // check allowed voltage values
  uint8_t tune = 0;
  if(fabsf(voltage - 1.6f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_1_6;
  } else if(fabsf(voltage - 1.7f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_1_7;
  } else if(fabsf(voltage - 1.8f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_1_8;
  } else if(fabsf(voltage - 2.2f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_2_2;
  } else if(fabsf(voltage - 2.4f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_2_4;
  } else if(fabsf(voltage - 2.7f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_2_7;
  } else if(fabsf(voltage - 3.0f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_3_0;
  } else if(fabsf(voltage - 3.3f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_3_3;
  } else {
    return(RADIOLIB_ERR_INVALID_TCXO_VOLTAGE);
  }

  // calculate delay value
  uint32_t delayValue = (uint32_t)((float)delay / 30.52f);
  if(delayValue == 0) {
    delayValue = 1;
  }
 
  // enable TCXO control
  return(setTcxoMode(tune, delayValue));
}

int16_t LR2021::setCRC(uint8_t len, uint32_t initial, uint32_t polynomial, bool inverted) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    // LoRa CRC doesn't allow to set CRC polynomial, initial value, or inversion
    this->crcTypeLoRa = len > 0 ? RADIOLIB_LR2021_LORA_CRC_ENABLED : RADIOLIB_LR2021_LORA_CRC_DISABLED;
    state = setLoRaPacketParams(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, (uint8_t)this->invertIQEnabled);
  }
  
  //! \TODO: [LR2021] implement other modems
  (void)initial;
  (void)polynomial;
  (void)inverted;

  return(state);
}

int16_t LR2021::invertIQ(bool enable) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  this->invertIQEnabled = enable;
  return(setLoRaPacketParams(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, (uint8_t)this->invertIQEnabled));
}

#endif