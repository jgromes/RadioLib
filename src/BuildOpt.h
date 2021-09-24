#if !defined(_RADIOLIB_BUILD_OPTIONS_H)
#define _RADIOLIB_BUILD_OPTIONS_H

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #error "Unsupported Arduino version (< 1.0.0)"
#endif

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
 * RADIOLIB_PROGMEM - macro to place variable into program storage (usually Flash).
 * RADIOLIB_PROGMEM_READ_BYTE -  function/macro to read variables saved in program storage (usually Flash).
 * RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED - defined if the specific platform does not support SoftwareSerial.
 * RADIOLIB_HARDWARE_SERIAL_PORT - which hardware serial port should be used on platform that do not have SoftwareSerial support.
 * RADIOLIB_TONE_UNSUPPORTED - some platforms do not have tone()/noTone(), which is required for AFSK.
 *
 * In addition, some platforms may require RadioLib to disable specific drivers (such as ESP8266).
 *
 * Users may also specify their own configuration by uncommenting the RADIOLIB_CUSTOM_PLATFORM,
 * and then specifying all platform parameters in the section below. This will override automatic
 * platform detection.
 */

// uncomment to enable custom platform definition
//#define RADIOLIB_CUSTOM_PLATFORM

#if defined(RADIOLIB_CUSTOM_PLATFORM)
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
  #define RADIOLIB_PROGMEM                            PROGMEM
  #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)

  // the following must be defined if the Arduino core does not support SoftwareSerial library
  //#define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  //#define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

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
  //#define RADIOLIB_EXCLUDE_ESP8266
  //#define RADIOLIB_EXCLUDE_HC05
  //#define RADIOLIB_EXCLUDE_JDY08
  //#define RADIOLIB_EXCLUDE_NRF24
  //#define RADIOLIB_EXCLUDE_RF69
  //#define RADIOLIB_EXCLUDE_SX1231     // dependent on RADIOLIB_EXCLUDE_RF69
  //#define RADIOLIB_EXCLUDE_SI443X
  //#define RADIOLIB_EXCLUDE_RFM2X      // dependent on RADIOLIB_EXCLUDE_SI443X
  //#define RADIOLIB_EXCLUDE_SX127X
  //#define RADIOLIB_EXCLUDE_RFM9X      // dependent on RADIOLIB_EXCLUDE_SX127X
  //#define RADIOLIB_EXCLUDE_SX126X
  //#define RADIOLIB_EXCLUDE_SX128X
  //#define RADIOLIB_EXCLUDE_XBEE
  //#define RADIOLIB_EXCLUDE_AFSK
  //#define RADIOLIB_EXCLUDE_AX25
  //#define RADIOLIB_EXCLUDE_HELLSCHREIBER
  //#define RADIOLIB_EXCLUDE_HTTP
  //#define RADIOLIB_EXCLUDE_MORSE
  //#define RADIOLIB_EXCLUDE_MQTT
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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)

    // RadioLib has ESP8266 driver, this must be disabled to use ESP8266 as platform
    #define RADIOLIB_EXCLUDE_ESP8266
    #define RADIOLIB_EXCLUDE_HTTP

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)
    #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
    #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

    // ESP32 doesn't support tone(), but it can be emulated via LED control peripheral
    #define RADIOLIB_TONE_UNSUPPORTED
    #define RADIOLIB_TONE_ESP32_CHANNEL                 (1)

  #elif defined(ARDUINO_ARCH_STM32)
    // official STM32 Arduino core (https://github.com/stm32duino/Arduino_Core_STM32)
    #define RADIOLIB_PLATFORM                           "Arduino STM32 (official)"
    #define RADIOLIB_PIN_TYPE                           uint32_t
    #define RADIOLIB_PIN_MODE                           uint32_t
    #define RADIOLIB_PIN_STATUS                         uint32_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFFFFFFFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)
    #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
    #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

    // slow down SX126x/8x SPI on this platform
    #define RADIOLIB_SPI_SLOWDOWN

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)
    #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
    #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

    // slow down SX126x/8x SPI on this platform
    #define RADIOLIB_SPI_SLOWDOWN

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)
    #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
    #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)
    #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
    #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1
    #define RADIOLIB_TONE_UNSUPPORTED

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)
    #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
    #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

    // Apollo3 uses mbed libraries, which already contain ESP8266 driver
    #define RADIOLIB_EXCLUDE_ESP8266

    // slow down SX126x/8x SPI on this platform
    #define RADIOLIB_SPI_SLOWDOWN

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)
    #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
    #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

    // Nano 33 BLE uses mbed libraries, which already contain ESP8266 driver
    #define RADIOLIB_EXCLUDE_ESP8266

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)
    #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
    #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

    // Arduino Portenta H7 uses mbed libraries, which already contain ESP8266 driver
    #define RADIOLIB_EXCLUDE_ESP8266

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)
    #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
    #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)

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
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)
    #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
    #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1
    #define RADIOLIB_EXCLUDE_ESP8266

  #else
    // other platforms not covered by the above list - this may or may not work
    #define RADIOLIB_PLATFORM                           "Unknown"
    #define RADIOLIB_UNKNOWN_PLATFORM
    #define RADIOLIB_PIN_TYPE                           uint8_t
    #define RADIOLIB_PIN_MODE                           uint8_t
    #define RADIOLIB_PIN_STATUS                         uint8_t
    #define RADIOLIB_INTERRUPT_STATUS                   RADIOLIB_PIN_STATUS
    #define RADIOLIB_DIGITAL_PIN_TO_INTERRUPT(p)        digitalPinToInterrupt(p)
    #define RADIOLIB_NC                                 (0xFF)
    #define RADIOLIB_DEFAULT_SPI                        SPI
    #define RADIOLIB_PROGMEM                            PROGMEM
    #define RADIOLIB_PROGMEM_READ_BYTE(addr)            pgm_read_byte(addr)

  #endif
