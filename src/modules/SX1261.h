#ifndef _RADIOLIB_SX1261_H
#define _RADIOLIB_SX1261_H

#include "TypeDef.h"
#include "Module.h"
#include "SX126x.h"
#include "SX1262.h"


// TODO: implement SX1261 class
class SX1261 : public SX1262 {
  public:
    /*!
      \brief Default constructor.

      \param mod Instance of Module that will be used to communicate with the radio.
    */
    SX1261(Module* mod)
      : SX1262(mod) {
    }

    int16_t setOutputPower(int8_t power);

  private:
    int16_t setOptimalLowPowerPaConfig(int8_t* inOutPower);
};


#endif
