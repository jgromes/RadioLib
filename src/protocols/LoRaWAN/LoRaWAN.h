#if !defined(_RADIOLIB_LORAWAN_H) && !defined(RADIOLIB_EXCLUDE_LORAWAN)
#define _RADIOLIB_LORAWAN_H

#include "../../TypeDef.h"
#include "../PhysicalLayer/PhysicalLayer.h"
#include "../../utils/Cryptography.h"

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

// port field
#define RADIOLIB_LORAWAN_FPORT_MAC_COMMAND                      (0x00 << 0) //  7     0     payload contains MAC commands only
#define RADIOLIB_LORAWAN_FPORT_RESERVED                         (0xE0 << 0) //  7     0     reserved port values

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
#define RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES                (0)
#define RADIOLIB_LORAWAN_CFLIST_TYPE_MASK                       (1)
#define RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES                  (16)

// recommended default settings
#define RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS                     (1000)
#define RADIOLIB_LORAWAN_RECEIVE_DELAY_2_MS                     ((RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS) + 1000)
#define RADIOLIB_LORAWAN_RX_WINDOW_LEN_MS                       (500)
#define RADIOLIB_LORAWAN_RX1_DR_OFFSET                          (0)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DELAY_1_MS                 (5000)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DELAY_2_MS                 (6000)
#define RADIOLIB_LORAWAN_MAX_FCNT_GAP                           (16384)
#define RADIOLIB_LORAWAN_ADR_ACK_LIMIT                          (64)
#define RADIOLIB_LORAWAN_ADR_ACK_DELAY                          (32)
#define RADIOLIB_LORAWAN_RETRANSMIT_TIMEOUT_MIN_MS              (1000)
#define RADIOLIB_LORAWAN_RETRANSMIT_TIMEOUT_MAX_MS              (3000)
#define RADIOLIB_LORAWAN_POWER_STEP_SIZE_DBM                    (-2)

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
#define RADIOLIB_LORAWAN_FHDR_FOPTS_MAX_LEN                     (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 16)
#define RADIOLIB_LORAWAN_FHDR_FPORT_POS(FOPTS)                  (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 8 + (FOPTS))
#define RADIOLIB_LORAWAN_FRAME_PAYLOAD_POS(FOPTS)               (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 9 + (FOPTS))
#define RADIOLIB_LORAWAN_FRAME_LEN(PAYLOAD, FOPTS)              (16 + 13 + (PAYLOAD) + (FOPTS))

// payload encryption/MIC blocks common layout
#define RADIOLIB_LORAWAN_BLOCK_MAGIC_POS                        (0)
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

// magic word saved in persistent memory upon activation
#define RADIOLIB_LORAWAN_MAGIC                                  (0x12AD101B)

// MAC commands
#define RADIOLIB_LORAWAN_MAC_CMD_RESET                          (0x01)
#define RADIOLIB_LORAWAN_MAC_CMD_LINK_CHECK                     (0x02)
#define RADIOLIB_LORAWAN_MAC_CMD_LINK_ADR                       (0x03)
#define RADIOLIB_LORAWAN_MAC_CMD_DUTY_CYCLE                     (0x04)
#define RADIOLIB_LORAWAN_MAC_CMD_RX_PARAM_SETUP                 (0x05)
#define RADIOLIB_LORAWAN_MAC_CMD_DEV_STATUS                     (0x06)
#define RADIOLIB_LORAWAN_MAC_CMD_NEW_CHANNEL                    (0x07)
#define RADIOLIB_LORAWAN_MAC_CMD_RX_TIMING_SETUP                (0x08)
#define RADIOLIB_LORAWAN_MAC_CMD_TX_PARAM_SETUP                 (0x09)
#define RADIOLIB_LORAWAN_MAC_CMD_DL_CHANNEL                     (0x0A)
#define RADIOLIB_LORAWAN_MAC_CMD_REKEY                          (0x0B)
#define RADIOLIB_LORAWAN_MAC_CMD_ADR_PARAM_SETUP                (0x0C)
#define RADIOLIB_LORAWAN_MAC_CMD_DEVICE_TIME                    (0x0D)
#define RADIOLIB_LORAWAN_MAC_CMD_FORCE_REJOIN                   (0x0E)
#define RADIOLIB_LORAWAN_MAC_CMD_REJOIN_PARAM_SETUP             (0x0F)
#define RADIOLIB_LORAWAN_MAC_CMD_PROPRIETARY                    (0x80)

