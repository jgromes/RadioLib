#include "LoRaWAN.h"

#include <string.h>

#if !defined(RADIOLIB_EXCLUDE_LORAWAN)

// flag to indicate whether we have received a downlink
static volatile bool downlinkReceived = false;

#if defined(RADIOLIB_EEPROM_UNSUPPORTED)
  #warning "Persistent storage not supported!"
#endif

// interrupt service routine to handle downlinks automatically
#if defined(ESP8266) || defined(ESP32)
  IRAM_ATTR
#endif
static void LoRaWANNodeOnDownlink(void) {
  downlinkReceived = true;
}

// flag to indicate whether channel scan operation is complete
static volatile bool scanFlag = false;

// interrupt service routine to handle downlinks automatically
#if defined(ESP8266) || defined(ESP32)
  IRAM_ATTR
#endif
static void LoRaWANNodeOnChannelScan(void) {
  scanFlag = true;
}

LoRaWANNode::LoRaWANNode(PhysicalLayer* phy, const LoRaWANBand_t* band) {
  this->phyLayer = phy;
  this->band = band;
  this->FSK = false;
  this->startChannel = -1;
  this->numChannels = -1;
  this->backupFreq = this->band->backupChannel.freqStart;
  this->difsSlots = 2;
  this->backoffMax = 6;
  this->enableCSMA = false;

}

void LoRaWANNode::wipe() {
  Module* mod = this->phyLayer->getMod();
  mod->hal->wipePersistentStorage();
}

void LoRaWANNode::setCSMA(uint8_t backoffMax, uint8_t difsSlots, bool enableCSMA) {
    this->backoffMax = backoffMax;
    this->difsSlots = difsSlots;
    this->enableCSMA = enableCSMA;
}


