#if !defined(_RADIOLIB_BUILD_OPTIONS_H)
#define _RADIOLIB_BUILD_OPTIONS_H

#if ARDUINO >= 100
  // Arduino build
  #include "Arduino.h"
  #define RADIOLIB_BUILD_ARDUINO
#else
  // generic build
  #define RADIOLIB_BUILD_GENERIC
#endif

#if defined(RADIOLIB_BUILD_ARDUINO)

/*
 * Platform-specific configuration.
 *
 * RADIOLIB_PLATFORM - platform name, used in debugging to quickly check the correct platform is detected.
 * RADIOLIB_PIN_TYPE - which type should be used for pin numbers in functions like digitalRead().
 * RADIOLIB_PIN_MODE - which type should be used for pin modes in functions like pinMode().
 * RADIOLIB_PIN_STATUS - which type should be used for pin values in functions like digitalWrite().
 * RADIOLIB_INTERRUPT_STATUS - which type should be used for pin changes in functions like attachInterrupt().
 * RADIOLIB_DIGITAL_PIN_TO_INTERRUPT - function/macro to be used to convert digital pin number to interrupt pin number.
 * RADIOLIB_NC - alias for unused pin, usually the largest possible value of RADIOLIB_PIN_TYPE.
 * RADIOLIB_DEFAULT_SPI - default SPIClass instance to use.
 * RADIOLIB_NONVOLATILE - macro to place variable into program storage (usually Flash).
 * RADIOLIB_NONVOLATILE_READ_BYTE - function/macro to read variables saved in program storage (usually Flash).
 * RADIOLIB_TYPE_ALIAS - construct to create an alias for a type, usually vai the `using` keyword.
 * RADIOLIB_TONE_UNSUPPORTED - some platforms do not have tone()/noTone(), which is required for AFSK.
 * RADIOLIB_BUILTIN_MODULE - some platforms have a builtin radio module on fixed pins, this macro is used to specify that pinout.
 *
 * In addition, some platforms may require RadioLib to disable specific drivers (such as ESP8266).
 *
 * Users may also specify their own configuration by uncommenting the RADIOLIB_CUSTOM_ARDUINO,
 * and then specifying all platform parameters in the section below. This will override automatic
 * platform detection.
 */

// uncomment to enable custom platform definition
//#define RADIOLIB_CUSTOM_ARDUINO

#if defined(RADIOLIB_CUSTOM_ARDUINO)
  // name for your platform
  #define RADIOLIB_PLATFORM                           "Custom"

  // the following parameters must always be defined
  #define RADIOLIB_PIN_TYPE                           uint8_t
  #define RADIOLIB_PIN_MODE                           uint8_t
  #define RADIOLIB_PIN_STATUS                         uint8_t
  #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
  #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
  #define RADIOLIB_NC                                 (0xFF)
  #define RADIOLIB_DEFAULT_SPI                        SPI
  #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
  #define RADIOLIB_NONVOLATILE                        PROGMEM
  #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
  #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

  // Arduino API callbacks
  // the following are signatures of Arduino API functions of the custom platform
  // for example, pinMode on Arduino Uno is defined as void pinMode(uint8_t pin, uint8_t mode)
  // all fo the callbacks below are taken from Arduino Uno
  #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint8_t pin, uint8_t mode)
  #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint8_t pin, uint8_t value)
  #define RADIOLIB_CB_ARGS_DIGITAL_READ               (int, digitalRead, uint8_t pin)
  #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t _pin, unsigned int frequency, unsigned long duration)
  #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t _pin)
  #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint8_t interruptNum, void (*userFunc)(void), int mode)
  #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint8_t interruptNum)
  #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
  #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long ms)
  #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int us)
  #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
  #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
  #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
  #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
  #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
  #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
  #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  // the following must be defined if the Arduino core does not support tone function
  //#define RADIOLIB_TONE_UNSUPPORTED

  // some platforms seem to have issues with SPI modules that use a command interface
  // this can be mitigated by adding delays into SPI communication
  // (see https://github.com/jgromes/RadioLib/issues/158 for details)
  //#define RADIOLIB_SPI_SLOWDOWN

  // some of RadioLib drivers may be excluded, to prevent collisions with platforms (or to speed up build process)
  // the following is a complete list of all possible exclusion macros, uncomment any of them to disable that driver
  // NOTE: Some of the exclusion macros are dependent on each other. For example, it is not possible to exclude RF69
  //       while keeping SX1231 (because RF69 is the base class for SX1231). The dependency is always uni-directional,
  //       so excluding SX1231 and keeping RF69 is valid.
  //#define RADIOLIB_EXCLUDE_CC1101
  //#define RADIOLIB_EXCLUDE_NRF24
  //#define RADIOLIB_EXCLUDE_RF69
  //#define RADIOLIB_EXCLUDE_SX1231     // dependent on RADIOLIB_EXCLUDE_RF69
  //#define RADIOLIB_EXCLUDE_SI443X
  //#define RADIOLIB_EXCLUDE_RFM2X      // dependent on RADIOLIB_EXCLUDE_SI443X
  //#define RADIOLIB_EXCLUDE_SX127X
  //#define RADIOLIB_EXCLUDE_RFM9X      // dependent on RADIOLIB_EXCLUDE_SX127X
  //#define RADIOLIB_EXCLUDE_SX126X
  //#define RADIOLIB_EXCLUDE_SX128X
  //#define RADIOLIB_EXCLUDE_AFSK
  //#define RADIOLIB_EXCLUDE_AX25
  //#define RADIOLIB_EXCLUDE_HELLSCHREIBER
  //#define RADIOLIB_EXCLUDE_MORSE
  //#define RADIOLIB_EXCLUDE_RTTY
  //#define RADIOLIB_EXCLUDE_SSTV

