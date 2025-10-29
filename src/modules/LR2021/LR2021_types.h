#if !defined(RADIOLIB_LR2021_TYPES_H)
#define RADIOLIB_LR2021_TYPES_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR2021

/*!
  \struct LR2021LrFhssHopTableEntry_t
  \brief Structure to save information about LR-FHSS hopping table entry.
*/
struct LR2021LrFhssHopTableEntry_t {
  bool hoppingEnable;
  bool convertFreq;
  uint16_t packetLen;
  uint8_t numUsedFrequencies;
  uint8_t numHoppingBlocks;
  uint32_t freq;
  uint16_t numSymbols;
};

#endif

#endif