int16_t LoRaWANNode::restoreOTAA() {
  int16_t state = this->setPhyProperties();
  RADIOLIB_ASSERT(state);

  // check the magic value
  Module* mod = this->phyLayer->getMod();
  if(mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID) != RADIOLIB_LORAWAN_MAGIC) {
    // the magic value is not set, user will have to do perform the join procedure
    return(RADIOLIB_ERR_NETWORK_NOT_JOINED);
  }

  // in case of future revisions of NVM, use a version parameter to allow transitioning from one version to another while keeping session alive
  uint16_t nvm_table_version = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_VERSION_ID);
  // if (RADIOLIB_PERSISTENT_PARAM_LORAWAN_VERSION > nvm_table_version) {
  //  // set default values for variables that are new or something
  // }
  (void)nvm_table_version;

  // pull all needed information from persistent storage
  this->devAddr = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_ADDR_ID);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_APP_S_KEY_ID), this->appSKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FNWK_SINT_KEY_ID), this->fNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_SNWK_SINT_KEY_ID), this->sNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NWK_SENC_KEY_ID), this->nwkSEncKey, RADIOLIB_AES128_BLOCK_SIZE);
  RADIOLIB_DEBUG_PRINTLN("appSKey:");
  RADIOLIB_DEBUG_HEXDUMP(this->appSKey, RADIOLIB_AES128_BLOCK_SIZE);

  uint32_t dlSettings = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DL_SETTINGS_ID);
  this->rev = (dlSettings & RADIOLIB_LORAWAN_JOIN_ACCEPT_R_1_1) >> 7;
  uint8_t rx1DrOffset = (dlSettings & 0x70) >> 4;
  uint8_t rx2DataRate = dlSettings & 0x0F;
  RADIOLIB_DEBUG_PRINTLN("LoRaWAN revision: %d", this->rev);
  
  // TODO process the RX2 data rate
  (void)rx2DataRate;

  // TODO process the data rate offset
  (void)rx1DrOffset;

  // parse Rx1 delay (and subsequently Rx2)
  this->rxDelays[0] = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX_DELAY_ID);
  if(this->rxDelays[0] == 0) {
    this->rxDelays[0] = RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS;
  }
  this->rxDelays[1] = this->rxDelays[0] + 1000;

  // process CFlist if any bit is non-zero
  uint8_t cfList[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN] = { 0 };
  uint8_t allZero[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN] = { 0 };
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_CF_LIST_ID), cfList, RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN);
  RADIOLIB_DEBUG_PRINTLN("cfList:");
  RADIOLIB_DEBUG_HEXDUMP(cfList, RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN);
  if(memcmp(cfList, allZero, RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN)) {
    if(this->band->cfListType == RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES) {
      // list of frequencies
      for(uint8_t i = 0; i < 5; i++) {
        uint32_t freq = LoRaWANNode::ntoh<uint32_t>(&cfList[3*i], 3);
        availableChannelsFreq[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i] = (float)freq/10000.0;
        availableChannelsFreq[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i] = availableChannelsFreq[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i];
        RADIOLIB_DEBUG_PRINTLN("Channel UL/DL %d frequency = %f MHz", i, availableChannelsFreq[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i]);
      }

    } else {
      // frequency mask, we need to find out which frequencies are actually being used
      uint8_t channelId = 0;
      uint8_t chSpan = 0;
      uint8_t chNum = 0;
      for(uint8_t i = 0; i < 5; i++) {
        uint16_t mask = LoRaWANNode::ntoh<uint16_t>(&cfList[2*i]);
        RADIOLIB_DEBUG_PRINTLN("mask[%d] = 0x%04x", i, mask);
        for(uint8_t j = 0; j < 16; j++) {
          if(chNum >= this->band->defaultChannels[chSpan].numChannels) {
            chNum -= this->band->defaultChannels[chSpan].numChannels;
            chSpan++;

            if(chSpan >= this->band->numChannelSpans) {
              RADIOLIB_DEBUG_PRINTLN("channel bitmask overrun!");
              return(RADIOLIB_ERR_UNKNOWN);
            }
          }

          if(mask & (1UL << j)) {
            RADIOLIB_DEBUG_PRINTLN("chNum = %d, chSpan = %d", chNum, chSpan);
            uint8_t dir = this->band->defaultChannels[chSpan].direction;
            float freq = this->band->defaultChannels[chSpan].freqStart + chNum*this->band->defaultChannels[chSpan].freqStep;
            availableChannelsFreq[dir][channelId] = freq;
            RADIOLIB_DEBUG_PRINTLN("Channel %cL %d frequency = %f MHz", dir ? 'U': 'D', channelId, availableChannelsFreq[dir][channelId]);
            channelId++;
          }

          chNum++;
        }
      }

    }
  }

  uint8_t queueBuff[sizeof(LoRaWANMacCommandQueue_t)] = { 0 };
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FOPTS_ID), queueBuff, sizeof(LoRaWANMacCommandQueue_t));
  memcpy(&queueBuff, &this->commandsUp, sizeof(LoRaWANMacCommandQueue_t));

  state = this->setupChannels();
  RADIOLIB_ASSERT(state);
  
  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::beginOTAA(uint64_t joinEUI, uint64_t devEUI, uint8_t* nwkKey, uint8_t* appKey, bool force) {
  // check if we actually need to send the join request
  Module* mod = this->phyLayer->getMod();
  if(!force && (mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID) == RADIOLIB_LORAWAN_MAGIC)) {
    // the device has joined already, we can just pull the data from persistent storage
    return(this->restoreOTAA());
  }

  // set the physical layer configuration
  int16_t state = this->setPhyProperties();
  RADIOLIB_ASSERT(state);

  // setup uplink/downlink frequencies and datarates
  state = this->setupChannels();
  RADIOLIB_ASSERT(state);

  // get dev nonce from persistent storage and increment it
  uint16_t devNonce = mod->hal->getPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_NONCE_ID);
  mod->hal->setPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_NONCE_ID, devNonce + 1);

  // build the join-request message
  uint8_t joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_LEN];
  
  // set the packet fields
  joinRequestMsg[0] = RADIOLIB_LORAWAN_MHDR_MTYPE_JOIN_REQUEST | RADIOLIB_LORAWAN_MHDR_MAJOR_R1;
  LoRaWANNode::hton<uint64_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_JOIN_EUI_POS], joinEUI);
  LoRaWANNode::hton<uint64_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_DEV_EUI_POS], devEUI);
  LoRaWANNode::hton<uint16_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_DEV_NONCE_POS], devNonce);

  // add the authentication code
  uint32_t mic = this->generateMIC(joinRequestMsg, RADIOLIB_LORAWAN_JOIN_REQUEST_LEN - sizeof(uint32_t), nwkKey);
  LoRaWANNode::hton<uint32_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_LEN - sizeof(uint32_t)], mic);

  // send it
  state = this->phyLayer->transmit(joinRequestMsg, RADIOLIB_LORAWAN_JOIN_REQUEST_LEN);
  RADIOLIB_ASSERT(state);

  // configure for downlink with default configuration
  state = this->configureChannel(RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK);
  RADIOLIB_ASSERT(state);
  
  // set the function that will be called when the reply is received
  this->phyLayer->setPacketReceivedAction(LoRaWANNodeOnDownlink);

  // downlink messages are sent with inverted IQ
  // TODO use downlink() for this
  if(!this->FSK) {
    state = this->phyLayer->invertIQ(true);
    RADIOLIB_ASSERT(state);
  }
  
  // start receiving
  uint32_t start = mod->hal->millis();
  downlinkReceived = false;
  state = this->phyLayer->startReceive();
  RADIOLIB_ASSERT(state);

  // wait for the reply or timeout
  while(!downlinkReceived) {
    if(mod->hal->millis() - start >= RADIOLIB_LORAWAN_JOIN_ACCEPT_DELAY_2_MS + 2000) {
      downlinkReceived = false;
      if(!this->FSK) {
        this->phyLayer->invertIQ(false);
      }
      return(RADIOLIB_ERR_RX_TIMEOUT);
    }
  }

  // we have a message, reset the IQ inversion
  downlinkReceived = false;
  this->phyLayer->clearPacketReceivedAction();
  if(!this->FSK) {
    state = this->phyLayer->invertIQ(false);
    RADIOLIB_ASSERT(state);
  }

  // build the buffer for the reply data
  uint8_t joinAcceptMsgEnc[RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN];

  // check received length
  size_t lenRx = this->phyLayer->getPacketLength(true);
  if((lenRx != RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN) && (lenRx != RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN - RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN)) {
    RADIOLIB_DEBUG_PRINTLN("joinAccept reply length mismatch, expected %luB got %luB", RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN, lenRx);
    return(RADIOLIB_ERR_DOWNLINK_MALFORMED);
  }

  // read the packet
  state = this->phyLayer->readData(joinAcceptMsgEnc, lenRx);
  // downlink frames are sent without CRC, which will raise error on SX127x
  // we can ignore that error
  if(state != RADIOLIB_ERR_LORA_HEADER_DAMAGED) {
    RADIOLIB_ASSERT(state);
  }

  // check reply message type
  if((joinAcceptMsgEnc[0] & RADIOLIB_LORAWAN_MHDR_MTYPE_MASK) != RADIOLIB_LORAWAN_MHDR_MTYPE_JOIN_ACCEPT) {
    RADIOLIB_DEBUG_PRINTLN("joinAccept reply message type invalid, expected 0x%02x got 0x%02x", RADIOLIB_LORAWAN_MHDR_MTYPE_JOIN_ACCEPT, joinAcceptMsgEnc[0]);
    return(RADIOLIB_ERR_DOWNLINK_MALFORMED);
  }

  // decrypt the join accept message
  // this is done by encrypting again in ECB mode
  // the first byte is the MAC header which is not encrypted
  uint8_t joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN];
  joinAcceptMsg[0] = joinAcceptMsgEnc[0];
  RadioLibAES128Instance.init(nwkKey);
  RadioLibAES128Instance.encryptECB(&joinAcceptMsgEnc[1], RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN - 1, &joinAcceptMsg[1]);
  
  RADIOLIB_DEBUG_PRINTLN("joinAcceptMsg:");
  RADIOLIB_DEBUG_HEXDUMP(joinAcceptMsg, lenRx);

  // get current JoinNonce from downlink and previous JoinNonce from NVM
  uint32_t joinNonce = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_NONCE_POS], 3);
  uint32_t joinNoncePrev = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_JOIN_NONCE_ID);
  RADIOLIB_DEBUG_PRINTLN("JoinNoncePrev: %d, JoinNonce: %d", joinNoncePrev, joinNonce);
  
  // JoinNonce received must be greater than the last JoinNonce heard, else error
  if(joinNonce <= joinNoncePrev) {
    return(RADIOLIB_ERR_JOIN_NONCE_INVALID);
  }

  // check LoRaWAN revision (the MIC verification depends on this)
  uint8_t dlSettings = joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_DL_SETTINGS_POS];
  this->rev = (dlSettings & RADIOLIB_LORAWAN_JOIN_ACCEPT_R_1_1) >> 7;
  RADIOLIB_DEBUG_PRINTLN("LoRaWAN revision: 1.%d", this->rev);

  // verify MIC
  if(this->rev == 1) {
    // 1.1 version, first we need to derive the join accept integrity key
    uint8_t keyDerivationBuff[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_JS_INT_KEY;
    LoRaWANNode::hton<uint64_t>(&keyDerivationBuff[1], devEUI);
    RadioLibAES128Instance.init(nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->jSIntKey);

    // prepare the buffer for MIC calculation
    uint8_t micBuff[3*RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
    micBuff[0] = RADIOLIB_LORAWAN_JOIN_REQUEST_TYPE;
    LoRaWANNode::hton<uint64_t>(&micBuff[1], joinEUI);
    LoRaWANNode::hton<uint16_t>(&micBuff[9], devNonce);
    memcpy(&micBuff[11], joinAcceptMsg, lenRx);
    
    if(!verifyMIC(micBuff, lenRx + 11, this->jSIntKey)) {
      return(RADIOLIB_ERR_CRC_MISMATCH);
    }
  
  } else {
    // 1.0 version
    if(!verifyMIC(joinAcceptMsg, lenRx, nwkKey)) {
      return(RADIOLIB_ERR_CRC_MISMATCH);
    }

  }
  uint8_t rx1DrOffset = (dlSettings & 0x70) >> 4;
  uint8_t rx2DataRate = dlSettings & 0x0F;
  
  // TODO process the RX2 data rate
  (void)rx2DataRate;

  // TODO process the data rate offset
  (void)rx1DrOffset;

  // parse other contents
  uint32_t homeNetId = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_HOME_NET_ID_POS], 3);
  this->devAddr = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_ADDR_POS]);

  // parse Rx1 delay (and subsequently Rx2)
  this->rxDelays[0] = joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_RX_DELAY_POS]*1000;
  if(this->rxDelays[0] == 0) {
    this->rxDelays[0] = RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS;
  }
  this->rxDelays[1] = this->rxDelays[0] + 1000;

  // process CFlist if present
  uint8_t cfList[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN] = { 0 };
  if(lenRx == RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN) {
    memcpy(&cfList[0], &joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_POS], RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN);
    if(this->band->cfListType == RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES) {
      // list of frequencies
      for(uint8_t i = 0; i < 5; i++) {
        uint32_t freq = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_POS + 3*i], 3);
        availableChannelsFreq[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i] = (float)freq/10000.0;
        availableChannelsFreq[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i] = availableChannelsFreq[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i];
        RADIOLIB_DEBUG_PRINT("Channel UL/DL %d frequency = ", i);
        RADIOLIB_DEBUG_PRINT_FLOAT(availableChannelsFreq[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i], 3);
        RADIOLIB_DEBUG_PRINTLN(" MHz");
      }

    } else {
      // frequency mask, we need to find out which frequencies are actually being used
      uint8_t channelId = 0;
      uint8_t chSpan = 0;
      uint8_t chNum = 0;
      for(uint8_t i = 0; i < 5; i++) {
        uint16_t mask = LoRaWANNode::ntoh<uint16_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_POS + 2*i]);
        RADIOLIB_DEBUG_PRINTLN("mask[%d] = 0x%04x", i, mask);
        for(uint8_t j = 0; j < 16; j++) {
          if(chNum >= this->band->defaultChannels[chSpan].numChannels) {
            chNum -= this->band->defaultChannels[chSpan].numChannels;
            chSpan++;

            if(chSpan >= this->band->numChannelSpans) {
              RADIOLIB_DEBUG_PRINTLN("channel bitmask overrun!");
              return(RADIOLIB_ERR_UNKNOWN);
            }
          }

          if(mask & (1UL << j)) {
            RADIOLIB_DEBUG_PRINTLN("chNum = %d, chSpan = %d", chNum, chSpan);
            uint8_t dir = this->band->defaultChannels[chSpan].direction;
            float freq = this->band->defaultChannels[chSpan].freqStart + chNum*this->band->defaultChannels[chSpan].freqStep;
            availableChannelsFreq[dir][channelId] = freq;
            RADIOLIB_DEBUG_PRINT("Channel %cL %d frequency = ", dir ? 'U': 'D', channelId);
            RADIOLIB_DEBUG_PRINT_FLOAT(availableChannelsFreq[dir][channelId], 3);
            RADIOLIB_DEBUG_PRINTLN(" MHz");
            channelId++;
          }

          chNum++;
        }
      }

    }
  } 

  // prepare buffer for key derivation
  uint8_t keyDerivationBuff[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  LoRaWANNode::hton<uint32_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_NONCE_POS], joinNonce, 3);

  // check protocol version (1.0 vs 1.1)
  if(this->rev == 1) {
    // 1.1 version, derive the keys
    LoRaWANNode::hton<uint64_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_EUI_POS], joinEUI);
    LoRaWANNode::hton<uint16_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_NONCE_POS], devNonce);
    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_APP_S_KEY;

    RadioLibAES128Instance.init(appKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->appSKey);

    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_F_NWK_S_INT_KEY;
    RadioLibAES128Instance.init(nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->fNwkSIntKey);

    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_S_NWK_S_INT_KEY;
    RadioLibAES128Instance.init(nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->sNwkSIntKey);

    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_NWK_S_ENC_KEY;
    RadioLibAES128Instance.init(nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->nwkSEncKey);

    // enqueue the RekeyInd MAC command to be sent in the next uplink
    LoRaWANMacCommand_t cmd = {
      .cid = RADIOLIB_LORAWAN_MAC_CMD_REKEY,
      .len = sizeof(uint8_t),
      .payload = { this->rev },
      .repeat = RADIOLIB_LORAWAN_ADR_ACK_LIMIT,
    };
    state = pushMacCommand(&cmd, &this->commandsUp);
    RADIOLIB_ASSERT(state);
  
  } else {
    // 1.0 version, just derive the keys
    LoRaWANNode::hton<uint32_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_HOME_NET_ID_POS], homeNetId, 3);
    LoRaWANNode::hton<uint16_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_ADDR_POS], devNonce);
    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_APP_S_KEY;
    RadioLibAES128Instance.init(nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->appSKey);

    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_F_NWK_S_INT_KEY;
    RadioLibAES128Instance.init(nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->fNwkSIntKey);

    memcpy(this->sNwkSIntKey, this->fNwkSIntKey, RADIOLIB_AES128_KEY_SIZE);
    memcpy(this->nwkSEncKey, this->fNwkSIntKey, RADIOLIB_AES128_KEY_SIZE);
  
  }

  // save the device address
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_ADDR_ID, this->devAddr);

  // update the keys
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_APP_S_KEY_ID), this->appSKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FNWK_SINT_KEY_ID), this->fNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_SNWK_SINT_KEY_ID), this->sNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NWK_SENC_KEY_ID), this->nwkSEncKey, RADIOLIB_AES128_BLOCK_SIZE);

  // save uplink parameters
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_JOIN_NONCE_ID, joinNonce);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_HOME_NET_ID, homeNetId);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX_DELAY_ID, this->rxDelays[0]);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DL_SETTINGS_ID, (uint32_t)dlSettings);

  // save cfList (all 0 if none is present)
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_CF_LIST_ID), cfList, RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN);

  // all complete, reset device counters and set the magic number
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID, 0);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_N_FCNT_DOWN_ID, 0);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_A_FCNT_DOWN_ID, 0);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID, RADIOLIB_LORAWAN_MAGIC);

  // everything written to NVM, write current version to NVM
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_VERSION_ID, RADIOLIB_PERSISTENT_PARAM_LORAWAN_VERSION);
  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::beginABP(uint32_t addr, uint8_t* nwkSKey, uint8_t* appSKey, uint8_t* fNwkSIntKey, uint8_t* sNwkSIntKey) {
  this->devAddr = addr;
  memcpy(this->appSKey, appSKey, RADIOLIB_AES128_KEY_SIZE);
  memcpy(this->nwkSEncKey, nwkSKey, RADIOLIB_AES128_KEY_SIZE);
  if(fNwkSIntKey) {
    this->rev = 1;
    memcpy(this->fNwkSIntKey, fNwkSIntKey, RADIOLIB_AES128_KEY_SIZE);
  } else {
    memcpy(this->fNwkSIntKey, nwkSKey, RADIOLIB_AES128_KEY_SIZE);
  }
  if(sNwkSIntKey) {
    memcpy(this->sNwkSIntKey, sNwkSIntKey, RADIOLIB_AES128_KEY_SIZE);
  }

  // set the physical layer configuration
  int16_t state = this->setPhyProperties();
  RADIOLIB_ASSERT(state);

  // setup uplink/downlink frequencies and datarates
  state = this->setupChannels();
  RADIOLIB_ASSERT(state);

  // everything written to NVM, write current version to NVM
  Module* mod = this->phyLayer->getMod();
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_VERSION_ID, RADIOLIB_PERSISTENT_PARAM_LORAWAN_VERSION);

  return(RADIOLIB_ERR_NONE);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t LoRaWANNode::uplink(String& str, uint8_t port) {
  return(this->uplink(str.c_str(), port));
}
#endif