#endif

/*
 * Uncomment to enable debug output.
 * Warning: Debug output will slow down the whole system significantly.
 *          Also, it will result in larger compiled binary.
 * Levels: debug - only main info
 *         verbose - full transcript of all SPI/UART communication
 */

//#define RADIOLIB_DEBUG
//#define RADIOLIB_VERBOSE

// set which Serial port should be used for debug output
#define RADIOLIB_DEBUG_PORT   Serial

#if defined(RADIOLIB_DEBUG)
  #define RADIOLIB_DEBUG_PRINT(...) { RADIOLIB_DEBUG_PORT.print(__VA_ARGS__); }
  #define RADIOLIB_DEBUG_PRINTLN(...) { RADIOLIB_DEBUG_PORT.println(__VA_ARGS__); }
#else
  #define RADIOLIB_DEBUG_PRINT(...) {}
  #define RADIOLIB_DEBUG_PRINTLN(...) {}
#endif

#if defined(RADIOLIB_VERBOSE)
  #define RADIOLIB_VERBOSE_PRINT(...) { RADIOLIB_DEBUG_PORT.print(__VA_ARGS__); }
  #define RADIOLIB_VERBOSE_PRINTLN(...) { RADIOLIB_DEBUG_PORT.println(__VA_ARGS__); }
#else
  #define RADIOLIB_VERBOSE_PRINT(...) {}
  #define RADIOLIB_VERBOSE_PRINTLN(...) {}
#endif

/*
 * Uncomment to enable "paranoid" SPI mode
 * Every write to an SPI register using SPI set function will be verified by a subsequent read operation.
 * This improves reliablility, but slightly slows down communication.
 * Note: Enabled by default.
 */
#define RADIOLIB_SPI_PARANOID

/*
 * Uncomment to enable parameter range checking
 * RadioLib will check provided parameters (such as frequency) against limits determined by the device manufacturer.
 * It is highly advised to keep this macro defined, removing it will allow invalid values to be set,
 * possibly leading to bricked module and/or program crashing.
 * Note: Enabled by default.
 */
#define RADIOLIB_CHECK_PARAMS

/*
 * Uncomment to enable god mode - all methods and member variables in all classes will be made public, thus making them accessible from Arduino code.
 * Warning: Come on, it's called GOD mode - obviously only use this if you know what you're doing.
 *          Failure to heed the above warning may result in bricked module.
 */
//#define RADIOLIB_GODMODE

/*
 * Uncomment to enable low-level hardware access
 * This will make some hardware methods like SPI get/set accessible from the user sketch - think of it as "god mode lite"
 * Warning: RadioLib won't stop you from writing invalid stuff into your device, so it's quite easy to brick your module with this.
 */
//#define RADIOLIB_LOW_LEVEL

/*
 * Uncomment to enable pre-defined modules when using RadioShield.
 */
//#define RADIOLIB_RADIOSHIELD

/*
 * Uncomment to enable static-only memory management: no dynamic allocation will be performed.
 * Warning: Large static arrays will be created in some methods. It is not advised to send large packets in this mode.
 */
//#define RADIOLIB_STATIC_ONLY

// set the size of static arrays to use
#if !defined(RADIOLIB_STATIC_ARRAY_SIZE)
#define RADIOLIB_STATIC_ARRAY_SIZE   256
#endif

/*!
  \brief A simple assert macro, will return on error.
*/
#define RADIOLIB_ASSERT(STATEVAR) { if((STATEVAR) != ERR_NONE) { return(STATEVAR); } }

/*!
  \brief Macro to check variable is within constraints - this is commonly used to check parameter ranges. Requires RADIOLIB_CHECK_RANGE to be enabled
*/
#if defined(RADIOLIB_CHECK_PARAMS)
#define RADIOLIB_CHECK_RANGE(VAR, MIN, MAX, ERR) { if(!(((VAR) >= (MIN)) && ((VAR) <= (MAX)))) { return(ERR); } }
#else
#define RADIOLIB_CHECK_RANGE(VAR, MIN, MAX, ERR) {}
#endif

// version definitions
#define RADIOLIB_VERSION_MAJOR  (0x04)
#define RADIOLIB_VERSION_MINOR  (0x05)
#define RADIOLIB_VERSION_PATCH  (0x00)
#define RADIOLIB_VERSION_EXTRA  (0x00)

#define RADIOLIB_VERSION ((RADIOLIB_VERSION_MAJOR << 24) | (RADIOLIB_VERSION_MINOR << 16) | (RADIOLIB_VERSION_PATCH << 8) | (RADIOLIB_VERSION_EXTRA))

#endif
