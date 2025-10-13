#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::readRadioRxFifo(uint8_t* data, size_t len) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_READ_RX_FIFO, false, data, len, NULL, 0));
}

int16_t LR2021::writeRadioTxFifo(uint8_t* data, size_t len) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_WRITE_TX_FIFO, true, data, len, NULL, 0));
}

#endif