int16_t LoRaWANNode::uplink(const char* str, uint8_t port) {
  return(this->uplink((uint8_t*)str, strlen(str), port));
}

int16_t LoRaWANNode::uplink(uint8_t* data, size_t len, uint8_t port) {
  // check destination port
  if(port > 0xDF) {
    return(RADIOLIB_ERR_INVALID_PORT);
  }
  // port 0 is only allowed for MAC-only payloads
  if(port == RADIOLIB_LORAWAN_FPORT_MAC_COMMAND) {
    if (!isMACPayload) {
      return(RADIOLIB_ERR_INVALID_PORT);
    }
    // if this is MAC only payload, continue and reset for next uplink
    isMACPayload = false;
  }
  
  Module* mod = this->phyLayer->getMod();

  // check if there are some MAC commands to piggyback (only when piggybacking onto a application-frame)
  uint8_t foptsLen = 0;
  size_t foptsBufSize = 0;
  if(this->commandsUp.numCommands > 0 && port != RADIOLIB_LORAWAN_FPORT_MAC_COMMAND) {
    // there are, assume the maximum possible FOpts len for buffer allocation
    foptsLen = this->commandsUp.len;
    foptsBufSize = 15;
  }

  // check maximum payload len as defined in phy
  if(len > this->band->payloadLenMax[this->dataRate[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK]]) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // configure for uplink
  // TODO select randomly from available channels
  int16_t state = this->configureChannel(RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK);
  RADIOLIB_ASSERT(state);

  // check if sufficient time has elapsed since the last uplink
  if(mod->hal->millis() - this->rxDelayStart < rxDelays[1]) {
    // not enough time elapsed since the last uplink, we may still be in an RX window
    return(RADIOLIB_ERR_UPLINK_UNAVAILABLE);
  }

  // build the uplink message
  // the first 16 bytes are reserved for MIC calculation blocks
  size_t uplinkMsgLen = RADIOLIB_LORAWAN_FRAME_LEN(len, foptsBufSize);
  #if defined(RADIOLIB_STATIC_ONLY)
  uint8_t uplinkMsg[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
  uint8_t* uplinkMsg = new uint8_t[uplinkMsgLen];
  #endif
  
  // set the packet fields
  uplinkMsg[RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS] = RADIOLIB_LORAWAN_MHDR_MTYPE_UNCONF_DATA_UP | RADIOLIB_LORAWAN_MHDR_MAJOR_R1;
  LoRaWANNode::hton<uint32_t>(&uplinkMsg[RADIOLIB_LORAWAN_FHDR_DEV_ADDR_POS], this->devAddr);

  // TODO implement adaptive data rate
  // length of fopts will be added later
  uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] = 0x00;

  // get frame counter from persistent storage
  uint32_t fcnt = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID) + 1;
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID, fcnt);
  LoRaWANNode::hton<uint16_t>(&uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCNT_POS], (uint16_t)fcnt);

  // check if we have some MAC commands to append
  if(foptsLen > 0) {
    uint8_t foptsNum = this->commandsUp.numCommands;
    uint8_t foptsBuff[foptsBufSize];
    size_t idx = 0;
    for (size_t i = 0; i < foptsNum; i++) {
      LoRaWANMacCommand_t cmd = { .cid = 0, .len = 0, .payload = { 0 }, .repeat = 0, };
      popMacCommand(&cmd, &this->commandsUp, i);
      if (cmd.cid == 0) {
        break;
      }
      foptsBuff[idx] = cmd.cid;
      for(size_t i = 1; i < cmd.len; i++) {
        foptsBuff[idx + i] = cmd.payload[i];
      }
      idx += cmd.len + 1;
    }

    RADIOLIB_DEBUG_PRINTLN("Uplink MAC payload (%d commands):", foptsNum);
    RADIOLIB_DEBUG_HEXDUMP(foptsBuff, foptsBufSize);

    uplinkMsgLen = RADIOLIB_LORAWAN_FRAME_LEN(len, foptsLen);
    uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] |= foptsLen;

    // encrypt it
    processAES(foptsBuff, foptsLen, this->nwkSEncKey, &uplinkMsg[RADIOLIB_LORAWAN_FHDR_FOPTS_POS], fcnt, RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK, 0x01, true);
    
    // write the current MAC command queue to nvm for next uplink
    uint8_t queueBuff[sizeof(LoRaWANMacCommandQueue_t)];
    memcpy(&queueBuff, &this->commandsUp, sizeof(LoRaWANMacCommandQueue_t));
    mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FOPTS_ID), queueBuff, sizeof(LoRaWANMacCommandQueue_t));
  }

  // set the port
  uplinkMsg[RADIOLIB_LORAWAN_FHDR_FPORT_POS(foptsLen)] = port;

  // select encryption key based on the target port
  uint8_t* encKey = this->appSKey;
  if(port == RADIOLIB_LORAWAN_FPORT_MAC_COMMAND) {
    encKey = this->nwkSEncKey;
  }

  // encrypt the frame payload
  // TODO check ctrId --> erratum says it should be 0x01?
  processAES(data, len, encKey, &uplinkMsg[RADIOLIB_LORAWAN_FRAME_PAYLOAD_POS(foptsLen)], fcnt, RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK, 0x00, true);

  // create blocks for MIC calculation
  uint8_t block0[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  block0[RADIOLIB_LORAWAN_BLOCK_MAGIC_POS] = RADIOLIB_LORAWAN_MIC_BLOCK_MAGIC;
  block0[RADIOLIB_LORAWAN_BLOCK_DIR_POS] = RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK;
  LoRaWANNode::hton<uint32_t>(&block0[RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS], this->devAddr);
  LoRaWANNode::hton<uint32_t>(&block0[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], fcnt);
  block0[RADIOLIB_LORAWAN_MIC_BLOCK_LEN_POS] = uplinkMsgLen - RADIOLIB_AES128_BLOCK_SIZE - sizeof(uint32_t);

  uint8_t block1[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  memcpy(block1, block0, RADIOLIB_AES128_BLOCK_SIZE);
  // TODO implement confirmed frames 
  block1[RADIOLIB_LORAWAN_MIC_DATA_RATE_POS] = this->dataRate[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK];
  block1[RADIOLIB_LORAWAN_MIC_CH_INDEX_POS] = this->chIndex[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK];
  
  RADIOLIB_DEBUG_PRINTLN("uplinkMsg pre-MIC:");
  RADIOLIB_DEBUG_HEXDUMP(uplinkMsg, uplinkMsgLen);

  // calculate authentication codes
  memcpy(uplinkMsg, block1, RADIOLIB_AES128_BLOCK_SIZE);
  uint32_t micS = this->generateMIC(uplinkMsg, uplinkMsgLen - sizeof(uint32_t), this->sNwkSIntKey);
  memcpy(uplinkMsg, block0, RADIOLIB_AES128_BLOCK_SIZE);
  uint32_t micF = this->generateMIC(uplinkMsg, uplinkMsgLen - sizeof(uint32_t), this->fNwkSIntKey);

  // check LoRaWAN revision
  if(this->rev == 1) {
    uint32_t mic = ((uint32_t)(micF & 0x0000FF00) << 16) | ((uint32_t)(micF & 0x0000000FF) << 16) | ((uint32_t)(micS & 0x0000FF00) >> 0) | ((uint32_t)(micS & 0x0000000FF) >> 0);
    LoRaWANNode::hton<uint32_t>(&uplinkMsg[uplinkMsgLen - sizeof(uint32_t)], mic);
  } else {
    LoRaWANNode::hton<uint32_t>(&uplinkMsg[uplinkMsgLen - sizeof(uint32_t)], micF);
  }

  // perform CSMA if enabled.
  if (enableCSMA) {
    performCSMA();
  }

  RADIOLIB_DEBUG_PRINTLN("uplinkMsg:");
  RADIOLIB_DEBUG_HEXDUMP(uplinkMsg, uplinkMsgLen);

  // send it (without the MIC calculation blocks)
  uint32_t txStart = mod->hal->millis();
  uint32_t timeOnAir = this->phyLayer->getTimeOnAir(uplinkMsgLen - RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS) / 1000;
  state = this->phyLayer->transmit(&uplinkMsg[RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS], uplinkMsgLen - RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS);
  #if !defined(RADIOLIB_STATIC_ONLY)
  delete[] uplinkMsg;
  #endif
  RADIOLIB_ASSERT(state);

  // set the timestamp so that we can measure when to start receiving
  this->rxDelayStart = txStart + timeOnAir;
  return(RADIOLIB_ERR_NONE);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t LoRaWANNode::downlink(String& str) {
  int16_t state = RADIOLIB_ERR_NONE;

  // build a temporary buffer
  // LoRaWAN downlinks can have 250 bytes at most with 1 extra byte for NULL
  size_t length = 0;
  uint8_t data[251];

  // wait for downlink
  state = this->downlink(data, &length);
  if(state == RADIOLIB_ERR_NONE) {
    // add null terminator
    data[length] = '\0';

    // initialize Arduino String class
    str = String((char*)data);
  }

  return(state);
}
#endif

int16_t LoRaWANNode::downlink(uint8_t* data, size_t* len) {
  // check if there are any upcoming Rx windows
  Module* mod = this->phyLayer->getMod();
  const uint32_t scanGuard = 500;
  if(mod->hal->millis() - this->rxDelayStart > (this->rxDelays[1] + scanGuard)) {
    // time since last Tx is greater than RX2 delay + some guard period
    // we have nothing to downlink
    return(RADIOLIB_ERR_NO_RX_WINDOW);
  }

  // configure for downlink
  int16_t state = this->configureChannel(RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK);
  RADIOLIB_ASSERT(state);

  // downlink messages are sent with inverted IQ
  if(!this->FSK) {
    state = this->phyLayer->invertIQ(true);
    RADIOLIB_ASSERT(state);
  }

  // calculate the channel scanning timeout
  // according to the spec, this must be at least enough time to effectively detect a preamble
  uint32_t scanTimeout = this->phyLayer->getTimeOnAir(0)/1000;

  // set up everything for channel scan
  downlinkReceived = false;
  scanFlag = false;
  bool packetDetected = false;
  this->phyLayer->setChannelScanAction(LoRaWANNodeOnChannelScan);

  // perform listening in the two Rx windows
  for(uint8_t i = 0; i < 2; i++) {
    // wait for the start of the Rx window
    // the waiting duration is shortened a bit to cover any possible timing errors
    uint32_t waitLen = this->rxDelays[i] - (mod->hal->millis() - this->rxDelayStart);
    if(waitLen > scanGuard) {
      waitLen -= scanGuard;
    }
    mod->hal->delay(waitLen);

    // wait until we get a preamble
    uint32_t scanStart = mod->hal->millis();
    while((mod->hal->millis() - scanStart) < (scanTimeout + scanGuard)) {
      // check channel detection timeout
      state = this->phyLayer->startChannelScan();
      RADIOLIB_ASSERT(state);

      // wait with some timeout, though it should not be hit
      uint32_t cadStart = mod->hal->millis();
      while(!scanFlag) {
        mod->hal->yield();
        if(mod->hal->millis() - cadStart >= 3000) {
          // timed out, stop waiting
          break;
        }
      }

      // check the scan result
      scanFlag = false;
      state = this->phyLayer->getChannelScanResult();
      if((state == RADIOLIB_PREAMBLE_DETECTED) || (state == RADIOLIB_LORA_DETECTED)) {
        packetDetected = true;
        break;
      }
    
    }

    // check if we have a packet
    if(packetDetected) {
      break;

    } else if(i == 0) {
      // nothing in the first window, configure for the second
      state = this->phyLayer->setFrequency(this->backupFreq);
      RADIOLIB_ASSERT(state);

      DataRate_t dataRate;
      findDataRate(RADIOLIB_LORAWAN_DATA_RATE_UNUSED, &dataRate, &this->band->backupChannel);
      state = this->phyLayer->setDataRate(dataRate);
      RADIOLIB_ASSERT(state);
    
    }
    
  }

  // check if we received a packet at all
  if(!packetDetected) {
    this->phyLayer->standby();
    if(!this->FSK) {
      this->phyLayer->invertIQ(false);
    }

    // restore the original uplink channel
    this->configureChannel(RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK);

    return(RADIOLIB_ERR_RX_TIMEOUT);
  }

  // channel scan is finished, swap the actions
  this->phyLayer->clearChannelScanAction();
  downlinkReceived = false;
  this->phyLayer->setPacketReceivedAction(LoRaWANNodeOnDownlink);

  // start receiving
  state = this->phyLayer->startReceive();
  RADIOLIB_ASSERT(state);

  // wait for reception with some timeout
  uint32_t rxStart = mod->hal->millis();
  while(!downlinkReceived) {
    mod->hal->yield();
    // let's hope 30 seconds is long enough timeout
    if(mod->hal->millis() - rxStart >= 30000) {
      // timed out
      this->phyLayer->standby();
      if(!this->FSK) {
        this->phyLayer->invertIQ(false);
      }
      return(RADIOLIB_ERR_RX_TIMEOUT);
    }
  }

  // we have a message, clear actions, go to standby and reset the IQ inversion
  downlinkReceived = false;
  this->phyLayer->standby();
  this->phyLayer->clearPacketReceivedAction();
  if(!this->FSK) {
    state = this->phyLayer->invertIQ(false);
    RADIOLIB_ASSERT(state);
  }

  // get the packet length
  size_t downlinkMsgLen = this->phyLayer->getPacketLength();

  // check the minimum required frame length
  // an extra byte is subtracted because downlink frames may not have a port
  if(downlinkMsgLen < RADIOLIB_LORAWAN_FRAME_LEN(0, 0) - 1 - RADIOLIB_AES128_BLOCK_SIZE) {
    RADIOLIB_DEBUG_PRINTLN("Downlink message too short (%lu bytes)", downlinkMsgLen);
    return(RADIOLIB_ERR_DOWNLINK_MALFORMED);
  }

  // build the buffer for the downlink message
  // the first 16 bytes are reserved for MIC calculation block
  #if !defined(RADIOLIB_STATIC_ONLY)
    uint8_t* downlinkMsg = new uint8_t[RADIOLIB_AES128_BLOCK_SIZE + downlinkMsgLen];
  #else
    uint8_t downlinkMsg[RADIOLIB_STATIC_ARRAY_SIZE];
  #endif

  // set the MIC calculation block
  // TODO implement confirmed frames
  memset(downlinkMsg, 0x00, RADIOLIB_AES128_BLOCK_SIZE);
  downlinkMsg[RADIOLIB_LORAWAN_BLOCK_MAGIC_POS] = RADIOLIB_LORAWAN_MIC_BLOCK_MAGIC;
  LoRaWANNode::hton<uint32_t>(&downlinkMsg[RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS], this->devAddr);
  downlinkMsg[RADIOLIB_LORAWAN_BLOCK_DIR_POS] = RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK;
  downlinkMsg[RADIOLIB_LORAWAN_MIC_BLOCK_LEN_POS] = downlinkMsgLen - sizeof(uint32_t);

  // read the data
  state = this->phyLayer->readData(&downlinkMsg[RADIOLIB_AES128_BLOCK_SIZE], downlinkMsgLen);
  // downlink frames are sent without CRC, which will raise error on SX127x
  // we can ignore that error
  if(state == RADIOLIB_ERR_LORA_HEADER_DAMAGED) {
    state = RADIOLIB_ERR_NONE;
  }
  
  if(state != RADIOLIB_ERR_NONE) {
    #if !defined(RADIOLIB_STATIC_ONLY)
      delete[] downlinkMsg;
    #endif
    return(state);
  }

  // get the frame counter and set it to the MIC calculation block
  // TODO cache the ADR bit?
  uint16_t fcnt16 = LoRaWANNode::ntoh<uint16_t>(&downlinkMsg[RADIOLIB_LORAWAN_FHDR_FCNT_POS]);
  LoRaWANNode::hton<uint16_t>(&downlinkMsg[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], fcnt16);
  uint32_t fcnt32 = fcnt16;   // calculate possible rollover once decided if this is network downlink or application downlink
  
  RADIOLIB_DEBUG_PRINTLN("downlinkMsg:");
  RADIOLIB_DEBUG_HEXDUMP(downlinkMsg, RADIOLIB_AES128_BLOCK_SIZE + downlinkMsgLen);

  // calculate length of FOpts and payload
  uint8_t foptsLen = downlinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] & RADIOLIB_LORAWAN_FHDR_FOPTS_LEN_MASK;
  int payLen = downlinkMsgLen - 8 - foptsLen - sizeof(uint32_t);

  bool isAppDownlink = true;
  if (payLen <= 0 && this->rev == 1) {  // no payload => MAC commands only => Network frame (LoRaWAN v1.1 only)
    isAppDownlink = false;
  }

  // check the FcntDown value (Network or Application)
  uint32_t fcntDownPrev = 0;
  if (isAppDownlink) {
    fcntDownPrev = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_A_FCNT_DOWN_ID);
  } else {
    fcntDownPrev = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_N_FCNT_DOWN_ID);
  }

  // assume a 16-bit to 32-bit rollover when difference in LSB is smaller than MAX_FCNT_GAP
  // if that isn't the case and the received fcnt is smaller or equal to the last heard fcnt, then error
  if ((fcnt16 <= fcntDownPrev) && ((0xFFFF - (uint16_t)fcntDownPrev + fcnt16) > RADIOLIB_LORAWAN_MAX_FCNT_GAP)) {
    #if !defined(RADIOLIB_STATIC_ONLY)
      delete[] downlinkMsg;
    #endif
    if (isAppDownlink) {
      return(RADIOLIB_ERR_A_FCNT_DOWN_INVALID);
    } else {
      return(RADIOLIB_ERR_N_FCNT_DOWN_INVALID);
    }
  } else if (fcnt16 <= fcntDownPrev) {
    uint16_t msb = (fcntDownPrev >> 16) + 1; // assume a rollover
    fcnt32 |= (msb << 16);                    // add back the MSB part
  }

  // save current fcnt to NVM
  if (isAppDownlink) {
    mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_A_FCNT_DOWN_ID, fcnt32);
  } else {
    mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_N_FCNT_DOWN_ID, fcnt32);
  }
  
  // check the MIC
  if(!verifyMIC(downlinkMsg, RADIOLIB_AES128_BLOCK_SIZE + downlinkMsgLen, this->sNwkSIntKey)) {
    #if !defined(RADIOLIB_STATIC_ONLY)
      delete[] downlinkMsg;
    #endif
    return(RADIOLIB_ERR_CRC_MISMATCH);
  }

  // check the address
  uint32_t addr = LoRaWANNode::ntoh<uint32_t>(&downlinkMsg[RADIOLIB_LORAWAN_FHDR_DEV_ADDR_POS]);
  if(addr != this->devAddr) {
    RADIOLIB_DEBUG_PRINTLN("Device address mismatch, expected 0x%08X, got 0x%08X", this->devAddr, addr);
    #if !defined(RADIOLIB_STATIC_ONLY)
      delete[] downlinkMsg;
    #endif
    return(RADIOLIB_ERR_DOWNLINK_MALFORMED);
  }

  // process FOpts (if there are any)
  if(foptsLen > 0) {
    // there are some Fopts, decrypt them
    uint8_t fopts[RADIOLIB_LORAWAN_FHDR_FOPTS_LEN_MASK];

    // TODO it COULD be the case that the assumed rollover is incorrect, if possible figure out a way to catch this and retry with just fcnt16
    uint8_t ctrId = 0x01 + isAppDownlink; // see LoRaWAN v1.1 errata
    processAES(&downlinkMsg[RADIOLIB_LORAWAN_FHDR_FOPTS_POS], (size_t)foptsLen, this->nwkSEncKey, fopts, fcnt32, RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK, ctrId, true);

    RADIOLIB_DEBUG_PRINTLN("fopts:");
    RADIOLIB_DEBUG_HEXDUMP(fopts, foptsLen);

    // process the MAC command(s)
    int8_t remLen = foptsLen;
    uint8_t* foptsPtr = fopts;
    while(remLen > 0) {
      LoRaWANMacCommand_t cmd = {
        .cid = *foptsPtr,
        .len = (uint8_t)(remLen - 1),
        .payload = { 0 },
        .repeat = 0,
      };
      memcpy(cmd.payload, foptsPtr + 1, cmd.len);

      // try to process the mac command
      // TODO how to handle incomplete commands?
      size_t processedLen = execMacCommand(&cmd) + 1;

      // processing succeeded, move in the buffer to the next command
      remLen -= processedLen;
      foptsPtr += processedLen;
    }

    // if FOptsLen for the next uplink is larger than can be piggybacked onto an uplink, send separate uplink
    if(this->commandsUp.len > 15) {
      uint8_t foptsNum = this->commandsUp.numCommands;
      size_t foptsBufSize = this->commandsUp.len;
      uint8_t foptsBuff[foptsBufSize];
      size_t idx = 0;
      for(size_t i = 0; i < foptsNum; i++) {
        LoRaWANMacCommand_t cmd = { .cid = 0, .len = 0, .payload = { 0 }, .repeat = 0, };
        popMacCommand(&cmd, &this->commandsUp, i);
        if(cmd.cid == 0) {
          break;
        }
        foptsBuff[idx] = cmd.cid;
        for(size_t i = 1; i < cmd.len; i++) {
          foptsBuff[idx + i] = cmd.payload[i];
        }
        idx += cmd.len + 1;
      }
      RADIOLIB_DEBUG_PRINTLN("Uplink MAC payload (%d commands):", foptsNum);
      RADIOLIB_DEBUG_HEXDUMP(foptsBuff, foptsBufSize);

      isMACPayload = true;
      this->uplink(foptsBuff, foptsBufSize, RADIOLIB_LORAWAN_FPORT_MAC_COMMAND);
      uint8_t strDown[this->band->payloadLenMax[this->dataRate[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK]]];
      size_t lenDown = 0;
      state = this->downlink(strDown, &lenDown);
      RADIOLIB_ASSERT(state);
    }

    // write the MAC command queue to nvm for next uplink
    uint8_t queueBuff[sizeof(LoRaWANMacCommandQueue_t)];
    memcpy(&queueBuff, &this->commandsUp, sizeof(LoRaWANMacCommandQueue_t));
    mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FOPTS_ID), queueBuff, sizeof(LoRaWANMacCommandQueue_t));
  }

  // process payload (if there is any)
  if(payLen <= 0) {
    // no payload
    *len = 0;
    #if !defined(RADIOLIB_STATIC_ONLY)
      delete[] downlinkMsg;
    #endif

    return(RADIOLIB_ERR_NONE);
  }

  // there is payload, and so there should be a port too
  // TODO pass the port?
  *len = payLen - 1;
  // TODO it COULD be the case that the assumed rollover is incorrect, then figure out a way to catch this and retry with just fcnt16
  // TODO does the erratum hold here as well?
  processAES(&downlinkMsg[RADIOLIB_LORAWAN_FHDR_FOPTS_POS], downlinkMsgLen, this->appSKey, data, fcnt32, RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK, 0x00, true);
  
  #if !defined(RADIOLIB_STATIC_ONLY)
    delete[] downlinkMsg;
  #endif

  return(state);
}

