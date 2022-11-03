/*

Copyright (c) 2022 STMicroelectronics

This file is licensed under the MIT License: https://opensource.org/licenses/MIT
*/

#include "STM32WLx_Module.h"

#if !defined(RADIOLIB_EXCLUDE_STM32WLX)

#include <SubGhz.h>

// This defines some dummy pin numbers (starting at NUM_DIGITAL_PINS to
// guarantee these are not valid regular pin numbers) that can be passed
// to the parent Module class, to be stored here and then passed back to
// the overridden callbacks when these are used.
enum {
  RADIOLIB_STM32WLx_VIRTUAL_PIN_NSS = NUM_DIGITAL_PINS,
  RADIOLIB_STM32WLx_VIRTUAL_PIN_BUSY,
  RADIOLIB_STM32WLx_VIRTUAL_PIN_IRQ,
  RADIOLIB_STM32WLx_VIRTUAL_PIN_RESET,
};


STM32WLx_Module::STM32WLx_Module():
  Module(
    RADIOLIB_STM32WLx_VIRTUAL_PIN_NSS,
    RADIOLIB_STM32WLx_VIRTUAL_PIN_IRQ,
    RADIOLIB_STM32WLx_VIRTUAL_PIN_RESET,
    RADIOLIB_STM32WLx_VIRTUAL_PIN_BUSY,
    SubGhz.SPI,
    SubGhz.spi_settings
  )
{
  setCb_pinMode(virtualPinMode);
  setCb_digitalWrite(virtualDigitalWrite);
  setCb_digitalRead(virtualDigitalRead);
}

void STM32WLx_Module::virtualPinMode(uint32_t dwPin, uint32_t dwMode) {
  switch(dwPin) {
    case RADIOLIB_STM32WLx_VIRTUAL_PIN_NSS:
    case RADIOLIB_STM32WLx_VIRTUAL_PIN_BUSY:
    case RADIOLIB_STM32WLx_VIRTUAL_PIN_IRQ:
    case RADIOLIB_STM32WLx_VIRTUAL_PIN_RESET:
      // Nothing to do
      break;
    default:
      ::pinMode(dwPin, dwMode);
      break;
  }
}

void STM32WLx_Module::virtualDigitalWrite(uint32_t dwPin, uint32_t dwVal) {
  switch (dwPin) {
    case RADIOLIB_STM32WLx_VIRTUAL_PIN_NSS:
      SubGhz.setNssActive(dwVal == LOW);
      break;

    case RADIOLIB_STM32WLx_VIRTUAL_PIN_RESET:
      SubGhz.setResetActive(dwVal == LOW);
      break;

    case RADIOLIB_STM32WLx_VIRTUAL_PIN_BUSY:
    case RADIOLIB_STM32WLx_VIRTUAL_PIN_IRQ:
      // Should not (and cannot) be written, just ignore
      break;

    default:
      ::digitalWrite(dwPin, dwVal);
      break;
  }
}

int STM32WLx_Module::virtualDigitalRead(uint32_t ulPin) {
  switch (ulPin) {
    case RADIOLIB_STM32WLx_VIRTUAL_PIN_BUSY:
      return(SubGhz.isBusy() ? HIGH : LOW);

    case RADIOLIB_STM32WLx_VIRTUAL_PIN_IRQ:
      // If the IRQ is disabled, we can read the value by clearing and
      // seeing if it immediately becomes pending again.
      // TODO: This seems a bit fragile, but it does work.
      SubGhz.clearPendingInterrupt();
      return(SubGhz.isInterruptPending() ? HIGH : LOW);

    case RADIOLIB_STM32WLx_VIRTUAL_PIN_NSS:
      return(SubGhz.isNssActive() ? LOW : HIGH);

    case RADIOLIB_STM32WLx_VIRTUAL_PIN_RESET:
      return(SubGhz.isResetActive() ? LOW : HIGH);

    default:
      return(::digitalRead(ulPin));
  }
}

#endif // !defined(RADIOLIB_EXCLUDE_STM32WLX)