// the length of internal MAC command queue - hopefully this is enough for most use cases
#define RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE                 (8)

// the maximum number of simultaneously available channels
#define RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS                 (8)

/*!
  \struct LoRaWANChannelSpan_t
  \brief Structure to save information about LoRaWAN channels.
  To save space, adjacent channels are saved in "spans".
*/
struct LoRaWANChannelSpan_t {
  /*! \brief Whether this channel span is for uplink, downlink, or both directions*/
  uint8_t direction;

  /*! \brief Allowed data rates for a join request message */
  uint8_t joinRequestDataRate;

  /*! \brief Total number of channels in the span */
  uint8_t numChannels;

  /*! \brief Center frequency of the first channel in span */
  float freqStart;

  /*! \brief Frequency step between adjacent channels */
  float freqStep;

  /*! \brief Array of datarates supported by all channels in the span */
  uint8_t dataRates[RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES];
};

// alias for unused channel span
#define RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE    { .direction = RADIOLIB_LORAWAN_CHANNEL_DIR_NONE, .joinRequestDataRate = RADIOLIB_LORAWAN_DATA_RATE_UNUSED, .numChannels = 0, .freqStart = 0, .freqStep = 0, .dataRates = { 0 } }

/*!
  \struct LoRaWANBand_t
  \brief Structure to save information about LoRaWAN band
*/
struct LoRaWANBand_t {
  /*! \brief The base downlink data rate. Used to calculate data rate changes for adaptive data rate */
  uint8_t downlinkDataRateBase;

  /*! \brief The minimum allowed downlink data rate. Used to calculate data rate changes for adaptive data rate */
  uint8_t downlinkDataRateMin;

  /*! \brief Array of allowed maximum payload lengths for each data rate */
  uint8_t payloadLenMax[RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES];

  /*! \brief Maximum allowed output power in this band in dBm */
  int8_t powerMax;

  /*! \brief Number of power steps in this band */
  int8_t powerNumSteps;

  /*! \brief Whether the optional channels are defined as list of frequencies or bit mask */
  uint8_t cfListType;

  /*! \brief FSK channel frequency */
  float fskFreq;
  
  /*! \brief Number of channel spans in the band */
  uint8_t numChannelSpans;

  /*! \brief Default uplink (TX/RX1) channels defined by LoRaWAN Regional Parameters */
  LoRaWANChannelSpan_t defaultChannels[3];
  
  /*! \brief Backup downlink (RX2) channel - just a single channel, but using the same structure for convenience */
  LoRaWANChannelSpan_t backupChannel;
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
  \struct LoRaWANMacCommand_t
  \brief Structure to save information about MAC command
*/
struct LoRaWANMacCommand_t {
  /*! \brief The command ID */
  uint8_t cid;

  /*! \brief Length of the payload */
  size_t len;

  /*! \brief Payload buffer (5 bytes is the longest possible) */
  uint8_t payload[5];

  /*! \brief Repetition counter (the command will be uplinked repeat + 1 times) */
  uint8_t repeat;
};

struct LoRaWANMacCommandQueue_t {
  LoRaWANMacCommand_t commands[RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE];
  size_t numCommands;
};

/*!
  \class LoRaWANNode
  \brief LoRaWAN-compatible node (class A device).
*/
class LoRaWANNode {
  public:
    /*! \brief Set to true to force the node to only use FSK channels. Set to false by default. */
    bool FSK;

    /*! \brief Starting channel offset.
        Some band plans only support a subset of available channels.
        Set to a positive value to set the first channel that will be used (e.g. 8 for US915 FSB2 used by TTN).
        By default -1 (no channel offset). */
    int8_t startChannel;