#else
  #if defined(__AVR__) && !(defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_AVR_NANO_EVERY) || defined(ARDUINO_ARCH_MEGAAVR))
    // Arduino AVR boards (except for megaAVR) - Uno, Mega etc.
    #define RADIOLIB_PLATFORM                           "Arduino AVR"
    #define RADIOLIB_PIN_TYPE                           uint8_t
    #define RADIOLIB_PIN_MODE                           uint8_t
    #define RADIOLIB_PIN_STATUS                         uint8_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint8_t pin, uint8_t mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint8_t pin, uint8_t value)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (int, digitalRead, uint8_t pin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t _pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t _pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint8_t interruptNum, void (*userFunc)(void), int mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint8_t interruptNum)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long ms)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(ESP8266)
    // ESP8266 boards
    #define RADIOLIB_PLATFORM                           "ESP8266"
    #define RADIOLIB_PIN_TYPE                           uint8_t
    #define RADIOLIB_PIN_MODE                           uint8_t
    #define RADIOLIB_PIN_STATUS                         uint8_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint8_t pin, uint8_t mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint8_t pin, uint8_t value)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (int, digitalRead, uint8_t pin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t _pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t _pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint8_t pin, void (*)(void), int mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint8_t interruptNum)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(ESP32)
    // ESP32 boards
    #define RADIOLIB_PLATFORM                           "ESP32"
    #define RADIOLIB_PIN_TYPE                           uint8_t
    #define RADIOLIB_PIN_MODE                           uint8_t
    #define RADIOLIB_PIN_STATUS                         uint8_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // ESP32 doesn't support tone(), but it can be emulated via LED control peripheral
    #define RADIOLIB_TONE_UNSUPPORTED
    #define RADIOLIB_TONE_ESP32_CHANNEL                 (1)

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint8_t pin, uint8_t mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint8_t pin, uint8_t value)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (int, digitalRead, uint8_t pin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, void)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, void)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint8_t pin, void (*)(void), int mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint8_t pin)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, uint32_t)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, uint32_t us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(ARDUINO_ARCH_STM32)
    // official STM32 Arduino core (https://github.com/stm32duino/Arduino_Core_STM32)
    #define RADIOLIB_PLATFORM                           "Arduino STM32 (official)"
    #define RADIOLIB_PIN_TYPE                           uint32_t
    #define RADIOLIB_PIN_MODE                           uint32_t
    #define RADIOLIB_PIN_STATUS                         uint32_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt((PinName)p)
    #define RADIOLIB_NC                                 (0xFFFFFFFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // slow down SX126x/8x SPI on this platform
    #define RADIOLIB_SPI_SLOWDOWN

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint32_t dwPin, uint32_t dwMode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint32_t dwPin, uint32_t dwVal)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (int, digitalRead, uint32_t ulPin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t _pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t _pin, bool destruct)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint32_t pin, void (*callback)(void), uint32_t mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint32_t pin)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, uint32_t ms)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, uint32_t us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (uint32_t, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (uint32_t, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(SAMD_SERIES)
    // Adafruit SAMD boards (M0 and M4)
    #define RADIOLIB_PLATFORM                           "Adafruit SAMD"
    #define RADIOLIB_PIN_TYPE                           uint32_t
    #define RADIOLIB_PIN_MODE                           uint32_t
    #define RADIOLIB_PIN_STATUS                         uint32_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFFFFFFFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // slow down SX126x/8x SPI on this platform
    #define RADIOLIB_SPI_SLOWDOWN

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint32_t dwPin, uint32_t dwMode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint32_t dwPin, uint32_t dwVal)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (int, digitalRead, uint32_t ulPin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint32_t _pin, uint32_t frequency, uint32_t duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint32_t _pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint32_t pin, voidFuncPtr callback, uint32_t mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint32_t pin)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long dwMs)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(ARDUINO_ARCH_SAMD)
    // Arduino SAMD (Zero, MKR, etc.)
    #define RADIOLIB_PLATFORM                           "Arduino SAMD"
    #define RADIOLIB_PIN_TYPE                           pin_size_t
    #define RADIOLIB_PIN_MODE                           PinMode
    #define RADIOLIB_PIN_STATUS                         PinStatus
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, pin_size_t pinNumber, PinMode pinMode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, pin_size_t pinNumber, PinStatus status)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (PinStatus, digitalRead, pin_size_t pinNumber)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, unsigned char outputPin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t outputPin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, pin_size_t pin, voidFuncPtr callback, PinStatus mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, pin_size_t pin)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(__SAM3X8E__)
    // Arduino Due
    #define RADIOLIB_PLATFORM                           "Arduino Due"
    #define RADIOLIB_PIN_TYPE                           uint32_t
    #define RADIOLIB_PIN_MODE                           uint32_t
    #define RADIOLIB_PIN_STATUS                         uint32_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFFFFFFFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;
    #define RADIOLIB_TONE_UNSUPPORTED

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint32_t dwPin, uint32_t dwMode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint32_t dwPin, uint32_t dwVal)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (int, digitalRead, uint32_t ulPin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, void)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, void)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint32_t pin, void (*callback)(void), uint32_t mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint32_t pin)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, uint32_t dwMs)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, uint32_t usec)
    #define RADIOLIB_CB_ARGS_MILLIS                     (uint32_t, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (uint32_t, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif (defined(NRF52832_XXAA) || defined(NRF52840_XXAA)) && !defined(ARDUINO_ARDUINO_NANO33BLE)
    // Adafruit nRF52 boards
    #define RADIOLIB_PLATFORM                           "Adafruit nRF52"
    #define RADIOLIB_PIN_TYPE                           uint32_t
    #define RADIOLIB_PIN_MODE                           uint32_t
    #define RADIOLIB_PIN_STATUS                         uint32_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFFFFFFFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint32_t dwPin, uint32_t dwMode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint32_t dwPin, uint32_t dwVal)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (int, digitalRead, uint32_t ulPin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (int, attachInterrupt, uint32_t pin, voidFuncPtr callback, uint32_t mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint32_t pin)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, uint32_t dwMs)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, uint32_t usec)
    #define RADIOLIB_CB_ARGS_MILLIS                     (uint32_t, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (uint32_t, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(ARDUINO_ARC32_TOOLS)
    // Intel Curie
    #define RADIOLIB_PLATFORM                           "Intel Curie"
    #define RADIOLIB_PIN_TYPE                           uint8_t
    #define RADIOLIB_PIN_MODE                           uint8_t
    #define RADIOLIB_PIN_STATUS                         uint8_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint8_t pin, uint8_t mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint8_t pin, uint8_t val)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (int, digitalRead, uint8_t pin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint32_t _pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint32_t _pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint32_t pin, void (*callback)(void), uint32_t mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint32_t pin)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, uint32_t dwMs)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, uint32_t dwUs)
    #define RADIOLIB_CB_ARGS_MILLIS                     (uint64_t, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (uint64_t, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_AVR_NANO_EVERY)
    // Arduino megaAVR boards - Uno Wifi Rev.2, Nano Every
    #define RADIOLIB_PLATFORM                           "Arduino megaAVR"
    #define RADIOLIB_PIN_TYPE                           uint8_t
    #define RADIOLIB_PIN_MODE                           PinMode
    #define RADIOLIB_PIN_STATUS                         PinStatus
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint8_t pin, PinMode mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint8_t pin, PinStatus val)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (PinStatus, digitalRead, uint8_t pin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint8_t pin, void (*userFunc)(void), PinStatus mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint8_t pin)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long ms)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(ARDUINO_ARCH_APOLLO3)
    // Sparkfun Apollo3 boards
    #define RADIOLIB_PLATFORM                           "Sparkfun Apollo3"
    #define RADIOLIB_PIN_TYPE                           pin_size_t
    #define RADIOLIB_PIN_MODE                           Arduino_PinMode
    #define RADIOLIB_PIN_STATUS                         PinStatus
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // slow down SX126x/8x SPI on this platform
    #define RADIOLIB_SPI_SLOWDOWN

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, pin_size_t pinName, Arduino_PinMode pinMode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, pin_size_t pinName, PinStatus val)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (PinStatus, digitalRead, pin_size_t pinName)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t _pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t _pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, pin_size_t interruptNumber, voidFuncPtr callback, PinStatus mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, pin_size_t interruptNumber)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long ms)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(ARDUINO_ARDUINO_NANO33BLE)
    // Arduino Nano 33 BLE
    #define RADIOLIB_PLATFORM                           "Arduino Nano 33 BLE"
    #define RADIOLIB_PIN_TYPE                           pin_size_t
    #define RADIOLIB_PIN_MODE                           PinMode
    #define RADIOLIB_PIN_STATUS                         PinStatus
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino mbed OS boards have a really bad tone implementation which will crash after a couple seconds
    #define RADIOLIB_TONE_UNSUPPORTED
    #define RADIOLIB_MBED_TONE_OVERRIDE

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, pin_size_t pin, PinMode mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, pin_size_t pin, PinStatus val)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (PinStatus, digitalRead, pin_size_t pin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, pin_size_t interruptNum, voidFuncPtr func, PinStatus mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, pin_size_t interruptNum)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long ms)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_PORTENTA_H7_M4)
    // Arduino Portenta H7
    #define RADIOLIB_PLATFORM                           "Portenta H7"
    #define RADIOLIB_PIN_TYPE                           pin_size_t
    #define RADIOLIB_PIN_MODE                           PinMode
    #define RADIOLIB_PIN_STATUS                         PinStatus
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino mbed OS boards have a really bad tone implementation which will crash after a couple seconds
    #define RADIOLIB_TONE_UNSUPPORTED
    #define RADIOLIB_MBED_TONE_OVERRIDE

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, pin_size_t pin, PinMode mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, pin_size_t pin, PinStatus val)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (PinStatus, digitalRead, pin_size_t pin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, pin_size_t interruptNum, voidFuncPtr func, PinStatus mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, pin_size_t interruptNum)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long ms)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(__STM32F4__) || defined(__STM32F1__)
    // Arduino STM32 core by Roger Clark (https://github.com/rogerclarkmelbourne/Arduino_STM32)
    #define RADIOLIB_PLATFORM                           "STM32duino (unofficial)"
    #define RADIOLIB_PIN_TYPE                           uint8_t
    #define RADIOLIB_PIN_MODE                           WiringPinMode
    #define RADIOLIB_PIN_STATUS                         uint8_t
    #define RADIOLIB_INTERRUPT_STATUS                   ExtIntTriggerMode
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint8 pin, WiringPinMode mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint8 pin, uint8 val)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (uint32_t, digitalRead, uint8 pin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint32_t _pin, uint32_t frequency, uint32_t duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint32_t _pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint8 pin, voidFuncPtr handler, ExtIntTriggerMode mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint8 pin)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long ms)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, uint32 us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (uint32_t, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (uint32_t, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(ARDUINO_ARCH_MEGAAVR)
    // MegaCoreX by MCUdude (https://github.com/MCUdude/MegaCoreX)
    #define RADIOLIB_PLATFORM                           "MegaCoreX"
    #define RADIOLIB_PIN_TYPE                           uint8_t
    #define RADIOLIB_PIN_MODE                           uint8_t
    #define RADIOLIB_PIN_STATUS                         uint8_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint8_t pin, uint8_t mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint8_t pin, uint8_t value)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (uint8_t, digitalRead, uint8_t pin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t _pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t _pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint8_t pin, void (*userFunc)(void), uint8_t mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint8_t interruptNum)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long ms)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(ARDUINO_ARCH_RP2040)
    // Raspberry Pi Pico
    #define RADIOLIB_PLATFORM                           "Raspberry Pi Pico"
    #define RADIOLIB_PIN_TYPE                           pin_size_t
    #define RADIOLIB_PIN_MODE                           PinMode
    #define RADIOLIB_PIN_STATUS                         PinStatus
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino mbed OS boards have a really bad tone implementation which will crash after a couple seconds
    #define RADIOLIB_TONE_UNSUPPORTED
    #define RADIOLIB_MBED_TONE_OVERRIDE

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, pin_size_t pin, PinMode mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, pin_size_t pin, PinStatus val)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (PinStatus, digitalRead, pin_size_t pin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, pin_size_t interruptNum, voidFuncPtr func, PinStatus mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, pin_size_t interruptNum)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long ms)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #elif defined(__ASR6501__) || defined(ARDUINO_ARCH_ASR650X) || defined(DARDUINO_ARCH_ASR6601)
    // CubeCell
    #define RADIOLIB_PLATFORM                           "CubeCell"
    #define RADIOLIB_PIN_TYPE                           uint8_t
    #define RADIOLIB_PIN_MODE                           PINMODE
    #define RADIOLIB_PIN_STATUS                         uint8_t
    #define RADIOLIB_INTERRUPT_STATUS                   IrqModes
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint8_t pin, PINMODE mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint8_t pin_name, uint8_t level)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (uint8_t, digitalRead, uint8_t pin_name)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, void)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, void)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint8_t pin_name, GpioIrqHandler GpioIrqHandlerCallback, IrqModes interrupt_mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint8_t pin_name)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, uint32_t milliseconds)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, uint16 microseconds)
    #define RADIOLIB_CB_ARGS_MILLIS                     (uint32_t, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (uint32_t, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

    // provide an easy access to the on-board module
    #include "board-config.h"
    #define RADIOLIB_BUILTIN_MODULE                      RADIO_NSS, RADIO_DIO_1, RADIO_RESET, RADIO_BUSY

    // CubeCell doesn't seem to define nullptr, let's do something like that now
    #define nullptr                                     NULL

    // ... and also defines pinMode() as a macro, which is by far the stupidest thing I have seen on Arduino
    #undef pinMode

    // ... and uses an outdated GCC which does not support type aliases
    #define RADIOLIB_TYPE_ALIAS(type, alias)            typedef class type alias;

    // ... and it also has no tone(). This platform was designed by an idiot.
    #define RADIOLIB_TONE_UNSUPPORTED

    // ... AND as the (hopefully) final nail in the coffin, IT F*CKING DEFINES YIELD() AS A MACRO THAT DOES NOTHING!!!
    #define RADIOLIB_YIELD_UNSUPPORTED
    #if defined(yield)
    #undef yield
    #endif

