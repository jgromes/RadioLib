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

// data rat encoding
#define RADIOLIB_LORAWAN_DATA_RATE_SF_12                        (0x06 << 4) //  7     4     LoRaWAN spreading factor: SF12
#define RADIOLIB_LORAWAN_DATA_RATE_SF_11                        (0x05 << 4) //  7     4                               SF11
#define RADIOLIB_LORAWAN_DATA_RATE_SF_10                        (0x04 << 4) //  7     4                               SF10
#define RADIOLIB_LORAWAN_DATA_RATE_SF_9                         (0x03 << 4) //  7     4                               SF9
#define RADIOLIB_LORAWAN_DATA_RATE_SF_8                         (0x02 << 4) //  7     4                               SF8
#define RADIOLIB_LORAWAN_DATA_RATE_SF_7                         (0x01 << 4) //  7     4                               SF7
#define RADIOLIB_LORAWAN_DATA_RATE_FSK_50_K                     (0x00 << 4) //  7     4                               FSK @ 50 kbps
#define RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ                   (0x00 << 0) //  3     0     LoRaWAN bandwidth: 500 kHz
#define RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ                   (0x01 << 0) //  3     0                        250 kHz
#define RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ                   (0x02 << 0) //  3     0                        125 kHz
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

// join accept message layout
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN                    (33)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_NONCE_POS             (1)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_HOME_NET_ID_POS            (4)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_ADDR_POS               (7)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DL_SETTINGS_POS            (11)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_RX_DELAY_POS               (12)
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

// uplink message layout
#define RADIOLIB_LORAWAN_UPLINK_LEN(PAYLOAD, FOPTS)             (16 + 13 + (PAYLOAD) + (FOPTS))
#define RADIOLIB_LORAWAN_UPLINK_LEN_START_OFFS                  (16)
#define RADIOLIB_LORAWAN_UPLINK_DEV_ADDR_POS                    (RADIOLIB_LORAWAN_UPLINK_LEN_START_OFFS + 1)
#define RADIOLIB_LORAWAN_UPLINK_FCTRL_POS                       (RADIOLIB_LORAWAN_UPLINK_LEN_START_OFFS + 5)
#define RADIOLIB_LORAWAN_UPLINK_FCNT_POS                        (RADIOLIB_LORAWAN_UPLINK_LEN_START_OFFS + 6)
#define RADIOLIB_LORAWAN_UPLINK_FOPTS_POS                       (RADIOLIB_LORAWAN_UPLINK_LEN_START_OFFS + 8)
#define RADIOLIB_LORAWAN_UPLINK_FOPTS_MAX_LEN                   (RADIOLIB_LORAWAN_UPLINK_LEN_START_OFFS + 16)
#define RADIOLIB_LORAWAN_UPLINK_FPORT_POS(FOPTS)                (RADIOLIB_LORAWAN_UPLINK_LEN_START_OFFS + 8 + (FOPTS))
#define RADIOLIB_LORAWAN_UPLINK_PAYLOAD_POS(FOPTS)              (RADIOLIB_LORAWAN_UPLINK_LEN_START_OFFS + 9 + (FOPTS))

// payload encryption/MIC blocks common layout
#define RADIOLIB_LORAWAN_BLOCK_MAGIC_POS                        (0)
#define RADIOLIB_LORAWAN_BLOCK_DIR_POS                          (5)
#define RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS                     (6)
#define RADIOLIB_LORAWAN_BLOCK_FCNT_POS                         (10)

// payload encryption block layout
#define RADIOLIB_LORAWAN_ENC_BLOCK_MAGIC                        (0x01)
#define RADIOLIB_LORAWAN_ENC_BLOCK_COUNTER_POS                  (15)

// payload MIC blocks layout
#define RADIOLIB_LORAWAN_MIC_BLOCK_MAGIC                        (0x49)
#define RADIOLIB_LORAWAN_MIC_BLOCK_LEN_POS                      (15)
#define RADIOLIB_LORAWAN_MIC_DATA_RATE_POS                      (3)
#define RADIOLIB_LORAWAN_MIC_CH_INDEX_POS                       (4)

