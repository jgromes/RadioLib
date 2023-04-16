#include "TypeDef.h"

#if !defined(_RADIOLIB_ARDUINOHAL_H)
#define _RADIOLIB_ARDUINOHAL_H

#if defined(RADIOLIB_BUILD_ARDUINO)

#if defined(RADIOLIB_MBED_TONE_OVERRIDE)
#include "mbed.h"
#endif

#include "Hal.h"
#include <stdint.h>

#include <SPI.h>

class ArduinoHal : public Hal {
  public:
    /*!
      \brief Arduino Hal constructor. Will use the default SPI interface and automatically initialize it.
    */
    ArduinoHal();

    /*!
      \brief Arduino Hal constructor. Will not attempt SPI interface initialization.

      \param spi SPI interface to be used, can also use software SPI implementations.

      \param spiSettings SPI interface settings.
    */
    ArduinoHal(SPIClass& spi, SPISettings spiSettings = RADIOLIB_DEFAULT_SPI_SETTINGS);

    void init() override;
    void term() override;

    void pinMode(uint8_t pin, uint8_t mode) override;
    void digitalWrite(uint8_t pin, uint8_t value) override;
    uint8_t digitalRead(uint8_t pin) override;
    void attachInterrupt(uint8_t interruptNum, void (*interruptCb)(void), uint8_t mode) override;
    void detachInterrupt(uint8_t interruptNum) override;
    void delay(unsigned long ms) override;
    void delayMicroseconds(unsigned long us) override;
    unsigned long millis() override;
    unsigned long micros() override;
    long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout) override;
    void spiBegin() override;
    void spiBeginTransaction() override;
    uint8_t spiTransfer(uint8_t b) override;
    void spiEndTransaction() override;
    void spiEnd() override;
    void tone(uint8_t pin, unsigned int frequency, unsigned long duration = 0) override;
    void noTone(uint8_t pin) override;
    void yield() override;
    uint8_t pinToInterrupt(uint8_t pin) override;

#if !defined(RADIOLIB_GODMODE)
  private:
#endif
    SPIClass* _spi = NULL;
    SPISettings _spiSettings = RADIOLIB_DEFAULT_SPI_SETTINGS;
    bool _initInterface = false;

    #if defined(RADIOLIB_MBED_TONE_OVERRIDE)
    mbed::PwmOut *pwmPin = NULL;
    #endif

    #if defined(ESP32)
    int32_t _prev = -1;
    #endif
};

#endif
#endif