#elif defined(RASPI)
    // RaspiDuino framework (https://github.com/me-no-dev/RasPiArduino)
    #define RADIOLIB_PLATFORM                           "RasPiArduino"
    #define RADIOLIB_PIN_TYPE                           uint8_t
    #define RADIOLIB_PIN_MODE                           uint8_t
    #define RADIOLIB_PIN_STATUS                         uint8_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint8_t pin, uint8_t mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint8_t pin, uint8_t value)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (int, digitalRead, uint8_t pin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t _pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t _pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint8_t interruptNum, void (*userFunc)(void), int mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint8_t interruptNum)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, uint32_t ms)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

    // let's start off easy - no tone on this platform, that can happen
    #define RADIOLIB_TONE_UNSUPPORTED

    // hmm, no yield either - weird on something like Raspberry PI, but sure, we can handle it
    #define RADIOLIB_YIELD_UNSUPPORTED

    // aight, getting to the juicy stuff - PGM_P seems missing, that's the first time
    #define PGM_P                                       const char *

    // ... and for the grand finale, we have millis() and micros() DEFINED AS MACROS!
    #if defined(millis)
    #undef millis
    inline unsigned long millis() { return((unsigned long)(STCV / 1000)); };
    #endif

    #if defined(micros)
    #undef micros
    inline unsigned long micros() { return((unsigned long)(STCV)); };
    #endif

  #else
    // other Arduino platforms not covered by the above list - this may or may not work
    #define RADIOLIB_PLATFORM                           "Unknown Arduino"
    #define RADIOLIB_UNKNOWN_PLATFORM
    #define RADIOLIB_PIN_TYPE                           uint8_t
    #define RADIOLIB_PIN_MODE                           uint8_t
    #define RADIOLIB_PIN_STATUS                         uint8_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_DEFAULT_SPI_SETTINGS               SPISettings(2000000, MSBFIRST, SPI_MODE0)
    #define RADIOLIB_NONVOLATILE                        PROGMEM
    #define RADIOLIB_NONVOLATILE_READ_BYTE(addr)        pgm_read_byte(addr)
    #define RADIOLIB_TYPE_ALIAS(type, alias)            using alias = type;

    // Arduino API callbacks
    #define RADIOLIB_CB_ARGS_PIN_MODE                   (void, pinMode, uint8_t pin, uint8_t mode)
    #define RADIOLIB_CB_ARGS_DIGITAL_WRITE              (void, digitalWrite, uint8_t pin, uint8_t value)
    #define RADIOLIB_CB_ARGS_DIGITAL_READ               (int, digitalRead, uint8_t pin)
    #define RADIOLIB_CB_ARGS_TONE                       (void, tone, uint8_t _pin, unsigned int frequency, unsigned long duration)
    #define RADIOLIB_CB_ARGS_NO_TONE                    (void, noTone, uint8_t _pin)
    #define RADIOLIB_CB_ARGS_ATTACH_INTERRUPT           (void, attachInterrupt, uint8_t interruptNum, void (*userFunc)(void), int mode)
    #define RADIOLIB_CB_ARGS_DETACH_INTERRUPT           (void, detachInterrupt, uint8_t interruptNum)
    #define RADIOLIB_CB_ARGS_YIELD                      (void, yield, void)
    #define RADIOLIB_CB_ARGS_DELAY                      (void, delay, unsigned long ms)
    #define RADIOLIB_CB_ARGS_DELAY_MICROSECONDS         (void, delayMicroseconds, unsigned int us)
    #define RADIOLIB_CB_ARGS_MILLIS                     (unsigned long, millis, void)
    #define RADIOLIB_CB_ARGS_MICROS                     (unsigned long, micros, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN                  (void, SPIbegin, void)
    #define RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION      (void, SPIbeginTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_TRANSFER               (uint8_t, SPItransfer, uint8_t b)
    #define RADIOLIB_CB_ARGS_SPI_END_TRANSACTION        (void, SPIendTransaction, void)
    #define RADIOLIB_CB_ARGS_SPI_END                    (void, SPIend, void)

  #endif
