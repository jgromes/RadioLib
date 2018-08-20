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
  #define DEBUG_PRINT_BIN(x)            Serial.print (x, BIN)
  #define DEBUG_PRINTLN_BIN(x)          Serial.println (x, BIN)
  #define DEBUG_PRINT_DEC(x)            Serial.print (x, DEC)
  #define DEBUG_PRINTLN_DEC(x)          Serial.println (x, DEC)
  #define DEBUG_PRINT_HEX(x)            Serial.print (x, HEX)
  #define DEBUG_PRINTLN_HEX(x)          Serial.println (x, HEX)
  #define DEBUG_PRINTLN(x)              Serial.println (x)
  #define DEBUG_PRINT_STR(x)            Serial.print (F(x))
  #define DEBUG_PRINTLN_STR(x)          Serial.println (F(x))
#else
  #define DEBUG_BEGIN(x)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINT_BIN(x)
  #define DEBUG_PRINTLN_BIN(x)
  #define DEBUG_PRINT_DEC(x)
  #define DEBUG_PRINTLN_DEC(x)
  #define DEBUG_PRINT_HEX(x)
  #define DEBUG_PRINTLN_HEX(x)
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
#define ERR_NONE                               0
#define ERR_UNKNOWN                            -1

// SX1278/SX1272/RF69 status codes
#define ERR_CHIP_NOT_FOUND                    -2
#define ERR_EEPROM_NOT_INITIALIZED            -3
#define ERR_PACKET_TOO_LONG                   -4
#define ERR_TX_TIMEOUT                        -5
#define ERR_RX_TIMEOUT                        -6
#define ERR_CRC_MISMATCH                      -7
#define ERR_INVALID_BANDWIDTH                 -8
#define ERR_INVALID_SPREADING_FACTOR          -9
#define ERR_INVALID_CODING_RATE               -10
#define ERR_INVALID_BIT_RANGE                 -11
#define ERR_INVALID_FREQUENCY                 -12
#define ERR_INVALID_OUTPUT_POWER              -13
#define PREAMBLE_DETECTED                     -14
#define CHANNEL_FREE                          -15
#define ERR_SPI_WRITE_FAILED                  -16
#define ERR_INVALID_CURRENT_LIMIT             -17
#define ERR_INVALID_PREAMBLE_LENGTH           -18
#define ERR_INVALID_GAIN                      -19
#define ERR_WRONG_MODEM                       -20
#define ERR_INVALID_RTTY_SHIFT                -21

// RF69-specific status codes
#define ERR_INVALID_BIT_RATE                  -101
#define ERR_INVALID_FREQUENCY_DEVIATION       -102
#define ERR_INVALID_BIT_RATE_BW_RATIO         -103
#define ERR_INVALID_RX_BANDWIDTH              -104
#define ERR_INVALID_SYNC_WORD                 -105

// ESP8266 status codes
#define ERR_AT_FAILED                         -201
#define ERR_URL_MALFORMED                     -202
#define ERR_RESPONSE_MALFORMED_AT             -203
#define ERR_RESPONSE_MALFORMED                -204
#define ERR_MQTT_CONN_VERSION_REJECTED        -205
#define ERR_MQTT_CONN_ID_REJECTED             -206
#define ERR_MQTT_CONN_SERVER_UNAVAILABLE      -207
#define ERR_MQTT_CONN_BAD_USERNAME_PASSWORD   -208
#define ERR_MQTT_CONN_NOT_AUTHORIZED          -208
#define ERR_MQTT_UNEXPECTED_PACKET_ID         -209
#define ERR_MQTT_NO_NEW_PACKET_AVAILABLE      -210
#define MQTT_SUBS_SUCCESS_QOS_0               0x00
#define MQTT_SUBS_SUCCESS_QOS_1               0x01
#define MQTT_SUBS_SUCCESS_QOS_2               0x02
#define ERR_MQTT_SUBS_FAILED                  0x80

// XBee status codes
#define ERR_CMD_MODE_FAILED                   -301
#define ERR_FRAME_MALFORMED                   -302
#define ERR_FRAME_INCORRECT_CHECKSUM          -303
#define ERR_FRAME_UNEXPECTED_ID               -304

#endif
