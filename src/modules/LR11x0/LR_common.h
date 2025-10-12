#if !defined(RADIOLIB_LR_COMMON_H)
#define RADIOLIB_LR_COMMON_H

#include "../../Module.h"

int16_t LR_writeCommon(Module* mod, uint16_t cmd, uint32_t addrOffset, const uint32_t* data, size_t len, bool nonvolatile);

#endif
