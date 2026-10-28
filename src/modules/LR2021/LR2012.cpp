#include "LR2012.h"
#if !RADIOLIB_EXCLUDE_LR2021

LR2012::LR2012(Module* mod) : LR2021(mod) {
  chipType = RADIOLIB_LR2021_DEVICE_LR2012;
}

#endif
