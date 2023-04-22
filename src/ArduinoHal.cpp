#include "ArduinoHal.h"

#if defined(RADIOLIB_BUILD_ARDUINO)

ArduinoHal::ArduinoHal(): RadioLibHal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING), spi(&RADIOLIB_DEFAULT_SPI), initInterface(true) {}

ArduinoHal::ArduinoHal(SPIClass& spi, SPISettings spiSettings): RadioLibHal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING), spi(&spi), spiSettings(spiSettings) {}

void ArduinoHal::init() {
  if(initInterface) {
    spiBegin();
  }
}

void ArduinoHal::term() {
  if(initInterface) {
    spiEnd();
  }
}

void inline ArduinoHal::pinMode(uint32_t pin, uint32_t mode) {
  if(pin == RADIOLIB_NC) {
    return;
  }
  ::pinMode(pin, RADIOLIB_ARDUINOHAL_PIN_MODE_CAST mode);
}

void inline ArduinoHal::digitalWrite(uint32_t pin, uint32_t value) {
  if(pin == RADIOLIB_NC) {
    return;
  }
  ::digitalWrite(pin, RADIOLIB_ARDUINOHAL_PIN_STATUS_CAST value);
}

uint32_t inline ArduinoHal::digitalRead(uint32_t pin) {
  if(pin == RADIOLIB_NC) {
    return 0;
  }
  return(::digitalRead(pin));
}

void inline ArduinoHal::attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) {
  if(interruptNum == RADIOLIB_NC) {
    return;
  }
  ::attachInterrupt(interruptNum, interruptCb,  RADIOLIB_ARDUINOHAL_INTERRUPT_MODE_CAST mode);
}

void inline ArduinoHal::detachInterrupt(uint32_t interruptNum) {
  if(interruptNum == RADIOLIB_NC) {
    return;
  }
  ::detachInterrupt(interruptNum);
}

void inline ArduinoHal::delay(unsigned long ms) {
  ::delay(ms);
}

void inline ArduinoHal::delayMicroseconds(unsigned long us) {
  ::delayMicroseconds(us);
}

unsigned long inline ArduinoHal::millis() {
  return(::millis());
}

unsigned long inline ArduinoHal::micros() {
  return(::micros());
}

long inline ArduinoHal::pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) {
  if(pin == RADIOLIB_NC) {
    return 0;
  }
  return(::pulseIn(pin, state, timeout));
}

void inline ArduinoHal::spiBegin() {
  spi->begin();
}

void inline ArduinoHal::spiBeginTransaction() {
  spi->beginTransaction(spiSettings);
}

uint8_t inline ArduinoHal::spiTransfer(uint8_t b) {
  return(spi->transfer(b));
}

void inline ArduinoHal::spiEndTransaction() {
  spi->endTransaction();
}

void inline ArduinoHal::spiEnd() {
  spi->end();
}

void inline ArduinoHal::tone(uint32_t pin, unsigned int frequency, unsigned long duration) {
  #if !defined(RADIOLIB_TONE_UNSUPPORTED)
    if(pin == RADIOLIB_NC) {
      return;
    }
    ::tone(pin, frequency, duration);
  #elif defined(ESP32)
    // ESP32 tone() emulation
    (void)duration;
    if(prev == -1) {
      ledcAttachPin(pin, RADIOLIB_TONE_ESP32_CHANNEL);
    }
    if(prev != frequency) {
      ledcWriteTone(RADIOLIB_TONE_ESP32_CHANNEL, frequency);
    }
    prev = frequency;
  #elif defined(RADIOLIB_MBED_TONE_OVERRIDE)
    // better tone for mbed OS boards
    (void)duration;
    if(!pwmPin) {
      pwmPin = new mbed::PwmOut(digitalPinToPinName(pin));
    }
    pwmPin->period(1.0 / frequency);
    pwmPin->write(0.5);
  #endif
}

void inline ArduinoHal::noTone(uint32_t pin) {
  #if !defined(RADIOLIB_TONE_UNSUPPORTED) and defined(ARDUINO_ARCH_STM32)
    if(pin == RADIOLIB_NC) {
      return;
    }
    ::noTone(pin, false);
  #elif !defined(RADIOLIB_TONE_UNSUPPORTED)
    if(pin == RADIOLIB_NC) {
      return;
    }
    ::noTone(pin);
  #elif defined(ESP32)
    if(pin == RADIOLIB_NC) {
      return;
    }
    // ESP32 tone() emulation
    ledcDetachPin(pin);
    ledcWrite(RADIOLIB_TONE_ESP32_CHANNEL, 0);
    prev = -1;
  #elif defined(RADIOLIB_MBED_TONE_OVERRIDE)
    if(pin == RADIOLIB_NC) {
      return;
    }
    // better tone for mbed OS boards
    (void)pin;
    pwmPin->suspend();
  #endif
}

void inline ArduinoHal::yield() {
  #if !defined(RADIOLIB_YIELD_UNSUPPORTED)
  ::yield();
  #endif
}

uint32_t inline ArduinoHal::pinToInterrupt(uint32_t pin) {
  return(digitalPinToInterrupt(pin));
}

#endif