void LoRaWANNode::setDeviceStatus(uint8_t battLevel) {
  this->battLevel = battLevel;
}

uint32_t LoRaWANNode::generateMIC(uint8_t* msg, size_t len, uint8_t* key) {
  if((msg == NULL) || (len == 0)) {
    return(0);
  }

  RadioLibAES128Instance.init(key);
  uint8_t cmac[RADIOLIB_AES128_BLOCK_SIZE];
  RadioLibAES128Instance.generateCMAC(msg, len, cmac);
  return(((uint32_t)cmac[0]) | ((uint32_t)cmac[1] << 8) | ((uint32_t)cmac[2] << 16) | ((uint32_t)cmac[3]) << 24);
}

bool LoRaWANNode::verifyMIC(uint8_t* msg, size_t len, uint8_t* key) {
  if((msg == NULL) || (len < sizeof(uint32_t))) {
    return(0);
  }

  // extract MIC from the message
  uint32_t micReceived = LoRaWANNode::ntoh<uint32_t>(&msg[len - sizeof(uint32_t)]);

  // calculate the expected value and compare
  uint32_t micCalculated = generateMIC(msg, len - sizeof(uint32_t), key);
  if(micCalculated != micReceived) {
    RADIOLIB_DEBUG_PRINTLN("MIC mismatch, expected %08x, got %08x", micCalculated, micReceived);
    return(false);
  }

  return(true);
}

