#include <stdint.h>
#include "Hal.h"

Hal::Hal(const uint32_t input, const uint32_t output, const uint32_t low, const uint32_t high, const uint32_t rising, const uint32_t falling)
    : GpioModeInput(input),
      GpioModeOutput(output),
      GpioLevelLow(low),
      GpioLevelHigh(high),
      GpioInterruptRising(rising),
      GpioInterruptFalling(falling) {}

void Hal::init(){};
void Hal::term(){};
void Hal::tone(uint32_t pin, unsigned int frequency, unsigned long duration){
  (void)pin;
  (void)frequency;
  (void)duration;
};
void Hal::noTone(uint32_t pin){
  (void)pin;
};
void Hal::yield(){};
uint32_t Hal::pinToInterrupt(uint32_t pin) {
  return pin;
};
