#if !defined(_RADIOLIB_LORAWAN_H) && !RADIOLIB_EXCLUDE_LORAWAN
#define _RADIOLIB_LORAWAN_H

#include "../../TypeDef.h"
#include "../PhysicalLayer/PhysicalLayer.h"
#include "../../utils/Cryptography.h"

// activation mode
#define RADIOLIB_LORAWAN_MODE_OTAA                              (0x07AA)
#define RADIOLIB_LORAWAN_MODE_ABP                               (0x0AB9)
#define RADIOLIB_LORAWAN_MODE_NONE                              (0x0000)

// operation mode
#define RADIOLIB_LORAWAN_CLASS_A                                (0x0A)
#define RADIOLIB_LORAWAN_CLASS_B                                (0x0B)
#define RADIOLIB_LORAWAN_CLASS_C                                (0x0C)

// preamble format
#define RADIOLIB_LORAWAN_LORA_SYNC_WORD                         (0x34)
#define RADIOLIB_LORAWAN_LORA_PREAMBLE_LEN                      (8)
#define RADIOLIB_LORAWAN_GFSK_SYNC_WORD                         (0xC194C1)
#define RADIOLIB_LORAWAN_GFSK_PREAMBLE_LEN                      (5)

// MAC header field encoding                                                    MSB   LSB   DESCRIPTION
#define RADIOLIB_LORAWAN_MHDR_MTYPE_JOIN_REQUEST                (0x00 << 5) //  7     5     message type: join request
#define RADIOLIB_LORAWAN_MHDR_MTYPE_JOIN_ACCEPT                 (0x01 << 5) //  7     5                   join accept
#define RADIOLIB_LORAWAN_MHDR_MTYPE_UNCONF_DATA_UP              (0x02 << 5) //  7     5                   unconfirmed data up
#define RADIOLIB_LORAWAN_MHDR_MTYPE_UNCONF_DATA_DOWN            (0x03 << 5) //  7     5                   unconfirmed data down
#define RADIOLIB_LORAWAN_MHDR_MTYPE_CONF_DATA_UP                (0x04 << 5) //  7     5                   confirmed data up
#define RADIOLIB_LORAWAN_MHDR_MTYPE_CONF_DATA_DOWN              (0x05 << 5) //  7     5                   confirmed data down
#define RADIOLIB_LORAWAN_MHDR_MTYPE_PROPRIETARY                 (0x07 << 5) //  7     5                   proprietary
#define RADIOLIB_LORAWAN_MHDR_MTYPE_MASK                        (0x07 << 5) //  7     5                   bitmask of all possible options
#define RADIOLIB_LORAWAN_MHDR_MAJOR_R1                          (0x00 << 0) //  1     0     major version: LoRaWAN R1

// frame control field encoding
#define RADIOLIB_LORAWAN_FCTRL_ADR_ENABLED                      (0x01 << 7) //  7     7     adaptive data rate: enabled
#define RADIOLIB_LORAWAN_FCTRL_ADR_DISABLED                     (0x00 << 7) //  7     7                         disabled
#define RADIOLIB_LORAWAN_FCTRL_ADR_ACK_REQ                      (0x01 << 6) //  6     6     adaptive data rate ACK request
#define RADIOLIB_LORAWAN_FCTRL_ACK                              (0x01 << 5) //  5     5     confirmed message acknowledge
#define RADIOLIB_LORAWAN_FCTRL_FRAME_PENDING                    (0x01 << 4) //  4     4     downlink frame is pending

// fPort field
#define RADIOLIB_LORAWAN_FPORT_MAC_COMMAND                      (0x00 << 0) //  7     0     payload contains MAC commands only
#define RADIOLIB_LORAWAN_FPORT_TS009                            (0xE0 << 0) //  7     0     fPort used for TS009 testing
#define RADIOLIB_LORAWAN_FPORT_RESERVED                         (0xE0 << 0) //  7     0     fPort values equal to and larger than this are reserved

// MAC commands - only those sent from end-device to gateway
#define RADIOLIB_LORAWAN_LINK_CHECK_REQ                         (0x02 << 0) //  7     0     MAC command: request to check connectivity to network
#define RADIOLIB_LORAWAN_LINK_ADR_ANS                           (0x03 << 0) //  7     0                  answer to ADR change
#define RADIOLIB_LORAWAN_DUTY_CYCLE_ANS                         (0x04 << 0) //  7     0                  answer to duty cycle change
#define RADIOLIB_LORAWAN_RX_PARAM_SETUP_ANS                     (0x05 << 0) //  7     0                  answer to reception slot setup request
#define RADIOLIB_LORAWAN_DEV_STATUS_ANS                         (0x06 << 0) //  7     0                  device status information
#define RADIOLIB_LORAWAN_NEW_CHANNEL_ANS                        (0x07 << 0) //  7     0                  acknowledges change of a radio channel
#define RADIOLIB_LORAWAN_RX_TIMING_SETUP_ANS                    (0x08 << 0) //  7     0                  acknowledges change of a reception slots timing

#define RADIOLIB_LORAWAN_NOPTS_LEN                              (8)

// data rate encoding
#define RADIOLIB_LORAWAN_DATA_RATE_FSK_50_K                     (0x01 << 7) //  7     7     FSK @ 50 kbps
#define RADIOLIB_LORAWAN_DATA_RATE_SF_12                        (0x06 << 4) //  6     4     LoRa spreading factor: SF12
#define RADIOLIB_LORAWAN_DATA_RATE_SF_11                        (0x05 << 4) //  6     4                             SF11
#define RADIOLIB_LORAWAN_DATA_RATE_SF_10                        (0x04 << 4) //  6     4                             SF10
#define RADIOLIB_LORAWAN_DATA_RATE_SF_9                         (0x03 << 4) //  6     4                             SF9
#define RADIOLIB_LORAWAN_DATA_RATE_SF_8                         (0x02 << 4) //  6     4                             SF8
#define RADIOLIB_LORAWAN_DATA_RATE_SF_7                         (0x01 << 4) //  6     4                             SF7
#define RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ                   (0x00 << 2) //  3     2     LoRa bandwidth: 500 kHz
#define RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ                   (0x01 << 2) //  3     2                     250 kHz
#define RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ                   (0x02 << 2) //  3     2                     125 kHz
#define RADIOLIB_LORAWAN_DATA_RATE_BW_RESERVED                  (0x03 << 2) //  3     2                     reserved value
#define RADIOLIB_LORAWAN_DATA_RATE_CR_4_5                       (0x00 << 0) //  1     0     LoRa coding rate: 4/5
#define RADIOLIB_LORAWAN_DATA_RATE_CR_4_6                       (0x01 << 0) //  1     0                       4/6
#define RADIOLIB_LORAWAN_DATA_RATE_CR_4_7                       (0x02 << 0) //  1     0                       4/7
#define RADIOLIB_LORAWAN_DATA_RATE_CR_4_8                       (0x03 << 0) //  1     0                       4/8
#define RADIOLIB_LORAWAN_DATA_RATE_UNUSED                       (0xFF << 0) //  7     0     unused data rate

