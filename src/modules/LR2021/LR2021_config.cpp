#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

// maximum number of allowed frontend calibration attempts
#define RADIOLIB_LR2021_MAX_CAL_ATTEMPTS    (10)

int16_t LR2021::setFrequency(float freq) {
  return(this->setFrequency(freq, false));
}

int16_t LR2021::setFrequency(float freq, bool skipCalibration) {
  #if RADIOLIB_CHECK_PARAMS
  if(!(((freq >= 150.0f) && (freq <= 960.0f)) ||
    ((freq >= 1900.0f) && (freq <= 2200.0f)) ||
    ((freq >= 2400.0f) && (freq <= 2500.0f)))) {
      return(RADIOLIB_ERR_INVALID_FREQUENCY);
  }
  #endif

  // check if we need to recalibrate image
  int16_t state;
  if(!skipCalibration && (fabsf(freq - this->freqMHz) >= RADIOLIB_LR2021_CAL_IMG_FREQ_TRIG_MHZ)) {
    // calibration can fail if there is a strong interfering source
    // run it several times until it passes
    int i = 0;
    for(; i < RADIOLIB_LR2021_MAX_CAL_ATTEMPTS; i++) {
      // get the nearest multiple of 4 MHz
      uint16_t frequencies[3] = { (uint16_t)((freq / 4.0f) + 0.5f), 0, 0 };
      frequencies[0] |= (freq > 1000.0f) ? RADIOLIB_LR2021_CALIBRATE_FE_HF_PATH : RADIOLIB_LR2021_CALIBRATE_FE_LF_PATH;
      state = calibrateFrontEnd(const_cast<const uint16_t*>(frequencies));

      // if something failed, check the device errors
      if(state != RADIOLIB_ERR_NONE) {
        uint16_t errors = 0;
        getErrors(&errors);
        RADIOLIB_DEBUG_BASIC_PRINTLN("Frontend calibration #%d failed, device errors: 0x%X", i, errors);

        // if this is casued by something else than RSSI saturation, repeating will not help
        if((errors & RADIOLIB_LR2021_SRC_SATURATION_CALIB_ERR) == 0) {
          return(state);
        }

        // wait a little while before the next attempt
        this->mod->hal->delay(5);
      
      } else {
        // calibration passed
        break;
      }

    }

    if(i == RADIOLIB_LR2021_MAX_CAL_ATTEMPTS) {
      return(RADIOLIB_ERR_FRONTEND_CALIBRATION_FAILED);
    }

  }

  // set frequency
  state = setRfFrequency((uint32_t)(freq*1000000.0f));
  RADIOLIB_ASSERT(state);
  this->freqMHz = freq;
  this->highFreq = (freq > 1000.0f);
  return(state);
}

int16_t LR2021::setOutputPower(int8_t power) {
  return(this->setOutputPower(power, false));
}

int16_t LR2021::setOutputPower(int8_t power, uint32_t rampTimeUs) {
  // check if power value is configurable
  int16_t state = this->checkOutputPower(power, NULL);
  RADIOLIB_ASSERT(state);
  
  //! \TODO: [LR2021] how and when to configure OCP?
  //! \TODO: [LR2021] Determine the optimal PA configuration
  // update PA config
  state = setPaConfig(this->highFreq, 
    RADIOLIB_LR2021_PA_LF_MODE_FSM, 
    RADIOLIB_LR2021_PA_LF_DUTY_CYCLE_UNUSED, 
    RADIOLIB_LR2021_PA_LF_SLICES_UNUSED, 
    RADIOLIB_LR2021_PA_HF_DUTY_CYCLE_UNUSED);
  RADIOLIB_ASSERT(state);

  // set output power
  state = setTxParams(power, roundRampTime(rampTimeUs));
  return(state);
}

int16_t LR2021::checkOutputPower(int8_t power, int8_t* clipped) {
  if(this->highFreq) {
    if(clipped) {
      *clipped = RADIOLIB_MAX(-19, RADIOLIB_MIN(12, power));
    }
    RADIOLIB_CHECK_RANGE(power, -19, 12, RADIOLIB_ERR_INVALID_OUTPUT_POWER);

  } else {
    if(clipped) {
      *clipped = RADIOLIB_MAX(-9, RADIOLIB_MIN(22, power));
    }
    RADIOLIB_CHECK_RANGE(power, -9, 22, RADIOLIB_ERR_INVALID_OUTPUT_POWER);

  }
  
  return(RADIOLIB_ERR_NONE);
}

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