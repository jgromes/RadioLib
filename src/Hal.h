#include <stdint.h>
#include <stddef.h>
#if !defined(_RADIOLIB_HAL_H)
#define _RADIOLIB_HAL_H

/*!
  \class Hal

  \brief Hardware abstraction library base interface.
*/
class Hal {
  public:
    const uint32_t GpioModeInput;
    const uint32_t GpioModeOutput;
    const uint32_t GpioLevelLow;
    const uint32_t GpioLevelHigh;
    const uint32_t GpioInterruptRising;
    const uint32_t GpioInterruptFalling;

    Hal(const uint32_t input, const uint32_t output, const uint32_t low, const uint32_t high, const uint32_t rising, const uint32_t falling);

    virtual void init();
    virtual void term();

    virtual void pinMode(uint32_t pin, uint32_t mode) = 0;
    virtual void digitalWrite(uint32_t pin, uint32_t value) = 0;
    virtual uint32_t digitalRead(uint32_t pin) = 0;
    virtual void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) = 0;
    virtual void detachInterrupt(uint32_t interruptNum) = 0;
    virtual void delay(unsigned long ms) = 0;
    virtual void delayMicroseconds(unsigned long us) = 0;
    virtual unsigned long millis() = 0;
    virtual unsigned long micros() = 0;
    virtual long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) = 0;
    virtual void spiBegin() = 0;
    virtual void spiBeginTransaction() = 0;
    virtual uint8_t spiTransfer(uint8_t b) = 0;
    virtual void spiEndTransaction() = 0;
    virtual void spiEnd() = 0;

    virtual void tone(uint32_t pin, unsigned int frequency, unsigned long duration = 0);
    virtual void noTone(uint32_t pin);
    virtual void yield();
    virtual uint32_t pinToInterrupt(uint32_t pin);
};

#endif