#define RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK                     (0x00 << 0)
#define RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK                   (0x01 << 0)
#define RADIOLIB_LORAWAN_CHANNEL_DIR_BOTH                       (0x02 << 0)
#define RADIOLIB_LORAWAN_CHANNEL_DIR_NONE                       (0x03 << 0)
#define RADIOLIB_LORAWAN_BAND_DYNAMIC                           (0)
#define RADIOLIB_LORAWAN_BAND_FIXED                             (1)
#define RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES                  (15)
#define RADIOLIB_LORAWAN_CHANNEL_INDEX_NONE                     (0xFF >> 0)

// recommended default settings
#define RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS                     (1000)
#define RADIOLIB_LORAWAN_RECEIVE_DELAY_2_MS                     ((RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS) + 1000)
#define RADIOLIB_LORAWAN_RX1_DR_OFFSET                          (0)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DELAY_1_MS                 (5000)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DELAY_2_MS                 (6000)
#define RADIOLIB_LORAWAN_MAX_FCNT_GAP                           (16384)
#define RADIOLIB_LORAWAN_ADR_ACK_LIMIT_EXP                      (0x06)
#define RADIOLIB_LORAWAN_ADR_ACK_DELAY_EXP                      (0x05)
#define RADIOLIB_LORAWAN_RETRANSMIT_TIMEOUT_MIN_MS              (1000)
#define RADIOLIB_LORAWAN_RETRANSMIT_TIMEOUT_MAX_MS              (3000)
#define RADIOLIB_LORAWAN_POWER_STEP_SIZE_DBM                    (-2)
#define RADIOLIB_LORAWAN_REJOIN_MAX_COUNT_N                     (10)  // send rejoin request 16384 uplinks
#define RADIOLIB_LORAWAN_REJOIN_MAX_TIME_N                      (15)  // once every year, not actually implemented

// join request message layout
#define RADIOLIB_LORAWAN_JOIN_REQUEST_LEN                       (23)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_JOIN_EUI_POS              (1)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_DEV_EUI_POS               (9)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_DEV_NONCE_POS             (17)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_TYPE                      (0xFF)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_TYPE_0                    (0x00)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_TYPE_1                    (0x01)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_TYPE_2                    (0x02)

// join accept message layout
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN                    (33)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_NONCE_POS             (1)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_HOME_NET_ID_POS            (4)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_ADDR_POS               (7)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_EUI_POS               (4)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DL_SETTINGS_POS            (11)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_RX_DELAY_POS               (12)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_NONCE_POS              (12)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_POS                 (13)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN                 (16)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_TYPE_POS            (RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_POS + RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN - 1)

// join accept message variables
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_R_1_0                      (0x00 << 7) //  7     7     LoRaWAN revision: 1.0
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_R_1_1                      (0x01 << 7) //  7     7                       1.1
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_F_NWK_S_INT_KEY            (0x01)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_APP_S_KEY                  (0x02)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_S_NWK_S_INT_KEY            (0x03)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_NWK_S_ENC_KEY              (0x04)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_JS_ENC_KEY                 (0x05)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_JS_INT_KEY                 (0x06)

// frame header layout
#define RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS                    (16)
#define RADIOLIB_LORAWAN_FHDR_DEV_ADDR_POS                      (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 1)
#define RADIOLIB_LORAWAN_FHDR_FCTRL_POS                         (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 5)
#define RADIOLIB_LORAWAN_FHDR_FCNT_POS                          (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 6)
#define RADIOLIB_LORAWAN_FHDR_FOPTS_POS                         (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 8)
#define RADIOLIB_LORAWAN_FHDR_FOPTS_LEN_MASK                    (0x0F)
#define RADIOLIB_LORAWAN_FHDR_FOPTS_MAX_LEN                     (15)
#define RADIOLIB_LORAWAN_FHDR_FPORT_POS(FOPTS)                  (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 8 + (FOPTS))
#define RADIOLIB_LORAWAN_FRAME_PAYLOAD_POS(FOPTS)               (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 9 + (FOPTS))
#define RADIOLIB_LORAWAN_FRAME_LEN(PAYLOAD, FOPTS)              (16 + 13 + (PAYLOAD) + (FOPTS))

// payload encryption/MIC blocks common layout
#define RADIOLIB_LORAWAN_BLOCK_MAGIC_POS                        (0)
#define RADIOLIB_LORAWAN_BLOCK_CONF_FCNT_POS                    (1)
#define RADIOLIB_LORAWAN_BLOCK_DIR_POS                          (5)
#define RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS                     (6)
#define RADIOLIB_LORAWAN_BLOCK_FCNT_POS                         (10)

// payload encryption block layout
#define RADIOLIB_LORAWAN_ENC_BLOCK_MAGIC                        (0x01)
#define RADIOLIB_LORAWAN_ENC_BLOCK_COUNTER_ID_POS               (4)
#define RADIOLIB_LORAWAN_ENC_BLOCK_COUNTER_POS                  (15)

// payload MIC blocks layout
#define RADIOLIB_LORAWAN_MIC_BLOCK_MAGIC                        (0x49)
#define RADIOLIB_LORAWAN_MIC_BLOCK_LEN_POS                      (15)
#define RADIOLIB_LORAWAN_MIC_DATA_RATE_POS                      (3)
#define RADIOLIB_LORAWAN_MIC_CH_INDEX_POS                       (4)

// maximum allowed dwell time on bands that implement dwell time limitations
#define RADIOLIB_LORAWAN_DWELL_TIME                             (400)

// unused frame counter value
#define RADIOLIB_LORAWAN_FCNT_NONE                              (0xFFFFFFFF)

// MAC commands
#define RADIOLIB_LORAWAN_NUM_MAC_COMMANDS                       (16)

#define RADIOLIB_LORAWAN_MAC_RESET                              (0x01)
#define RADIOLIB_LORAWAN_MAC_LINK_CHECK                         (0x02)
#define RADIOLIB_LORAWAN_MAC_LINK_ADR                           (0x03)
#define RADIOLIB_LORAWAN_MAC_DUTY_CYCLE                         (0x04)
#define RADIOLIB_LORAWAN_MAC_RX_PARAM_SETUP                     (0x05)
#define RADIOLIB_LORAWAN_MAC_DEV_STATUS                         (0x06)
#define RADIOLIB_LORAWAN_MAC_NEW_CHANNEL                        (0x07)
#define RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP                    (0x08)
#define RADIOLIB_LORAWAN_MAC_TX_PARAM_SETUP                     (0x09)
#define RADIOLIB_LORAWAN_MAC_DL_CHANNEL                         (0x0A)
#define RADIOLIB_LORAWAN_MAC_REKEY                              (0x0B)
#define RADIOLIB_LORAWAN_MAC_ADR_PARAM_SETUP                    (0x0C)
#define RADIOLIB_LORAWAN_MAC_DEVICE_TIME                        (0x0D)
#define RADIOLIB_LORAWAN_MAC_FORCE_REJOIN                       (0x0E)
#define RADIOLIB_LORAWAN_MAC_REJOIN_PARAM_SETUP                 (0x0F)
#define RADIOLIB_LORAWAN_MAC_PROPRIETARY                        (0x80)

// the length of internal MAC command queue - hopefully this is enough for most use cases
#define RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE                 (9)