#endif

#else
  // generic non-Arduino platform
  #define RADIOLIB_PLATFORM                           "Generic"

  // platform properties may be defined here, or somewhere else in the build system

#endif

/*
 * Uncomment to enable debug output.
 * Warning: Debug output will slow down the whole system significantly.
 *          Also, it will result in larger compiled binary.
 * Levels: debug - only main info
 *         verbose - full transcript of all SPI communication
 */
#if !defined(RADIOLIB_DEBUG)
  //#define RADIOLIB_DEBUG
#endif
#if !defined(RADIOLIB_VERBOSE)
  //#define RADIOLIB_VERBOSE
#endif

// set which output port should be used for debug output
// may be Serial port (on Arduino) or file like stdout or stderr (on generic platforms)
#if !defined(RADIOLIB_DEBUG_PORT)
  #define RADIOLIB_DEBUG_PORT   Serial
#endif

/*
 * Uncomment to enable "paranoid" SPI mode
 * Every write to an SPI register using SPI set function will be verified by a subsequent read operation.
 * This improves reliablility, but slightly slows down communication.
 * Note: Enabled by default.
 */
#if !defined(RADIOLIB_SPI_PARANOID)
  #define RADIOLIB_SPI_PARANOID
#endif

/*
 * Uncomment to enable parameter range checking
 * RadioLib will check provided parameters (such as frequency) against limits determined by the device manufacturer.
 * It is highly advised to keep this macro defined, removing it will allow invalid values to be set,
 * possibly leading to bricked module and/or program crashing.
 * Note: Enabled by default.
 */
