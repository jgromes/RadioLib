#include <stdint.h>
#include "Hal.h"

Hal::Hal(const uint8_t input, const uint8_t output, const uint8_t low, const uint8_t high, const uint8_t rising, const uint8_t falling)
    : GpioModeInput(input),
      GpioModeOutput(output),
      GpioLevelLow(low),
      GpioLevelHigh(high),
      GpioInterruptRising(rising),
      GpioInterruptFalling(falling) {}

void Hal::init(){};
void Hal::term(){};
void Hal::tone(uint8_t pin, unsigned int frequency, unsigned long duration){
  (void)pin;
  (void)frequency;
  (void)duration;
};
void Hal::noTone(uint8_t pin){
  (void)pin;
};
void Hal::yield(){};
uint8_t Hal::pinToInterrupt(uint8_t pin) {
  return pin;
};