// the maximum number of simultaneously available channels
#define RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS                 (16)

// maximum MAC command sizes
#define RADIOLIB_LORAWAN_MAX_MAC_COMMAND_LEN_DOWN               (5)
#define RADIOLIB_LORAWAN_MAX_MAC_COMMAND_LEN_UP                 (2)
#define RADIOLIB_LORAWAN_MAX_NUM_ADR_COMMANDS                   (8)

/*!
  \struct LoRaWANMacSpec_t
  \brief MAC command specification structure.
*/
struct LoRaWANMacSpec_t {
  /*! \brief Command ID */
  const uint8_t cid;
  
  /*! \brief Uplink message length */
  const uint8_t lenDn;
  
  /*! \brief Downlink message length */
  const uint8_t lenUp;
  
  /*! \brief Whether this MAC command can be issued by the user or not */
  const bool user;
};

constexpr LoRaWANMacSpec_t MacTable[RADIOLIB_LORAWAN_NUM_MAC_COMMANDS + 1] = {
  { 0x00, 0, 0, false }, // not an actual MAC command, exists for index offsetting
  { RADIOLIB_LORAWAN_MAC_RESET,               1, 1, false },
  { RADIOLIB_LORAWAN_MAC_LINK_CHECK,          2, 0, true  },
  { RADIOLIB_LORAWAN_MAC_LINK_ADR,            4, 1, false },
  { RADIOLIB_LORAWAN_MAC_DUTY_CYCLE,          1, 0, false },
  { RADIOLIB_LORAWAN_MAC_RX_PARAM_SETUP,      4, 1, false },
  { RADIOLIB_LORAWAN_MAC_DEV_STATUS,          0, 2, false },
  { RADIOLIB_LORAWAN_MAC_NEW_CHANNEL,         5, 1, false },
  { RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP,     1, 0, false },
  { RADIOLIB_LORAWAN_MAC_TX_PARAM_SETUP,      1, 0, false },
  { RADIOLIB_LORAWAN_MAC_DL_CHANNEL,          4, 1, false },
  { RADIOLIB_LORAWAN_MAC_REKEY,               1, 1, false },
  { RADIOLIB_LORAWAN_MAC_ADR_PARAM_SETUP,     1, 0, false },
  { RADIOLIB_LORAWAN_MAC_DEVICE_TIME,         5, 0, true  },
  { RADIOLIB_LORAWAN_MAC_FORCE_REJOIN,        2, 0, false },
  { RADIOLIB_LORAWAN_MAC_REJOIN_PARAM_SETUP,  1, 1, false },
  { RADIOLIB_LORAWAN_MAC_PROPRIETARY,         5, 0, true  } 
};

/*!
  \struct LoRaWANMacCommand_t
  \brief Structure to save information about MAC command
*/
struct LoRaWANMacCommand_t {
  /*! \brief The command ID */
  uint8_t cid;

  /*! \brief Payload buffer (5 bytes is the longest possible) */
  uint8_t payload[RADIOLIB_LORAWAN_MAX_MAC_COMMAND_LEN_DOWN];

  /*! \brief Length of the payload */
  uint8_t len;

  /*! \brief Repetition counter (the command will be uplinked repeat + 1 times) */
  uint8_t repeat;
};

/*!
  \struct LoRaWANMacCommandQueue_t
  \brief Structure to hold information about a queue of MAC commands
*/
struct LoRaWANMacCommandQueue_t {
  /*! \brief Number of commands in the queue */
  uint8_t numCommands;

  /*! \brief Total length of the queue */
  uint8_t len;

  /*! \brief MAC command buffer */
  LoRaWANMacCommand_t commands[RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE];
};

#define RADIOLIB_LORAWAN_NONCES_VERSION_VAL (0x0001)

enum LoRaWANSchemeBase_t {
  RADIOLIB_LORAWAN_NONCES_START       = 0x00,
  RADIOLIB_LORAWAN_NONCES_VERSION     = RADIOLIB_LORAWAN_NONCES_START,                        // 2 bytes
  RADIOLIB_LORAWAN_NONCES_MODE        = RADIOLIB_LORAWAN_NONCES_VERSION + sizeof(uint16_t),   // 2 bytes
  RADIOLIB_LORAWAN_NONCES_CLASS       = RADIOLIB_LORAWAN_NONCES_MODE + sizeof(uint16_t),      // 1 byte
  RADIOLIB_LORAWAN_NONCES_PLAN        = RADIOLIB_LORAWAN_NONCES_CLASS + sizeof(uint8_t),      // 1 byte
  RADIOLIB_LORAWAN_NONCES_CHECKSUM    = RADIOLIB_LORAWAN_NONCES_PLAN + sizeof(uint8_t),       // 2 bytes
  RADIOLIB_LORAWAN_NONCES_DEV_NONCE   = RADIOLIB_LORAWAN_NONCES_CHECKSUM + sizeof(uint16_t),  // 2 bytes
  RADIOLIB_LORAWAN_NONCES_JOIN_NONCE  = RADIOLIB_LORAWAN_NONCES_DEV_NONCE + sizeof(uint16_t), // 3 bytes
  RADIOLIB_LORAWAN_NONCES_ACTIVE      = RADIOLIB_LORAWAN_NONCES_JOIN_NONCE + 3,               // 1 byte
  RADIOLIB_LORAWAN_NONCES_SIGNATURE   = RADIOLIB_LORAWAN_NONCES_ACTIVE + sizeof(uint8_t),     // 2 bytes
  RADIOLIB_LORAWAN_NONCES_BUF_SIZE    = RADIOLIB_LORAWAN_NONCES_SIGNATURE + sizeof(uint16_t)  // Nonces buffer size
};