#if !defined(RADIOLIB_CHECK_PARAMS)
  #define RADIOLIB_CHECK_PARAMS
#endif

/*
 * Uncomment to enable SX127x errata fix
 * Warning: SX127x errata fix has been reported to cause issues with LoRa bandwidths lower than 62.5 kHz.
 *          It should only be enabled if you really are observing some errata-related issue.
 * Note: Disabled by default.
 */
#if !defined(RADIOLIB_FIX_ERRATA_SX127X)
  //#define RADIOLIB_FIX_ERRATA_SX127X
#endif

/*
 * Uncomment to enable god mode - all methods and member variables in all classes will be made public, thus making them accessible from Arduino code.
 * Warning: Come on, it's called GOD mode - obviously only use this if you know what you're doing.
 *          Failure to heed the above warning may result in bricked module.
 */
#if !defined(RADIOLIB_GODMODE)
  //#define RADIOLIB_GODMODE
#endif

/*
 * Uncomment to enable low-level hardware access
 * This will make some hardware methods like SPI get/set accessible from the user sketch - think of it as "god mode lite"
 * Warning: RadioLib won't stop you from writing invalid stuff into your device, so it's quite easy to brick your module with this.
 */
#if !defined(RADIOLIB_LOW_LEVEL)
  //#define RADIOLIB_LOW_LEVEL