int16_t LoRaWANNode::setPhyProperties() {
  // set the physical layer configuration
  int16_t state = RADIOLIB_ERR_NONE;
  if(this->FSK) {
    // for FSK, configure the channel
    state = this->phyLayer->setFrequency(this->band->fskFreq);
    RADIOLIB_ASSERT(state);
    DataRate_t dr;
    dr.fsk.bitRate = 50;
    dr.fsk.freqDev = 25;
    state = this->phyLayer->setDataRate(dr);
    RADIOLIB_ASSERT(state);
    state = this->phyLayer->setDataShaping(RADIOLIB_SHAPING_1_0);
    RADIOLIB_ASSERT(state);
    state = this->phyLayer->setEncoding(RADIOLIB_ENCODING_WHITENING);
  }
  RADIOLIB_ASSERT(state);
  
  // set the maximum power supported by both the module and the band
  int8_t pwr = this->band->powerMax;
  state = RADIOLIB_ERR_INVALID_OUTPUT_POWER;
  while(state == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    // go from the highest power in band and lower it until we hit one supported by the module
    state = this->phyLayer->setOutputPower(pwr--);
  }
  RADIOLIB_ASSERT(state);

  uint8_t syncWord[3] = { 0 };
  uint8_t syncWordLen = 0;
  size_t preLen = 0;
  if(this->FSK) {
    preLen = 8*RADIOLIB_LORAWAN_GFSK_PREAMBLE_LEN;
    syncWord[0] = (uint8_t)(RADIOLIB_LORAWAN_GFSK_SYNC_WORD >> 16);
    syncWord[1] = (uint8_t)(RADIOLIB_LORAWAN_GFSK_SYNC_WORD >> 8);
    syncWord[2] = (uint8_t)RADIOLIB_LORAWAN_GFSK_SYNC_WORD;
    syncWordLen = 3;
  
  } else {
    preLen = RADIOLIB_LORAWAN_LORA_PREAMBLE_LEN;
    syncWord[0] = RADIOLIB_LORAWAN_LORA_SYNC_WORD;
    syncWordLen = 1;
  
  }

  state = this->phyLayer->setSyncWord(syncWord, syncWordLen);
  RADIOLIB_ASSERT(state);

  state = this->phyLayer->setPreambleLength(preLen);
  return(state);
}