    /*! \brief Number of supported channels.
        Some band plans only support a subset of available channels.
        Set to a positive value to set the number of channels that will be used
        (e.g. 8 for US915 FSB2 used by TTN). By default -1 (no channel offset). */
    int8_t numChannels;

    /*!
      \brief Default constructor.
      \param phy Pointer to the PhysicalLayer radio module.
      \param band Pointer to the LoRaWAN band to use.
    */
    LoRaWANNode(PhysicalLayer* phy, const LoRaWANBand_t* band);

    /*!
      \brief Wipe internal persistent parameters.
      This will reset all counters and saved variables, so the device will have to rejoin the network.
    */
    void wipe();

    /*!
      \brief Join network by loading information from persistent storage.
      \returns \ref status_codes
    */
    int16_t begin();

    /*!
      \brief Join network by performing over-the-air activation. By this procedure,
      the device will perform an exchange with the network server and set all necessary configuration. 
      \param joinEUI 8-byte application identifier.
      \param devEUI 8-byte device identifier.
      \param nwkKey Pointer to the network AES-128 key.
      \param appKey Pointer to the application AES-128 key.
      \param force Set to true to force joining even if previously joined.
      \returns \ref status_codes
    */
    int16_t beginOTAA(uint64_t joinEUI, uint64_t devEUI, uint8_t* nwkKey, uint8_t* appKey, bool force = false);

    /*!
      \brief Join network by performing activation by personalization.
      In this procedure, all necessary configuration must be provided by the user.
      \param addr Device address.
      \param nwkSKey Pointer to the network session AES-128 key (LoRaWAN 1.0) or MAC command network session key (LoRaWAN 1.1).
      \param appSKey Pointer to the application session AES-128 key.
      \param fNwkSIntKey Pointer to the network session F key (LoRaWAN 1.1), unused for LoRaWAN 1.0.
      \param sNwkSIntKey Pointer to the network session S key (LoRaWAN 1.1), unused for LoRaWAN 1.0.
      \returns \ref status_codes
    */
    int16_t beginAPB(uint32_t addr, uint8_t* nwkSKey, uint8_t* appSKey, uint8_t* fNwkSIntKey = NULL, uint8_t* sNwkSIntKey = NULL);

    #if defined(RADIOLIB_BUILD_ARDUINO)
    /*!
      \brief Send a message to the server.
      \param str Address of Arduino String that will be transmitted.
      \param port Port number to send the message to.
      \returns \ref status_codes
    */
    int16_t uplink(String& str, uint8_t port);
    #endif

    /*!
      \brief Send a message to the server.
      \param str C-string that will be transmitted.
      \param port Port number to send the message to.
      \returns \ref status_codes
    */
    int16_t uplink(const char* str, uint8_t port);

    /*!
      \brief Send a message to the server.
      \param data Data to send.
      \param len Length of the data.
      \param port Port number to send the message to.
      \returns \ref status_codes
    */
    int16_t uplink(uint8_t* data, size_t len, uint8_t port);

    #if defined(RADIOLIB_BUILD_ARDUINO)
    /*!
      \brief Wait for downlink from the server in either RX1 or RX2 window.
      \param str Address of Arduino String to save the received data.
      \returns \ref status_codes
    */
    int16_t downlink(String& str);
    #endif

    /*!
      \brief Wait for downlink from the server in either RX1 or RX2 window.
      \param data Buffer to save received data into.
      \param len Pointer to variable that will be used to save the number of received bytes.
      \returns \ref status_codes
    */
    int16_t downlink(uint8_t* data, size_t* len);

    /*!
      \brief Set device status.
      \param battLevel Battery level to set. 0 for external power source, 1 for lowest battery,
      254 for highest battery, 255 for unable to measure.
    */
    void setDeviceStatus(uint8_t battLevel);

#if !defined(RADIOLIB_GODMODE)
  private:
#endif
    PhysicalLayer* phyLayer = NULL;
    const LoRaWANBand_t* band = NULL;