#endif

/*
 * Uncomment to enable pre-defined modules when using RadioShield.
 */
#if !defined(RADIOLIB_RADIOSHIELD)
  //#define RADIOLIB_RADIOSHIELD
#endif

/*
 * Uncomment to enable static-only memory management: no dynamic allocation will be performed.
 * Warning: Large static arrays will be created in some methods. It is not advised to send large packets in this mode.
 */
#if !defined(RADIOLIB_STATIC_ONLY)
  //#define RADIOLIB_STATIC_ONLY
#endif

// set the size of static arrays to use
#if !defined(RADIOLIB_STATIC_ARRAY_SIZE)
  #define RADIOLIB_STATIC_ARRAY_SIZE   (256)
#endif

#if defined(RADIOLIB_DEBUG)
  #if defined(RADIOLIB_BUILD_ARDUINO)
    #define RADIOLIB_DEBUG_PRINT(...) { RADIOLIB_DEBUG_PORT.print(__VA_ARGS__); }
    #define RADIOLIB_DEBUG_PRINTLN(...) { RADIOLIB_DEBUG_PORT.println(__VA_ARGS__); }
  #else
    #if !defined(RADIOLIB_DEBUG_PRINT)
      #define RADIOLIB_DEBUG_PRINT(...) { frintf(RADIOLIB_DEBUG_PORT, __VA_ARGS__); }
    #endif
    #if !defined(RADIOLIB_DEBUG_PRINTLN)
      #define RADIOLIB_DEBUG_PRINTLN(...)  { printf(RADIOLIB_DEBUG_PORT, __VA_ARGS__ "\n"); }
    #endif
  #endif
