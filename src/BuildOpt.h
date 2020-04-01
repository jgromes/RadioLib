#ifndef _RADIOLIB_BUILD_OPTIONS_H
#define _RADIOLIB_BUILD_OPTIONS_H

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #error "Unsupported Arduino version (< 1.0.0)"
#endif

/*
 * Platform-specific configuration.
 *
 * RADIOLIB_PIN_TYPE - which type should be used for pin numbers in functions like digitalRead().
 * RADIOLIB_PIN_MODE - which type should be used for pin modes in functions like pinMode().
 * RADIOLIB_PIN_STATUS - which type should be used for pin values in functions like digitalWrite().
 * RADIOLIB_NC - alias for unused pin, usually the largest possible value of RADIOLIB_PIN_TYPE.
 * RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED - defined if the specific platform does not support SoftwareSerial.
 * RADIOLIB_HARDWARE_SERIAL_PORT - which hardware serial port should be used on platform that do not have SoftwareSerial support.
 *
 * In addition, some platforms may require RadioLib to disable specific drivers (such as ESP8266).
 */
#if defined(__AVR__) && !(defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_AVR_NANO_EVERY))
   // Arduino AVR boards (except for megaAVR) - Uno, Mega etc.
   #define RADIOLIB_PIN_TYPE                          uint8_t
   #define RADIOLIB_PIN_MODE                          uint8_t
   #define RADIOLIB_PIN_STATUS                        uint8_t
   #define RADIOLIB_NC                                (0xFF)

#elif defined(ESP8266)
   // ESP8266 boards
   #define RADIOLIB_PIN_TYPE                          uint8_t
   #define RADIOLIB_PIN_MODE                          uint8_t
   #define RADIOLIB_PIN_STATUS                        uint8_t
   #define RADIOLIB_NC                                (0xFF)

   // RadioLib has ESPS8266 driver, this must be disabled to use ESP8266 as platform
   #define _RADIOLIB_ESP8266_H

#elif defined(ESP32)
  // ESP32 boards
  #define RADIOLIB_PIN_TYPE                           uint8_t
  #define RADIOLIB_PIN_MODE                           uint8_t
  #define RADIOLIB_PIN_STATUS                         uint8_t
  #define RADIOLIB_NC                                 (0xFF)
  #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

#elif defined(ARDUINO_ARCH_STM32)
  // STM32duino boards
  #define RADIOLIB_PIN_TYPE                           uint32_t
  #define RADIOLIB_PIN_MODE                           uint32_t
  #define RADIOLIB_PIN_STATUS                         uint32_t
  #define RADIOLIB_NC                                 (0xFFFFFFFF)
  #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

#elif defined(SAMD_SERIES)
  // Arduino SAMD boards - Zero, MKR, etc.
  #define RADIOLIB_PIN_TYPE                           uint32_t
  #define RADIOLIB_PIN_MODE                           uint32_t
  #define RADIOLIB_PIN_STATUS                         uint32_t
  #define RADIOLIB_NC                                 (0xFFFFFFFF)
  #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

#elif defined(__SAM3X8E__)
  // Arduino Due
  #define RADIOLIB_PIN_TYPE                           uint32_t
  #define RADIOLIB_PIN_MODE                           uint32_t
  #define RADIOLIB_PIN_STATUS                         uint32_t
  #define RADIOLIB_NC                                 (0xFFFFFFFF)
  #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

#elif (defined(NRF52832_XXAA) || defined(NRF52840_XXAA)) && !defined(ARDUINO_ARDUINO_NANO33BLE)
  // Adafruit nRF52 boards
  #define RADIOLIB_PIN_TYPE                           uint32_t
  #define RADIOLIB_PIN_MODE                           uint32_t
  #define RADIOLIB_PIN_STATUS                         uint32_t
  #define RADIOLIB_NC                                 (0xFFFFFFFF)

#elif defined(ARDUINO_ARC32_TOOLS)
  // Intel Curie
  #define RADIOLIB_PIN_TYPE                           uint8_t
  #define RADIOLIB_PIN_MODE                           uint8_t
  #define RADIOLIB_PIN_STATUS                         uint8_t
  #define RADIOLIB_NC                                 (0xFF)

