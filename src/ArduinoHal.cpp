#include "ArduinoHal.h"

#if defined(RADIOLIB_BUILD_ARDUINO)

ArduinoHal::ArduinoHal(SPIClass& spi, SPISettings spiSettings): Hal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING), _spi(&spi), _spiSettings(spiSettings) {}

ArduinoHal::ArduinoHal(): Hal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING), _spi(&RADIOLIB_DEFAULT_SPI), _initInterface(true) {}

void ArduinoHal::init() {
  if(_initInterface) {
    spiBegin();
  }
}

void ArduinoHal::term() {
  if(_initInterface) {
    spiEnd();
  }
}

void inline ArduinoHal::pinMode(uint8_t pin, uint8_t mode) {
  if (pin == RADIOLIB_NC) {
    return;
  }
  ::pinMode(pin, RADIOLIB_ARDUINOHAL_PIN_MODE_CAST mode);
}
void inline ArduinoHal::digitalWrite(uint8_t pin, uint8_t value) {
  if (pin == RADIOLIB_NC) {
    return;
  }
  ::digitalWrite(pin, RADIOLIB_ARDUINOHAL_PIN_STATUS_CAST value);
}
uint8_t inline ArduinoHal::digitalRead(uint8_t pin) {
  if (pin == RADIOLIB_NC) {
    return 0;
  }
  return ::digitalRead(pin);
}
void inline ArduinoHal::attachInterrupt(uint8_t interruptNum, void (*interruptCb)(void), uint8_t mode) {
  if (interruptNum == RADIOLIB_NC) {
    return;
  }
  ::attachInterrupt(interruptNum, interruptCb,  RADIOLIB_ARDUINOHAL_INTERRUPT_MODE_CAST mode);
}
void inline ArduinoHal::detachInterrupt(uint8_t interruptNum) {
  if (interruptNum == RADIOLIB_NC) {
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
  return ::millis();
}
unsigned long inline ArduinoHal::micros() {
  return ::micros();
}
long inline ArduinoHal::pulseIn(uint8_t pin, uint8_t state, unsigned long timeout) {
  if (pin == RADIOLIB_NC) {
    return 0;
  }
  return ::pulseIn(pin, state, timeout);
}
void inline ArduinoHal::spiBegin() {
  _spi->begin();
}
void inline ArduinoHal::spiBeginTransaction() {
  _spi->beginTransaction(_spiSettings);
}
uint8_t inline ArduinoHal::spiTransfer(uint8_t b) {
  return _spi->transfer(b);
}
void inline ArduinoHal::spiEndTransaction() {
  _spi->endTransaction();
}
void inline ArduinoHal::spiEnd() {
  _spi->end();
}
void inline ArduinoHal::tone(uint8_t pin, unsigned int frequency, unsigned long duration) {
  #if !defined(RADIOLIB_TONE_UNSUPPORTED)
  if (pin == RADIOLIB_NC) {
    return;
  }
  ::tone(pin, frequency, duration);
  #elif defined(ESP32)
  // ESP32 tone() emulation
  (void)duration;
  if(_prev == -1) {
    ledcAttachPin(pin, RADIOLIB_TONE_ESP32_CHANNEL);
  }
  if(_prev != frequency) {
    ledcWriteTone(RADIOLIB_TONE_ESP32_CHANNEL, frequency);
  }
  _prev = frequency;
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
void inline ArduinoHal::noTone(uint8_t pin) {
  #if !defined(RADIOLIB_TONE_UNSUPPORTED) and defined(ARDUINO_ARCH_STM32)
  if (pin == RADIOLIB_NC) {
    return;
  }
  ::noTone(pin, false);
  #elif !defined(RADIOLIB_TONE_UNSUPPORTED)
  if (pin == RADIOLIB_NC) {
    return;
  }
  ::noTone(pin);
  #elif defined(ESP32)
  if (pin == RADIOLIB_NC) {
    return;
  }
  // ESP32 tone() emulation
  ledcDetachPin(pin);
  ledcWrite(RADIOLIB_TONE_ESP32_CHANNEL, 0);
  _prev = -1;
  #elif defined(RADIOLIB_MBED_TONE_OVERRIDE)
  if (pin == RADIOLIB_NC) {
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
uint8_t inline ArduinoHal::pinToInterrupt(uint8_t pin) {
  return digitalPinToInterrupt(pin);
}
#endif