#else
  #define RADIOLIB_DEBUG_PRINT(...) {}
  #define RADIOLIB_DEBUG_PRINTLN(...) {}
#endif

#if defined(RADIOLIB_VERBOSE)
  #define RADIOLIB_VERBOSE_PRINT(...) RADIOLIB_DEBUG_PRINT(__VA_ARGS__)
  #define RADIOLIB_VERBOSE_PRINTLN(...) RADIOLIB_DEBUG_PRINTLN(__VA_ARGS__)
#else
  #define RADIOLIB_VERBOSE_PRINT(...) {}
  #define RADIOLIB_VERBOSE_PRINTLN(...) {}
#endif

/*!
  \brief A simple assert macro, will return on error.
*/
#define RADIOLIB_ASSERT(STATEVAR) { if((STATEVAR) != RADIOLIB_ERR_NONE) { return(STATEVAR); } }

/*
 * Macros that create callback for the hardware abstraction layer.
 *
 * This is the most evil thing I have ever created. I am deeply sorry to anyone currently reading this text.
 * Come one, come all and witness the horror:
 * Variadics, forced expansions, inlined function, string concatenation, and it even messes up access specifiers.
 */
#define RADIOLIB_FIRST(arg, ...) arg
#define RADIOLIB_REST(arg, ...) __VA_ARGS__
#define RADIOLIB_EXP(...) __VA_ARGS__

