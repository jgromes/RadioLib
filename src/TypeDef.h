#ifndef _RADIOLIB_TYPES_H
#define _RADIOLIB_TYPES_H

#include "BuildOpt.h"

/*!
  \defgroup shield_config Shield Configuration

  \{
*/

/*!
  \brief Use SPI interface.
*/
#define RADIOLIB_USE_SPI                              0x00

/*!
  \brief Use UART interface.
*/
#define RADIOLIB_USE_UART                             0x01

/*!
  \brief Use I2C interface.
*/
#define RADIOLIB_USE_I2C                              0x02

/*!
  \}
*/

/*!
  \defgroup uart_config UART Configuration

  \{
*/

/*!
  \brief Use 1 bit stop.
*/
#define RADIOLIB_UART_STOPBIT_1                       0x01

/*!
  \brief Use 1.5 bit stop.
*/
#define RADIOLIB_UART_STOPBIT_1_5                     0x02

/*!
  \brief Use 2 bit stop.
*/
#define RADIOLIB_UART_STOPBIT_2                       0x03

/*!
  \brief No parity.
*/
#define RADIOLIB_UART_PARITY_NONE                     0x00

/*!
  \brief Odd parity.
*/
#define RADIOLIB_UART_PARITY_ODD                      0x01

/*!
  \brief Even parity.
*/
#define RADIOLIB_UART_PARITY_EVEN                     0x02

/*!
  \brief No flow control.
*/
#define RADIOLIB_UART_FLOW_NONE                       0x00

/*!
  \brief RTS only.
*/
#define RADIOLIB_UART_FLOW_RTS                        0x01

/*!
  \brief CTS only.
*/
#define RADIOLIB_UART_FLOW_CTS                        0x02

/*!
  \brief Both RTS and CTS.
*/
#define RADIOLIB_UART_FLOW_BOTH                       0x03

/*!
  \}
*/

/*!
  \defgroup config_shaping Data shaping filter values aliases.

  \{
*/

/*!
  \brief No shaping.
*/
#define RADIOLIB_SHAPING_NONE                         0x00

/*!
  \brief Gaussin shaping filter, BT = 0.3
*/
#define RADIOLIB_SHAPING_0_3                          0x01

/*!
  \brief Gaussin shaping filter, BT = 0.5
*/
#define RADIOLIB_SHAPING_0_5                          0x02

/*!
  \brief Gaussin shaping filter, BT = 0.7
*/
#define RADIOLIB_SHAPING_0_7                          0x03

/*!
  \brief Gaussin shaping filter, BT = 1.0
*/
#define RADIOLIB_SHAPING_1_0                          0x04

/*!
  \}
*/

/*!
  \defgroup config_encoding Encoding type aliases.

  \{
*/

/*!
  \brief Non-return to zero - no encoding.
*/
#define RADIOLIB_ENCODING_NRZ                         0x00

/*!
  \brief Manchester encoding.
*/
#define RADIOLIB_ENCODING_MANCHESTER                  0x01

/*!
  \brief Whitening.
*/
#define RADIOLIB_ENCODING_WHITENING                   0x02

/*!
  \}
*/

/*!
  \defgroup status_codes Status Codes

  \{
*/

// common status codes

/*!
  \brief No error, method executed successfully.
*/
#define ERR_NONE                                      0

/*!
  \brief There was an unexpected, unknown error. If you see this, something went incredibly wrong.
  Your Arduino may be possessed, contact your local exorcist to resolve this error.
*/
#define ERR_UNKNOWN                                   -1

// SX127x/RFM9x status codes

/*!
  \brief Radio chip was not found during initialization. This can be caused by specifying wrong chip type in the constructor
  (i.e. calling SX1272 constructor for SX1278 chip) or by a fault in your wiring (incorrect slave select pin).
*/
#define ERR_CHIP_NOT_FOUND                            -2

/*!
  \brief Failed to allocate memory for temporary buffer. This can be cause by not enough RAM or by passing invalid pointer.
*/
#define ERR_MEMORY_ALLOCATION_FAILED                  -3

/*!
  \brief Packet supplied to transmission method was longer than limit.
*/
#define ERR_PACKET_TOO_LONG                           -4

/*!
  \brief Timed out waiting for transmission finish.
*/
#define ERR_TX_TIMEOUT                                -5

/*!
  \brief Timed out waiting for incoming transmission.
*/
#define ERR_RX_TIMEOUT                                -6

/*!
  \brief The calculated and expected CRCs of received packet do not match.
  This means that the packet was damaged during transmission and should be sent again.
*/
#define ERR_CRC_MISMATCH                              -7

/*!
  \brief The supplied bandwidth value is invalid for this module.
*/
#define ERR_INVALID_BANDWIDTH                         -8

