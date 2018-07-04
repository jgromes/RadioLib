#ifndef _KITELIB_TYPES_H
#define _KITELIB_TYPES_H

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

//#define KITELIB_DEBUG

#ifdef KITELIB_DEBUG
  #define DEBUG_BEGIN(x)                Serial.begin (x)
  #define DEBUG_PRINT(x)                Serial.print (x)
  #define DEBUG_PRINT_DEC(x)            Serial.print (x, DEC)
  #define DEBUG_PRINT_HEX(x)            Serial.print (x, HEX)
  #define DEBUG_PRINTLN(x)              Serial.println (x)
  #define DEBUG_PRINT_STR(x)            Serial.print (F(x))
  #define DEBUG_PRINTLN_STR(x)          Serial.println (F(x))
#else
  #define DEBUG_BEGIN(x)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINT_DEC(x)
  #define DEBUG_PRINT_HEX(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINT_STR(x)
  #define DEBUG_PRINTLN_STR(x)
#endif

// Shield configuration
#define USE_SPI                               0x00
#define USE_UART                              0x01
#define USE_I2C                               0x02
#define INT_NONE                              0x00
#define INT_0                                 0x01
#define INT_1                                 0x02
#define INT_BOTH                              0x03

// UART configuration
#define UART_STOPBIT_1                        0x01
#define UART_STOPBIT_1_5                      0x02
#define UART_STOPBIT_2                        0x03
#define UART_PARITY_NONE                      0x00
#define UART_PARITY_ODD                       0x01
#define UART_PARITY_EVEN                      0x02
#define UART_FLOW_NONE                        0x00
#define UART_FLOW_RTS                         0x01
#define UART_FLOW_CTS                         0x02
#define UART_FLOW_BOTH                        0x03

// Common status codes
#define ERR_NONE                              0x00
#define ERR_UNKNOWN                           0x63  // maximum status code value is 99 DEC, so that it does not interfere with HTTP status codes

// SX1278/SX1272/RF69 status codes
#define ERR_CHIP_NOT_FOUND                    0x01
#define ERR_EEPROM_NOT_INITIALIZED            0x02
#define ERR_PACKET_TOO_LONG                   0x03
#define ERR_TX_TIMEOUT                        0x04
#define ERR_RX_TIMEOUT                        0x05
#define ERR_CRC_MISMATCH                      0x06
#define ERR_INVALID_BANDWIDTH                 0x07
#define ERR_INVALID_SPREADING_FACTOR          0x08
#define ERR_INVALID_CODING_RATE               0x09
#define ERR_INVALID_BIT_RANGE                 0x0A
#define ERR_INVALID_FREQUENCY                 0x0B
#define ERR_INVALID_OUTPUT_POWER              0x0C
#define PREAMBLE_DETECTED                     0x0D
#define CHANNEL_FREE                          0x0E
#define ERR_INVALID_BIT_RATE                  0x0F

// ESP8266 status codes
#define ERR_AT_FAILED                         0x01
#define ERR_URL_MALFORMED                     0x02
#define ERR_RESPONSE_MALFORMED_AT             0x03
#define ERR_RESPONSE_MALFORMED                0x04
#define ERR_MQTT_CONN_VERSION_REJECTED        0x05
#define ERR_MQTT_CONN_ID_REJECTED             0x06
#define ERR_MQTT_CONN_SERVER_UNAVAILABLE      0x07
#define ERR_MQTT_CONN_BAD_USERNAME_PASSWORD   0x08
#define ERR_MQTT_CONN_NOT_AUTHORIZED          0x09

// XBee status codes
#define ERR_CMD_MODE_FAILED                   0x02

#endif
