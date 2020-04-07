#ifndef _RADIOLIB_SX1280_H
#define _RADIOLIB_SX1280_H

#include "../../TypeDef.h"
#include "../../Module.h"
#include "SX128x.h"
#include "SX1281.h"

// TODO implement ranging

/*!
  \class SX1280

  \brief Derived class for %SX1280 modules.
*/
class SX1280: public SX1281 {
  public:
    /*!
      \brief Default constructor.

      \param mod Instance of Module that will be used to communicate with the radio.
    */
    SX1280(Module* mod);

#ifndef RADIOLIB_GODMODE
  private:
#endif

};

#endif
