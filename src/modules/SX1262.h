#ifndef _RADIOLIB_SX1262_H
#define _RADIOLIB_SX1262_H

#include "TypeDef.h"
#include "Module.h"
#include "SX126x.h"

//SX126X_CMD_SET_PA_CONFIG
#define SX126X_PA_CONFIG_SX1261                       0x01
#define SX126X_PA_CONFIG_SX1262                       0x00

class SX1262: public SX126x {
  public:
    // constructor
    SX1262(Module* mod);

    // basic methods
    int16_t begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint16_t syncWord = SX126X_SYNC_WORD_PRIVATE, int8_t power = 14, float currentLimit = 60.0, uint16_t preambleLength = 8);
    int16_t beginFSK(float freq = 434.0, float br = 48.0, float freqDev = 50.0, float rxBw = 117.3, int8_t power = 14, float currentLimit = 60.0, uint16_t preambleLength = 8, float dataShaping = 0.5);

    // configuration methods
    int16_t setFrequency(float freq);
    int16_t setOutputPower(int8_t power);

  private:


};

#endif