/*!
  \brief The supplied spreading factor value is invalid for this module.
*/
#define ERR_INVALID_SPREADING_FACTOR                  -9

/*!
  \brief The supplied coding rate value is invalid for this module.
*/
#define ERR_INVALID_CODING_RATE                       -10

/*!
  \brief Internal only.
*/
#define ERR_INVALID_BIT_RANGE                         -11

/*!
  \brief The supplied frequency value is invalid for this module.
*/
#define ERR_INVALID_FREQUENCY                         -12

/*!
  \brief The supplied output power value is invalid for this module.
*/
#define ERR_INVALID_OUTPUT_POWER                      -13

/*!
  \brief LoRa preamble was detected during channel activity detection.
  This means that there is some LoRa device currently transmitting in your channel.
*/
#define PREAMBLE_DETECTED                             -14

/*!
  \brief No LoRa preambles were detected during channel activity detection. Your channel is free.
*/
#define CHANNEL_FREE                                  -15

/*!
  \brief Real value in SPI register does not match the expected one. This can be caused by faulty SPI wiring.
*/
#define ERR_SPI_WRITE_FAILED                          -16

/*!
  \brief The supplied current limit value is invalid.
*/
#define ERR_INVALID_CURRENT_LIMIT                     -17

/*!
  \brief The supplied preamble length is invalid.
*/
#define ERR_INVALID_PREAMBLE_LENGTH                   -18

/*!
  \brief The supplied gain value is invalid.
*/
#define ERR_INVALID_GAIN                              -19

/*!
  \brief User tried to execute modem-exclusive method on a wrong modem.
  For example, this can happen when you try to change LoRa configuration when FSK modem is active.
*/
#define ERR_WRONG_MODEM                               -20

/*!
  \brief The supplied number of RSSI samples is invalid.
*/
#define ERR_INVALID_NUM_SAMPLES                       -21

/*!
  \brief The supplied RSSI offset is invalid.
*/
#define ERR_INVALID_RSSI_OFFSET                       -22

/*!
  \brief The supplied encoding is invalid.
*/
#define ERR_INVALID_ENCODING                          -23

/*!
  \brief LoRa packet header has been damaged.
*/
#define ERR_LORA_HEADER_DAMAGED                        -24

// RF69-specific status codes

/*!
  \brief The supplied bit rate value is invalid.
*/
#define ERR_INVALID_BIT_RATE                          -101

/*!
  \brief The supplied frequency deviation value is invalid.
*/
#define ERR_INVALID_FREQUENCY_DEVIATION               -102

/*!
  \brief The supplied bit rate to bandwidth ratio is invalid. See the module datasheet for more information.
*/
#define ERR_INVALID_BIT_RATE_BW_RATIO                 -103

/*!
  \brief The supplied receiver bandwidth value is invalid.
*/
#define ERR_INVALID_RX_BANDWIDTH                      -104

/*!
  \brief The supplied FSK sync word is invalid.
*/
#define ERR_INVALID_SYNC_WORD                         -105

/*!
  \brief The supplied FSK data shaping option is invalid.
*/
#define ERR_INVALID_DATA_SHAPING                      -106

/*!
  \brief The current modulation is invalid for the requested operation.
*/
#define ERR_INVALID_MODULATION                        -107

// ESP8266 status codes

/*!
  \brief AT command failed to execute, or timed out.
*/
#define ERR_AT_FAILED                                 -201

/*!
  \brief Supplied URL is malformed or invalid.
*/
#define ERR_URL_MALFORMED                             -202

/*!
  \brief AT command response was malformed.
*/
#define ERR_RESPONSE_MALFORMED_AT                     -203

/*!
  \brief Data response was malformed.
*/
#define ERR_RESPONSE_MALFORMED                        -204

/*!
  \brief MQTT broker rejected connection due to version mismatch.
*/
#define ERR_MQTT_CONN_VERSION_REJECTED                -205

/*!
  \brief MQTT broker rejected connection due to unknown ID.
*/
#define ERR_MQTT_CONN_ID_REJECTED                     -206

/*!
  \brief Failed to establish connection with MQTT broker.
*/
#define ERR_MQTT_CONN_SERVER_UNAVAILABLE              -207

/*!
  \brief Supplied username/password combination is incorrect.
*/
#define ERR_MQTT_CONN_BAD_USERNAME_PASSWORD           -208

/*!
  \brief Unauthorized connection to MQTT broker.
*/
#define ERR_MQTT_CONN_NOT_AUTHORIZED                  -208

/*!
  \brief Received packet ID does not match the expected ID.
*/
#define ERR_MQTT_UNEXPECTED_PACKET_ID                 -209