enum LoRaWANSchemeSession_t {
  RADIOLIB_LORAWAN_SESSION_START              = 0x00,
  RADIOLIB_LORAWAN_SESSION_NWK_SENC_KEY       = RADIOLIB_LORAWAN_SESSION_START,                 // 16 bytes
  RADIOLIB_LORAWAN_SESSION_APP_SKEY           = RADIOLIB_LORAWAN_SESSION_NWK_SENC_KEY + RADIOLIB_AES128_BLOCK_SIZE,   // 16 bytes
  RADIOLIB_LORAWAN_SESSION_FNWK_SINT_KEY      = RADIOLIB_LORAWAN_SESSION_APP_SKEY + RADIOLIB_AES128_BLOCK_SIZE,       // 16 bytes
  RADIOLIB_LORAWAN_SESSION_SNWK_SINT_KEY      = RADIOLIB_LORAWAN_SESSION_FNWK_SINT_KEY + RADIOLIB_AES128_BLOCK_SIZE,  // 16 bytes
  RADIOLIB_LORAWAN_SESSION_DEV_ADDR           = RADIOLIB_LORAWAN_SESSION_SNWK_SINT_KEY + RADIOLIB_AES128_BLOCK_SIZE,  // 4 bytes
  RADIOLIB_LORAWAN_SESSION_NONCES_SIGNATURE   = RADIOLIB_LORAWAN_SESSION_DEV_ADDR + sizeof(uint32_t),         // 2 bytes
  RADIOLIB_LORAWAN_SESSION_A_FCNT_DOWN        = RADIOLIB_LORAWAN_SESSION_NONCES_SIGNATURE + sizeof(uint16_t), // 4 bytes
  RADIOLIB_LORAWAN_SESSION_CONF_FCNT_UP       = RADIOLIB_LORAWAN_SESSION_A_FCNT_DOWN + sizeof(uint32_t), 	    // 4 bytes
  RADIOLIB_LORAWAN_SESSION_CONF_FCNT_DOWN     = RADIOLIB_LORAWAN_SESSION_CONF_FCNT_UP + sizeof(uint32_t), 	  // 4 bytes
  RADIOLIB_LORAWAN_SESSION_RJ_COUNT0          = RADIOLIB_LORAWAN_SESSION_CONF_FCNT_DOWN + sizeof(uint32_t), 	// 2 bytes
  RADIOLIB_LORAWAN_SESSION_RJ_COUNT1          = RADIOLIB_LORAWAN_SESSION_RJ_COUNT0 + sizeof(uint16_t), 	      // 2 bytes
  RADIOLIB_LORAWAN_SESSION_HOMENET_ID         = RADIOLIB_LORAWAN_SESSION_RJ_COUNT1 + sizeof(uint16_t), 	      // 4 bytes
  RADIOLIB_LORAWAN_SESSION_VERSION            = RADIOLIB_LORAWAN_SESSION_HOMENET_ID + sizeof(uint32_t), 	    // 1 byte
  RADIOLIB_LORAWAN_SESSION_DUTY_CYCLE         = RADIOLIB_LORAWAN_SESSION_VERSION + sizeof(uint8_t), 	        // 1 byte
  RADIOLIB_LORAWAN_SESSION_RX_PARAM_SETUP     = RADIOLIB_LORAWAN_SESSION_DUTY_CYCLE + MacTable[RADIOLIB_LORAWAN_MAC_DUTY_CYCLE].lenDn, 	// 4 bytes
  RADIOLIB_LORAWAN_SESSION_RX_TIMING_SETUP    = RADIOLIB_LORAWAN_SESSION_RX_PARAM_SETUP + MacTable[RADIOLIB_LORAWAN_MAC_RX_PARAM_SETUP].lenDn, 	// 1 byte
  RADIOLIB_LORAWAN_SESSION_TX_PARAM_SETUP     = RADIOLIB_LORAWAN_SESSION_RX_TIMING_SETUP + MacTable[RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP].lenDn, 	// 1 byte
  RADIOLIB_LORAWAN_SESSION_ADR_PARAM_SETUP    = RADIOLIB_LORAWAN_SESSION_TX_PARAM_SETUP + MacTable[RADIOLIB_LORAWAN_MAC_TX_PARAM_SETUP].lenDn, 	// 1 byte
  RADIOLIB_LORAWAN_SESSION_REJOIN_PARAM_SETUP = RADIOLIB_LORAWAN_SESSION_ADR_PARAM_SETUP + MacTable[RADIOLIB_LORAWAN_MAC_ADR_PARAM_SETUP].lenDn, 	// 1 byte
  RADIOLIB_LORAWAN_SESSION_BEACON_FREQ        = RADIOLIB_LORAWAN_SESSION_REJOIN_PARAM_SETUP + MacTable[RADIOLIB_LORAWAN_MAC_REJOIN_PARAM_SETUP].lenDn, 	// 3 bytes
  RADIOLIB_LORAWAN_SESSION_PING_SLOT_CHANNEL  = RADIOLIB_LORAWAN_SESSION_BEACON_FREQ + 3, 	    // 4 bytes
  RADIOLIB_LORAWAN_SESSION_PERIODICITY        = RADIOLIB_LORAWAN_SESSION_PING_SLOT_CHANNEL + 4, // 1 byte
  RADIOLIB_LORAWAN_SESSION_LAST_TIME          = RADIOLIB_LORAWAN_SESSION_PERIODICITY + 1, 	    // 4 bytes
  RADIOLIB_LORAWAN_SESSION_UL_CHANNELS        = RADIOLIB_LORAWAN_SESSION_LAST_TIME + 4, 	      // 16*5 bytes
  RADIOLIB_LORAWAN_SESSION_DL_CHANNELS        = RADIOLIB_LORAWAN_SESSION_UL_CHANNELS + 16*MacTable[RADIOLIB_LORAWAN_MAC_NEW_CHANNEL].lenDn, // 16*4 bytes
  RADIOLIB_LORAWAN_SESSION_MAC_QUEUE_UL       = RADIOLIB_LORAWAN_SESSION_DL_CHANNELS + 16*MacTable[RADIOLIB_LORAWAN_MAC_DL_CHANNEL].lenDn,  // 9*8+2 bytes
  RADIOLIB_LORAWAN_SESSION_N_FCNT_DOWN        = RADIOLIB_LORAWAN_SESSION_MAC_QUEUE_UL + sizeof(LoRaWANMacCommandQueue_t), // 4 bytes
  RADIOLIB_LORAWAN_SESSION_ADR_FCNT           = RADIOLIB_LORAWAN_SESSION_N_FCNT_DOWN + sizeof(uint32_t),      // 4 bytes
  RADIOLIB_LORAWAN_SESSION_LINK_ADR           = RADIOLIB_LORAWAN_SESSION_ADR_FCNT + sizeof(uint32_t),         // 4 bytes
  RADIOLIB_LORAWAN_SESSION_FCNT_UP            = RADIOLIB_LORAWAN_SESSION_LINK_ADR + MacTable[RADIOLIB_LORAWAN_MAC_LINK_ADR].lenDn,  // 4 bytes
  RADIOLIB_LORAWAN_SESSION_SIGNATURE          = RADIOLIB_LORAWAN_SESSION_FCNT_UP + sizeof(uint32_t),          // 2 bytes
  RADIOLIB_LORAWAN_SESSION_BUF_SIZE           = RADIOLIB_LORAWAN_SESSION_SIGNATURE + sizeof(uint16_t)         // Session buffer size
};

/*!
  \struct LoRaWANChannel_t
  \brief Structure to save information about LoRaWAN channels.
  To save space, adjacent channels are saved in "spans".
*/
struct LoRaWANChannel_t {
  /*! \brief Whether this channel is enabled (can be used) or is disabled */
  bool enabled;

  /*! \brief The channel number, as specified by defaults or the network */
  uint8_t idx;

  /*! \brief The channel frequency */
  float freq;

  /*! \brief Minimum allowed datarate for this channel */
  uint8_t drMin;

  /*! \brief Maximum allowed datarate for this channel (inclusive) */
  uint8_t drMax;
};

// alias for unused channel
#define RADIOLIB_LORAWAN_CHANNEL_NONE    { .enabled = false, .idx = RADIOLIB_LORAWAN_CHANNEL_INDEX_NONE, .freq = 0, .drMin = 0, .drMax = 0 }

/*!
  \struct LoRaWANChannelSpan_t
  \brief Structure to save information about LoRaWAN channels.
  To save space, adjacent channels are saved in "spans".
*/
struct LoRaWANChannelSpan_t {
  /*! \brief Total number of channels in the span */
  uint8_t numChannels;

