#if !defined(_RADIOLIB_LR1121_H)
#define _RADIOLIB_LR1121_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR11X0

#include "../../Module.h"
#include "LR11x0.h"
#include "LR1120.h"

/*!
  \class LR1121
  \brief Derived class for %LR1121 modules.
*/
class LR1121: public LR1120 {
  public:
    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    LR1121(Module* mod); // cppcheck-suppress noExplicitConstructor

    /*!
      \brief Sets output power. Allowed values are in range from -17 to 22 dBm (high-power PA) or -18 to 13 dBm (High-frequency PA).
      \param power Output power to be set in dBm.
      \param useHighFreqPa  When using 2.4G frequency, need to switch to High-frequency PA
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power,bool useHighFreqPa = false);


    // TODO this is where overrides to disable GNSS+WiFi scanning methods on LR1121
    // will be put once those are implemented

#if !RADIOLIB_GODMODE
  private:
#endif

};

#endif

#endif