#define RADIOLIB_GENERATE_CALLBACK_RET_FUNC(RET, FUNC, ...) public: typedef RET (*FUNC##_cb_t)(__VA_ARGS__); void setCb_##FUNC(FUNC##_cb_t cb) { cb_##FUNC = cb; }; private: FUNC##_cb_t cb_##FUNC;
#define RADIOLIB_GENERATE_CALLBACK_RET(RET, ...) RADIOLIB_GENERATE_CALLBACK_RET_FUNC(RET, __VA_ARGS__)
#define RADIOLIB_GENERATE_CALLBACK(CB) RADIOLIB_GENERATE_CALLBACK_RET(RADIOLIB_EXP(RADIOLIB_FIRST CB), RADIOLIB_EXP(RADIOLIB_REST CB))

#define RADIOLIB_GENERATE_CALLBACK_SPI_RET_FUNC(RET, FUNC, ...) public: typedef RET (Module::*FUNC##_cb_t)(__VA_ARGS__); void setCb_##FUNC(FUNC##_cb_t cb) { cb_##FUNC = cb; }; private: FUNC##_cb_t cb_##FUNC;
#define RADIOLIB_GENERATE_CALLBACK_SPI_RET(RET, ...) RADIOLIB_GENERATE_CALLBACK_SPI_RET_FUNC(RET, __VA_ARGS__)
#define RADIOLIB_GENERATE_CALLBACK_SPI(CB) RADIOLIB_GENERATE_CALLBACK_SPI_RET(RADIOLIB_EXP(RADIOLIB_FIRST CB), RADIOLIB_EXP(RADIOLIB_REST CB))

/*!
  \brief Macro to check variable is within constraints - this is commonly used to check parameter ranges. Requires RADIOLIB_CHECK_RANGE to be enabled
*/
#if defined(RADIOLIB_CHECK_PARAMS)
  #define RADIOLIB_CHECK_RANGE(VAR, MIN, MAX, ERR) { if(!(((VAR) >= (MIN)) && ((VAR) <= (MAX)))) { return(ERR); } }
#else
  #define RADIOLIB_CHECK_RANGE(VAR, MIN, MAX, ERR) {}
#endif

#if defined(RADIOLIB_FIX_ERRATA_SX127X)
  #define RADIOLIB_ERRATA_SX127X(...) { errataFix(__VA_ARGS__); }
#else
  #define RADIOLIB_ERRATA_SX127X(...) {}
#endif

// version definitions
#define RADIOLIB_VERSION_MAJOR  (0x05)
#define RADIOLIB_VERSION_MINOR  (0x01)
#define RADIOLIB_VERSION_PATCH  (0x02)
#define RADIOLIB_VERSION_EXTRA  (0x00)

#define RADIOLIB_VERSION ((RADIOLIB_VERSION_MAJOR << 24) | (RADIOLIB_VERSION_MINOR << 16) | (RADIOLIB_VERSION_PATCH << 8) | (RADIOLIB_VERSION_EXTRA))

#endif