  /*! \brief Center frequency of the first channel in span */
  float freqStart;

  /*! \brief Frequency step between adjacent channels */
  float freqStep;

  /*! \brief Minimum allowed datarate for all channels in this span */
  uint8_t drMin;

  /*! \brief Maximum allowed datarate for all channels in this span (inclusive) */
  uint8_t drMax;
  
  /*! \brief Allowed data rates for a join request message */
  uint8_t joinRequestDataRate;
};

// alias for unused channel span
#define RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE    { .numChannels = 0, .freqStart = 0, .freqStep = 0, .drMin = 0, .drMax = 0, .joinRequestDataRate = RADIOLIB_LORAWAN_DATA_RATE_UNUSED }

/*!
  \struct LoRaWANBand_t
  \brief Structure to save information about LoRaWAN band
*/
struct LoRaWANBand_t {
  /*! \brief Identier for this band */
  uint8_t bandNum;

  /*! \brief Whether the channels are fixed per specification, or dynamically allocated through the network (plus defaults) */
  uint8_t bandType;

  /*! \brief Array of allowed maximum payload lengths for each data rate */
  uint8_t payloadLenMax[RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES];

  /*! \brief Maximum allowed output power in this band in dBm */
  int8_t powerMax;

  /*! \brief Number of power steps in this band */
  int8_t powerNumSteps;

  /*! \brief Number of milliseconds per hour of allowed Time-on-Air */
  RadioLibTime_t dutyCycle;

  /*! \brief Maximum dwell time per uplink message in milliseconds */
  RadioLibTime_t dwellTimeUp;

  /*! \brief Maximum dwell time per downlink message in milliseconds */
  RadioLibTime_t dwellTimeDn;

  /*! \brief A set of default uplink (TX) channels for frequency-type bands */
  LoRaWANChannel_t txFreqs[3];

  /*! \brief A set of possible extra channels for the Join-Request message for frequency-type bands */
  LoRaWANChannel_t txJoinReq[3];
  
  /*! \brief The number of TX channel spans for mask-type bands */
  uint8_t numTxSpans;

  /*! \brief Default uplink (TX) channel spans for mask-type bands, including Join-Request parameters */
  LoRaWANChannelSpan_t txSpans[2];

  /*! \brief Default downlink (RX1) channel span for mask-type bands */
  LoRaWANChannelSpan_t rx1Span;

  /*! \brief The base downlink data rate. Used to calculate data rate changes for adaptive data rate */
  uint8_t rx1DataRateBase;

  /*! \brief Backup channel for downlink (RX2) window */
  LoRaWANChannel_t rx2;
  
  /*! \brief The corresponding datarates, bandwidths and coding rates for DR index */
  uint8_t dataRates[RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES];
};

// supported bands
extern const LoRaWANBand_t EU868;
extern const LoRaWANBand_t US915;
extern const LoRaWANBand_t CN780;
extern const LoRaWANBand_t EU433;
extern const LoRaWANBand_t AU915;
extern const LoRaWANBand_t CN500;
extern const LoRaWANBand_t AS923;
extern const LoRaWANBand_t KR920;
extern const LoRaWANBand_t IN865;

/*!
  \struct LoRaWANBandNum_t
  \brief IDs of all currently supported bands
*/
enum LoRaWANBandNum_t {
  BandEU868,
  BandUS915,
  BandCN780,
  BandEU433,
  BandAU915,
  BandCN500,
  BandAS923,
  BandKR920,
  BandIN865,
  BandLast
};

// provide easy access to the number of currently supported bands
#define RADIOLIB_LORAWAN_NUM_SUPPORTED_BANDS      (BandLast - BandEU868)

// array of currently supported bands
extern const LoRaWANBand_t* LoRaWANBands[];

/*!
  \struct LoRaWANJoinEvent_t
  \brief Structure to save extra information about activation event.
*/
struct LoRaWANJoinEvent_t {
  /*! \brief Whether a new session was started */
  bool newSession = false;

  /*! \brief The transmitted Join-Request DevNonce value */
  uint16_t devNonce = 0;

  /*! \brief The received Join-Request JoinNonce value */
  uint32_t joinNonce = 0;
};

/*!
  \struct LoRaWANEvent_t
  \brief Structure to save extra information about uplink/downlink event.
*/
struct LoRaWANEvent_t {
  /*! \brief Event direction, one of RADIOLIB_LORAWAN_CHANNEL_DIR_* */
  uint8_t dir;
  
  /*! \brief Whether the event is confirmed or not (e.g., confirmed uplink sent by user application) */
  bool confirmed;
  
  /*! \brief Whether the event is confirming a previous request
  (e.g., server downlink reply to confirmed uplink sent by user application)*/
  bool confirming;

  /*! \brief Datarate */
  uint8_t datarate;
  
  /*! \brief Frequency in MHz */
  float freq;
  
  /*! \brief Transmit power in dBm for uplink, or RSSI for downlink */
  int16_t power;
  
  /*! \brief The appropriate frame counter - for different events, different frame counters will be reported! */
  uint32_t fCnt;
  
  /*! \brief Port number */
  uint8_t fPort;
};

/*!
  \class LoRaWANNode
  \brief LoRaWAN-compatible node (class A device).
*/
class LoRaWANNode {
  public:

    /*!
      \brief Default constructor.
      \param phy Pointer to the PhysicalLayer radio module.
      \param band Pointer to the LoRaWAN band to use.
      \param subBand The sub-band to be used (starting from 1!)
    */
    LoRaWANNode(PhysicalLayer* phy, const LoRaWANBand_t* band, uint8_t subBand = 0);

    /*!
      \brief Clear an active session, so that the device will have to rejoin the network.
    */
    void clearSession();

    /*!
      \brief Returns the pointer to the internal buffer that holds the LW base parameters
      \returns Pointer to uint8_t array of size RADIOLIB_LORAWAN_NONCES_BUF_SIZE
    */
    uint8_t* getBufferNonces();

    /*!
      \brief Fill the internal buffer that holds the LW base parameters with a supplied buffer
      \param persistentBuffer Buffer that should match the internal format (previously extracted using getBufferNonces)
      \returns \ref status_codes
    */
    int16_t setBufferNonces(uint8_t* persistentBuffer);

    /*!
      \brief Returns the pointer to the internal buffer that holds the LW session parameters
      \returns Pointer to uint8_t array of size RADIOLIB_LORAWAN_SESSION_BUF_SIZE
    */
    uint8_t* getBufferSession();

    /*!
      \brief Fill the internal buffer that holds the LW session parameters with a supplied buffer
      \param persistentBuffer Buffer that should match the internal format (previously extracted using getBufferSession)
      \returns \ref status_codes
    */
    int16_t setBufferSession(uint8_t* persistentBuffer);

    /*!
      \brief Set the device credentials and activation configuration
      \param joinEUI 8-byte application identifier.
      \param devEUI 8-byte device identifier.
      \param nwkKey Pointer to the network AES-128 key.
      \param appKey Pointer to the application AES-128 key.
    */
    void beginOTAA(uint64_t joinEUI, uint64_t devEUI, uint8_t* nwkKey, uint8_t* appKey);

