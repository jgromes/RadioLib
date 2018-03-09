#ifndef _KITELIB_TYPES_H
#define _KITELIB_TYPES_H

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

//#define DEBUG

// Shield configuration
#define USE_SPI                         0x00
#define USE_UART                        0x01
#define USE_I2C                         0x02
#define INT_NONE                        0x00
#define INT_0                           0x01
#define INT_1                           0x02
#define INT_BOTH                        0x03

// UART configuration
#define UART_STOPBIT_1                  0x01
#define UART_STOPBIT_1_5                0x02
#define UART_STOPBIT_2                  0x03
#define UART_PARITY_NONE                0x00
#define UART_PARITY_ODD                 0x01
#define UART_PARITY_EVEN                0x02
#define UART_FLOW_NONE                  0x00
#define UART_FLOW_RTS                   0x01
#define UART_FLOW_CTS                   0x02
#define UART_FLOW_BOTH                  0x03

// Common error codes
#define ERR_NONE                        0x00
#define ERR_UNKNOWN                     0x63  // maximum error code value is 99, so that it does not interfere with HTTP status codes

// SX1278/SX1272 error codes
#define ERR_CHIP_NOT_FOUND              0x01
#define ERR_EEPROM_NOT_INITIALIZED      0x02
#define ERR_PACKET_TOO_LONG             0x03
#define ERR_TX_TIMEOUT                  0x04
#define ERR_RX_TIMEOUT                  0x05
#define ERR_CRC_MISMATCH                0x06
#define ERR_INVALID_BANDWIDTH           0x07
#define ERR_INVALID_SPREADING_FACTOR    0x08
#define ERR_INVALID_CODING_RATE         0x09
#define ERR_INVALID_BIT_RANGE           0x10

// ESP8266 error codes
#define ERR_AT_FAILED                   0x01
#define ERR_URL_MALFORMED               0x02
#define ERR_RESPONSE_MALFORMED_AT       0x03
#define ERR_RESPONSE_MALFORMED          0x04


enum Slot {SlotA, SlotB};

enum Bandwidth {BW_7_80_KHZ, BW_10_40_KHZ, BW_15_60_KHZ, BW_20_80_KHZ, BW_31_25_KHZ, BW_41_70_KHZ, BW_62_50_KHZ, BW_125_00_KHZ, BW_250_00_KHZ, BW_500_00_KHZ};
enum SpreadingFactor {SF_6, SF_7, SF_8, SF_9, SF_10, SF_11, SF_12};
enum CodingRate {CR_4_5, CR_4_6, CR_4_7, CR_4_8};

#endif
