#include "Hal.h"

static RadioLibHal* rlb_timestamp_hal = nullptr;

RadioLibHal::RadioLibHal(const uint32_t input, const uint32_t output, const uint32_t low, const uint32_t high, const uint32_t rising, const uint32_t falling)
    : GpioModeInput(input),
      GpioModeOutput(output),
      GpioLevelLow(low),
      GpioLevelHigh(high),
      GpioInterruptRising(rising),
      GpioInterruptFalling(falling) {
        if(!rlb_timestamp_hal) {
          rlb_timestamp_hal = this;
        }
      }

int16_t RadioLibHal::init() {
  // the default implementation has no failure mode
  return(RADIOLIB_ERR_NONE);
}

int16_t RadioLibHal::term() {
  // the default implementation has no failure mode
  return(RADIOLIB_ERR_NONE);
}

void RadioLibHal::tone(uint32_t pin, unsigned int frequency, RadioLibTime_t duration) {
  (void)pin;
  (void)frequency;
  (void)duration;
}

void RadioLibHal::noTone(uint32_t pin) {
  (void)pin;
}

void RadioLibHal::yield() {

}

uint32_t RadioLibHal::pinToInterrupt(uint32_t pin) {
  return(pin);
}

void RadioLibHal::pullUpDown(uint32_t pin, bool enable, bool up) {
  // the default implementation does nothing
  (void)pin;
  (void)enable;
  (void)up;
}

RadioLibTime_t rlb_time_us() {
  return(rlb_timestamp_hal == nullptr ? 0 : rlb_timestamp_hal->micros());
}