    /*!
      \brief Join network by restoring OTAA session or performing over-the-air activation. By this procedure,
      the device will perform an exchange with the network server and set all necessary configuration. 
      \param joinDr The datarate at which to send the join-request and any subsequent uplinks (unless ADR is enabled)
      \returns \ref status_codes
    */
    int16_t activateOTAA(uint8_t initialDr = RADIOLIB_LORAWAN_DATA_RATE_UNUSED, LoRaWANJoinEvent_t *joinEvent = NULL);

    /*!
      \brief Set the device credentials and activation configuration
      \param addr Device address.
      \param fNwkSIntKey Pointer to the Forwarding network session (LoRaWAN 1.1), NULL for LoRaWAN 1.0.
      \param sNwkSIntKey Pointer to the Serving network session (LoRaWAN 1.1), NULL for LoRaWAN 1.0.
      \param nwkSEncKey Pointer to the MAC command network session key [NwkSEncKey] (LoRaWAN 1.1) 
                                    or network session AES-128 key [NwkSKey] (LoRaWAN 1.0).
      \param appSKey Pointer to the application session AES-128 key.
    */
    void beginABP(uint32_t addr, uint8_t* fNwkSIntKey, uint8_t* sNwkSIntKey, uint8_t* nwkSEncKey, uint8_t* appSKey);

    /*!
      \brief Join network by restoring ABP session or performing over-the-air activation. 
      In this procedure, all necessary configuration must be provided by the user.
      \param initialDr The datarate at which to send the first uplink and any subsequent uplinks (unless ADR is enabled).
      \returns \ref status_codes
    */
    int16_t activateABP(uint8_t initialDr = RADIOLIB_LORAWAN_DATA_RATE_UNUSED);

    /*! \brief Whether there is an ongoing session active */
    bool isActivated();

    /*! 
      \brief Configure the Rx2 datarate for ABP mode.
      This should not be needed for LoRaWAN 1.1 as it is configured through the first downlink.
      \param dr The datarate to be used for listening for downlinks in Rx2.
      \returns \ref status_codes
    */
    int16_t setRx2Dr(uint8_t dr);

    /*!
      \brief Add a MAC command to the uplink queue.
      Only LinkCheck and DeviceTime are available to the user. 
      Other commands are ignored; duplicate MAC commands are discarded.
      \param cid ID of the MAC command
      \returns \ref status_codes
    */
    int16_t sendMacCommandReq(uint8_t cid);

    #if defined(RADIOLIB_BUILD_ARDUINO)
    /*!
      \brief Send a message to the server.
      \param str Address of Arduino String that will be transmitted.
      \param fPort Port number to send the message to.
      \param isConfirmed Whether to send a confirmed uplink or not.
      \param event Pointer to a structure to store extra information about the event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns \ref status_codes
    */
    int16_t uplink(String& str, uint8_t fPort, bool isConfirmed = false, LoRaWANEvent_t* event = NULL);
    #endif

    /*!
      \brief Send a message to the server.
      \param str C-string that will be transmitted.
      \param fPort Port number to send the message to.
      \param isConfirmed Whether to send a confirmed uplink or not.
      \param event Pointer to a structure to store extra information about the event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns \ref status_codes
    */
    int16_t uplink(const char* str, uint8_t fPort, bool isConfirmed = false, LoRaWANEvent_t* event = NULL);

    /*!
      \brief Send a message to the server.
      \param data Data to send.
      \param len Length of the data.
      \param fPort Port number to send the message to.
      \param isConfirmed Whether to send a confirmed uplink or not.
      \param event Pointer to a structure to store extra information about the event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns \ref status_codes
    */
    int16_t uplink(uint8_t* data, size_t len, uint8_t fPort, bool isConfirmed = false, LoRaWANEvent_t* event = NULL);

    #if defined(RADIOLIB_BUILD_ARDUINO)
    /*!
      \brief Wait for downlink from the server in either RX1 or RX2 window.
      \param str Address of Arduino String to save the received data.
      \param event Pointer to a structure to store extra information about the event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns \ref status_codes
    */
    int16_t downlink(String& str, LoRaWANEvent_t* event = NULL);
    #endif

    /*!
      \brief Wait for downlink from the server in either RX1 or RX2 window.
      \param data Buffer to save received data into.
      \param len Pointer to variable that will be used to save the number of received bytes.
      \param event Pointer to a structure to store extra information about the event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns \ref status_codes
    */
    int16_t downlink(uint8_t* data, size_t* len, LoRaWANEvent_t* event = NULL);

    /*!
      \brief Wait for downlink, simplified to allow for simpler sendReceive
      \param event Pointer to a structure to store extra information about the event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns \ref status_codes
    */
    int16_t downlink(LoRaWANEvent_t* event = NULL);

    #if defined(RADIOLIB_BUILD_ARDUINO)
    /*!
      \brief Send a message to the server and wait for a downlink during Rx1 and/or Rx2 window.
      \param strUp Address of Arduino String that will be transmitted.
      \param fPort Port number to send the message to.
      \param strDown Address of Arduino String to save the received data.
      \param isConfirmed Whether to send a confirmed uplink or not.
      \param eventUp Pointer to a structure to store extra information about the uplink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \param eventDown Pointer to a structure to store extra information about the downlink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns \ref status_codes
    */
    int16_t sendReceive(String& strUp, uint8_t fPort, String& strDown, bool isConfirmed = false, LoRaWANEvent_t* eventUp = NULL, LoRaWANEvent_t* eventDown = NULL);
    #endif

    /*!
      \brief Send a message to the server and wait for a downlink during Rx1 and/or Rx2 window.
      \param strUp C-string that will be transmitted.
      \param fPort Port number to send the message to.
      \param dataDown Buffer to save received data into.
      \param lenDown Pointer to variable that will be used to save the number of received bytes.
      \param isConfirmed Whether to send a confirmed uplink or not.
      \param eventUp Pointer to a structure to store extra information about the uplink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \param eventDown Pointer to a structure to store extra information about the downlink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns \ref status_codes
    */
    int16_t sendReceive(const char* strUp, uint8_t fPort, uint8_t* dataDown, size_t* lenDown, bool isConfirmed = false, LoRaWANEvent_t* eventUp = NULL, LoRaWANEvent_t* eventDown = NULL);

    /*!
      \brief Send a message to the server and wait for a downlink during Rx1 and/or Rx2 window.
      \param dataUp Data to send.
      \param lenUp Length of the data.
      \param fPort Port number to send the message to.
      \param dataDown Buffer to save received data into.
      \param lenDown Pointer to variable that will be used to save the number of received bytes.
      \param isConfirmed Whether to send a confirmed uplink or not.
      \param eventUp Pointer to a structure to store extra information about the uplink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \param eventDown Pointer to a structure to store extra information about the downlink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns \ref status_codes
    */
    int16_t sendReceive(uint8_t* dataUp, size_t lenUp, uint8_t fPort, uint8_t* dataDown, size_t* lenDown, bool isConfirmed = false, LoRaWANEvent_t* eventUp = NULL, LoRaWANEvent_t* eventDown = NULL);

