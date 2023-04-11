#include <stdint.h>
#include <stddef.h>
#if !defined(_RADIOLIB_HAL_H)
#define _RADIOLIB_HAL_H

class Hal {
  public:
    const uint8_t GpioModeInput;
    const uint8_t GpioModeOutput;
    const uint8_t GpioLevelLow;
    const uint8_t GpioLevelHigh;
    const uint8_t GpioInterruptRising;
    const uint8_t GpioInterruptFalling;

    Hal(const uint8_t input, const uint8_t output, const uint8_t low, const uint8_t high, const uint8_t rising, const uint8_t falling);

    virtual void init();
    virtual void term();

    virtual void pinMode(uint8_t pin, uint8_t mode) = 0;
    virtual void digitalWrite(uint8_t pin, uint8_t value) = 0;
    virtual uint8_t digitalRead(uint8_t pin) = 0;
    virtual void attachInterrupt(uint8_t interruptNum, void (*interruptCb)(void), uint8_t mode) = 0;
    virtual void detachInterrupt(uint8_t interruptNum) = 0;
    virtual void delay(unsigned long ms) = 0;
    virtual void delayMicroseconds(unsigned long us) = 0;
    virtual unsigned long millis() = 0;
    virtual unsigned long micros() = 0;
    virtual long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout) = 0;
    virtual void spiBegin() = 0;
    virtual void spiBeginTransaction() = 0;
    virtual uint8_t spiTransfer(uint8_t b) = 0;
    virtual void spiEndTransaction() = 0;
    virtual void spiEnd() = 0;

    virtual void tone(uint8_t pin, unsigned int frequency, unsigned long duration = 0);
    virtual void noTone(uint8_t pin);
    virtual void yield();
    virtual uint8_t pinToInterrupt(uint8_t pin);
};

#endif