#define RADIOLIB_LORAWAN_MAGIC                                  (0x12AD101B)

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
  /*! \brief The base downlik data rate. Used to calculate data rate changes for adaptive data rate */
  uint8_t downlinkDataRateBase;

  /*! \brief The minimum allowed downlik data rate. Used to calculate data rate changes for adaptive data rate */
  uint8_t downlinkDataRateMin;

  /*! \brief Array of allowed maximum payload lengths for each data rate */
  uint8_t payloadLenMax[RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES];

  /*! \brief Maximum allowed output power in this band in dBm */
  int8_t powerMax;

  /*! \brief Number of power steps in this band */
  int8_t powerNumSteps;

  /*! \brief Whether the optional channels are defined as list of frequencies or bit mask */
  uint8_t cfListType;
  
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
  \class LoRaWANNode
  \brief LoRaWAN-compatible node (class A device).
*/
class LoRaWANNode {
  public:
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
      \param appEUI 8-byte application identifier.
      \param devEUI 8-byte device identifier.
      \param nwkKey Pointer to the network AES-128 key.
      \param appKey Pointer to the application AES-128 key.
      \param force Set to true to force joining even if previously joined.
      \returns \ref status_codes
    */
    int16_t beginOTAA(uint64_t appEUI, uint64_t devEUI, uint8_t* nwkKey, uint8_t* appKey, bool force = false);

    /*!
      \brief Join network by performing activation by personalization.
      In this procedure, all necessary configuration must be provided by the user.
      \param addr Device address.
      \param net Network ID.
      \param nwkSKey Pointer to the network session AES-128 key.
      \param appSKey Pointer to the application session AES-128 key.
      \returns \ref status_codes
    */
    int16_t beginAPB(uint32_t addr, uint8_t net, uint8_t* nwkSKey, uint8_t* appSKey);

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

    /*!
      \brief Configure the radio to a given channel frequency and data rate.
      \param chan Channel ID to set.
      \param dr Data rate to set, DR0 - DR15.
      \returns \ref status_codes
    */
    int16_t configureChannel(uint8_t chan, uint8_t dr);

#if !defined(RADIOLIB_GODMODE)
  private:
#endif
    PhysicalLayer* phyLayer;
    const LoRaWANBand_t* band;

    // the following is either provided by the network server (OTAA)
    // or directly entered by the user (ABP)
    uint32_t devAddr;
    uint8_t appSKey[RADIOLIB_AES128_KEY_SIZE];
    uint8_t fNwkSIntKey[RADIOLIB_AES128_KEY_SIZE];
    uint8_t sNwkSIntKey[RADIOLIB_AES128_KEY_SIZE];
    uint8_t nwkSEncKey[RADIOLIB_AES128_KEY_SIZE];
    uint8_t rxDelay;
    float availableChannelsFreq[5];
    uint16_t availableChannelsMask[6];

    // LoRaWAN revision (1.0 vs 1.1)
    uint8_t rev;

    // currently configured data rate DR0 - DR15 (band-dependent!)
    uint8_t dataRate;

    // currently configured channel (band-dependent!)
    uint8_t chIndex;

    // method to generate message integrity code
    uint32_t generateMIC(uint8_t* msg, size_t len, uint8_t* key);

    // method to verify message integrity code
    // it assumes that the MIC is the last 4 bytes of the message
    bool verifyMIC(uint8_t* msg, size_t len, uint8_t* key);

    int16_t setPhyProperties();

    // network-to-host conversion method - takes data from network packet and converts it to the host endians
    template<typename T>
    static T ntoh(uint8_t* buff, size_t size = 0);

    // host-to-network conversion method - takes data from host variable and and converts it to network packet endiands
    template<typename T>
    static void hton(uint8_t* buff, T val, size_t size = 0);
};

#endif