    /*!
      \brief Send a message to the server and wait for a downlink but don't bother the user with downlink contents
      \param dataUp Data to send.
      \param lenUp Length of the data.
      \param fPort Port number to send the message to.
      \param isConfirmed Whether to send a confirmed uplink or not.
      \param eventUp Pointer to a structure to store extra information about the uplink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \param eventDown Pointer to a structure to store extra information about the downlink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns \ref status_codes
    */
    int16_t sendReceive(uint8_t* dataUp, size_t lenUp, uint8_t fPort = 1, bool isConfirmed = false, LoRaWANEvent_t* eventUp = NULL, LoRaWANEvent_t* eventDown = NULL);

    /*!
      \brief Set device status.
      \param battLevel Battery level to set. 0 for external power source, 1 for lowest battery,
      254 for highest battery, 255 for unable to measure.
    */
    void setDeviceStatus(uint8_t battLevel);

    /*! 
        \brief Returns the last uplink's frame counter; 
        also 0 if no uplink occured yet. 
    */
    uint32_t getFCntUp();

    /*! 
        \brief Returns the last network downlink's frame counter; 
        also 0 if no network downlink occured yet. 
    */
    uint32_t getNFCntDown();

    /*! 
        \brief Returns the last application downlink's frame counter; 
        also 0 if no application downlink occured yet. 
    */
    uint32_t getAFCntDown();

    /*! 
        \brief Reset the downlink frame counters (application and network)
        This is unsafe and can possibly allow replay attacks using downlinks.
        It mainly exists as part of the TS009 Specification Verification protocol.
    */
    void resetFCntDown();

    /*!
      \brief Set uplink datarate. This should not be used when ADR is enabled.
      \param drUp Datarate to use for uplinks.
      \returns \ref status_codes
    */
    int16_t setDatarate(uint8_t drUp);

    /*!
      \brief Toggle ADR to on or off.
      \param enable Whether to disable ADR or not.
    */
    void setADR(bool enable = true);

    /*!
      \brief Toggle adherence to dutyCycle limits to on or off.
      \param enable Whether to adhere to dutyCycle limits or not (default true).
      \param msPerHour The maximum allowed Time-on-Air per hour in milliseconds 
      (default 0 = maximum allowed for configured band).
    */
    void setDutyCycle(bool enable = true, RadioLibTime_t msPerHour = 0);

    /*!
      \brief Calculate the minimum interval to adhere to a certain dutyCycle.
      This interval is based on the ToA of one uplink and does not actually keep track of total airtime.
      \param msPerHour The maximum allowed duty cycle (in milliseconds per hour).
      \param airtime The airtime of the uplink.
      \returns Required interval (delay) in milliseconds between consecutive uplinks.
    */
    RadioLibTime_t dutyCycleInterval(RadioLibTime_t msPerHour, RadioLibTime_t airtime);

    /*! \brief Returns time in milliseconds until next uplink is available under dutyCycle limits */
    RadioLibTime_t timeUntilUplink();

    /*!
      \brief Toggle adherence to dwellTime limits to on or off.
      \param enable Whether to adhere to dwellTime limits or not (default true).
      \param msPerUplink The maximum allowed Time-on-Air per uplink in milliseconds 
      (default 0 = maximum allowed for configured band).
    */
    void setDwellTime(bool enable, RadioLibTime_t msPerUplink = 0);

    /*! 
      \brief Returns the maximum payload given the currently present dwell time limits.
      WARNING: the addition of MAC commands may cause uplink errors;
      if you want to be sure that your payload fits within dwell time limits, subtract 16 from the result!
    */
    uint8_t maxPayloadDwellTime();

    /*!
      \brief Configure TX power of the radio module.
      \param txPower Output power during TX mode to be set in dBm.
      \returns \ref status_codes
    */
    int16_t setTxPower(int8_t txPower);

    /*!
      \brief Returns the quality of connectivity after requesting a LinkCheck MAC command.
      Returns 'true' if a network response was successfully parsed.
      Returns 'false' if there was no network response / parsing failed.
      \param margin Link margin in dB of LinkCheckReq demodulation at gateway side.
      \param gwCnt Number of gateways that received the LinkCheckReq.
      \returns \ref status_codes
    */
    int16_t getMacLinkCheckAns(uint8_t* margin, uint8_t* gwCnt);

    /*!
      \brief Returns the network time after requesting a DeviceTime MAC command.
      Returns 'true' if a network response was successfully parsed.
      Returns 'false' if there was no network response / parsing failed.
      \param gpsEpoch Number of seconds since GPS epoch (Jan. 6th 1980)
      \param fraction Fractional-second, in 1/256-second steps
      \param returnUnix If true, returns Unix timestamp instead of GPS (default true)
      \returns \ref status_codes
    */
    int16_t getMacDeviceTimeAns(uint32_t* gpsEpoch, uint8_t* fraction, bool returnUnix = true);

    /*!
      \brief Returns the DevAddr of the device, regardless of OTAA or ABP mode
      \returns 8-byte DevAddr
    */
    uint64_t getDevAddr();

    /*!
      \brief Get the Time-on-air of the last uplink message.
      \returns (RadioLibTime_t) time-on-air (ToA) of last uplink message.
    */
    RadioLibTime_t getLastToA();

    /*! 
      \brief TS009 Protocol Specification Verification switch
      (allows FPort 224 and cuts off uplink payload instead of rejecting if maximum length exceeded).
    */
    bool TS009 = false;

#if !RADIOLIB_GODMODE
  private:
#endif
    PhysicalLayer* phyLayer = NULL;
    const LoRaWANBand_t* band = NULL;

    static int16_t checkBufferCommon(uint8_t *buffer, uint16_t size);

    void activateCommon(uint8_t initialDr);

    // a buffer that holds all LW base parameters that should persist at all times!
    uint8_t bufferNonces[RADIOLIB_LORAWAN_NONCES_BUF_SIZE] = { 0 };