/*!
  \brief No new packet was received since the last check.
*/
#define ERR_MQTT_NO_NEW_PACKET_AVAILABLE              -210

/*!
  \brief Successfully subscribed to MQTT topic with QoS 0.
*/
#define MQTT_SUBS_SUCCESS_QOS_0                       0x00

/*!
  \brief Successfully subscribed to MQTT topic with QoS 1.
*/
#define MQTT_SUBS_SUCCESS_QOS_1                       0x01

/*!
  \brief Successfully subscribed to MQTT topic with QoS 2.
*/
#define MQTT_SUBS_SUCCESS_QOS_2                       0x02

/*!
  \brief Failed to subscribe to MQTT topic.
*/
#define ERR_MQTT_SUBS_FAILED                          0x80

// XBee status codes

/*!
  \brief Failed to enter command mode.
*/
#define ERR_CMD_MODE_FAILED                           -301

/*!
  \brief Received ZigBee frame is malformed.
*/
#define ERR_FRAME_MALFORMED                           -302

/*!
  \brief Received ZigBee frame checksum does not match the calculated.
*/
#define ERR_FRAME_INCORRECT_CHECKSUM                  -303

/*!
  \brief Received ZigBee frame with unexpected ID.
*/
#define ERR_FRAME_UNEXPECTED_ID                       -304

/*!
  \brief Timed out waiting for response to ZigBee frame.
*/
#define ERR_FRAME_NO_RESPONSE                         -305

// RTTY status codes

/*!
  \brief Supplied RTTY frequency shift is invalid for this module.
*/
#define ERR_INVALID_RTTY_SHIFT                        -401

/*!
  \brief Supplied RTTY encoding is invalid.
*/
#define ERR_UNSUPPORTED_ENCODING                      -402

// nRF24-specific status codes

/*!
  \brief Supplied data rate is invalid.
*/
#define ERR_INVALID_DATA_RATE                         -501

/*!
  \brief Supplied address width is invalid.
*/
#define ERR_INVALID_ADDRESS_WIDTH                     -502

/*!
  \brief Supplied data pipe number is invalid.
*/
#define ERR_INVALID_PIPE_NUMBER                       -503

/*!
  \brief ACK packet from destination module was not received within 15 retries.
*/
#define ERR_ACK_NOT_RECEIVED                          -504

// CC1101-specific status codes

/*!
  \brief Supplied number of broadcast addresses is invalid.
*/
#define ERR_INVALID_NUM_BROAD_ADDRS                   -601

// SX126x-specific status codes

/*!
  \brief Supplied CRC configuration is invalid.
*/
#define ERR_INVALID_CRC_CONFIGURATION                 -701

/*!
  \brief Detected LoRa transmission while scanning channel.
*/
#define LORA_DETECTED                                 -702

/*!
  \brief Supplied TCXO reference voltage is invalid.
*/
#define ERR_INVALID_TCXO_VOLTAGE                      -703

/*!
  \brief Bit rate / bandwidth / frequency deviation ratio is invalid. See SX126x datasheet for details.
*/
#define ERR_INVALID_MODULATION_PARAMETERS             -704

/*!
  \brief SX126x timed out while waiting for complete SPI command.
*/
#define ERR_SPI_CMD_TIMEOUT                           -705

/*!
  \brief SX126x received invalid SPI command.
*/
#define ERR_SPI_CMD_INVALID                           -706

/*!
  \brief SX126x failed to execute SPI command.
*/
#define ERR_SPI_CMD_FAILED                            -707

/*!
  \brief The supplied sleep period is invalid.

  The specified sleep period is shorter than the time necessary to sleep and wake the hardware
  including TCXO delay, or longer than the maximum possible
*/
#define ERR_INVALID_SLEEP_PERIOD                      -708

/*!
  \brief The supplied Rx period is invalid.

  The specified Rx period is shorter or longer than the hardware can handle.
*/
#define ERR_INVALID_RX_PERIOD                         -709

// AX.25-specific status codes

/*!
  \brief The provided callsign is invalid.

  The specified callsign is longer than 6 ASCII characters.
*/
#define ERR_INVALID_CALLSIGN                          -801

/*!
  \brief The provided repeater configuration is invalid.

  The specified number of repeaters does not match number of repeater IDs or their callsigns.
*/
#define ERR_INVALID_NUM_REPEATERS                     -802

/*!
  \brief One of the provided repeater callsigns is invalid.

  The specified callsign is longer than 6 ASCII characters.
*/
#define ERR_INVALID_REPEATER_CALLSIGN                 -803

// SX128x-specific status codes

/*!
  \brief Timed out waiting for ranging exchange finish.
*/
#define ERR_RANGING_TIMEOUT                           -901

/*!
  \}
*/

#endif