    LoRaWANMacCommandQueue_t commandsUp = { 
      .commands = { { .cid = 0, .len = 0, .payload = { 0 }, .repeat = 0, } },
      .numCommands = 0,
    };
    LoRaWANMacCommandQueue_t commandsDown = { 
      .commands = { { .cid = 0, .len = 0, .payload = { 0 }, .repeat = 0, } },
      .numCommands = 0,
    };

    // the following is either provided by the network server (OTAA)
    // or directly entered by the user (ABP)
    uint32_t devAddr = 0;
    uint8_t appSKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t fNwkSIntKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t sNwkSIntKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t nwkSEncKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t jSIntKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };

    // available channel frequencies from list passed during OTA activation
    float availableChannelsFreq[2][RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS] = { { 0 }, { 0 } };

    // currently configured channel frequency
    float channelFreq[2] = { 0 };

    // LoRaWAN revision (1.0 vs 1.1)
    uint8_t rev = 0;

    // currently configured data rate for uplink and downlink: DR0 - DR15 (band-dependent!)
    uint8_t dataRate[2] = { 0 };

    // currently configured channel for uplink and downlink (band-dependent!)
    uint8_t chIndex[2] = { 0 };

    // backup channel properties - may be changed by MAC command
    float backupFreq = 0;
    uint8_t backupDataRate = 0;

    // timestamp to measure the RX1/2 delay (from uplink end)
    uint32_t rxDelayStart = 0;

    // delays between the uplink and RX1/2 windows
    uint32_t rxDelays[2] = { RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS, RADIOLIB_LORAWAN_RECEIVE_DELAY_2_MS };

    // device status - battery level
    uint8_t battLevel = 0xFF;

    // method to generate message integrity code
    uint32_t generateMIC(uint8_t* msg, size_t len, uint8_t* key);

    // method to verify message integrity code
    // it assumes that the MIC is the last 4 bytes of the message
    bool verifyMIC(uint8_t* msg, size_t len, uint8_t* key);

    // configure the common physical layer properties (preamble, sync word etc.)
    // channels must be configured separately by setupChannels()!
    int16_t setPhyProperties();

    // setup uplink/downlink channel data rates and frequencies
    // will attempt to randomly select based on currently used band plan
    int16_t setupChannels();

    // find the first usable data rate in a given channel span
    uint8_t findDataRate(uint8_t dr, DataRate_t* dataRate, const LoRaWANChannelSpan_t* span);

    // find a channel ID that conforms to the requested direction and ID range
    int16_t findChannelId(uint8_t dir, uint8_t* ch, uint8_t* dr, int8_t min, int8_t max);

    // find a channel span that any given channel id belongs to
    LoRaWANChannelSpan_t* findChannelSpan(uint8_t dir, uint8_t ch, uint8_t* spanChannelId);

    // calculate channel frequency in MHz based on channel ID and direction
    int16_t findChannelFreq(uint8_t dir, uint8_t ch, float* freq);

    // configure channel based on cached data rate ID and frequency
    int16_t configureChannel(uint8_t dir);

    // send a MAC command to the network server
    int16_t sendMacCommand(uint8_t cid, uint8_t* payload, size_t payloadLen, uint8_t* reply, size_t replyLen);

    // push MAC command to queue, done by copy
    int16_t pushMacCommand(LoRaWANMacCommand_t* cmd, LoRaWANMacCommandQueue_t* queue);
    
    // pop MAC command from queue, done by copy unless CMD is NULL
    int16_t popMacCommand(LoRaWANMacCommand_t* cmd, LoRaWANMacCommandQueue_t* queue, bool force = false);

    // execute mac command, return the number of processed bytes for sequential processing
    size_t execMacCommand(LoRaWANMacCommand_t* cmd);

    // function to encrypt and decrypt payloads
    void processAES(uint8_t* in, size_t len, uint8_t* key, uint8_t* out, uint32_t fcnt, uint8_t dir, uint8_t ctrId, bool counter);

    // network-to-host conversion method - takes data from network packet and converts it to the host endians
    template<typename T>
    static T ntoh(uint8_t* buff, size_t size = 0);

    // host-to-network conversion method - takes data from host variable and and converts it to network packet endians
    template<typename T>
    static void hton(uint8_t* buff, T val, size_t size = 0);
};

#endif