int16_t LoRaWANNode::setupChannels() {
  // find appropriate channel IDs for uplink and downlink, the uplink channel is random
  int8_t chMin = -1;
  int8_t chMax = -1;
  if(this->band->cfListType == RADIOLIB_LORAWAN_CFLIST_TYPE_MASK) {
    chMin = this->startChannel;
    chMax = this->startChannel + this->numChannels;
  }
  int16_t state = this->findChannelId(RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK,
                              &this->chIndex[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK],
                              &this->dataRate[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK], chMin, chMax);
  RADIOLIB_ASSERT(state);

  // RX1 channel is not random, but determined by uplink channel
  if(this->band->cfListType == RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES) {
    // for frequency-list type bands, it's just the previous uplink channel
    this->chIndex[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = this->chIndex[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK];
    this->dataRate[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = this->dataRate[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK];
    
  } else {
    // for mask type bands, it's the uplink mod num_downlink_channels
    for(uint8_t i = 0; i < this->band->numChannelSpans; i++) {
      const LoRaWANChannelSpan_t* span = &this->band->defaultChannels[i];
      if(span->direction == RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK) {
        this->chIndex[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = this->chIndex[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] % span->numChannels;
        this->dataRate[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = span->joinRequestDataRate;
        break;
      }
    }
  }

  // based on the channel IDs, find the frequencies
  state = this->findChannelFreq(RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK,
                                this->chIndex[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK], 
                                &this->channelFreq[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK]);
  RADIOLIB_ASSERT(state);
  state = this->findChannelFreq(RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK,
                                this->chIndex[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK], 
                                &this->channelFreq[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK]);
  RADIOLIB_ASSERT(state);

  // configure channel for uplink
  state = this->configureChannel(RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK);
  return(state);
}

uint8_t LoRaWANNode::findDataRate(uint8_t dr, DataRate_t* dataRate, const LoRaWANChannelSpan_t* span) {
  uint8_t dataRateBand = 0;
  uint8_t dataRateFound = 0;
  if(dr == RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
    for(uint8_t i = 0; i < RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES; i++) {
      if(span->dataRates[i] != RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
        dataRateBand = span->dataRates[i];
        dataRateFound = i;
        break;
      }
    }
  } else {
    dataRateBand = span->dataRates[dr];
    dataRateFound = dr;
  }

  if(dataRateBand & RADIOLIB_LORAWAN_DATA_RATE_FSK_50_K) {
    dataRate->fsk.bitRate = 50;
    dataRate->fsk.freqDev = 25;
  
  } else {
    uint8_t bw = dataRateBand & 0x0C;
    switch(bw) {
      case(RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ):
        dataRate->lora.bandwidth = 125.0;
        break;
      case(RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ):
        dataRate->lora.bandwidth = 250.0;
        break;
      case(RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ):
        dataRate->lora.bandwidth = 500.0;
        break;
      default:
        dataRate->lora.bandwidth = 125.0;
    }
    
    dataRate->lora.spreadingFactor = ((dataRateBand & 0x70) >> 4) + 6;
    dataRate->lora.codingRate = (dataRateBand & 0x03) + 5;
  }

  return(dataRateFound);
}

int16_t LoRaWANNode::findChannelId(uint8_t dir, uint8_t* ch, uint8_t* dr, int8_t min, int8_t max) {
  // find the first channel span that supports the requested direction
  uint8_t spanId = 0;
  LoRaWANChannelSpan_t* span = NULL;
  for(; spanId < this->band->numChannelSpans; spanId++) {
    span = (LoRaWANChannelSpan_t*)&this->band->defaultChannels[spanId];
    if((span->direction == dir) || (span->direction == RADIOLIB_LORAWAN_CHANNEL_DIR_BOTH)) {
      break;
    }
  }

  // shouldn't happen, but just to be sure
  if(!span) {
    RADIOLIB_DEBUG_PRINTLN("findChannelId span not found");
    return(RADIOLIB_ERR_INVALID_CHANNEL);
  }

  // if requested, save the data rate
  if(dr) {
    *dr = span->joinRequestDataRate;
  }

  // determine min and max based on number of channels in span and user constraints
  uint8_t chMin = (min > 0) ? min : 0;
  uint8_t chMax = (max > 0) ? max : span->numChannels;

  // select channel ID as random number between min and max (global number 0 - N for single direction)
  int32_t chId = this->phyLayer->random(chMin, chMax);
  *ch = chId;
  return(RADIOLIB_ERR_NONE);
}

LoRaWANChannelSpan_t* LoRaWANNode::findChannelSpan(uint8_t dir, uint8_t ch, uint8_t* spanChannelId) {
  // find the span based on the channel ID
  uint8_t chanCtr = 0;
  *spanChannelId = 0;
  for(uint8_t span = 0; span < this->band->numChannelSpans; span++) {
    // check if this channel span can be used
    uint8_t direction = this->band->defaultChannels[span].direction;
    if((direction != dir) && (direction != RADIOLIB_LORAWAN_CHANNEL_DIR_BOTH)) {
      continue;
    }

    // iterate over the usable spans to the channel ID
    for(; *spanChannelId < this->band->defaultChannels[span].numChannels; (*spanChannelId)++) {
      if(chanCtr >= ch) {
        // we found it, return the pointer (channel ID within the span is already set)
        return((LoRaWANChannelSpan_t*)&this->band->defaultChannels[span]);
      }
      chanCtr++;
    }
  }

  return(NULL);
}

int16_t LoRaWANNode::findChannelFreq(uint8_t dir, uint8_t ch, float* freq) {
  // find the channel span based on channel ID and direction
  uint8_t spanChannelId = 0;
  LoRaWANChannelSpan_t* span = findChannelSpan(dir, ch, &spanChannelId);
  if(!span) {
    return(RADIOLIB_ERR_INVALID_CHANNEL);
  }

  // set the frequency
  *freq = span->freqStart + span->freqStep * (float)spanChannelId;
  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::configureChannel(uint8_t dir) {
  // set the frequency
  RADIOLIB_DEBUG_PRINT("Channel frequency %cL = ", dir ? 'D' : 'U');
  RADIOLIB_DEBUG_PRINT_FLOAT(this->channelFreq[dir], 3);
  RADIOLIB_DEBUG_PRINTLN(" MHz");
  int state = this->phyLayer->setFrequency(this->channelFreq[dir]);
  RADIOLIB_ASSERT(state);

  // find the channel span based on channel ID and direction
  uint8_t spanChannelId = 0;
  LoRaWANChannelSpan_t* span = findChannelSpan(dir, this->chIndex[dir], &spanChannelId);
  if(!span) {
    return(RADIOLIB_ERR_INVALID_CHANNEL);
  }

  // set the data rate
  DataRate_t dataRate;
  this->dataRate[dir] = findDataRate(this->dataRate[dir], &dataRate, span);
  state = this->phyLayer->setDataRate(dataRate);
  return(state);
}

int16_t LoRaWANNode::sendMacCommand(uint8_t cid, uint8_t* payload, size_t payloadLen, uint8_t* reply, size_t replyLen) {
  // build the command
  size_t macReqLen = 1 + payloadLen;
  #if !defined(RADIOLIB_STATIC_ONLY)
    uint8_t* macReqBuff = new uint8_t[macReqLen];
  #else
    uint8_t macReqBuff[RADIOLIB_STATIC_ARRAY_SIZE];
  #endif
  macReqBuff[0] = cid;
  memcpy(&macReqBuff[1], payload, payloadLen);

  // uplink it
  int16_t state = this->uplink(macReqBuff, macReqLen, RADIOLIB_LORAWAN_FPORT_MAC_COMMAND);
  #if !defined(RADIOLIB_STATIC_ONLY)
    delete[] macReqBuff;
  #endif
  RADIOLIB_ASSERT(state);

  // build the reply buffer
  size_t macRplLen = 1 + replyLen;
  #if !defined(RADIOLIB_STATIC_ONLY)
    uint8_t* macRplBuff = new uint8_t[this->band->payloadLenMax[this->dataRate[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK]]];
  #else
    uint8_t macRplBuff[RADIOLIB_STATIC_ARRAY_SIZE];
  #endif

  // wait for reply from the server
  size_t rxRplLen = 0;
  state = this->downlink(macRplBuff, &rxRplLen);
  if(state != RADIOLIB_ERR_NONE) {
    #if !defined(RADIOLIB_STATIC_ONLY)
      delete[] macRplBuff;
    #endif
    return(state);
  }

  RADIOLIB_DEBUG_PRINTLN("macRplBuff:");
  RADIOLIB_DEBUG_HEXDUMP(macRplBuff, rxRplLen);

  // check the length - it may be longer than expected
  // if the server decided to append more MAC commands, but never shorter
  // TODO how to handle the additional command(s)?
  if(rxRplLen < macRplLen) {
    #if !defined(RADIOLIB_STATIC_ONLY)
      delete[] macRplBuff;
    #endif
    return(RADIOLIB_ERR_DOWNLINK_MALFORMED);
  }

  // check the CID
  if(macRplBuff[0] != cid) {
    #if !defined(RADIOLIB_STATIC_ONLY)
      delete[] macRplBuff;
    #endif
    return(RADIOLIB_ERR_INVALID_CID);
  }

  // copy the data
  memcpy(reply, &macRplBuff[1], replyLen);
  #if !defined(RADIOLIB_STATIC_ONLY)
    delete[] macRplBuff;
  #endif

  return(state);
}

int16_t LoRaWANNode::pushMacCommand(LoRaWANMacCommand_t* cmd, LoRaWANMacCommandQueue_t* queue) {
  if(queue->numCommands >= RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE) {
    return(RADIOLIB_ERR_COMMAND_QUEUE_FULL);
  }

  memcpy(&queue->commands[queue->numCommands], cmd, sizeof(LoRaWANMacCommand_t));
  /*RADIOLIB_DEBUG_PRINTLN("push MAC CID = %02x, len = %d, payload = %02x %02x %02x %02x %02x, repeat = %d ", 
    queue->commands[queue->numCommands - 1].cid,
    queue->commands[queue->numCommands - 1].len,
    queue->commands[queue->numCommands - 1].payload[0],
    queue->commands[queue->numCommands - 1].payload[1],
    queue->commands[queue->numCommands - 1].payload[2],
    queue->commands[queue->numCommands - 1].payload[3],
    queue->commands[queue->numCommands - 1].payload[4],
    queue->commands[queue->numCommands - 1].repeat);*/
  queue->numCommands++;
  queue->len += 1 + cmd->len; // 1 byte for command ID, len bytes for payload

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::popMacCommand(LoRaWANMacCommand_t* cmd, LoRaWANMacCommandQueue_t* queue, size_t index) {
  if(queue->numCommands == 0) {
    return(RADIOLIB_ERR_COMMAND_QUEUE_EMPTY);
  }

  if(cmd) {
    // RADIOLIB_DEBUG_PRINTLN("pop MAC CID = %02x, len = %d, payload = %02x %02x %02x %02x %02x, repeat = %d ", 
    // queue->commands[index].cid,
    // queue->commands[index].len,
    // queue->commands[index].payload[0],
    // queue->commands[index].payload[1],
    // queue->commands[index].payload[2],
    // queue->commands[index].payload[3],
    // queue->commands[index].payload[4],
    // queue->commands[index].repeat);
    memcpy(cmd, &queue->commands[index], sizeof(LoRaWANMacCommand_t));
  }

  if(queue->commands[index].repeat > 0) {
    queue->commands[index].repeat--;
  } else {
    deleteMacCommand(queue->commands[index].cid, queue);
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::deleteMacCommand(uint8_t cid, LoRaWANMacCommandQueue_t* queue) {
  if(queue->numCommands == 0) {
    return(RADIOLIB_ERR_COMMAND_QUEUE_EMPTY);
  }

  for(size_t index = 0; index < queue->numCommands; index++) {
    if(queue->commands[index].cid == cid) {
      // RADIOLIB_DEBUG_PRINTLN("delete MAC CID = %02x, len = %d, payload = %02x %02x %02x %02x %02x, repeat = %d ", 
      // queue->commands[index].cid,
      // queue->commands[index].len,
      // queue->commands[index].payload[0],
      // queue->commands[index].payload[1],
      // queue->commands[index].payload[2],
      // queue->commands[index].payload[3],
      // queue->commands[index].payload[4],
      // queue->commands[index].repeat);
      queue->len -= (1 + queue->commands[index].len); // 1 byte for command ID, len for payload
      // move all subsequent commands one forward in the queue
      if(index < RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE - 1) {
        memmove(&queue->commands[index], &queue->commands[index + 1], (RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE - index) * sizeof(LoRaWANMacCommand_t));
      }
      // set the latest element to all 0
      memset(&queue->commands[RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE - 1], 0x00, sizeof(LoRaWANMacCommand_t));
      queue->numCommands--;
      return(RADIOLIB_ERR_NONE);
    }
  }

  return(RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND);
}

size_t LoRaWANNode::execMacCommand(LoRaWANMacCommand_t* cmd) {
  RADIOLIB_DEBUG_PRINTLN("exe MAC CID = %02x, len = %d", cmd->cid, cmd->len);

  if(cmd->cid >= RADIOLIB_LORAWAN_MAC_CMD_PROPRIETARY) {
    // TODO call user-provided callback for proprietary MAC commands?
    return(cmd->len - 1);
  }

  switch(cmd->cid) {
    case(RADIOLIB_LORAWAN_MAC_CMD_RESET): {
      // get the server version
      uint8_t srvVersion = cmd->payload[0];
      RADIOLIB_DEBUG_PRINTLN("Server version: 1.%d", srvVersion);
      if(srvVersion == this->rev) {
        // valid server version, stop sending the ResetInd MAC command
        deleteMacCommand(RADIOLIB_LORAWAN_MAC_CMD_RESET, &this->commandsUp);
      }
      return(1);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_LINK_CHECK): {
      // TODO sent by gateway as reply to node request, how to get this info to the user?
      uint8_t margin = cmd->payload[0];
      uint8_t gwCnt = cmd->payload[1];
      RADIOLIB_DEBUG_PRINTLN("Link check: margin = %d dB, gwCnt = %d", margin, gwCnt);
      (void)margin;
      (void)gwCnt;
      return(2);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_LINK_ADR): {
      // get the ADR configuration
      uint8_t dr = (cmd->payload[0] & 0xF0) >> 4;
      uint8_t txPower = cmd->payload[0] & 0x0F;
      uint16_t chMask = LoRaWANNode::ntoh<uint16_t>(&cmd->payload[1]);
      uint8_t chMaskCntl = (cmd->payload[3] & 0x70) >> 4;
      uint8_t nbTrans = cmd->payload[3] & 0x0F;
      RADIOLIB_DEBUG_PRINTLN("ADR REQ: dataRate = %d, txPower = %d, chMask = 0x%04x, chMaskCntl = %02x, nbTrans = %d", dr, txPower, chMask, chMaskCntl, nbTrans);

      // apply the configuration
      uint8_t drAck = 0;
      if(dr != 0x0F) {
        // first figure out which channel span this data rate applies to
        // TODO do that by processing the chMask/chMaskCntl?
        uint8_t spanChannelId = 0;
        LoRaWANChannelSpan_t* span = findChannelSpan(RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK, this->chIndex[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK], &spanChannelId);

        // seems to be only applicable to uplink
        if(span) {
          DataRate_t dataRate;
          this->dataRate[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = findDataRate(dr, &dataRate, span);
          if(this->phyLayer->setDataRate(dataRate) == RADIOLIB_ERR_NONE) {
            RADIOLIB_DEBUG_PRINTLN("ADR set dr = %d channel = %d", dr, spanChannelId);
            drAck = 1;
          }
        }

      } else {
        drAck = 1;
      
      }

      // try to apply the power configuration
      uint8_t pwrAck = 0;
      if(txPower != 0x0F) {
        int8_t pwr = this->band->powerMax - 2*txPower;
        if(this->phyLayer->setOutputPower(pwr) == RADIOLIB_ERR_NONE) {
          RADIOLIB_DEBUG_PRINTLN("ADR set pwr = %d", pwr);
          pwrAck = 1;
        }

      } else {
        pwrAck = 1;
      }

      // TODO implement repeated uplinks with nbTrans
      (void)nbTrans;
      // TODO implement channel mask
      uint8_t chMaskAck = 0;
      (void)chMask;
      (void)chMaskCntl;

      // send the reply
      cmd->len = 1;
      cmd->payload[0] = (pwrAck << 2) | (drAck << 1) | (chMaskAck << 0);
      RADIOLIB_DEBUG_PRINTLN("ADR ANS: status = 0x%02x", cmd->payload[0]);
      pushMacCommand(cmd, &this->commandsUp);
      return(4);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_DUTY_CYCLE): {
      uint8_t maxDutyCycle = cmd->payload[0] & 0x0F;
      RADIOLIB_DEBUG_PRINTLN("Max duty cycle: 1/2^%d", maxDutyCycle);

      // TODO implement this
      (void)maxDutyCycle;
      return(1);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_RX_PARAM_SETUP): {
      // get the configuration
      uint8_t rx1DrOffset = (cmd->payload[0] & 0x70) >> 4;
      uint8_t rx2DataRate = cmd->payload[0] & 0x0F;
      uint32_t freqRaw = LoRaWANNode::ntoh<uint32_t>(&cmd->payload[1], 3);
      RADIOLIB_DEBUG_PRINTLN("RX Param: rx1DrOffset = %d, rx2DataRate = %d, freq = %d", rx1DrOffset, rx2DataRate, freqRaw);   

      // apply the configuration
      float freq = (float)freqRaw/10000.0;
      float prevFreq = this->channelFreq[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK];
      uint8_t chanAck = 0;
      if(this->phyLayer->setFrequency(freq) == RADIOLIB_ERR_NONE) {
        this->backupFreq = freq;
        chanAck = 1;
        this->phyLayer->setFrequency(prevFreq);
      }

      // TODO process the RX2 data rate
      (void)rx2DataRate;
      uint8_t rx2Ack = 0;

      // TODO process the data rate offset
      (void)rx1DrOffset;
      uint8_t rx1OffsAck = 0;

      // send the reply
      cmd->len = 1;
      cmd->payload[0] = (rx1OffsAck << 2) | (rx2Ack << 1) | (chanAck << 0);
      RADIOLIB_DEBUG_PRINTLN("Rx param ANS: status = 0x%02x", cmd->payload[0]);
      pushMacCommand(cmd, &this->commandsUp);
      return(4);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_DEV_STATUS): {
      // set the uplink reply
      cmd->len = 2;
      cmd->payload[1] = this->battLevel;
      int8_t snr = this->phyLayer->getSNR();
      cmd->payload[0] = snr & 0x3F;

      // push it to the uplink queue
      pushMacCommand(cmd, &this->commandsUp);
      return(0);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_NEW_CHANNEL): {
      // get the configuration
      uint8_t chIndex = cmd->payload[0];
      uint32_t freqRaw = LoRaWANNode::ntoh<uint32_t>(&cmd->payload[1], 3);
      float freq = (float)freqRaw/10000.0;
      uint8_t maxDr = (cmd->payload[4] & 0xF0) >> 4;
      uint8_t minDr = cmd->payload[4] & 0x0F;
      RADIOLIB_DEBUG_PRINT("New channel: index = %d, freq = ", chIndex);
      RADIOLIB_DEBUG_PRINT_FLOAT(freq, 3);
      RADIOLIB_DEBUG_PRINTLN(" MHz, maxDr = %d, minDr = %d", maxDr, minDr);

      // TODO implement this
      (void)chIndex;
      (void)freq;
      (void)maxDr;
      (void)minDr;
      return(5);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_RX_TIMING_SETUP): {
      // get the configuration
      uint8_t delay = cmd->payload[0] & 0x0F;
      RADIOLIB_DEBUG_PRINTLN("RX timing: delay = %d sec", delay);
      
      // apply the configuration
      if(delay == 0) {
        delay = 1;
      }
      this->rxDelays[0] = delay * 1000;
      this->rxDelays[1] = this->rxDelays[0] + 1000;

      // send the reply
      cmd->len = 0;

      // TODO this should be sent repeatedly until the next downlink
      pushMacCommand(cmd, &this->commandsUp);
      
      return(1);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_TX_PARAM_SETUP): {
      uint8_t dlDwell = (cmd->payload[0] & 0x20) >> 5;
      uint8_t ulDwell = (cmd->payload[0] & 0x10) >> 4;
      uint8_t maxEirpRaw = cmd->payload[0] & 0x0F;

      // who the f came up with this ...
      const uint8_t eirpEncoding[] = { 8, 10, 12, 13, 14, 16, 18, 20, 21, 24, 26, 27, 29, 30, 33, 36 };
      uint8_t maxEirp = eirpEncoding[maxEirpRaw];
      RADIOLIB_DEBUG_PRINTLN("TX timing: dlDwell = %d, dlDwell = %d, maxEirp = %d dBm", dlDwell, ulDwell, maxEirp);

      // TODO implement this
      (void)dlDwell;
      (void)ulDwell;
      (void)maxEirp;
      return(1);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_DL_CHANNEL): {
      // get the configuration
      uint8_t chIndex = cmd->payload[0];
      uint32_t freqRaw = LoRaWANNode::ntoh<uint32_t>(&cmd->payload[1], 3);
      float freq = (float)freqRaw/10000.0;
      RADIOLIB_DEBUG_PRINT("DL channel: index = %d, freq = ", chIndex);
      RADIOLIB_DEBUG_PRINT_FLOAT(freq, 3);
      RADIOLIB_DEBUG_PRINTLN(" MHz");

      // TODO implement this
      (void)chIndex;
      (void)freq;
      return(4);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_REKEY): {
      // get the server version
      uint8_t srvVersion = cmd->payload[0];
      RADIOLIB_DEBUG_PRINTLN("Server version: 1.%d", srvVersion);
      if((srvVersion > 0) && (srvVersion <= this->rev)) {
        // valid server version, stop sending the ReKey MAC command
        deleteMacCommand(RADIOLIB_LORAWAN_MAC_CMD_REKEY, &this->commandsUp);
      }
      return(1);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_ADR_PARAM_SETUP): {
      // TODO implement this
      uint8_t limitExp = (cmd->payload[0] & 0xF0) >> 4;
      uint8_t delayExp = cmd->payload[0] & 0x0F;
      RADIOLIB_DEBUG_PRINTLN("ADR param setup: limitExp = %d, delayExp = %d", limitExp, delayExp);
      (void)limitExp;
      (void)delayExp;
      return(1);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_DEVICE_TIME): {
      // TODO implement this - sent by gateway as reply to node request
      uint32_t gpsEpoch = LoRaWANNode::ntoh<uint32_t>(&cmd->payload[0]);
      uint8_t fraction = cmd->payload[4];
      RADIOLIB_DEBUG_PRINT("Network time: gpsEpoch = %d s, delayExp = ", gpsEpoch, (float)fraction/256.0f);
      RADIOLIB_DEBUG_PRINT_FLOAT((float)fraction/256.0f, 2);
      RADIOLIB_DEBUG_PRINTLN();
      (void)gpsEpoch;
      (void)fraction;
      return(5);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_FORCE_REJOIN): {
      // TODO implement this
      uint16_t rejoinReq = LoRaWANNode::ntoh<uint16_t>(&cmd->payload[0]);
      uint8_t period = (rejoinReq & 0x3800) >> 11;
      uint8_t maxRetries = (rejoinReq & 0x0700) >> 8;
      uint8_t rejoinType = (rejoinReq & 0x0070) >> 4;
      uint8_t dr = rejoinReq & 0x000F;
      RADIOLIB_DEBUG_PRINTLN("Force rejoin: period = %d, maxRetries = %d, rejoinType = %d, dr = %d", period, maxRetries, rejoinType, dr);
      (void)period;
      (void)maxRetries;
      (void)rejoinType;
      (void)dr;
      return(2);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_REJOIN_PARAM_SETUP): {
      // TODO implement this
      uint8_t maxTime = (cmd->payload[0] & 0xF0) >> 4;
      uint8_t maxCount = cmd->payload[0] & 0x0F;
      RADIOLIB_DEBUG_PRINTLN("Rejoin setup: maxTime = %d, maxCount = %d", maxTime, maxCount);
      (void)maxTime;
      (void)maxCount;
      return(0);
    } break;
  }

  return(0);
}

void LoRaWANNode::processAES(uint8_t* in, size_t len, uint8_t* key, uint8_t* out, uint32_t fcnt, uint8_t dir, uint8_t ctrId, bool counter) {
  // figure out how many encryption blocks are there
  size_t numBlocks = len/RADIOLIB_AES128_BLOCK_SIZE;
  if(len % RADIOLIB_AES128_BLOCK_SIZE) {
    numBlocks++;
  }

  // generate the encryption blocks
  uint8_t encBuffer[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  uint8_t encBlock[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  encBlock[RADIOLIB_LORAWAN_BLOCK_MAGIC_POS] = RADIOLIB_LORAWAN_ENC_BLOCK_MAGIC;
  encBlock[RADIOLIB_LORAWAN_ENC_BLOCK_COUNTER_ID_POS] = ctrId;
  encBlock[RADIOLIB_LORAWAN_BLOCK_DIR_POS] = dir;
  LoRaWANNode::hton<uint32_t>(&encBlock[RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS], this->devAddr);
  LoRaWANNode::hton<uint32_t>(&encBlock[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], fcnt);

  // now encrypt the input
  // on downlink frames, this has a decryption effect because server actually "decrypts" the plaintext
  size_t remLen = len;
  for(size_t i = 0; i < numBlocks; i++) {
    if(counter) {
      encBlock[RADIOLIB_LORAWAN_ENC_BLOCK_COUNTER_POS] = i + 1;
    }

    // encrypt the buffer
    RadioLibAES128Instance.init(key);
    RadioLibAES128Instance.encryptECB(encBlock, RADIOLIB_AES128_BLOCK_SIZE, encBuffer);

    // now xor the buffer with the input
    size_t xorLen = remLen;
    if(xorLen > RADIOLIB_AES128_BLOCK_SIZE) {
      xorLen = RADIOLIB_AES128_BLOCK_SIZE;
    }
    for(uint8_t j = 0; j < xorLen; j++) {
      out[i*RADIOLIB_AES128_BLOCK_SIZE + j] = in[i*RADIOLIB_AES128_BLOCK_SIZE + j] ^ encBuffer[j];
    }
    remLen -= xorLen;
  }
}

template<typename T>
T LoRaWANNode::ntoh(uint8_t* buff, size_t size) {
  uint8_t* buffPtr = buff;
  size_t targetSize = sizeof(T);
  if(size != 0) {
    targetSize = size;
  }
  T res = 0;
  for(size_t i = 0; i < targetSize; i++) {
    res |= (uint32_t)(*(buffPtr++)) << 8*i;
  }
  return(res);
}

template<typename T>
void LoRaWANNode::hton(uint8_t* buff, T val, size_t size) {
  uint8_t* buffPtr = buff;
  size_t targetSize = sizeof(T);
  if(size != 0) {
    targetSize = size;
  }
  for(size_t i = 0; i < targetSize; i++) {
    *(buffPtr++) = val >> 8*i;
  }
}

// The following function enables LMAC, a CSMA scheme for LoRa as specified 
// in the LoRa Alliance Technical Recommendation #13.
// A user may enable CSMA to provide frames an additional layer of protection from interference.
// https://resources.lora-alliance.org/technical-recommendations/tr013-1-0-0-csma
void LoRaWANNode::performCSMA() {
    
    // Compute initial random back-off. 
    // When BO is reduced to zero, the function returns and the frame is transmitted.
    uint32_t BO = this->phyLayer->random(1, this->backoffMax + 1);

    while (BO > 0) {
        // DIFS: Check channel for DIFS_slots
        bool channelFreeDuringDIFS = true;
        for (uint8_t i = 0; i < this->difsSlots; i++) {
            if (performCAD()) {
                RADIOLIB_DEBUG_PRINTLN("OCCUPIED CHANNEL DURING DIFS");
                channelFreeDuringDIFS = false;
                // Channel is occupied during DIFS, hop to another.
                this->setupChannels();
                break;
            }
        }

        // Start reducing BO counter if DIFS slot was free.
        if (channelFreeDuringDIFS) {
            // Continue decrementing BO with per each CAD reporting free channel.
            while (BO > 0) {
                if (performCAD()) {
                    RADIOLIB_DEBUG_PRINTLN("OCCUPIED CHANNEL DURING BO");
                    // Channel is busy during CAD, hop to another and return to DIFS state again.
                    this->setupChannels();
                    break;  // Exit loop. Go back to DIFS state.
                }
                BO--;  // Decrement BO by one if channel is free
            }
        }
    }
}

bool LoRaWANNode::performCAD() {
    int16_t state = this->phyLayer->scanChannel();

    if ((state == RADIOLIB_PREAMBLE_DETECTED) || (state == RADIOLIB_LORA_DETECTED)) {
        return true; // Channel is busy
    }
    return false; // Channel is free
}

#endif