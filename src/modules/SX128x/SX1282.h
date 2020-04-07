#ifndef _RADIOLIB_SX1282_H
#define _RADIOLIB_SX1282_H

#include "../../TypeDef.h"
#include "../../Module.h"
#include "SX128x.h"
#include "SX1280.h"

// TODO implement advanced ranging

/*!
  \class SX1282

  \brief Derived class for %SX1282 modules.
*/
class SX1282: public SX1280 {
  public:
    /*!
      \brief Default constructor.

      \param mod Instance of Module that will be used to communicate with the radio.
    */
    SX1282(Module* mod);

#ifndef RADIOLIB_GODMODE
  private:
#endif

};

#endif