#elif defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_AVR_NANO_EVERY)
  // Arduino megaAVR boards - Uno Wifi Rev.2, Nano Every
  #define RADIOLIB_PIN_TYPE                           uint8_t
  #define RADIOLIB_PIN_MODE                           PinMode
  #define RADIOLIB_PIN_STATUS                         PinStatus
  #define RADIOLIB_NC                                 (0xFF)

#elif defined(AM_PART_APOLLO3)
  // Sparkfun Artemis boards
  #define RADIOLIB_PIN_TYPE                           uint8_t
  #define RADIOLIB_PIN_MODE                           uint8_t
  #define RADIOLIB_PIN_STATUS                         uint8_t
  #define RADIOLIB_NC                                 (0xFF)
  #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

#elif defined(ARDUINO_ARDUINO_NANO33BLE)
  // Arduino Nano 33 BLE
  #define RADIOLIB_PIN_TYPE                           pin_size_t
  #define RADIOLIB_PIN_MODE                           PinMode
  #define RADIOLIB_PIN_STATUS                         PinStatus
  #define RADIOLIB_NC                                 (0xFF)
  #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  #define RADIOLIB_HARDWARE_SERIAL_PORT               Serial1

  // Nano 33 BLE uses mbed libraries, which already contain ESP8266 driver
  #define _RADIOLIB_ESP8266_H

#else
  // other platforms not covered by the above list - this may or may not work
  #define RADIOLIB_UNKNOWN_PLATFORM
  #define RADIOLIB_PIN_TYPE                           uint8_t
  #define RADIOLIB_PIN_MODE                           uint8_t
  #define RADIOLIB_PIN_STATUS                         uint8_t
  #define RADIOLIB_NC                                 (0xFF)

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

#ifdef RADIOLIB_DEBUG
  #define RADIOLIB_DEBUG_PRINT(...) { RADIOLIB_DEBUG_PORT.print(__VA_ARGS__); }
  #define RADIOLIB_DEBUG_PRINTLN(...) { RADIOLIB_DEBUG_PORT.println(__VA_ARGS__); }
#else
  #define RADIOLIB_DEBUG_PRINT(...) {}
  #define RADIOLIB_DEBUG_PRINTLN(...) {}
#endif

#ifdef RADIOLIB_VERBOSE
  #define RADIOLIB_VERBOSE_PRINT(...) { RADIOLIB_DEBUG_PORT.print(__VA_ARGS__); }
  #define RADIOLIB_VERBOSE_PRINTLN(...) { RADIOLIB_DEBUG_PORT.println(__VA_ARGS__); }
#else
  #define RADIOLIB_VERBOSE_PRINT(...) {}
  #define RADIOLIB_VERBOSE_PRINTLN(...) {}
#endif

/*
 * Uncomment to enable god mode - all methods and member variables in all classes will be made public, thus making them accessible from Arduino code.
 * Warning: Come on, it's called GOD mode - obviously only use this if you know what you're doing.
 *          Failure to heed the above warning may result in bricked module.
 */
//#define RADIOLIB_GODMODE

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
#define RADIOLIB_STATIC_ARRAY_SIZE   256

/*!
  \brief A simple assert macro, will return on error.
*/
#define RADIOLIB_ASSERT(STATEVAR) { if((STATEVAR) != ERR_NONE) { return(STATEVAR); } }

/*!
  \brief Macro to check variable is within constraints - this is commonly used to check parameter ranges.
*/
#define RADIOLIB_CHECK_RANGE(VAR, MIN, MAX, ERR) { if(!(((VAR) >= (MIN)) && ((VAR) <= (MAX)))) { return(ERR); } }


// version definitions
#define RADIOLIB_VERSION_MAJOR  (0x03)
#define RADIOLIB_VERSION_MINOR  (0x04)
#define RADIOLIB_VERSION_PATCH  (0x00)
#define RADIOLIB_VERSION_EXTRA  (0x00)

#define RADIOLIB_VERSION ((RADIOLIB_VERSION_MAJOR << 24) | (RADIOLIB_VERSION_MINOR << 16) | (RADIOLIB_VERSION_PATCH << 8) | (RADIOLIB_VERSION_EXTRA))

#endif
