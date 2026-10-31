#if !defined(RADIOLIB_LR2012_H)
#define RADIOLIB_LR2012_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR2021

#include "../../Module.h"
#include "LR2021.h"

/*!
  \class LR2012
  \brief Derived class for %LR2012 modules. The LR2012 is the sub-GHz member of the LR20xx family:
  it shares the LR2021 command set, but has no high-frequency front end and no FLRC, O-QPSK,
  Bluetooth LE or Z-Wave support.
*/
class LR2012: public LR2021 {
  public:
    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    LR2012(Module* mod); // cppcheck-suppress noExplicitConstructor

#if !RADIOLIB_GODMODE
  private:
#endif

};

#endif

#endif
