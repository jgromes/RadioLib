#if !defined(_RADIOLIB_AX5243_H)
#define _RADIOLIB_AX5243_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_AX5X43)

#include "../../Module.h"
#include "AX5x43.h"

/*!
  \class AX5243

  \brief Derived class for %AX5243 modules.
*/
class AX5243: public AX5x43 {
  public:

    // constructor

    /*!
      \brief Default constructor. Called from application layer when creating new module instance.

      \param mod Instance of Module that will be used to communicate with the transceiver chip.
    */
    AX5243(Module* mod);

    // basic methods

    int16_t begin(float freq = 434.0, float br = 4.8, float freqDev = 5.0, float rxBw = 125.0, int8_t power = 10, uint16_t preambleLength = 16);

#if !defined(RADIOLIB_GODMODE)
  private:
#endif
};

#endif

#endif
