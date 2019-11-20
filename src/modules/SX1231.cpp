#include "SX1231.h"

SX1231::SX1231(Module* mod) : RF69(mod) {

}

int16_t SX1231::begin(float freq, float br, float rxBw, float freqDev, int8_t power) {
  // set module properties
  _mod->init(RADIOLIB_USE_SPI, RADIOLIB_INT_BOTH);

  // try to find the SX1231 chip
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    uint8_t version = _mod->SPIreadRegister(RF69_REG_VERSION);
    if((version == 0x21) || (version == 0x22) || (version == 0x23)) {
      flagFound = true;
      _chipRevision = version;
    } else {
      #ifdef RADIOLIB_DEBUG
        RADIOLIB_DEBUG_PRINT(F("SX127x not found! ("));
        RADIOLIB_DEBUG_PRINT(i + 1);
        RADIOLIB_DEBUG_PRINT(F(" of 10 tries) SX127X_REG_VERSION == "));

        char buffHex[7];
        sprintf(buffHex, "0x%04X", version);
        RADIOLIB_DEBUG_PRINT(buffHex);
        RADIOLIB_DEBUG_PRINT(F(", expected 0x0021 / 0x0022 / 0x0023"));
        RADIOLIB_DEBUG_PRINTLN();
      #endif
      delay(1000);
      i++;
    }
  }

  if(!flagFound) {
    RADIOLIB_DEBUG_PRINTLN(F("No SX1231 found!"));
    SPI.end();
    return(ERR_CHIP_NOT_FOUND);
  } else {
    RADIOLIB_DEBUG_PRINTLN(F("Found SX1231!"));
  }

  // configure settings not accessible by API
  int16_t state = config();
  if(state != ERR_NONE) {
    return(state);
  }

  // configure publicly accessible settings
  state = setFrequency(freq);
  if(state != ERR_NONE) {
    return(state);
  }

  // configure bitrate
  _rxBw = 125.0;
  state = setBitRate(br);
  if(state != ERR_NONE) {
    return(state);
  }

  // configure default RX bandwidth
  state = setRxBandwidth(rxBw);
  if(state != ERR_NONE) {
    return(state);
  }

  // configure default frequency deviation
  state = setFrequencyDeviation(freqDev);
  if(state != ERR_NONE) {
    return(state);
  }

  // configure default TX output power
  state = setOutputPower(power);
  if(state != ERR_NONE) {
    return(state);
  }

  // default sync word values 0x2D01 is the same as the default in LowPowerLab RFM69 library
  uint8_t syncWord[] = {0x2D, 0x01};
  state = setSyncWord(syncWord, 2);
  if(state != ERR_NONE) {
    return(state);
  }

  // set default packet length mode
  state = variablePacketLengthMode();
  if (state != ERR_NONE) {
    return(state);
  }

  // SX1231 V2a only
  if(_chipRevision == SX1231_CHIP_REVISION_2_A) {
    // modify default OOK threshold value
    state = _mod->SPIsetRegValue(SX1231_REG_TEST_OOK, SX1231_OOK_DELTA_THRESHOLD);
    if(state != ERR_NONE) {
      return(state);
    }

    // enable OCP with 95 mA limit
    state = _mod->SPIsetRegValue(RF69_REG_OCP, RF69_OCP_ON | RF69_OCP_TRIM, 4, 0);
    if(state != ERR_NONE) {
      return(state);
    }
  }

  return(ERR_NONE);
}
