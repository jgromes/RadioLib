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
      \brief Read the chip's hardware / device / firmware version. Overrides
      the LR11x0 implementation so it does NOT issue the WiFi-scanning and
      GNSS-scanning version commands, which only exist on LR1110 / LR1120
      silicon. On LR1121 those commands return an error, causing the
      inherited findChip() to incorrectly report RADIOLIB_ERR_CHIP_NOT_FOUND
      even when the GetVersion command returned the correct device ID.
      \param info Pointer to LR11x0VersionInfo_t to fill in.
    */
    int16_t getVersionInfo(LR11x0VersionInfo_t* info) override;

#if !RADIOLIB_GODMODE
  private:
#endif

};

#endif

#endif