    // a buffer that holds all LW session parameters that preferably persist, but can be afforded to get lost
    uint8_t bufferSession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE] = { 0 };

    LoRaWANMacCommandQueue_t commandsUp = { 
      .numCommands = 0,
      .len = 0,
      .commands = { { .cid = 0, .payload = { 0 }, .len = 0, .repeat = 0, } },
    };
    LoRaWANMacCommandQueue_t commandsDown = { 
      .numCommands = 0,
      .len = 0,
      .commands = { { .cid = 0, .payload = { 0 }, .len = 0, .repeat = 0, } },
    };

    uint16_t lwMode = RADIOLIB_LORAWAN_MODE_NONE;
    uint8_t lwClass = RADIOLIB_LORAWAN_CLASS_A;
    bool isActive = false;

    uint64_t joinEUI = 0;
    uint64_t devEUI = 0;
    uint8_t nwkKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t appKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };

    // the following is either provided by the network server (OTAA)
    // or directly entered by the user (ABP)
    uint32_t devAddr = 0;
    uint8_t appSKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t fNwkSIntKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t sNwkSIntKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t nwkSEncKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t jSIntKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };

    uint16_t keyCheckSum = 0;
    
    // device-specific parameters, persistent through sessions
    uint16_t devNonce = 0;
    uint32_t joinNonce = 0;

    // session-specific parameters
    uint32_t homeNetId = 0;
    uint8_t adrLimitExp = RADIOLIB_LORAWAN_ADR_ACK_LIMIT_EXP;
    uint8_t adrDelayExp = RADIOLIB_LORAWAN_ADR_ACK_DELAY_EXP;
    uint8_t nbTrans = 1;            // Number of allowed frame retransmissions
    uint8_t txPowerSteps = 0;
    uint8_t txPowerMax = 0;
    uint32_t fCntUp = 0;
    uint32_t aFCntDown = 0;
    uint32_t nFCntDown = 0;
    uint32_t confFCntUp = RADIOLIB_LORAWAN_FCNT_NONE;
    uint32_t confFCntDown = RADIOLIB_LORAWAN_FCNT_NONE;
    uint32_t adrFCnt = 0;

    // whether the current configured channel is in FSK mode
    bool FSK = false;

    // ADR is enabled by default
    bool adrEnabled = true;

    // duty cycle is set upon initialization and activated in regions that impose this
    bool dutyCycleEnabled = false;
    uint32_t dutyCycle = 0;

    // dwell time is set upon initialization and activated in regions that impose this
    bool dwellTimeEnabledUp = false;
    uint16_t dwellTimeUp = 0;
    bool dwellTimeEnabledDn = false;
    uint16_t dwellTimeDn = 0;
    
    // enable/disable CSMA for LoRaWAN
    bool enableCSMA;

    // number of backoff slots to be decremented after DIFS phase. 0 to disable BO.
    // A random BO avoids collisions in the case where two or more nodes start the CSMA
    // process at the same time. 
    uint8_t backoffMax;
    
    // number of CADs to estimate a clear CH
    uint8_t difsSlots;

    // available channel frequencies from list passed during OTA activation
    LoRaWANChannel_t availableChannels[2][RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS];

    // currently configured channels for TX and RX1
    LoRaWANChannel_t currentChannels[2] = { RADIOLIB_LORAWAN_CHANNEL_NONE, RADIOLIB_LORAWAN_CHANNEL_NONE };

    // currently configured datarates for TX and RX1
    uint8_t dataRates[2] = { RADIOLIB_LORAWAN_DATA_RATE_UNUSED, RADIOLIB_LORAWAN_DATA_RATE_UNUSED };

    // Rx2 channel properties - may be changed by MAC command
    LoRaWANChannel_t rx2 = RADIOLIB_LORAWAN_CHANNEL_NONE;

    // offset between TX and RX1 (such that RX1 has equal or lower DR)
    uint8_t rx1DrOffset = 0;

    // LoRaWAN revision (1.0 vs 1.1)
    uint8_t rev = 0;

    // Time on Air of last uplink
    RadioLibTime_t lastToA = 0;

    // timestamp to measure the RX1/2 delay (from uplink end)
    RadioLibTime_t rxDelayStart = 0;

    // timestamp when the Rx1/2 windows were closed (timeout or uplink received)
    RadioLibTime_t rxDelayEnd = 0;

    // delays between the uplink and RX1/2 windows
    RadioLibTime_t rxDelays[2] = { RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS, RADIOLIB_LORAWAN_RECEIVE_DELAY_2_MS };

    // device status - battery level
    uint8_t battLevel = 0xFF;

    // indicates whether an uplink has MAC commands as payload
    bool isMACPayload = false;

    // save the selected sub-band in case this must be restored in ADR control
    uint8_t subBand = 0;

    // this will reset the device credentials, so the device starts completely new
    void clearNonces();

    // wait for, open and listen during Rx1 and Rx2 windows; only performs listening
    int16_t downlinkCommon();

    // method to generate message integrity code
    uint32_t generateMIC(uint8_t* msg, size_t len, uint8_t* key);

    // method to verify message integrity code
    // it assumes that the MIC is the last 4 bytes of the message
    bool verifyMIC(uint8_t* msg, size_t len, uint8_t* key);

    // configure the common physical layer properties (preamble, sync word etc.)
    // channels must be configured separately by setupChannelsDyn()!
    int16_t setPhyProperties(uint8_t dir);

    // setup uplink/downlink channel data rates and frequencies
    // for dynamic channels, there is a small set of predefined channels
    // in case of JoinRequest, add some optional extra frequencies 
    int16_t setupChannelsDyn(bool joinRequest = false);

    // setup uplink/downlink channel data rates and frequencies
    // for fixed bands, we only allow one sub-band at a time to be selected
    int16_t setupChannelsFix(uint8_t subBand);

    // a join-accept can piggy-back a set of channels or channel masks
    int16_t processCFList(uint8_t* cfList);

    // select a set of random TX/RX channels for up- and downlink
    int16_t selectChannels();

    // find the first usable data rate for the given band
    int16_t findDataRate(uint8_t dr, DataRate_t* dataRate);

    // restore all available channels from persistent storage
    int16_t restoreChannels();

    // push MAC command to queue, done by copy
    int16_t pushMacCommand(LoRaWANMacCommand_t* cmd, LoRaWANMacCommandQueue_t* queue);

    // delete a specific MAC command from queue, indicated by the command ID
    // if a payload pointer is supplied, this returns the payload of the MAC command
    int16_t deleteMacCommand(uint8_t cid, LoRaWANMacCommandQueue_t* queue, uint8_t* payload = NULL);

    // execute mac command, return the number of processed bytes for sequential processing
    bool execMacCommand(LoRaWANMacCommand_t* cmd);

    // apply a channel mask to a set of readily defined channels (dynamic bands only)
    bool applyChannelMaskDyn(uint8_t chMaskCntl, uint16_t chMask);

    // define or delete channels from a fixed set of channels (fixed bands only)
    bool applyChannelMaskFix(uint8_t chMaskCntl, uint16_t chMask);

    // get the payload length for a specific MAC command
    uint8_t getMacPayloadLength(uint8_t cid);
    
    /*!
      \brief Configures CSMA for LoRaWAN as per TR-13, LoRa Alliance.
      \param backoffMax Num of BO slots to be decremented after DIFS phase. 0 to disable BO.
      \param difsSlots Num of CADs to estimate a clear CH.
      \param enableCSMA enable/disable CSMA for LoRaWAN.
    */
    void setCSMA(uint8_t backoffMax, uint8_t difsSlots, bool enableCSMA = false);

    // Performs CSMA as per LoRa Alliance Technical Recommendation 13 (TR-013).
    void performCSMA();

    // perform a single CAD operation for the under SF/CH combination. Returns either busy or otherwise.
    bool performCAD();

    // function to encrypt and decrypt payloads
    void processAES(const uint8_t* in, size_t len, uint8_t* key, uint8_t* out, uint32_t fCnt, uint8_t dir, uint8_t ctrId, bool counter);

    // 16-bit checksum method that takes a uint8_t array of even length and calculates the checksum
    static uint16_t checkSum16(uint8_t *key, uint16_t keyLen);

    // network-to-host conversion method - takes data from network packet and converts it to the host endians
    template<typename T>
    static T ntoh(uint8_t* buff, size_t size = 0);

    // host-to-network conversion method - takes data from host variable and and converts it to network packet endians
    template<typename T>
    static void hton(uint8_t* buff, T val, size_t size = 0);
};

#endif
