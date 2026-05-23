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

/*!
  \struct LR2021LoRaSideDetector_t
  \brief Structure to configure multi-SF detection
*/
struct LR2021LoRaSideDetector_t {
  /*! \brief Spreading factor value */
  uint8_t sf;
  
  /*! \brief Low datarate optimization enabled for this detector */
  bool ldro;
  
  /*! \brief IQ inversion for this detector */
  bool invertIQ;

  /*! \brief LoRa sync word for this detector */
  uint8_t syncWord;
};

/*!
  \struct paTableEntry_t
  \brief This structure is used as entry in the PA lookup table, 
  to optimize PA configuration for minimum power consumption.
*/
struct __attribute__((packed)) LR2021PaTableEntry_t {
  uint8_t paDutyCycle: 4;
  uint8_t paSlices: 4;
  int8_t paVal;
};

#endif

#endif
