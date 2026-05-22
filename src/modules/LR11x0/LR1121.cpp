#include "LR1121.h"
#if !RADIOLIB_EXCLUDE_LR11X0

LR1121::LR1121(Module* mod) : LR1120(mod) {
  chipType = RADIOLIB_LR11X0_DEVICE_LR1121;
}

int16_t LR1121::getVersionInfo(LR11x0VersionInfo_t* info) {
  if(!info) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }

  // LR1121 only implements GetVersion. WiFi-scanning and GNSS-scanning
  // firmware blocks don't exist on this silicon, so the corresponding
  // SPI commands return errors. Reading just the GetVersion fields is
  // sufficient for findChip() and any caller that wants the device
  // identifier or base firmware version.
  int16_t state = this->getVersion(&info->hardware, &info->device,
                                   &info->fwMajor, &info->fwMinor);
  RADIOLIB_ASSERT(state);

  // Zero the LR1110 / LR1120-only fields so callers don't see garbage.
  info->fwMajorWiFi = 0;
  info->fwMinorWiFi = 0;
  info->fwGNSS      = 0;
  info->almanacGNSS = 0;
  return(RADIOLIB_ERR_NONE);
}

#endif