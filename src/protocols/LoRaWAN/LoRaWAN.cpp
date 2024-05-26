#include "LoRaWAN.h"
#include <string.h>
#if defined(ESP_PLATFORM)
#include "esp_attr.h"
#endif

#if !RADIOLIB_EXCLUDE_LORAWAN

// flag to indicate whether there was some action during Rx mode (timeout or downlink)
static volatile bool downlinkAction = false;

// interrupt service routine to handle downlinks automatically
#if defined(ESP8266) || defined(ESP32)
  IRAM_ATTR
#endif
static void LoRaWANNodeOnDownlinkAction(void) {
  downlinkAction = true;
}

uint8_t getDownlinkDataRate(uint8_t uplink, uint8_t offset, uint8_t base, uint8_t min, uint8_t max) {
  int8_t dr = uplink - offset + base;
  if(dr < min) {
    dr = min;
  } else if (dr > max) {
    dr = max;
  }
  return(dr);
}

LoRaWANNode::LoRaWANNode(PhysicalLayer* phy, const LoRaWANBand_t* band, uint8_t subBand) {
  this->phyLayer = phy;
  this->band = band;
  this->rx2 = this->band->rx2;
  this->txPowerMax = this->band->powerMax;
  this->subBand = subBand;
  this->difsSlots = 2;
  this->backoffMax = 6;
  this->enableCSMA = false;
  memset(this->availableChannels, 0, sizeof(this->availableChannels));
}

void LoRaWANNode::setCSMA(uint8_t backoffMax, uint8_t difsSlots, bool enableCSMA) {
    this->backoffMax = backoffMax;
    this->difsSlots = difsSlots;
    this->enableCSMA = enableCSMA;
}

void LoRaWANNode::clearNonces() {
  // clear & set all the device credentials
  memset(this->bufferNonces, 0, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
  this->keyCheckSum = 0;
  this->devNonce = 0;
  this->joinNonce = 0;
  this->isActive = false;
}

void LoRaWANNode::clearSession() {
  memset(this->bufferSession, 0, RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
  memset(&(this->commandsUp), 0, sizeof(LoRaWANMacCommandQueue_t));
  memset(&(this->commandsDown), 0, sizeof(LoRaWANMacCommandQueue_t));
  this->bufferNonces[RADIOLIB_LORAWAN_NONCES_ACTIVE] = (uint8_t)false;
  this->isActive = false;
}

uint8_t* LoRaWANNode::getBufferNonces() {
  // generate the signature of the Nonces buffer, and store it in the last two bytes of the Nonces buffer
  uint16_t signature = LoRaWANNode::checkSum16(this->bufferNonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE - 2);
  LoRaWANNode::hton<uint16_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_SIGNATURE], signature);

  return(this->bufferNonces);
}

int16_t LoRaWANNode::setBufferNonces(uint8_t* persistentBuffer) {
  if(this->isActivated()) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Did not update buffer: session already active");
    return(RADIOLIB_ERR_NONE);
  }

  int16_t state = LoRaWANNode::checkBufferCommon(persistentBuffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
  RADIOLIB_ASSERT(state);

  bool isSameKeys = LoRaWANNode::ntoh<uint16_t>(&persistentBuffer[RADIOLIB_LORAWAN_NONCES_CHECKSUM]) == this->keyCheckSum;
  bool isSameMode = LoRaWANNode::ntoh<uint16_t>(&persistentBuffer[RADIOLIB_LORAWAN_NONCES_MODE]) == this->lwMode;
  bool isSameClass = LoRaWANNode::ntoh<uint8_t>(&persistentBuffer[RADIOLIB_LORAWAN_NONCES_CLASS]) == this->lwClass;
  bool isSamePlan  = LoRaWANNode::ntoh<uint8_t>(&persistentBuffer[RADIOLIB_LORAWAN_NONCES_PLAN]) == this->band->bandNum;

  // check if Nonces buffer matches the current configuration
  if(!isSameKeys || !isSameMode || !isSameClass || !isSamePlan) {
    // if configuration did not match, discard whatever is currently in the buffers and start fresh
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Configuration mismatch (keys: %d, mode: %d, class: %d, plan: %d)", isSameKeys, isSameMode, isSameClass, isSamePlan);
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Discarding the Nonces buffer:");
    RADIOLIB_DEBUG_PROTOCOL_HEXDUMP(persistentBuffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
    return(RADIOLIB_LORAWAN_NONCES_DISCARDED);
  }

  // copy the whole buffer over
  memcpy(this->bufferNonces, persistentBuffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);

  this->devNonce  = LoRaWANNode::ntoh<uint16_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_DEV_NONCE]);
  this->joinNonce = LoRaWANNode::ntoh<uint32_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_JOIN_NONCE], 3);

  // revert to inactive as long as no session is restored
  this->bufferNonces[RADIOLIB_LORAWAN_NONCES_ACTIVE] = (uint8_t)false;
  this->isActive = false;

  return(state);
}

uint8_t* LoRaWANNode::getBufferSession() {
  // store all frame counters
  LoRaWANNode::hton<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_A_FCNT_DOWN], this->aFCntDown);
  LoRaWANNode::hton<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_N_FCNT_DOWN], this->nFCntDown);
  LoRaWANNode::hton<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_CONF_FCNT_UP], this->confFCntUp);
  LoRaWANNode::hton<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_CONF_FCNT_DOWN], this->confFCntDown);
  LoRaWANNode::hton<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_ADR_FCNT], this->adrFCnt);
  LoRaWANNode::hton<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_FCNT_UP], this->fCntUp);

  // save the current uplink MAC command queue
  memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_MAC_QUEUE_UL], &this->commandsUp, sizeof(LoRaWANMacCommandQueue_t));

  // generate the signature of the Session buffer, and store it in the last two bytes of the Session buffer
  uint16_t signature = LoRaWANNode::checkSum16(this->bufferSession, RADIOLIB_LORAWAN_SESSION_BUF_SIZE - 2);
  LoRaWANNode::hton<uint16_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_SIGNATURE], signature);
  
  return(this->bufferSession);
}

int16_t LoRaWANNode::setBufferSession(uint8_t* persistentBuffer) {
  if(this->isActivated()) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Did not update buffer: session already active");
    return(RADIOLIB_ERR_NONE);
  }

  int16_t state = LoRaWANNode::checkBufferCommon(persistentBuffer, RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
  RADIOLIB_ASSERT(state);

  // the Nonces buffer holds a checksum signature - compare this to the signature that is in the session buffer
  uint16_t signatureNonces = LoRaWANNode::ntoh<uint16_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_SIGNATURE]);
  uint16_t signatureInSession = LoRaWANNode::ntoh<uint16_t>(&persistentBuffer[RADIOLIB_LORAWAN_SESSION_NONCES_SIGNATURE]);
  if(signatureNonces != signatureInSession) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("The supplied session buffer does not match the Nonces buffer");
    return(RADIOLIB_LORAWAN_SESSION_DISCARDED);
  }

  // copy the whole buffer over
  memcpy(this->bufferSession, persistentBuffer, RADIOLIB_LORAWAN_SESSION_BUF_SIZE);

  //// this code can be used in case breaking chances must be caught:
  // uint8_t nvm_table_version = this->bufferNonces[RADIOLIB_LORAWAN_NONCES_VERSION];
  // if (RADIOLIB_LORAWAN_NONCES_VERSION_VAL > nvm_table_version) {
  //  // set default values for variables that are new or something
  // }

  // pull all authentication keys from persistent storage
  this->devAddr = LoRaWANNode::ntoh<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_DEV_ADDR]);
  memcpy(this->appSKey,     &this->bufferSession[RADIOLIB_LORAWAN_SESSION_APP_SKEY],      RADIOLIB_AES128_BLOCK_SIZE);
  memcpy(this->nwkSEncKey,  &this->bufferSession[RADIOLIB_LORAWAN_SESSION_NWK_SENC_KEY],  RADIOLIB_AES128_BLOCK_SIZE);
  memcpy(this->fNwkSIntKey, &this->bufferSession[RADIOLIB_LORAWAN_SESSION_FNWK_SINT_KEY], RADIOLIB_AES128_BLOCK_SIZE);
  memcpy(this->sNwkSIntKey, &this->bufferSession[RADIOLIB_LORAWAN_SESSION_SNWK_SINT_KEY], RADIOLIB_AES128_BLOCK_SIZE);

  // restore session parameters
  this->rev          = LoRaWANNode::ntoh<uint8_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_VERSION]);
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("LoRaWAN session: v1.%d", this->rev);
  this->homeNetId    = LoRaWANNode::ntoh<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_HOMENET_ID]);
  this->aFCntDown    = LoRaWANNode::ntoh<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_A_FCNT_DOWN]);
  this->nFCntDown    = LoRaWANNode::ntoh<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_N_FCNT_DOWN]);
  this->confFCntUp   = LoRaWANNode::ntoh<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_CONF_FCNT_UP]);
  this->confFCntDown = LoRaWANNode::ntoh<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_CONF_FCNT_DOWN]);
  this->adrFCnt      = LoRaWANNode::ntoh<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_ADR_FCNT]);
  this->fCntUp       = LoRaWANNode::ntoh<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_FCNT_UP]);
  
  // restore the complete MAC state

  // all-zero buffer used for checking if MAC commands are set
  uint8_t bufferZeroes[RADIOLIB_LORAWAN_MAX_MAC_COMMAND_LEN_DOWN] = { 0 };

  LoRaWANMacCommand_t cmd = {
    .cid = 0,
    .payload = { 0 }, 
    .len = 0,
    .repeat = 0,
  };

  // for dynamic bands, first restore the defined channels before restoring ADR
  // this is because the ADR command acts as a mask for the enabled channels
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
    // setup the default channels
    state = this->setupChannelsDyn();
    RADIOLIB_ASSERT(state);

    // restore the session channels
    uint8_t *startChannelsUp = &this->bufferSession[RADIOLIB_LORAWAN_SESSION_UL_CHANNELS];

    cmd.cid = RADIOLIB_LORAWAN_MAC_NEW_CHANNEL;
    for(int i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
      cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_NEW_CHANNEL].lenDn;
      memcpy(cmd.payload, startChannelsUp + (i * cmd.len), cmd.len);
      if(memcmp(cmd.payload, bufferZeroes, cmd.len) != 0) { // only execute if it is not all zeroes
        cmd.repeat = 1;
        (void)execMacCommand(&cmd);
      }
    }

    uint8_t *startChannelsDown = &this->bufferSession[RADIOLIB_LORAWAN_SESSION_DL_CHANNELS];

    cmd.cid = RADIOLIB_LORAWAN_MAC_DL_CHANNEL;
    for(int i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
      cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_DL_CHANNEL].lenDn;
      memcpy(cmd.payload, startChannelsDown + (i * cmd.len), cmd.len);
      if(memcmp(cmd.payload, bufferZeroes, cmd.len) != 0) { // only execute if it is not all zeroes
        (void)execMacCommand(&cmd);
      }
    }
  }

  cmd.cid = RADIOLIB_LORAWAN_MAC_TX_PARAM_SETUP,
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_TX_PARAM_SETUP].lenDn,
  memcpy(cmd.payload, &this->bufferSession[RADIOLIB_LORAWAN_SESSION_TX_PARAM_SETUP], cmd.len);
  (void)execMacCommand(&cmd);

  cmd.cid = RADIOLIB_LORAWAN_MAC_LINK_ADR;
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_LINK_ADR].lenDn;
  memcpy(cmd.payload, &this->bufferSession[RADIOLIB_LORAWAN_SESSION_LINK_ADR], cmd.len);
  (void)execMacCommand(&cmd);

  // for fixed bands, first restore ADR, then the defined channels
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_FIXED) {
    // setup the default channels
    state = this->setupChannelsFix(this->subBand);
    RADIOLIB_ASSERT(state);

    // restore the session channels
    uint8_t *startMACpayload = &this->bufferSession[RADIOLIB_LORAWAN_SESSION_UL_CHANNELS];
    
    // there are at most 8 channel masks present
    cmd.cid = RADIOLIB_LORAWAN_MAC_LINK_ADR;
    for(int i = 0; i < 8; i++) {
      cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_LINK_ADR].lenDn;
      memcpy(cmd.payload, startMACpayload + (i * cmd.len), cmd.len);
      // there COULD, according to spec, be an all zeroes ADR command - meh
      if(memcmp(cmd.payload, bufferZeroes, cmd.len) == 0) {
        break;
      }
      cmd.repeat = (i+1);
      (void)execMacCommand(&cmd);
    }

  }

  cmd.cid = RADIOLIB_LORAWAN_MAC_DUTY_CYCLE;
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_DUTY_CYCLE].lenDn;
  memcpy(cmd.payload, &this->bufferSession[RADIOLIB_LORAWAN_SESSION_DUTY_CYCLE], cmd.len);
  (void)execMacCommand(&cmd);

  cmd.cid = RADIOLIB_LORAWAN_MAC_RX_PARAM_SETUP;
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_RX_PARAM_SETUP].lenDn;
  memcpy(cmd.payload, &this->bufferSession[RADIOLIB_LORAWAN_SESSION_RX_PARAM_SETUP], cmd.len);
  (void)execMacCommand(&cmd);

  cmd.cid = RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP;
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP].lenDn;
  memcpy(cmd.payload, &this->bufferSession[RADIOLIB_LORAWAN_SESSION_RX_TIMING_SETUP], cmd.len);
  (void)execMacCommand(&cmd);

  cmd.cid = RADIOLIB_LORAWAN_MAC_ADR_PARAM_SETUP;
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_ADR_PARAM_SETUP].lenDn;
  memcpy(cmd.payload, &this->bufferSession[RADIOLIB_LORAWAN_SESSION_ADR_PARAM_SETUP], cmd.len);
  (void)execMacCommand(&cmd);

  cmd.cid = RADIOLIB_LORAWAN_MAC_REJOIN_PARAM_SETUP;
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_REJOIN_PARAM_SETUP].lenDn;
  memcpy(cmd.payload, &this->bufferSession[RADIOLIB_LORAWAN_SESSION_REJOIN_PARAM_SETUP], cmd.len);
  (void)execMacCommand(&cmd);

  // copy uplink MAC command queue back in place
  memcpy(&this->commandsUp, &this->bufferSession[RADIOLIB_LORAWAN_SESSION_MAC_QUEUE_UL], sizeof(LoRaWANMacCommandQueue_t));

  // as both the Nonces and session are restored, revert to active session
  this->bufferNonces[RADIOLIB_LORAWAN_NONCES_ACTIVE] = (uint8_t)true;

  return(state);
}

int16_t LoRaWANNode::checkBufferCommon(uint8_t *buffer, uint16_t size) {
  // check if there are actually values in the buffer
  size_t i = 0;
  for(; i < size; i++) {
    if(buffer[i]) {
      break;
    }
  }
  if(i == size) {
    return(RADIOLIB_ERR_NETWORK_NOT_JOINED);
  }

  // check integrity of the whole buffer (compare checksum to included checksum)
  uint16_t checkSum = LoRaWANNode::checkSum16(buffer, size - 2);
  uint16_t signature = LoRaWANNode::ntoh<uint16_t>(&buffer[size - 2]);
  if(signature != checkSum) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Calculated checksum: %04X, expected: %04X", checkSum, signature);
    return(RADIOLIB_ERR_CHECKSUM_MISMATCH);
  }
  return(RADIOLIB_ERR_NONE);
}

void LoRaWANNode::activateCommon(uint8_t initialDr) {
  uint8_t drUp = 0;
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
    // if join datarate is user-specified and valid, select that value
    if(initialDr != RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
      if(initialDr >= this->band->txFreqs[0].drMin && initialDr <= this->band->txFreqs[0].drMax) {
        drUp = initialDr;
      } else {
        // if there is no channel that allowed the user-specified datarate, revert to default datarate
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Datarate %d is not valid - using default", initialDr);
        initialDr = RADIOLIB_LORAWAN_DATA_RATE_UNUSED;
      }
    }
    
    // if there is no (channel that allowed the) user-specified datarate, use a default datarate
    // we use the floor of the average datarate of the first default channel
    if(initialDr == RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
      drUp = (this->band->txFreqs[0].drMin + this->band->txFreqs[0].drMax) / 2;
    }

  } else {
    // if the user specified a certain datarate, check if any of the configured channels allows it
    if(initialDr != RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
      uint8_t i = 0; 
      for(; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
        if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled) {
          if(initialDr >= this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMin
            && initialDr <= this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMax) {
              break;
          }
        }
      }
      // if there is no channel that allowed the user-specified datarate, revert to default datarate
      if(i == RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS) {
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Datarate %d is not valid - using default", initialDr);
        initialDr = RADIOLIB_LORAWAN_DATA_RATE_UNUSED;
      }
    }

    // if there is no (channel that allowed the) user-specified datarate, use a default datarate
    // we use the join-request datarate for one of the available channels
    if(initialDr == RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
      // randomly select one of 8 or 9 channels and find corresponding datarate
      uint8_t numChannels = this->band->numTxSpans == 1 ? 8 : 9;
      uint8_t rand = this->phyLayer->random(numChannels) + 1;     // range 1-8 or 1-9
      if(rand <= 8) {
        drUp = this->band->txSpans[0].joinRequestDataRate;        // if one of the first 8 channels, select datarate of span 0
      } else {
        drUp = this->band->txSpans[1].joinRequestDataRate;        // if ninth channel, select datarate of span 1
      }
    }

  }

  LoRaWANMacCommand_t cmd = {
    .cid = RADIOLIB_LORAWAN_MAC_LINK_ADR,
    .payload = { 0 }, 
    .len = MacTable[RADIOLIB_LORAWAN_MAC_LINK_ADR].lenDn,
    .repeat = 0,
  };
  cmd.payload[0]  = (drUp << 4);      // set uplink datarate
  cmd.payload[0] |= 0;                // default to max Tx Power
  cmd.payload[3]  = (1 << 7);         // set the RFU bit, which means that the channel mask gets ignored
  (void)execMacCommand(&cmd);

  cmd.cid = RADIOLIB_LORAWAN_MAC_DUTY_CYCLE;
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_DUTY_CYCLE].lenDn;
  uint8_t maxDCyclePower;
  switch(this->band->dutyCycle) {
    case(0):
      maxDCyclePower = 0;
      break;
    case(3600):
      maxDCyclePower = 10;
      break;
    case(36000):
      maxDCyclePower = 7;
      break;
    default:
      maxDCyclePower = 0;
      break;
  }
  cmd.payload[0]  = maxDCyclePower;
  (void)execMacCommand(&cmd);

  cmd.cid = RADIOLIB_LORAWAN_MAC_RX_PARAM_SETUP;
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_RX_PARAM_SETUP].lenDn;
  cmd.payload[0]  = (RADIOLIB_LORAWAN_RX1_DR_OFFSET << 4);
  cmd.payload[0] |= this->rx2.drMax; // may be set by user, otherwise band's default upon initialization
  uint32_t rx2Freq = uint32_t(this->rx2.freq * 10000);
  LoRaWANNode::hton<uint32_t>(&cmd.payload[1], rx2Freq, 3);
  (void)execMacCommand(&cmd);

  cmd.cid = RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP;
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP].lenDn;
  cmd.payload[0]  = (RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS / 1000);
  (void)execMacCommand(&cmd);

  cmd.cid = RADIOLIB_LORAWAN_MAC_TX_PARAM_SETUP;
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_TX_PARAM_SETUP].lenDn;
  cmd.payload[0]  = (this->band->dwellTimeDn > 0 ? 1 : 0) << 5;
  cmd.payload[0] |= (this->band->dwellTimeUp > 0 ? 1 : 0) << 4;
  uint8_t maxEIRPRaw;
  switch(this->band->powerMax) {
    case(12):
      maxEIRPRaw = 2;
      break;
    case(14):
      maxEIRPRaw = 4;
      break;
    case(16):
      maxEIRPRaw = 5;
      break;
    case(19):   // this option does not exist for the TxParamSetupReq but will be caught during execution
      maxEIRPRaw = 7;
      break;
    case(30):
      maxEIRPRaw = 13;
      break;
    default:
      maxEIRPRaw = 2;
      break;
  }
  cmd.payload[0] |= maxEIRPRaw;
  (void)execMacCommand(&cmd);

  cmd.cid = RADIOLIB_LORAWAN_MAC_ADR_PARAM_SETUP;
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_ADR_PARAM_SETUP].lenDn;
  cmd.payload[0]  = (RADIOLIB_LORAWAN_ADR_ACK_LIMIT_EXP << 4);
  cmd.payload[0] |= RADIOLIB_LORAWAN_ADR_ACK_DELAY_EXP;
  (void)execMacCommand(&cmd);

  cmd.cid = RADIOLIB_LORAWAN_MAC_REJOIN_PARAM_SETUP;
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_REJOIN_PARAM_SETUP].lenDn;
  cmd.payload[0]  = (RADIOLIB_LORAWAN_REJOIN_MAX_TIME_N << 4);
  cmd.payload[0] |= RADIOLIB_LORAWAN_REJOIN_MAX_COUNT_N;
  (void)execMacCommand(&cmd);
}

void LoRaWANNode::beginOTAA(uint64_t joinEUI, uint64_t devEUI, uint8_t* nwkKey, uint8_t* appKey) {
  this->joinEUI = joinEUI;
  this->devEUI = devEUI;
  memcpy(this->nwkKey, nwkKey, RADIOLIB_AES128_KEY_SIZE);
  memcpy(this->appKey, appKey, RADIOLIB_AES128_KEY_SIZE);

  // generate activation key checksum
  this->keyCheckSum ^= LoRaWANNode::checkSum16(reinterpret_cast<uint8_t*>(&joinEUI), 8);
  this->keyCheckSum ^= LoRaWANNode::checkSum16(reinterpret_cast<uint8_t*>(&devEUI), 8);
  this->keyCheckSum ^= LoRaWANNode::checkSum16(nwkKey, 16);
  this->keyCheckSum ^= LoRaWANNode::checkSum16(appKey, 16);

  this->clearNonces();
  this->lwMode = RADIOLIB_LORAWAN_MODE_OTAA;
  this->lwClass = RADIOLIB_LORAWAN_CLASS_A;
}

int16_t LoRaWANNode::activateOTAA(uint8_t joinDr, LoRaWANJoinEvent_t *joinEvent) {
  // check if there is an active session
  if(this->isActivated()) {
    // already activated, don't do anything
    return(RADIOLIB_ERR_NONE);
  }
  if(this->bufferNonces[RADIOLIB_LORAWAN_NONCES_ACTIVE]) {
    // session restored but not yet activated - do so now
    this->isActive = true;
    return(RADIOLIB_LORAWAN_SESSION_RESTORED);
  }

  int16_t state = RADIOLIB_ERR_UNKNOWN;
  
  // either no valid session was found or user forced a new session, so clear all activity
  this->clearSession();

  // starting a new session, so make sure to update event fields already
  if(joinEvent) {
    joinEvent->newSession = true;
    joinEvent->devNonce = this->devNonce;
    joinEvent->joinNonce = this->joinNonce;
  }

  // setup join-request uplink/downlink frequencies and datarates
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
    state = this->setupChannelsDyn(true);
  } else {
    state = this->setupChannelsFix(this->subBand);
  }
  RADIOLIB_ASSERT(state);

  // on fixed bands, the join-datarate is specified per specification
  // therefore, we ignore the value that was specified by the user
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_FIXED) {
    joinDr = RADIOLIB_LORAWAN_DATA_RATE_UNUSED;
  }
  // setup all MAC properties to default values
  this->activateCommon(joinDr);

  // select a random pair of Tx/Rx channels
  state = this->selectChannels();
  RADIOLIB_ASSERT(state);

  // set the physical layer configuration for uplink
  state = this->setPhyProperties(RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK);
  RADIOLIB_ASSERT(state);

  // copy devNonce currently in use
  uint16_t devNonceUsed = this->devNonce;

  // build the join-request message
  uint8_t joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_LEN];
  
  // set the packet fields
  joinRequestMsg[0] = RADIOLIB_LORAWAN_MHDR_MTYPE_JOIN_REQUEST | RADIOLIB_LORAWAN_MHDR_MAJOR_R1;
  LoRaWANNode::hton<uint64_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_JOIN_EUI_POS], this->joinEUI);
  LoRaWANNode::hton<uint64_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_DEV_EUI_POS], this->devEUI);
  LoRaWANNode::hton<uint16_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_DEV_NONCE_POS], devNonceUsed);

  // add the authentication code
  uint32_t mic = this->generateMIC(joinRequestMsg, RADIOLIB_LORAWAN_JOIN_REQUEST_LEN - sizeof(uint32_t), this->nwkKey);
  LoRaWANNode::hton<uint32_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_LEN - sizeof(uint32_t)], mic);

  // send it
  Module* mod = this->phyLayer->getMod();
  state = this->phyLayer->transmit(joinRequestMsg, RADIOLIB_LORAWAN_JOIN_REQUEST_LEN);
  this->rxDelayStart = mod->hal->millis();
  RADIOLIB_ASSERT(state);
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("JoinRequest sent (DevNonce = %d) <-- Rx Delay start", this->devNonce);

  // join-request successfully sent, so increase & save devNonce
  this->devNonce += 1;
  LoRaWANNode::hton<uint16_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_DEV_NONCE], this->devNonce);

  // configure Rx delay for join-accept message - these are re-configured once a valid join-request is received
  this->rxDelays[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_DELAY_1_MS;
  this->rxDelays[1] = RADIOLIB_LORAWAN_JOIN_ACCEPT_DELAY_2_MS;

  // make sure the Rx2 settings are back to this band's default
  this->rx2 = this->band->rx2;

  // handle Rx1 and Rx2 windows - returns RADIOLIB_ERR_NONE if a downlink is received
  state = downlinkCommon();
  RADIOLIB_ASSERT(state);

  // build the buffer for the reply data
  uint8_t joinAcceptMsgEnc[RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN];

  // check received length
  size_t lenRx = this->phyLayer->getPacketLength(true);
  if((lenRx != RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN) && (lenRx != RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN - RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN)) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("JoinAccept reply length mismatch, expected %dB got %luB", RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN, (unsigned long)lenRx);
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
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("JoinAccept reply message type invalid, expected 0x%02x got 0x%02x", RADIOLIB_LORAWAN_MHDR_MTYPE_JOIN_ACCEPT, joinAcceptMsgEnc[0]);
    return(RADIOLIB_ERR_DOWNLINK_MALFORMED);
  }

  // decrypt the join accept message
  // this is done by encrypting again in ECB mode
  // the first byte is the MAC header which is not encrypted
  uint8_t joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN];
  joinAcceptMsg[0] = joinAcceptMsgEnc[0];
  RadioLibAES128Instance.init(this->nwkKey);
  RadioLibAES128Instance.encryptECB(&joinAcceptMsgEnc[1], RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN - 1, &joinAcceptMsg[1]);

  // get current joinNonce from downlink
  uint32_t joinNonceNew = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_NONCE_POS], 3);
  
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("JoinAccept (JoinNonce = %lu, previously %lu):", (unsigned long)joinNonceNew, (unsigned long)this->joinNonce);
  RADIOLIB_DEBUG_PROTOCOL_HEXDUMP(joinAcceptMsg, lenRx);

  // joinNonce received must be greater than the last joinNonce heard, else error
  if((this->joinNonce > 0) && (joinNonceNew <= this->joinNonce)) {
    return(RADIOLIB_ERR_JOIN_NONCE_INVALID);
  }
  this->joinNonce = joinNonceNew;

  this->homeNetId = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_HOME_NET_ID_POS], 3);
  this->devAddr = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_ADDR_POS]);

  // check LoRaWAN revision (the MIC verification depends on this)
  uint8_t dlSettings = joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_DL_SETTINGS_POS];
  this->rev = (dlSettings & RADIOLIB_LORAWAN_JOIN_ACCEPT_R_1_1) >> 7;
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("LoRaWAN revision: 1.%d", this->rev);

  // verify MIC
  if(this->rev == 1) {
    // 1.1 version, first we need to derive the join accept integrity key
    uint8_t keyDerivationBuff[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_JS_INT_KEY;
    LoRaWANNode::hton<uint64_t>(&keyDerivationBuff[1], this->devEUI);
    RadioLibAES128Instance.init(this->nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->jSIntKey);

    // prepare the buffer for MIC calculation
    uint8_t micBuff[3*RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
    micBuff[0] = RADIOLIB_LORAWAN_JOIN_REQUEST_TYPE;
    LoRaWANNode::hton<uint64_t>(&micBuff[1], this->joinEUI);
    LoRaWANNode::hton<uint16_t>(&micBuff[9], devNonceUsed);
    memcpy(&micBuff[11], joinAcceptMsg, lenRx);
    
    if(!verifyMIC(micBuff, lenRx + 11, this->jSIntKey)) {
      return(RADIOLIB_ERR_CRC_MISMATCH);
    }
  
  } else {
    // 1.0 version
    if(!verifyMIC(joinAcceptMsg, lenRx, this->nwkKey)) {
      return(RADIOLIB_ERR_CRC_MISMATCH);
    }

  }

  LoRaWANMacCommand_t cmd = {
    .cid = RADIOLIB_LORAWAN_MAC_RX_PARAM_SETUP,
    .payload = { 0 }, 
    .len = MacTable[RADIOLIB_LORAWAN_MAC_RX_PARAM_SETUP].lenDn,
    .repeat = 0,
  };
  cmd.payload[0] = dlSettings & 0x7F;
  uint32_t rx2Freq = uint32_t(this->rx2.freq * 10000);      // default Rx2 frequency
  LoRaWANNode::hton<uint32_t>(&cmd.payload[1], rx2Freq, 3);
  (void)execMacCommand(&cmd);

  cmd.cid = RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP;
  memset(cmd.payload, 0, RADIOLIB_LORAWAN_MAX_MAC_COMMAND_LEN_DOWN);
  cmd.payload[0] = joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_RX_DELAY_POS];
  cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP].lenDn;
  (void)execMacCommand(&cmd);
  
  // in case of dynamic band, setup the default channels first
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
    this->setupChannelsDyn(false);
  }
  // process CFlist if present
  if(lenRx == RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN) {
    uint8_t cfList[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN] = { 0 };
    memcpy(&cfList[0], &joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_POS], RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN);
    this->processCFList(cfList);
  } 
  // if no CFList was received, default or subband are already setup so don't need to do anything else

  // prepare buffer for key derivation
  uint8_t keyDerivationBuff[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  LoRaWANNode::hton<uint32_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_NONCE_POS], joinNonce, 3);

  // check protocol version (1.0 vs 1.1)
  if(this->rev == 1) {
    // 1.1 version, derive the keys
    LoRaWANNode::hton<uint64_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_EUI_POS], this->joinEUI);
    LoRaWANNode::hton<uint16_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_NONCE_POS], devNonceUsed);
    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_APP_S_KEY;

    RadioLibAES128Instance.init(this->appKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->appSKey);

    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_F_NWK_S_INT_KEY;
    RadioLibAES128Instance.init(this->nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->fNwkSIntKey);

    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_S_NWK_S_INT_KEY;
    RadioLibAES128Instance.init(this->nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->sNwkSIntKey);

    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_NWK_S_ENC_KEY;
    RadioLibAES128Instance.init(this->nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->nwkSEncKey);

    // enqueue the RekeyInd MAC command to be sent in the next uplink
    cmd.cid = RADIOLIB_LORAWAN_MAC_REKEY;
    memset(cmd.payload, 0, RADIOLIB_LORAWAN_MAX_MAC_COMMAND_LEN_DOWN);
    cmd.payload[0] = this->rev;
    cmd.len = sizeof(uint8_t);
    cmd.repeat = 0x01 << RADIOLIB_LORAWAN_ADR_ACK_LIMIT_EXP;
    state = pushMacCommand(&cmd, &this->commandsUp);
    RADIOLIB_ASSERT(state);
  
  } else {
    // 1.0 version, just derive the keys
    LoRaWANNode::hton<uint32_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_HOME_NET_ID_POS], this->homeNetId, 3);
    LoRaWANNode::hton<uint16_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_ADDR_POS], devNonceUsed);
    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_APP_S_KEY;
    RadioLibAES128Instance.init(this->nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->appSKey);

    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_F_NWK_S_INT_KEY;
    RadioLibAES128Instance.init(this->nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->fNwkSIntKey);

    memcpy(this->sNwkSIntKey, this->fNwkSIntKey, RADIOLIB_AES128_KEY_SIZE);
    memcpy(this->nwkSEncKey, this->fNwkSIntKey, RADIOLIB_AES128_KEY_SIZE);
  
  }

  // reset all frame counters
  this->fCntUp = 0;
  this->aFCntDown = 0;
  this->nFCntDown = 0;
  this->confFCntUp = RADIOLIB_LORAWAN_FCNT_NONE;
  this->confFCntDown = RADIOLIB_LORAWAN_FCNT_NONE;
  this->adrFCnt = 0;

  // save the activation keys checksum, device address & keys as well as JoinAccept values; these are only ever set when joining
  LoRaWANNode::hton<uint16_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_VERSION], RADIOLIB_LORAWAN_NONCES_VERSION_VAL);
  LoRaWANNode::hton<uint16_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_MODE], RADIOLIB_LORAWAN_MODE_OTAA);
  LoRaWANNode::hton<uint8_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_CLASS], RADIOLIB_LORAWAN_CLASS_A);
  LoRaWANNode::hton<uint8_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_PLAN], this->band->bandNum);
  LoRaWANNode::hton<uint16_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_CHECKSUM], this->keyCheckSum);
  LoRaWANNode::hton<uint32_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_JOIN_NONCE], this->joinNonce, 3);

  this->bufferNonces[RADIOLIB_LORAWAN_NONCES_ACTIVE] = (uint8_t)true;

  // generate the signature of the Nonces buffer, and store it in the last two bytes of the Nonces buffer
  uint16_t signature = LoRaWANNode::checkSum16(this->bufferNonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE - 2);
  LoRaWANNode::hton<uint16_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_SIGNATURE], signature);

  // store DevAddr and all keys
  LoRaWANNode::hton<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_DEV_ADDR], this->devAddr);
  memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_APP_SKEY], this->appSKey, RADIOLIB_AES128_BLOCK_SIZE);
  memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_NWK_SENC_KEY], this->nwkSEncKey, RADIOLIB_AES128_BLOCK_SIZE);
  memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_FNWK_SINT_KEY], this->fNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_SNWK_SINT_KEY], this->sNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  
  // set the signature of the Nonces buffer in the Session buffer
  LoRaWANNode::hton<uint16_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_NONCES_SIGNATURE], signature);

  // store network parameters
  LoRaWANNode::hton<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_HOMENET_ID], this->homeNetId);
  LoRaWANNode::hton<uint8_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_VERSION], this->rev);

  this->isActive = true;

  // received join-accept, so update JoinNonce value in event
  if(joinEvent) {
    joinEvent->joinNonce = this->joinNonce;
  }

  return(RADIOLIB_LORAWAN_NEW_SESSION);
}

void LoRaWANNode::beginABP(uint32_t addr, uint8_t* fNwkSIntKey, uint8_t* sNwkSIntKey, uint8_t* nwkSEncKey, uint8_t* appSKey) {
  this->devAddr = addr;
  memcpy(this->appSKey, appSKey, RADIOLIB_AES128_KEY_SIZE);
  memcpy(this->nwkSEncKey, nwkSEncKey, RADIOLIB_AES128_KEY_SIZE);
  if(fNwkSIntKey) {
    this->rev = 1;
    memcpy(this->fNwkSIntKey, fNwkSIntKey, RADIOLIB_AES128_KEY_SIZE);
  } else {
    memcpy(this->fNwkSIntKey, nwkSEncKey, RADIOLIB_AES128_KEY_SIZE);
  }
  if(sNwkSIntKey) {
    memcpy(this->sNwkSIntKey, sNwkSIntKey, RADIOLIB_AES128_KEY_SIZE);
  }

  // generate activation key checksum
  this->keyCheckSum ^= LoRaWANNode::checkSum16(reinterpret_cast<uint8_t*>(&addr), 4);
  this->keyCheckSum ^= LoRaWANNode::checkSum16(nwkSEncKey, 16);
  this->keyCheckSum ^= LoRaWANNode::checkSum16(appSKey, 16);
  if(fNwkSIntKey) { this->keyCheckSum ^= LoRaWANNode::checkSum16(fNwkSIntKey, 16); }
  if(sNwkSIntKey) { this->keyCheckSum ^= LoRaWANNode::checkSum16(sNwkSIntKey, 16); }

  // clear & set all the device credentials
  this->clearNonces();
  this->lwMode = RADIOLIB_LORAWAN_MODE_ABP;
  this->lwClass = RADIOLIB_LORAWAN_CLASS_A;
}

int16_t LoRaWANNode::activateABP(uint8_t initialDr) {
  // check if there is an active session
  if(this->isActivated()) {
    // already activated, don't do anything
    return(RADIOLIB_ERR_NONE);
  }
  if(this->bufferNonces[RADIOLIB_LORAWAN_NONCES_ACTIVE]) {
    // session restored but not yet activated - do so now
    this->isActive = true;
    return(RADIOLIB_LORAWAN_SESSION_RESTORED);
  }

  // either no valid session was found or user forced a new session, so clear all activity
  this->clearSession();

  // setup the uplink/downlink channels and initial datarate
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
    this->setupChannelsDyn();
  } else {
    this->setupChannelsFix(this->subBand);
  }

  // setup all MAC properties to default values
  this->activateCommon(initialDr);

  // reset all frame counters
  this->fCntUp = 0;
  this->aFCntDown = 0;
  this->nFCntDown = 0;
  this->confFCntUp = RADIOLIB_LORAWAN_FCNT_NONE;
  this->confFCntDown = RADIOLIB_LORAWAN_FCNT_NONE;
  this->adrFCnt = 0;

  // save the activation keys checksum, mode, class, frequency plan
  LoRaWANNode::hton<uint16_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_VERSION], RADIOLIB_LORAWAN_NONCES_VERSION_VAL);
  LoRaWANNode::hton<uint16_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_MODE], RADIOLIB_LORAWAN_MODE_ABP);
  LoRaWANNode::hton<uint8_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_CLASS], RADIOLIB_LORAWAN_CLASS_A);
  LoRaWANNode::hton<uint8_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_PLAN], this->band->bandNum);
  LoRaWANNode::hton<uint16_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_CHECKSUM], this->keyCheckSum);

  // new session all good, so set active-bit to true
  this->bufferNonces[RADIOLIB_LORAWAN_NONCES_ACTIVE] = (uint8_t)true;

  // generate the signature of the Nonces buffer, and store it in the last two bytes of the Nonces buffer
  uint16_t signature = LoRaWANNode::checkSum16(this->bufferNonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE - 2);
  LoRaWANNode::hton<uint16_t>(&this->bufferNonces[RADIOLIB_LORAWAN_NONCES_SIGNATURE], signature);

  // store DevAddr and all keys
  LoRaWANNode::hton<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_DEV_ADDR], this->devAddr);
  memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_APP_SKEY], this->appSKey, RADIOLIB_AES128_BLOCK_SIZE);
  memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_NWK_SENC_KEY], this->nwkSEncKey, RADIOLIB_AES128_BLOCK_SIZE);
  memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_FNWK_SINT_KEY], this->fNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_SNWK_SINT_KEY], this->sNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  
  // set the signature of the Nonces buffer in the Session buffer
  LoRaWANNode::hton<uint16_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_NONCES_SIGNATURE], signature);
  
  // store network parameters
  LoRaWANNode::hton<uint32_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_HOMENET_ID], this->homeNetId);
  LoRaWANNode::hton<uint8_t>(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_VERSION], this->rev);

  this->isActive = true;

  return(RADIOLIB_LORAWAN_NEW_SESSION);
}

bool LoRaWANNode::isActivated() {
  return(this->isActive);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t LoRaWANNode::uplink(String& str, uint8_t fPort, bool isConfirmed, LoRaWANEvent_t* event) {
  return(this->uplink(str.c_str(), fPort, isConfirmed, event));
}
#endif

int16_t LoRaWANNode::uplink(const char* str, uint8_t fPort, bool isConfirmed, LoRaWANEvent_t* event) {
  return(this->uplink((uint8_t*)str, strlen(str), fPort, isConfirmed, event));
}

int16_t LoRaWANNode::uplink(uint8_t* data, size_t len, uint8_t fPort, bool isConfirmed, LoRaWANEvent_t* event) {
  // if not joined, don't do anything
  if(!this->isActivated()) {
    return(RADIOLIB_ERR_NETWORK_NOT_JOINED);
  }
  
  Module* mod = this->phyLayer->getMod();
  
  // check if the Rx windows were closed after sending the previous uplink
  // this FORCES a user to call downlink() after an uplink()
  if(this->rxDelayEnd < this->rxDelayStart) {
    // not enough time elapsed since the last uplink, we may still be in an Rx window
    return(RADIOLIB_ERR_UPLINK_UNAVAILABLE);
  }

  // if adhering to dutyCycle and the time since last uplink + interval has not elapsed, return an error
  if(this->dutyCycleEnabled && this->rxDelayStart + (RadioLibTime_t)dutyCycleInterval(this->dutyCycle, this->lastToA) > mod->hal->millis()) {
    return(RADIOLIB_ERR_UPLINK_UNAVAILABLE);
  }

  // check destination fPort
  if(fPort > RADIOLIB_LORAWAN_FPORT_RESERVED) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Requested uplink at FPort %d - rejected! This FPort is RFU.", fPort);
    return(RADIOLIB_ERR_INVALID_PORT);
  }
  if(fPort == RADIOLIB_LORAWAN_FPORT_TS009 && this->TS009 == false) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Requested uplink at FPort %d - rejected! TS009 was not enabled.", fPort);
  }
  // fPort 0 is only allowed for MAC-only payloads
  if(fPort == RADIOLIB_LORAWAN_FPORT_MAC_COMMAND) {
    if (!this->isMACPayload) {
      return(RADIOLIB_ERR_INVALID_PORT);
    }
    // if this is MAC only payload, continue and reset for next uplink
    this->isMACPayload = false;
  }

  int16_t state = RADIOLIB_ERR_UNKNOWN;

  // check if there are some MAC commands to piggyback (only when piggybacking onto a application-frame)
  uint8_t fOptsLen = 0;
  if(this->commandsUp.numCommands > 0 && fPort != RADIOLIB_LORAWAN_FPORT_MAC_COMMAND) {
    // there are, assume the maximum possible FOpts len for buffer allocation
    fOptsLen = this->commandsUp.len;
  }

  // check maximum payload len as defined in phy
  if(len > this->band->payloadLenMax[this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK]]) {
    // normally, throw an error if the packet is too long
    if(this->TS009 == false) {
      return(RADIOLIB_ERR_PACKET_TOO_LONG);
    }
    // if testing with TS009 Specification Verification Protocol, don't throw error but clip the message
    len = this->band->payloadLenMax[this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK]];
  }

  bool adrAckReq = false;
  if(this->adrEnabled) {
    // check if we need to do ADR stuff
    uint32_t adrLimit = 0x01 << this->adrLimitExp;
    uint32_t adrDelay = 0x01 << this->adrDelayExp;
    if((this->fCntUp - this->adrFCnt) >= adrLimit) {
      adrAckReq = true;
    }
    // if we hit the Limit + Delay, try one of three, in order: 
    // set TxPower to max, set DR to min, enable all default channels
    if ((this->fCntUp - this->adrFCnt) == (adrLimit + adrDelay)) {
      uint8_t adrStage = 1;
      while(adrStage != 0) {
        switch(adrStage) {
          case(1): {
              // if the TxPower field has some offset, remove it and switch to maximum power
              if(this->txPowerSteps > 0) {
                // set the maximum power supported by both the module and the band
                state = this->setTxPower(this->txPowerMax);
                if(state == RADIOLIB_ERR_NONE) {
                  this->txPowerSteps = 0;
                  adrStage = 0;                         // successfully did some ADR stuff
                }
              }
              if(adrStage == 1) {                       // if nothing succeeded, proceed to stage 2
                adrStage = 2;
              }
            }
            break;
          case(2): {
              // try to decrease the datarate
              if(this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] > 0) {
                if(this->setDatarate(this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] - 1) == RADIOLIB_ERR_NONE) {
                  adrStage = 0;                         // successfully did some ADR stuff
                }
              }
              if(adrStage == 2) {                       // if nothing succeeded, proceed to stage 3
                adrStage = 3;
              }
            }
            break;
          case(3): {
              if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
                this->setupChannelsDyn(false);          // revert to default frequencies
              } else {
                // go back to default selected subband
                // hopefully it'll help something, but probably not; at least we tried..
                this->setupChannelsFix(this->subBand);
              }
              adrStage = 0;                             // nothing else to do, so end the cycle
            }
            break;
        }
      }

      // we tried something to improve the range, so increase the ADR frame counter by 'ADR delay'
      this->adrFCnt += adrDelay;
    }
  }

  // set the physical layer configuration for uplink
  this->selectChannels();
  state = this->setPhyProperties(RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK);
  RADIOLIB_ASSERT(state);

  // if dwell time is imposed, calculated expected time on air and cancel if exceeds
  if(this->dwellTimeEnabledUp && this->phyLayer->getTimeOnAir(RADIOLIB_LORAWAN_FRAME_LEN(len, fOptsLen) - 16)/1000 > this->dwellTimeUp) {
    return(RADIOLIB_ERR_DWELL_TIME_EXCEEDED);
  }

  // build the uplink message
  // the first 16 bytes are reserved for MIC calculation blocks
  size_t uplinkMsgLen = RADIOLIB_LORAWAN_FRAME_LEN(len, fOptsLen);
  #if RADIOLIB_STATIC_ONLY
  uint8_t uplinkMsg[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
  uint8_t* uplinkMsg = new uint8_t[uplinkMsgLen];
  #endif
  
  // set the packet fields
  if(isConfirmed) {
    uplinkMsg[RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS] = RADIOLIB_LORAWAN_MHDR_MTYPE_CONF_DATA_UP;
    this->confFCntUp = this->fCntUp;
  } else {
    uplinkMsg[RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS] = RADIOLIB_LORAWAN_MHDR_MTYPE_UNCONF_DATA_UP;
  }
  uplinkMsg[RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS] |= RADIOLIB_LORAWAN_MHDR_MAJOR_R1;
  LoRaWANNode::hton<uint32_t>(&uplinkMsg[RADIOLIB_LORAWAN_FHDR_DEV_ADDR_POS], this->devAddr);

  // length of fOpts will be added later
  uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] = 0x00;
  if(this->adrEnabled) {
    uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] |= RADIOLIB_LORAWAN_FCTRL_ADR_ENABLED;
    if(adrAckReq) {
      uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] |= RADIOLIB_LORAWAN_FCTRL_ADR_ACK_REQ;
    }
  }

  // if the saved confirm-fCnt is set, set the ACK bit
  bool isConfirmingDown = false;
  if(this->confFCntDown != RADIOLIB_LORAWAN_FCNT_NONE) {
    isConfirmingDown = true;
    uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] |= RADIOLIB_LORAWAN_FCTRL_ACK;
  }

  LoRaWANNode::hton<uint16_t>(&uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCNT_POS], (uint16_t)this->fCntUp);

  // check if we have some MAC commands to append
  if(fOptsLen > 0) {
    // assume maximum possible buffer size
    uint8_t fOptsBuff[RADIOLIB_LORAWAN_FHDR_FOPTS_MAX_LEN];
    uint8_t* fOptsPtr = fOptsBuff;

    // append all MAC replies into fOpts buffer
    int16_t i = 0;
    for (; i < this->commandsUp.numCommands; i++) {
      LoRaWANMacCommand_t cmd = this->commandsUp.commands[i];
      memcpy(fOptsPtr, &cmd, 1 + cmd.len);
      fOptsPtr += cmd.len + 1;
    }
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Uplink MAC payload (%d commands):", this->commandsUp.numCommands);
    RADIOLIB_DEBUG_PROTOCOL_HEXDUMP(fOptsBuff, fOptsLen);

    // pop the commands from back to front
    for (; i >= 0; i--) {
      if(this->commandsUp.commands[i].repeat > 0) {
        this->commandsUp.commands[i].repeat--;
      } else {
        deleteMacCommand(this->commandsUp.commands[i].cid, &this->commandsUp);
      }
    }

    uplinkMsgLen = RADIOLIB_LORAWAN_FRAME_LEN(len, fOptsLen);
    uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] |= fOptsLen;

    // encrypt it
    processAES(fOptsBuff, fOptsLen, this->nwkSEncKey, &uplinkMsg[RADIOLIB_LORAWAN_FHDR_FOPTS_POS], this->fCntUp, RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK, 0x01, true);
    
  }

  // set the fPort
  uplinkMsg[RADIOLIB_LORAWAN_FHDR_FPORT_POS(fOptsLen)] = fPort;

  // select encryption key based on the target fPort
  uint8_t* encKey = this->appSKey;
  if(fPort == RADIOLIB_LORAWAN_FPORT_MAC_COMMAND) {
    encKey = this->nwkSEncKey;
  }

  // encrypt the frame payload
  processAES(data, len, encKey, &uplinkMsg[RADIOLIB_LORAWAN_FRAME_PAYLOAD_POS(fOptsLen)], this->fCntUp, RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK, 0x00, true);

  // create blocks for MIC calculation
  uint8_t block0[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  block0[RADIOLIB_LORAWAN_BLOCK_MAGIC_POS] = RADIOLIB_LORAWAN_MIC_BLOCK_MAGIC;
  block0[RADIOLIB_LORAWAN_BLOCK_DIR_POS] = RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK;
  LoRaWANNode::hton<uint32_t>(&block0[RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS], this->devAddr);
  LoRaWANNode::hton<uint32_t>(&block0[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], this->fCntUp);
  block0[RADIOLIB_LORAWAN_MIC_BLOCK_LEN_POS] = uplinkMsgLen - RADIOLIB_AES128_BLOCK_SIZE - sizeof(uint32_t);

  uint8_t block1[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  memcpy(block1, block0, RADIOLIB_AES128_BLOCK_SIZE);
  if(this->confFCntDown != RADIOLIB_LORAWAN_FCNT_NONE) {
    LoRaWANNode::hton<uint16_t>(&block1[RADIOLIB_LORAWAN_BLOCK_CONF_FCNT_POS], (uint16_t)this->confFCntDown);
  }
  block1[RADIOLIB_LORAWAN_MIC_DATA_RATE_POS] = this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK];
  block1[RADIOLIB_LORAWAN_MIC_CH_INDEX_POS] = this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK].idx;
  
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Uplink (FCntUp = %lu) decoded:", (unsigned long)this->fCntUp);

  RADIOLIB_DEBUG_PROTOCOL_HEXDUMP(uplinkMsg, uplinkMsgLen);

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

  // send it (without the MIC calculation blocks)
  state = this->phyLayer->transmit(&uplinkMsg[RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS], uplinkMsgLen - RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS);

  // set the timestamp so that we can measure when to start receiving
  this->rxDelayStart = mod->hal->millis();
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Uplink sent <-- Rx Delay start");

  // calculate Time on Air of this uplink in milliseconds
  this->lastToA = this->phyLayer->getTimeOnAir(uplinkMsgLen - RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS) / 1000;

  #if !RADIOLIB_STATIC_ONLY
  delete[] uplinkMsg;
  #endif
  RADIOLIB_ASSERT(state);
  
  // the downlink confirmation was acknowledged, so clear the counter value
  this->confFCntDown = RADIOLIB_LORAWAN_FCNT_NONE;

  // pass the extra info if requested
  if(event) {
    event->dir = RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK;
    event->confirmed = isConfirmed;
    event->confirming = isConfirmingDown;
    event->datarate = this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK];
    event->freq = currentChannels[event->dir].freq;
    event->power = this->txPowerMax - this->txPowerSteps * 2;
    event->fCnt = this->fCntUp;
    event->fPort = fPort;
  }

  // increase frame counter by one for the next uplink
  this->fCntUp += 1;

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::downlinkCommon() {
  Module* mod = this->phyLayer->getMod();

  // according to the spec, the Rx window must be at least enough time to effectively detect a preamble
  // but we pad it a bit on both sides (start and end) to make sure it is wide enough
  const RadioLibTime_t scanGuard = 10;      // Rx window padding in milliseconds

  // check if there are any upcoming Rx windows
  // if the Rx1 window has already started, you're too late, because most downlinks happen in Rx1
  RadioLibTime_t now = mod->hal->millis();  // fix the current timestamp to prevent negative delays
  if(now > this->rxDelayStart + this->rxDelays[0] - scanGuard) {
    // if between start of Rx1 and end of Rx2, wait until Rx2 closes
    if(now < this->rxDelayStart + this->rxDelays[1]) {
      mod->hal->delay(this->rxDelays[1] + this->rxDelayStart - now);
    }
    // update the end timestamp in case user got stuck between uplink and downlink
    this->rxDelayEnd = mod->hal->millis();
    return(RADIOLIB_ERR_NO_RX_WINDOW);
  }

  // set the physical layer configuration for downlink
  int16_t state = this->setPhyProperties(RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK);
  RADIOLIB_ASSERT(state);

  // create the masks that are required for receiving downlinks
  uint32_t irqFlags = 0;
  uint32_t irqMask = 0;
  this->phyLayer->irqRxDoneRxTimeout(irqFlags, irqMask);

  this->phyLayer->setPacketReceivedAction(LoRaWANNodeOnDownlinkAction);

  // perform listening in the two Rx windows
  for(uint8_t i = 0; i < 2; i++) {
    downlinkAction = false;

    // calculate the Rx timeout
    RadioLibTime_t timeoutHost = this->phyLayer->getTimeOnAir(0) + 2*scanGuard*1000;
    RadioLibTime_t timeoutMod  = this->phyLayer->calculateRxTimeout(timeoutHost);

    // wait for the start of the Rx window
    RadioLibTime_t waitLen = this->rxDelayStart + this->rxDelays[i] - mod->hal->millis();
    // make sure that no underflow occured; if so, clip the delay (although this will likely miss any downlink)
    if(waitLen > this->rxDelays[i]) {
      waitLen = this->rxDelays[i];
    }
    // the waiting duration is shortened a bit to cover any possible timing errors
    if(waitLen > scanGuard) {
      waitLen -= scanGuard;
    }
    mod->hal->delay(waitLen);

    // open Rx window by starting receive with specified timeout
    state = this->phyLayer->startReceive(timeoutMod, irqFlags, irqMask, 0);
    RADIOLIB_ASSERT(state);
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Opening Rx%d window (%d ms timeout)... <-- Rx Delay end ", i+1, (int)(timeoutHost / 1000 + scanGuard / 2));
    
    // wait for the timeout to complete (and a small additional delay)
    mod->hal->delay(timeoutHost / 1000 + scanGuard / 2);
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Closing Rx%d window", i+1);

    // check if the IRQ bit for Rx Timeout is set
    if(!this->phyLayer->isRxTimeout()) {
      break;

    } else if(i == 0) {
      // nothing in the first window, configure for the second
      this->phyLayer->standby();
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("PHY: Frequency %cL = %6.3f MHz", 'D', this->rx2.freq);
      state = this->phyLayer->setFrequency(this->rx2.freq);
      RADIOLIB_ASSERT(state);

      DataRate_t dataRate;
      state = findDataRate(this->rx2.drMax, &dataRate);
      RADIOLIB_ASSERT(state);
      state = this->phyLayer->setDataRate(dataRate);
      RADIOLIB_ASSERT(state);
    }
    
  }
  // Rx windows are now closed
  this->rxDelayEnd = mod->hal->millis();

  // if we got here due to a timeout, stop ongoing activities
  if(this->phyLayer->isRxTimeout()) {
    this->phyLayer->standby();  // TODO check: this should be done automagically due to RxSingle?
    if(!this->FSK) {
      this->phyLayer->invertIQ(false);
    }

    return(RADIOLIB_LORAWAN_NO_DOWNLINK);
  }

  // wait for the DIO to fire indicating a downlink is received
  now = mod->hal->millis();
  bool downlinkComplete = true;
  while(!downlinkAction) {
    mod->hal->yield();
    // this should never happen, but if it does this would be an infinite loop
    if(mod->hal->millis() - now > 3000UL) {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Downlink missing!");
      downlinkComplete = false;
      break;
    }
  }

  // we have a message, clear actions, go to standby and reset the IQ inversion
  this->phyLayer->standby();  // TODO check: this should be done automagically due to RxSingle?
  this->phyLayer->clearPacketReceivedAction();
  if(!this->FSK) {
    state = this->phyLayer->invertIQ(false);
    RADIOLIB_ASSERT(state);
  }

  if(!downlinkComplete) {
    state = RADIOLIB_LORAWAN_NO_DOWNLINK;
  }

  return(state);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t LoRaWANNode::downlink(String& str, LoRaWANEvent_t* event) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;

  // build a temporary buffer
  // LoRaWAN downlinks can have 250 bytes at most with 1 extra byte for NULL
  size_t length = 0;
  uint8_t data[251];

  // wait for downlink
  state = this->downlink(data, &length, event);
  if(state == RADIOLIB_ERR_NONE) {
    // add null terminator
    data[length] = '\0';

    // initialize Arduino String class
    str = String((char*)data);
  }

  return(state);
}
#endif

int16_t LoRaWANNode::downlink(LoRaWANEvent_t* event) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;

  // build a temporary buffer
  // LoRaWAN downlinks can have 250 bytes at most with 1 extra byte for NULL
  size_t length = 0;
  uint8_t data[251];

  // wait for downlink
  state = this->downlink(data, &length, event);

  return(state);
}

int16_t LoRaWANNode::downlink(uint8_t* data, size_t* len, LoRaWANEvent_t* event) {
  // handle Rx1 and Rx2 windows - returns RADIOLIB_ERR_NONE if a downlink is received
  int16_t state = downlinkCommon();
  RADIOLIB_ASSERT(state);

  // get the packet length
  size_t downlinkMsgLen = this->phyLayer->getPacketLength();

  // check the minimum required frame length
  // an extra byte is subtracted because downlink frames may not have a fPort
  if(downlinkMsgLen < RADIOLIB_LORAWAN_FRAME_LEN(0, 0) - 1 - RADIOLIB_AES128_BLOCK_SIZE) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Downlink message too short (%lu bytes)", (unsigned long)downlinkMsgLen);
    return(RADIOLIB_ERR_DOWNLINK_MALFORMED);
  }

  // build the buffer for the downlink message
  // the first 16 bytes are reserved for MIC calculation block
  #if !RADIOLIB_STATIC_ONLY
    uint8_t* downlinkMsg = new uint8_t[RADIOLIB_AES128_BLOCK_SIZE + downlinkMsgLen];
  #else
    uint8_t downlinkMsg[RADIOLIB_STATIC_ARRAY_SIZE];
  #endif

  // read the data
  state = this->phyLayer->readData(&downlinkMsg[RADIOLIB_AES128_BLOCK_SIZE], downlinkMsgLen);
  // downlink frames are sent without CRC, which will raise error on SX127x
  // we can ignore that error
  if(state == RADIOLIB_ERR_LORA_HEADER_DAMAGED) {
    state = RADIOLIB_ERR_NONE;
  }
  
  if(state != RADIOLIB_ERR_NONE) {
    #if !RADIOLIB_STATIC_ONLY
      delete[] downlinkMsg;
    #endif
    return(state);
  }

  // check the address
  uint32_t addr = LoRaWANNode::ntoh<uint32_t>(&downlinkMsg[RADIOLIB_LORAWAN_FHDR_DEV_ADDR_POS]);
  if(addr != this->devAddr) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Device address mismatch, expected 0x%08X, got 0x%08X", this->devAddr, addr);
    #if !RADIOLIB_STATIC_ONLY
      delete[] downlinkMsg;
    #endif
    return(RADIOLIB_ERR_DOWNLINK_MALFORMED);
  }

  // calculate length of FOpts and payload
  uint8_t fOptsLen = downlinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] & RADIOLIB_LORAWAN_FHDR_FOPTS_LEN_MASK;

  // check if the ACK bit is set, indicating this frame acknowledges the previous uplink
  bool isConfirmingUp = false;
  if((downlinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] & RADIOLIB_LORAWAN_FCTRL_ACK)) {
    isConfirmingUp = true;
  }

  // total - MHDR(1) - DevAddr(4) - FCtrl(1) - FCnt(2) - FOpts - MIC(4)
  // potentially also an FPort, but we'll find out soon enough
  uint8_t payLen = downlinkMsgLen - 1 - 4 - 1 - 2 - fOptsLen - 4;

  // get the frame counter
  uint16_t fCnt16 = LoRaWANNode::ntoh<uint16_t>(&downlinkMsg[RADIOLIB_LORAWAN_FHDR_FCNT_POS]);

  // set the MIC calculation blocks
  memset(downlinkMsg, 0x00, RADIOLIB_AES128_BLOCK_SIZE);
  downlinkMsg[RADIOLIB_LORAWAN_BLOCK_MAGIC_POS] = RADIOLIB_LORAWAN_MIC_BLOCK_MAGIC;
  // if this downlink is confirming an uplink, the MIC was generated with the least-significant 16 bits of that fCntUp
  if(isConfirmingUp && (this->rev == 1)) {
    LoRaWANNode::hton<uint16_t>(&downlinkMsg[RADIOLIB_LORAWAN_BLOCK_CONF_FCNT_POS], (uint16_t)this->confFCntUp);
  }
  downlinkMsg[RADIOLIB_LORAWAN_BLOCK_DIR_POS] = RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK;
  LoRaWANNode::hton<uint32_t>(&downlinkMsg[RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS], this->devAddr);
  LoRaWANNode::hton<uint16_t>(&downlinkMsg[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], fCnt16);
  downlinkMsg[RADIOLIB_LORAWAN_MIC_BLOCK_LEN_POS] = downlinkMsgLen - sizeof(uint32_t);
 
  // check the MIC
  if(!verifyMIC(downlinkMsg, RADIOLIB_AES128_BLOCK_SIZE + downlinkMsgLen, this->sNwkSIntKey)) {
    #if !RADIOLIB_STATIC_ONLY
      delete[] downlinkMsg;
    #endif
    return(RADIOLIB_ERR_CRC_MISMATCH);
  }

  // in LoRaWAN v1.1, a frame is a Network frame if there is no Application payload
  // i.e.: either no payload at all (empty frame or FOpts only), or MAC only payload (FPort = 0)
  uint8_t fPort = RADIOLIB_LORAWAN_FPORT_MAC_COMMAND;
  bool isAppDownlink = false;
  if(this->rev == 0) {
    isAppDownlink = true;
  }
  if(payLen > 0) {
    payLen -= 1;  // subtract one as fPort is set
    fPort = downlinkMsg[RADIOLIB_LORAWAN_FHDR_FPORT_POS(fOptsLen)];
    // check if fPort value is actually allowed
    if(fPort > RADIOLIB_LORAWAN_FPORT_RESERVED) {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Received downlink at FPort %d - rejected! This FPort is RFU!", fPort);
      #if !RADIOLIB_STATIC_ONLY
        delete[] downlinkMsg;
      #endif
      return(RADIOLIB_ERR_INVALID_PORT);
    }
    if(fPort == RADIOLIB_LORAWAN_FPORT_TS009 && this->TS009 == false) {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Received downlink at FPort %d - rejected! TS009 was not enabled.", fPort);
      #if !RADIOLIB_STATIC_ONLY
        delete[] downlinkMsg;
      #endif
      return(RADIOLIB_ERR_INVALID_PORT);
    }

    if(fPort > RADIOLIB_LORAWAN_FPORT_MAC_COMMAND) {
      isAppDownlink = true;
    } else {
      fOptsLen = payLen;
    }
  }

  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Downlink (%sFCntDown = %d) encoded:", isAppDownlink ? "A" : "N", fCnt16);
  RADIOLIB_DEBUG_PROTOCOL_HEXDUMP(downlinkMsg, RADIOLIB_AES128_BLOCK_SIZE + downlinkMsgLen);

  // check the fCntDown value (Network or Application)
  uint32_t fCntDownPrev = 0;
  if (isAppDownlink) {
    fCntDownPrev = this->aFCntDown;
  } else {
    fCntDownPrev = this->nFCntDown;
  }

  // if this is not the first downlink...
  // assume a 16-bit to 32-bit rollover if difference between counters in LSB is smaller than MAX_FCNT_GAP
  // if that isn't the case and the received fCnt is smaller or equal to the last heard fCnt, then error
  uint32_t fCnt32 = fCnt16;
  if(fCntDownPrev > 0) {
    if((fCnt16 <= fCntDownPrev) && ((0xFFFF - (uint16_t)fCntDownPrev + fCnt16) > RADIOLIB_LORAWAN_MAX_FCNT_GAP)) {
      #if !RADIOLIB_STATIC_ONLY
        delete[] downlinkMsg;
      #endif
      if (isAppDownlink) {
        return(RADIOLIB_ERR_A_FCNT_DOWN_INVALID);
      } else {
        return(RADIOLIB_ERR_N_FCNT_DOWN_INVALID);
      }
    } else if (fCnt16 <= fCntDownPrev) {
      uint16_t msb = (fCntDownPrev >> 16) + 1;  // assume a rollover
      fCnt32 |= ((uint32_t)msb << 16);          // add back the MSB part
    }
  }
  
  // save current fCnt to respective frame counter
  if (isAppDownlink) {
    this->aFCntDown = fCnt32;
  } else {
    this->nFCntDown = fCnt32;
  }

  // if this is a confirmed frame, save the downlink number (only app frames can be confirmed)
  bool isConfirmedDown = false;
  if((downlinkMsg[RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS] & 0xFE) == RADIOLIB_LORAWAN_MHDR_MTYPE_CONF_DATA_DOWN) {
    this->confFCntDown = this->aFCntDown;
    isConfirmedDown = true;
  }

  // process FOpts (if there are any)
  if(fOptsLen > 0) {
    // there are some Fopts, decrypt them
    #if !RADIOLIB_STATIC_ONLY
      uint8_t* fOpts = new uint8_t[RADIOLIB_MAX(RADIOLIB_LORAWAN_FHDR_FOPTS_LEN_MASK, (int)fOptsLen)];
    #else
      uint8_t fOpts[RADIOLIB_STATIC_ARRAY_SIZE];
    #endif

    // TODO it COULD be the case that the assumed FCnt rollover is incorrect, if possible figure out a way to catch this and retry with just fCnt16
    // if there are <= 15 bytes of FOpts, they are in the FHDR, otherwise they are in the payload
    // in case of the latter, process AES is if it were a normal payload but using the NwkSEncKey
    if(fOptsLen <= RADIOLIB_LORAWAN_FHDR_FOPTS_LEN_MASK) {
      uint8_t ctrId = 0x01 + isAppDownlink; // see LoRaWAN v1.1 errata
      processAES(&downlinkMsg[RADIOLIB_LORAWAN_FHDR_FOPTS_POS], (size_t)fOptsLen, this->nwkSEncKey, fOpts, fCnt32, RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK, ctrId, true);
    } else {
      processAES(&downlinkMsg[RADIOLIB_LORAWAN_FRAME_PAYLOAD_POS(0)], (size_t)fOptsLen, this->nwkSEncKey, fOpts, fCnt32, RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK, 0x00, true);
    }

    bool hasADR = false;
    uint8_t numADR = 0;
    uint8_t lastCID = 0;

    // process the MAC command(s)
    int8_t remLen = fOptsLen;
    uint8_t* fOptsPtrDn = fOpts;
    while(remLen > 0) {
      uint8_t cid = *fOptsPtrDn;
      uint8_t macLen = getMacPayloadLength(cid);
      if(cid == RADIOLIB_LORAWAN_MAC_LINK_ADR) {
        // if there was an earlier ADR command but it was not the last, ignore it
        if(hasADR && lastCID != RADIOLIB_LORAWAN_MAC_LINK_ADR) {
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Encountered non-consecutive block of ADR commands - skipping");
          remLen -= (macLen + 1);
          fOptsPtrDn += (macLen + 1);
          lastCID = cid;
          continue;
        }
        // otherwise, set ADR flag to true and increase counter
        hasADR = true;
        numADR++;
      }
      if(macLen + 1 > remLen)
        break;
      LoRaWANMacCommand_t cmd = {
        .cid = cid,
        .payload = { 0 },
        .len = macLen,
        .repeat = (cid == RADIOLIB_LORAWAN_MAC_LINK_ADR ? numADR : (uint8_t)0),
      };
      memcpy(cmd.payload, fOptsPtrDn + 1, macLen);

      // process the MAC command
      bool sendUp = execMacCommand(&cmd);
      if(sendUp) {
        pushMacCommand(&cmd, &this->commandsUp);
      }

      // processing succeeded, move in the buffer to the next command
      remLen -= (macLen + 1);
      fOptsPtrDn += (macLen + 1);
      lastCID = cid;
    }

    #if !RADIOLIB_STATIC_ONLY
      delete[] fOpts;
    #endif

    // if fOptsLen for the next uplink is larger than can be piggybacked onto an uplink, send separate uplink
    if(this->commandsUp.len > RADIOLIB_LORAWAN_FHDR_FOPTS_MAX_LEN) {
      size_t fOptsBufSize = this->commandsUp.len;
      #if RADIOLIB_STATIC_ONLY
        uint8_t fOptsBuff[RADIOLIB_STATIC_ARRAY_SIZE];
      #else
        uint8_t* fOptsBuff = new uint8_t[fOptsBufSize];
      #endif
      uint8_t* fOptsPtrUp = fOptsBuff;
      // append all MAC replies into fOpts buffer
      int16_t i = 0;
      for (; i < this->commandsUp.numCommands; i++) {
        LoRaWANMacCommand_t cmd = this->commandsUp.commands[i];
        memcpy(fOptsPtrUp, &cmd, 1 + cmd.len);
        fOptsPtrUp += cmd.len + 1;
      }
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Uplink MAC payload (%d commands):", this->commandsUp.numCommands);
      RADIOLIB_DEBUG_PROTOCOL_HEXDUMP(fOptsBuff, fOptsBufSize);

      // pop the commands from back to front
      for (; i >= 0; i--) {
        if(this->commandsUp.commands[i].repeat > 0) {
          this->commandsUp.commands[i].repeat--;
        } else {
          deleteMacCommand(this->commandsUp.commands[i].cid, &this->commandsUp);
        }
      }

      this->isMACPayload = true;
      // temporarily lift dutyCycle restrictions to allow immediate MAC response
      bool prevDC = this->dutyCycleEnabled;
      this->dutyCycleEnabled = false;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Sending MAC-only uplink .. ");
      state = this->uplink(fOptsBuff, fOptsBufSize, RADIOLIB_LORAWAN_FPORT_MAC_COMMAND);
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN(" .. state: %d", state);
      this->dutyCycleEnabled = prevDC;
      #if !RADIOLIB_STATIC_ONLY
        delete[] fOptsBuff;
      #endif
      RADIOLIB_ASSERT(state);

      #if RADIOLIB_STATIC_ONLY
        uint8_t strDown[RADIOLIB_STATIC_ARRAY_SIZE];
      #else
        uint8_t* strDown = new uint8_t[this->band->payloadLenMax[this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK]]];
      #endif
      size_t lenDown = 0;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Receiving after MAC-only uplink .. ");
      state = this->downlink(strDown, &lenDown);
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN(" .. state: %d", state);
      #if !RADIOLIB_STATIC_ONLY
        delete[] strDown;
      #endif
      RADIOLIB_ASSERT(state);
    }

  }

  // a downlink was received, so reset the ADR counter to the last uplink's fCnt
  this->adrFCnt = this->getFCntUp();

  // pass the extra info if requested
  if(event) {
    event->dir = RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK;
    event->confirmed = isConfirmedDown;
    event->confirming = isConfirmingUp;
    event->datarate = this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK];
    event->freq = currentChannels[event->dir].freq;
    event->power = this->txPowerMax - this->txPowerSteps * 2;
    event->fCnt = isAppDownlink ? this->aFCntDown : this->nFCntDown;
    event->fPort = fPort;
  }

  // if MAC-only payload, return now
  if(fPort == RADIOLIB_LORAWAN_FPORT_MAC_COMMAND) {
    // no payload
    *len = 0;
    #if !RADIOLIB_STATIC_ONLY
      delete[] downlinkMsg;
    #endif

    return(RADIOLIB_ERR_NONE);
  }

  // process Application payload
  *len = payLen;

  // TODO it COULD be the case that the assumed rollover is incorrect, then figure out a way to catch this and retry with just fCnt16
  processAES(&downlinkMsg[RADIOLIB_LORAWAN_FRAME_PAYLOAD_POS(fOptsLen)], payLen, this->appSKey, data, fCnt32, RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK, 0x00, true);
  
  #if !RADIOLIB_STATIC_ONLY
    delete[] downlinkMsg;
  #endif

  return(RADIOLIB_ERR_NONE);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t LoRaWANNode::sendReceive(String& strUp, uint8_t fPort, String& strDown, bool isConfirmed, LoRaWANEvent_t* eventUp, LoRaWANEvent_t* eventDown) {
  // send the uplink
  int16_t state = this->uplink(strUp, fPort, isConfirmed, eventUp);
  RADIOLIB_ASSERT(state);

  // wait for the downlink
  state = this->downlink(strDown, eventDown);
  return(state);
}
#endif

int16_t LoRaWANNode::sendReceive(uint8_t* dataUp, size_t lenUp, uint8_t fPort, bool isConfirmed, LoRaWANEvent_t* eventUp, LoRaWANEvent_t* eventDown) {
  // send the uplink
  int16_t state = this->uplink(dataUp, lenUp, fPort, isConfirmed, eventUp);
  RADIOLIB_ASSERT(state);

  // wait for the downlink
  state = this->downlink(eventDown);
  return(state);
}

int16_t LoRaWANNode::sendReceive(const char* strUp, uint8_t fPort, uint8_t* dataDown, size_t* lenDown, bool isConfirmed, LoRaWANEvent_t* eventUp, LoRaWANEvent_t* eventDown) {
  // send the uplink
  int16_t state = this->uplink(strUp, fPort, isConfirmed, eventUp);
  RADIOLIB_ASSERT(state);

  // wait for the downlink
  state = this->downlink(dataDown, lenDown, eventDown);
  return(state);
}

int16_t LoRaWANNode::sendReceive(uint8_t* dataUp, size_t lenUp, uint8_t fPort, uint8_t* dataDown, size_t* lenDown, bool isConfirmed, LoRaWANEvent_t* eventUp, LoRaWANEvent_t* eventDown) {
  // send the uplink
  int16_t state = this->uplink(dataUp, lenUp, fPort, isConfirmed, eventUp);
  RADIOLIB_ASSERT(state);

  // wait for the downlink
  state = this->downlink(dataDown, lenDown, eventDown);
  return(state);
}

void LoRaWANNode::setDeviceStatus(uint8_t battLevel) {
  this->battLevel = battLevel;
}

// return fCnt of last uplink; also return 0 if no uplink occured yet
uint32_t LoRaWANNode::getFCntUp() {
  if(this->fCntUp == 0) {
    return(0);
  }
  return(this->fCntUp - 1);
}

uint32_t LoRaWANNode::getNFCntDown() {
  return(this->nFCntDown);
}

uint32_t LoRaWANNode::getAFCntDown() {
  return(this->aFCntDown);
}

void LoRaWANNode::resetFCntDown() {
  this->nFCntDown = 0;
  this->aFCntDown = 0;
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
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("MIC mismatch, expected %08x, got %08x", micCalculated, micReceived);
    return(false);
  }

  return(true);
}

int16_t LoRaWANNode::setPhyProperties(uint8_t dir) {
  // set the physical layer configuration
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("");
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("PHY: Frequency %cL = %6.3f MHz", dir ? 'D' : 'U', this->currentChannels[dir].freq);
  int16_t state = this->phyLayer->setFrequency(this->currentChannels[dir].freq);
  RADIOLIB_ASSERT(state);

  // if this channel is an FSK channel, toggle the FSK switch
  if(this->band->dataRates[this->dataRates[dir]] == RADIOLIB_LORAWAN_DATA_RATE_FSK_50_K) {
    this->FSK = true;
  } else {
    this->FSK = false;
  }

  int8_t pwr = this->txPowerMax - this->txPowerSteps * 2;
  
  // at this point, assume that Tx power value is already checked, so ignore the return value
  (void)this->phyLayer->checkOutputPower(pwr, &pwr);
  state = this->phyLayer->setOutputPower(pwr);
  RADIOLIB_ASSERT(state);

  DataRate_t dr;
  state = findDataRate(this->dataRates[dir], &dr);
  RADIOLIB_ASSERT(state);
  state = this->phyLayer->setDataRate(dr);
  RADIOLIB_ASSERT(state);

  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("PHY: SF = %d, TX = %d dBm, BW = %6.3f kHz, CR = 4/%d", 
                            dr.lora.spreadingFactor, pwr, dr.lora.bandwidth, dr.lora.codingRate);

  if(this->FSK) {
    state = this->phyLayer->setDataShaping(RADIOLIB_SHAPING_1_0);
    RADIOLIB_ASSERT(state);
    state = this->phyLayer->setEncoding(RADIOLIB_ENCODING_WHITENING);
    RADIOLIB_ASSERT(state);
  }

  // downlink messages are sent with inverted IQ
  if(dir == RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK) {
    if(!this->FSK) {
      state = this->phyLayer->invertIQ(true);
      RADIOLIB_ASSERT(state);
    }
  }

  // this only needs to be done once-ish
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

int16_t LoRaWANNode::setupChannelsDyn(bool joinRequest) {
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Setting up dynamic channels");
  
  size_t num = 0;
  // copy the default defined channels into the first slots (where Tx = Rx)
  for(; num < 3 && this->band->txFreqs[num].enabled; num++) {
    this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num] = this->band->txFreqs[num];
    this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][num] = this->band->txFreqs[num];
  }

  // if we're about to send a join-request, copy the join-request channels to the next slots
  if(joinRequest) {
    size_t numJR = 0;
    for(; numJR < 3 && this->band->txJoinReq[num].enabled; numJR++, num++) {
      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num] = this->band->txFreqs[num];
      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][num] = this->band->txFreqs[num];
    }
  }

  // clear all remaining channels
  for(; num < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; num++) {
    this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num] = RADIOLIB_LORAWAN_CHANNEL_NONE;
  }

  for (int i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
    if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled) {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("UL: %3d %d %7.3f (%d - %d) | DL: %3d %d %7.3f (%d - %d)",
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].freq,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMin,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMax,

                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].idx,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].enabled,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].freq,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].drMin,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].drMax
                            );
    }
  }
  
  return(RADIOLIB_ERR_NONE);
}

// setup a subband and its corresponding join-request datarate
// WARNING: subBand starts at 1 (corresponds to all populair schemes)
int16_t LoRaWANNode::setupChannelsFix(uint8_t subBand) {
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Setting up fixed channels (subband %d)", subBand);

  // clear all existing channels
  for(size_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
    this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i] = RADIOLIB_LORAWAN_CHANNEL_NONE;
  }

  // if no subband is selected by user, cycle through banks of 8 using devNonce value
  if(subBand == 0) {
    uint8_t numBanks8 = this->band->txSpans[0].numChannels / 8;
    subBand = this->devNonce % numBanks8;
  }

  uint8_t chMaskCntl = 0;
  uint16_t chMask = 0;
  
  // if there are two channel spans, first set the channel from second span
  if(this->band->numTxSpans == 2) {
    chMaskCntl = 7;
    chMask = (1 << (subBand - 1));   // set channel mask
    this->applyChannelMaskFix(chMaskCntl, chMask);
  }

  // chMask is set for 16 channels at once, so widen the Cntl value
  chMaskCntl = (subBand - 1) / 2;   // compensate the 1 offset

  // now select the correct bank of 8 channels
  if(subBand % 2 == 0) {            // even subbands
    chMask = 0xFF00;
  } else {
    chMask = 0x00FF;                // odd subbands
  }
  this->applyChannelMaskFix(chMaskCntl, chMask);

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::processCFList(uint8_t* cfList) {
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Processing CFList");
  
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
    // retrieve number of existing (default) channels
    size_t num = 0;
    for(int i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
      if(!this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled) {
        break;
      }
      num++;
    }

    LoRaWANMacCommand_t cmd = {
      .cid = RADIOLIB_LORAWAN_MAC_NEW_CHANNEL,
      .payload = { 0 }, 
      .len = 0,
      .repeat = 0,
    };
    // datarate range for all new channels is equal to the default channels
    cmd.payload[4] = (this->band->txFreqs[0].drMax << 4) | this->band->txFreqs[0].drMin;
    for(uint8_t i = 0; i < 5; i++, num++) {
      cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_NEW_CHANNEL].lenDn;
      cmd.payload[0] = num;
      memcpy(&cmd.payload[1], &cfList[i*3], 3);
      (void)execMacCommand(&cmd);
    }
  } else {                // RADIOLIB_LORAWAN_BAND_FIXED
    // complete channel mask received, so clear all existing channels
    for(int i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i] = RADIOLIB_LORAWAN_CHANNEL_NONE;
    }

    LoRaWANMacCommand_t cmd = {
      .cid = RADIOLIB_LORAWAN_MAC_LINK_ADR,
      .payload = { 0 }, 
      .len = 0,
      .repeat = 0,
    };

    // in case of mask-type bands, copy those frequencies that are masked true into the available TX channels
    size_t numChMasks = 3 + this->band->numTxSpans;       // 4 masks for bands with 2 spans, 5 spans for bands with 1 span
    for(size_t chMaskCntl = 0; chMaskCntl < numChMasks; chMaskCntl++) {
      cmd.len = MacTable[RADIOLIB_LORAWAN_MAC_LINK_ADR].lenDn;
      cmd.payload[0] = 0xFF;                              // same datarate and payload
      memcpy(&cmd.payload[1], &cfList[chMaskCntl*2], 2);  // copy mask
      cmd.payload[3] = chMaskCntl << 4;                   // set chMaskCntl, set NbTrans = 0 -> keep the same
      cmd.repeat = (chMaskCntl + 1);
      (void)execMacCommand(&cmd);
    }
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::selectChannels() {
  // figure out which channel IDs are enabled (chMask may have disabled some) and are valid for the current datarate
  uint8_t numChannels = 0;
  uint8_t channelsEnabled[RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS];
  for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
    if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled) {
      if(this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] >= this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMin
         && this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] <= this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMax) {
          channelsEnabled[numChannels] = i;
          numChannels++;
      }
    }
  }
  if(numChannels == 0) {
    return(RADIOLIB_ERR_NO_CHANNEL_AVAILABLE);
  }
  // select a random ID & channel from the list of enabled and possible channels
  uint8_t channelID = channelsEnabled[this->phyLayer->random(numChannels)];
  this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][channelID];
  
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
    // for dynamic bands, the downlink channel is the one matched to the uplink channel
    this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][channelID];
  
  } else {                // RADIOLIB_LORAWAN_BAND_FIXED
    // for fixed bands, the downlink channel is the uplink channel ID `modulo` number of downlink channels
    LoRaWANChannel_t channelDn;
    channelDn.enabled = true;
    channelDn.idx = this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK].idx % this->band->rx1Span.numChannels;
    channelDn.freq = this->band->rx1Span.freqStart + channelDn.idx*this->band->rx1Span.freqStep;
    channelDn.drMin = this->band->rx1Span.drMin;
    channelDn.drMax = this->band->rx1Span.drMax;
    this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = channelDn;

  }
  uint8_t drDown = getDownlinkDataRate(this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK], this->rx1DrOffset, this->band->rx1DataRateBase, 
                                       this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].drMin, this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].drMax);
  this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = drDown;

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::setDatarate(uint8_t drUp) {
  // scan through all enabled channels and check if the requested datarate is available
  bool isValidDR = false;
  for(size_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
    LoRaWANChannel_t *chnl = &(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i]);
    if(chnl->enabled) {
      if(drUp >= chnl->drMin && drUp <= chnl->drMax) {
        isValidDR = true;
        break;
      }
    }
  }
  if(!isValidDR) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("No defined channel allows datarate %d", drUp);
    return(RADIOLIB_ERR_INVALID_DATA_RATE);
  }

  LoRaWANMacCommand_t cmd = {
    .cid = RADIOLIB_LORAWAN_MAC_LINK_ADR,
    .payload = { 0 }, 
    .len = MacTable[RADIOLIB_LORAWAN_MAC_LINK_ADR].lenDn,
    .repeat = 0,
  };
  cmd.payload[0]  = (drUp << 4);
  cmd.payload[0] |= 0x0F;               // keep Tx Power the same
  cmd.payload[3]  = (1 << 7);           // set the RFU bit, which means that the channel mask gets ignored
  cmd.payload[3] |= 0;                  // keep NbTrans the same
  (void)execMacCommand(&cmd);

  // check if ACK is set for Tx Power
  if((cmd.payload[0] >> 1) != 1) {
    return(RADIOLIB_ERR_INVALID_DATA_RATE);
  }
  
  return(RADIOLIB_ERR_NONE);
}

void LoRaWANNode::setADR(bool enable) {
  this->adrEnabled = enable;
}

void LoRaWANNode::setDutyCycle(bool enable, RadioLibTime_t msPerHour) {
  this->dutyCycleEnabled = enable;
  if(!enable) {
    this->dutyCycle = 0;
  }
  if(msPerHour == 0) {
    this->dutyCycle = this->band->dutyCycle;
  } else {
    this->dutyCycle = msPerHour;
  }
}

// given an airtime in milliseconds, calculate the minimum uplink interval
// to adhere to a given dutyCycle
RadioLibTime_t LoRaWANNode::dutyCycleInterval(RadioLibTime_t msPerHour, RadioLibTime_t airtime) {
  if(msPerHour == 0 || airtime == 0) {
    return(0);
  }
  RadioLibTime_t oneHourInMs = (RadioLibTime_t)60 * (RadioLibTime_t)60 * (RadioLibTime_t)1000;
  float numPackets = msPerHour / airtime;
  RadioLibTime_t delayMs = oneHourInMs / numPackets + 1;  // + 1 to prevent rounding problems
  return(delayMs);
}

RadioLibTime_t LoRaWANNode::timeUntilUplink() {
  Module* mod = this->phyLayer->getMod();
  RadioLibTime_t nextUplink = this->rxDelayStart + dutyCycleInterval(this->dutyCycle, this->lastToA);
  if(mod->hal->millis() > nextUplink){
    return(0);
  }
  return(nextUplink - mod->hal->millis() + 1);
}

void LoRaWANNode::setDwellTime(bool enable, RadioLibTime_t msPerUplink) {
  this->dwellTimeEnabledUp = enable;
  if(msPerUplink == 0) {
    this->dwellTimeUp = this->band->dwellTimeUp;
  } else {
    this->dwellTimeUp = msPerUplink;
  }
}

uint8_t LoRaWANNode::maxPayloadDwellTime() {
  // configure current datarate
  DataRate_t dr;
  // TODO this may fail horribly?
  (void)findDataRate(this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK], &dr);
  (void)this->phyLayer->setDataRate(dr);
  uint8_t minPayLen = 0;
  uint8_t maxPayLen = 255;
  uint8_t payLen = (minPayLen + maxPayLen) / 2;
  // do some binary search to find maximum allowed payload length
  while(payLen != minPayLen && payLen != maxPayLen) {
    if(this->phyLayer->getTimeOnAir(payLen) > this->dwellTimeUp) {
      maxPayLen = payLen;
    } else {
      minPayLen = payLen;
    }
    payLen = (minPayLen + maxPayLen) / 2;
  }
  return(payLen - 13);  // fixed 13-byte header
}

int16_t LoRaWANNode::setTxPower(int8_t txPower) {
  // only allow values within the band's (or MAC state) maximum
  if(txPower > this->txPowerMax) {
    return(RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  }
  // Tx Power is set in steps of two
  // the selected value is rounded down to nearest multiple of two away from txPowerMax
  // e.g. on EU868, max is 16; if 13 is selected then we set to 12
  uint8_t numSteps = (this->txPowerMax - txPower + 1) / (-RADIOLIB_LORAWAN_POWER_STEP_SIZE_DBM);

  LoRaWANMacCommand_t cmd = {
    .cid = RADIOLIB_LORAWAN_MAC_LINK_ADR,
    .payload = { 0 }, 
    .len = MacTable[RADIOLIB_LORAWAN_MAC_LINK_ADR].lenDn,
    .repeat = 0,
  };
  cmd.payload[0]  = 0xF0;               // keep datarate the same
  cmd.payload[0] |= numSteps;           // set the Tx Power
  cmd.payload[3]  = (1 << 7);           // set the RFU bit, which means that the channel mask gets ignored
  cmd.payload[3] |= 0;                  // keep NbTrans the same
  (void)execMacCommand(&cmd);

  // check if ACK is set for Tx Power
  if((cmd.payload[0] >> 2) != 1) {
    return(RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  }
  
  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::findDataRate(uint8_t dr, DataRate_t* dataRate) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;

  uint8_t dataRateBand = this->band->dataRates[dr];

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

  state = this->phyLayer->checkDataRate(*dataRate);

  return(state);
}

int16_t LoRaWANNode::setRx2Dr(uint8_t dr) {
  // this can only be configured in ABP mode
  if(this->lwMode != RADIOLIB_LORAWAN_MODE_ABP) {
    return(RADIOLIB_LORAWAN_INVALID_MODE);
  }

  // can only configure different datarate for dynamic bands
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_FIXED) {
    return(RADIOLIB_ERR_NO_CHANNEL_AVAILABLE);
  }

  // check if datarate is available in the selected band
  if(this->band->dataRates[dr] == RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
    return(RADIOLIB_ERR_INVALID_DATA_RATE);
  }
  
  // find and check if the datarate is available for this radio module
  DataRate_t dataRate;
  int16_t state = findDataRate(dr, &dataRate);
  RADIOLIB_ASSERT(state);

  // passed all checks, so configure the datarate
  this->rx2.drMax = dr;

  return(state);
}

int16_t LoRaWANNode::sendMacCommandReq(uint8_t cid) {
  bool valid = false;
  for(size_t i = 0; i < RADIOLIB_LORAWAN_NUM_MAC_COMMANDS; i++) {
    if(MacTable[i].cid == cid) {
      valid = MacTable[i].user;
    }
  }
  if(!valid) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("You are not allowed to request this MAC command");
    return(RADIOLIB_ERR_INVALID_CID);
  }

  // if there are already 15 MAC bytes in the uplink queue, we can't add a new one
  if(this->commandsUp.len + 1 > RADIOLIB_LORAWAN_FHDR_FOPTS_MAX_LEN) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("The maximum number of FOpts payload was reached");
    return(RADIOLIB_ERR_COMMAND_QUEUE_FULL);
  }
  if(this->commandsUp.numCommands > RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("The RadioLib internal MAC command queue was full");
    return(RADIOLIB_ERR_COMMAND_QUEUE_FULL);
  }

  // delete any prior requests for this MAC command, in case this is requested more than once
  (void)deleteMacCommand(cid, &this->commandsUp);
  
  LoRaWANMacCommand_t cmd = {
    .cid = cid,
    .payload = { 0 }, 
    .len = 0,
    .repeat = 0,
  };
  pushMacCommand(&cmd, &this->commandsUp);
  return(true);
}

int16_t LoRaWANNode::pushMacCommand(LoRaWANMacCommand_t* cmd, LoRaWANMacCommandQueue_t* queue) {
  if(queue->numCommands >= RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE) {
    return(RADIOLIB_ERR_COMMAND_QUEUE_FULL);
  }

  memcpy(&queue->commands[queue->numCommands], cmd, sizeof(LoRaWANMacCommand_t));
  queue->numCommands++;
  queue->len += 1 + cmd->len; // 1 byte for command ID, len bytes for payload

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::deleteMacCommand(uint8_t cid, LoRaWANMacCommandQueue_t* queue, uint8_t* payload) {
  for(size_t index = 0; index < queue->numCommands; index++) {
    if(queue->commands[index].cid == cid) {
      // if a pointer to a payload is supplied, copy the command's payload over
      if(payload) {
        memcpy(payload, queue->commands[index].payload, queue->commands[index].len);
      }
      queue->len -= (1 + queue->commands[index].len); // 1 byte for command ID, len for payload
      // move all subsequent commands one forward in the queue
      if(index < RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE - 1) {
        memmove(&queue->commands[index], &queue->commands[index + 1], (RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE - index - 1) * sizeof(LoRaWANMacCommand_t));
      }
      // set the latest element to all 0
      memset(&queue->commands[RADIOLIB_LORAWAN_MAC_COMMAND_QUEUE_SIZE - 1], 0x00, sizeof(LoRaWANMacCommand_t));
      queue->numCommands--;
      return(RADIOLIB_ERR_NONE);
    }
  }

  return(RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND);
}

bool LoRaWANNode::execMacCommand(LoRaWANMacCommand_t* cmd) {
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("[MAC] 0x%02X", cmd->cid);
  RADIOLIB_DEBUG_PROTOCOL_HEXDUMP(cmd->payload, cmd->len);

  if(cmd->cid >= RADIOLIB_LORAWAN_MAC_PROPRIETARY) {
    // TODO call user-provided callback for proprietary MAC commands?
    return(false);
  }

  switch(cmd->cid) {
    case(RADIOLIB_LORAWAN_MAC_RESET): {
      // get the server version
      uint8_t srvVersion = cmd->payload[0];
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ResetConf: server version 1.%d", srvVersion);
      if(srvVersion == this->rev) {
        // valid server version, stop sending the ResetInd MAC command
        deleteMacCommand(RADIOLIB_LORAWAN_MAC_RESET, &this->commandsUp);
      }
      return(false);
    } break;

    case(RADIOLIB_LORAWAN_MAC_LINK_CHECK): {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("LinkCheckAns: [user]");
      // delete any existing response (does nothing if there is none)
      deleteMacCommand(RADIOLIB_LORAWAN_MAC_LINK_CHECK, &this->commandsDown);

      // insert response into MAC downlink queue
      pushMacCommand(cmd, &this->commandsDown);
      return(false);
    } break;

    case(RADIOLIB_LORAWAN_MAC_LINK_ADR): {
      // get the ADR configuration
      uint8_t drUp = (cmd->payload[0] & 0xF0) >> 4;
      uint8_t txSteps = cmd->payload[0] & 0x0F;
      bool isInternalTxDr = cmd->payload[3] >> 7;

      uint16_t chMask = LoRaWANNode::ntoh<uint16_t>(&cmd->payload[1]);
      uint8_t chMaskCntl = (cmd->payload[3] & 0x70) >> 4;
      uint8_t nbTransMac = cmd->payload[3] & 0x0F;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("LinkADRReq: dataRate = %d, txSteps = %d, chMask = 0x%04x, chMaskCntl = %d, nbTrans = %d", drUp, txSteps, chMask, chMaskCntl, nbTransMac);

      // try to apply the datarate configuration
      int16_t state;
      uint8_t drAck = 0;
      if(drUp == 0x0F) {  // keep the same
        drAck = 1;

      } else if (this->band->dataRates[drUp] != RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
        // check if the module supports this data rate
        DataRate_t dr;
        state = findDataRate(drUp, &dr);
        if(state == RADIOLIB_ERR_NONE) {
          uint8_t drDown = getDownlinkDataRate(drUp, this->rx1DrOffset, this->band->rx1DataRateBase,
                                               this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].drMin,
                                               this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].drMax);
          this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = drUp;
          this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = drDown;
          drAck = 1;
        } else {
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ADR failed to configure dataRate %d, code %d!", drUp, state);
          drUp = 0x0F;  // set value to 'keep the same'
        }
      
      }

      // try to apply the power configuration
      uint8_t pwrAck = 0;
      if(txSteps == 0x0F) {
        pwrAck = 1;

      } else {
        int8_t power = this->txPowerMax - 2*txSteps;
        int8_t powerActual = 0;
        state = this->phyLayer->checkOutputPower(power, &powerActual);
        // only acknowledge if the radio is able to operate at or below the requested power level
        if(state == RADIOLIB_ERR_NONE || (state == RADIOLIB_ERR_INVALID_OUTPUT_POWER && powerActual < power)) {
          pwrAck = 1;
          this->txPowerSteps = txSteps;
        } else {
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ADR failed to configure Tx power %d, code %d!", power, state);
          txSteps = 0x0F;  // set value to 'keep the same'
        }
      }

      uint8_t chMaskAck = 1;
      // only apply channel mask when the RFU bit is not set
      // (which is only set in internal MAC commands for changing Tx/Dr)
      if(!isInternalTxDr) {
        if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
          chMaskAck = (uint8_t)this->applyChannelMaskDyn(chMaskCntl, chMask);

        } else {                // RADIOLIB_LORAWAN_BAND_FIXED
          if(cmd->repeat == 1) {
            // if this is the first ADR command in the queue, clear all saved channels
            // so we can apply the new channel mask
            RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ADR mask: clearing channels");
            for(size_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i] = RADIOLIB_LORAWAN_CHANNEL_NONE;
            }
            // clear all previous channel masks
            memset(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_UL_CHANNELS], 0, 16*8);
          } else {
            // if this is not the first ADR command, clear the ADR response that was in the queue
            (void)deleteMacCommand(RADIOLIB_LORAWAN_MAC_LINK_ADR, &this->commandsUp);
          }
          chMaskAck = (uint8_t)this->applyChannelMaskFix(chMaskCntl, chMask);

        }
      }

      if(nbTransMac) {  // if there is a value for NbTrans, set this value
        this->nbTrans = nbTransMac;
      }

      // replace 'placeholder' or failed values with the current values for saving
      // per spec, all these configuration should only be set if all ACKs are set, otherwise retain previous state
      // but we don't bother and try to set each individual command
      if(drUp == 0x0F || !drAck) {
        cmd->payload[0] = (cmd->payload[0] & 0x0F) | (this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] << 4);
      }
      if(txSteps == 0x0F || !pwrAck) {
        cmd->payload[0] = (cmd->payload[0] & 0xF0) | this->txPowerSteps;
      }
      if(nbTransMac == 0) {
        cmd->payload[3] = (cmd->payload[3] & 0xF0) | this->nbTrans;
      }

      if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
        // if RFU bit is set, this is just a change in Datarate or TxPower, so read ADR command and overwrite first byte
        if(isInternalTxDr) {
          memcpy(&(cmd->payload[1]), &this->bufferSession[RADIOLIB_LORAWAN_SESSION_LINK_ADR] + 1, 3);
        }
        
        // if there was no channel mask (all zeroes), we should never apply that channel mask, so set RFU bit again
        if(cmd->payload[1] == 0 && cmd->payload[2] == 0) {
          cmd->payload[3] |= (1 << 7);
        }

        // save to the single ADR MAC location
        memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_LINK_ADR], &(cmd->payload[0]), cmd->len);
      
      } else {                // RADIOLIB_LORAWAN_BAND_FIXED

        // save Tx/Dr to the Link ADR position in the session buffer 
        uint8_t bufTxDr[RADIOLIB_LORAWAN_MAX_MAC_COMMAND_LEN_DOWN] = { 0 };
        bufTxDr[0] = cmd->payload[0];
        bufTxDr[3] = 1 << 7;
        memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_LINK_ADR], bufTxDr, cmd->len);
        
        // if RFU bit is set, this is just a change in Datarate or TxPower, in which case we don't save the channel masks
        // if the RFU bit is not set, we must save this channel mask
        if(!isInternalTxDr) {
          // save the channel mask to the uplink channels position in session buffer, with Tx and DR set to 'same'
          cmd->payload[0] = 0xFF;
          memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_UL_CHANNELS] + (cmd->repeat - 1) * cmd->len, cmd->payload, cmd->len);
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Saving mask to ULChannels[%d]:", (cmd->repeat - 1) * cmd->len);
          RADIOLIB_DEBUG_PROTOCOL_HEXDUMP(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_UL_CHANNELS] + (cmd->repeat - 1) * cmd->len, cmd->len);
        }

      }

      // send the reply
      cmd->len = 1;
      cmd->payload[0] = (pwrAck << 2) | (drAck << 1) | (chMaskAck << 0);
      cmd->repeat = 0;  // discard any repeat value that may have been set
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("LinkADRAns: status = 0x%02x", cmd->payload[0]);
      return(true);
    } break;

    case(RADIOLIB_LORAWAN_MAC_DUTY_CYCLE): {
      uint8_t maxDutyCycle = cmd->payload[0] & 0x0F;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("DutyCycleReq: max duty cycle = 1/2^%d", maxDutyCycle);
      if(maxDutyCycle == 0) {
        this->dutyCycle = this->band->dutyCycle;
      } else {
        this->dutyCycle = (RadioLibTime_t)60 * (RadioLibTime_t)60 * (RadioLibTime_t)1000 / (RadioLibTime_t)(1UL << maxDutyCycle);
      }

      memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_DUTY_CYCLE], cmd->payload, cmd->len);

      cmd->len = 0;
      return(true);
    } break;

    case(RADIOLIB_LORAWAN_MAC_RX_PARAM_SETUP): {
      // get the configuration
      this->rx1DrOffset = (cmd->payload[0] & 0x70) >> 4;
      uint8_t rx1OffsAck = 1;
      this->rx2.drMax = cmd->payload[0] & 0x0F;
      uint8_t rx2Ack = 1;
      uint32_t freqRaw = LoRaWANNode::ntoh<uint32_t>(&cmd->payload[1], 3);
      this->rx2.freq = (float)freqRaw/10000.0;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("RXParamSetupReq: rx1DrOffset = %d, rx2DataRate = %d, freq = %f", this->rx1DrOffset, this->rx2.drMax, this->rx2.freq);
      
      // apply the configuration
      uint8_t chanAck = 0;
      if(this->phyLayer->setFrequency(this->rx2.freq) == RADIOLIB_ERR_NONE) {
        chanAck = 1;
        this->phyLayer->setFrequency(this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].freq);
      }

      memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_RX_PARAM_SETUP], cmd->payload, cmd->len);

      // TODO this should be sent repeatedly until the next downlink
      cmd->len = 1;
      cmd->payload[0] = (rx1OffsAck << 2) | (rx2Ack << 1) | (chanAck << 0);
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("RXParamSetupAns: status = 0x%02x", cmd->payload[0]);
      return(true);
    } break;

    case(RADIOLIB_LORAWAN_MAC_DEV_STATUS): {
      // set the uplink reply
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("DevStatusReq");
      cmd->len = 2;
      cmd->payload[0] = this->battLevel;
      int8_t snr = this->phyLayer->getSNR();
      cmd->payload[1] = snr & 0x3F;

      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("DevStatusAns: status = 0x%02x%02x", cmd->payload[0], cmd->payload[1]);
      return(true);
    } break;

    case(RADIOLIB_LORAWAN_MAC_NEW_CHANNEL): {
      // get the configuration
      uint8_t chIndex = cmd->payload[0];
      uint32_t freqRaw = LoRaWANNode::ntoh<uint32_t>(&cmd->payload[1], 3);
      float freq = (float)freqRaw/10000.0;
      uint8_t maxDr = (cmd->payload[4] & 0xF0) >> 4;
      uint8_t minDr = cmd->payload[4] & 0x0F;
      
      uint8_t newChAck = 0;
      uint8_t freqAck = 0;

      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chIndex].enabled = true;
      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chIndex].idx   = chIndex;
      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chIndex].freq  = freq;
      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chIndex].drMin = minDr;
      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chIndex].drMax = maxDr;
      
      // downlink channel is identical to uplink channel
      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][chIndex] = this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chIndex];
      newChAck = 1;
      
      // check if the frequency is possible
      if(this->phyLayer->setFrequency(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chIndex].freq) == RADIOLIB_ERR_NONE) {
        freqAck = 1;
        this->phyLayer->setFrequency(this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].freq);
      }
      
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("NewChannelReq:");
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("UL: %3d %d %7.3f (%d - %d) | DL: %3d %d %7.3f (%d - %d)", 
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chIndex].idx,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chIndex].enabled,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chIndex].freq,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chIndex].drMin,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chIndex].drMax,

                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][chIndex].idx,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][chIndex].enabled,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][chIndex].freq,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][chIndex].drMin,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][chIndex].drMax
                            );

      memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_UL_CHANNELS] + chIndex * cmd->len, cmd->payload, cmd->len);

      // send the reply
      cmd->len = 1;
      cmd->payload[0] = (newChAck << 1) | (freqAck << 0);
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("NewChannelAns: status = 0x%02x", cmd->payload[0]);

      return(true);
    } break;

    case(RADIOLIB_LORAWAN_MAC_DL_CHANNEL): {
      // get the configuration
      uint8_t chIndex = cmd->payload[0];
      uint32_t freqRaw = LoRaWANNode::ntoh<uint32_t>(&cmd->payload[1], 3);
      float freq = (float)freqRaw/10000.0;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("DlChannelReq: index = %d, freq = %f MHz", chIndex, freq);
      uint8_t freqDlAck = 0;
      uint8_t freqUlAck = 0;
      
      // check if the frequency is possible
      if(this->phyLayer->setFrequency(freq) == RADIOLIB_ERR_NONE) {
        freqDlAck = 1;
        this->phyLayer->setFrequency(this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].freq);
      }
      
      // update the downlink frequency
      for(int i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
        if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].idx == chIndex) {
          this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].freq = freq;
          // check if the corresponding uplink frequency is actually set
          if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].freq > 0) {
            freqUlAck = 1;
          }
        }
      }
      
      memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_DL_CHANNELS] + chIndex * cmd->len, cmd->payload, cmd->len);

      // TODO send this repeatedly until a downlink is received
      cmd->len = 1;
      cmd->payload[0] = (freqUlAck << 1) | (freqDlAck << 0);
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("DlChannelAns: status = 0x%02x", cmd->payload[0]);

      return(true);
    } break;

    case(RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP): {
      // get the configuration
      uint8_t delay = cmd->payload[0] & 0x0F;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("RXTimingSetupReq: delay = %d sec", delay);
      
      // apply the configuration
      if(delay == 0) {
        delay = 1;
      }
      this->rxDelays[0] = delay * 1000;
      this->rxDelays[1] = this->rxDelays[0] + 1000;

      memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_RX_TIMING_SETUP], cmd->payload, cmd->len);

      // send the reply
      cmd->len = 0;

      // TODO send this repeatedly until a downlink is received
      return(true);
    } break;

    case(RADIOLIB_LORAWAN_MAC_TX_PARAM_SETUP): {
      uint8_t dlDwell = (cmd->payload[0] & 0x20) >> 5;
      uint8_t ulDwell = (cmd->payload[0] & 0x10) >> 4;
      uint8_t maxEirpRaw = cmd->payload[0] & 0x0F;

      // who the f came up with this ...
      const uint8_t eirpEncoding[] = { 8, 10, 12, 13, 14, 16, 18, 20, 21, 24, 26, 27, 29, 30, 33, 36 };
      this->txPowerMax = eirpEncoding[maxEirpRaw];
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TxParamSetupReq: dlDwell = %d, ulDwell = %d, maxEirp = %d dBm", dlDwell, ulDwell, eirpEncoding[maxEirpRaw]);

      this->dwellTimeEnabledUp = ulDwell ? true : false;
      this->dwellTimeUp = ulDwell ? RADIOLIB_LORAWAN_DWELL_TIME : 0;

      this->dwellTimeEnabledDn = dlDwell ? true : false;
      this->dwellTimeDn = dlDwell ? RADIOLIB_LORAWAN_DWELL_TIME : 0;

      memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_TX_PARAM_SETUP], cmd->payload, cmd->len);

      cmd->len = 0;
      return(true);
    } break;

    case(RADIOLIB_LORAWAN_MAC_REKEY): {
      // get the server version
      uint8_t srvVersion = cmd->payload[0];
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("RekeyConf: server version = 1.%d", srvVersion);
      if((srvVersion > 0) && (srvVersion <= this->rev)) {
        // valid server version, stop sending the ReKey MAC command
        deleteMacCommand(RADIOLIB_LORAWAN_MAC_REKEY, &this->commandsUp);
      }
      return(false);
    } break;

    case(RADIOLIB_LORAWAN_MAC_ADR_PARAM_SETUP): {
      this->adrLimitExp = (cmd->payload[0] & 0xF0) >> 4;
      this->adrDelayExp = cmd->payload[0] & 0x0F;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ADRParamSetupReq: limitExp = %d, delayExp = %d", this->adrLimitExp, this->adrDelayExp);

      memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_ADR_PARAM_SETUP], cmd->payload, cmd->len);

      cmd->len = 0;
      return(true);
    } break;

    case(RADIOLIB_LORAWAN_MAC_DEVICE_TIME): {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("DeviceTimeAns: [user]");
      // delete any existing response (does nothing if there is none)
      deleteMacCommand(RADIOLIB_LORAWAN_MAC_DEVICE_TIME, &this->commandsDown);

      // insert response into MAC downlink queue
      pushMacCommand(cmd, &this->commandsDown);
      return(false);
    } break;

    case(RADIOLIB_LORAWAN_MAC_FORCE_REJOIN): {
      // TODO implement this
      uint16_t rejoinReq = LoRaWANNode::ntoh<uint16_t>(cmd->payload);
      uint8_t period = (rejoinReq & 0x3800) >> 11;
      uint8_t maxRetries = (rejoinReq & 0x0700) >> 8;
      uint8_t rejoinType = (rejoinReq & 0x0070) >> 4;
      uint8_t dr = rejoinReq & 0x000F;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ForceRejoinReq: period = %d, maxRetries = %d, rejoinType = %d, dr = %d", period, maxRetries, rejoinType, dr);
      (void)period;
      (void)maxRetries;
      (void)rejoinType;
      (void)dr;
      return(false);
    } break;

    case(RADIOLIB_LORAWAN_MAC_REJOIN_PARAM_SETUP): {
      // TODO implement this
      uint8_t maxTime = (cmd->payload[0] & 0xF0) >> 4;
      uint8_t maxCount = cmd->payload[0] & 0x0F;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("RejoinParamSetupReq: maxTime = %d, maxCount = %d", maxTime, maxCount);

      memcpy(&this->bufferSession[RADIOLIB_LORAWAN_SESSION_REJOIN_PARAM_SETUP], cmd->payload, cmd->len);

      cmd->len = 0;
      cmd->payload[0] = (1 << 1) | 1;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("RejoinParamSetupAns: status = 0x%02x", cmd->payload[0]);

      (void)maxTime;
      (void)maxCount;
      return(true);
    } break;
  }

  return(false);
}

bool LoRaWANNode::applyChannelMaskDyn(uint8_t chMaskCntl, uint16_t chMask) {
  for(size_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
    if(chMaskCntl == 0) {
      // apply the mask by looking at each channel bit
      if(chMask & (1UL << i)) {
        // if it should be enabled but is not currently defined, stop immediately
        if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx == RADIOLIB_LORAWAN_CHANNEL_INDEX_NONE) {
          return(false);
        }
        this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled = true;
      } else {
        this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled = false;
      }

    } else if(chMaskCntl == 6) {
      // enable all defined channels
      if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx != RADIOLIB_LORAWAN_CHANNEL_INDEX_NONE) {
        this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled = true;
      }
    }
    
  }

  for (int i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
    if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled) {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("UL: %3d %d %7.3f (%d - %d) | DL: %3d %d %7.3f (%d - %d)",
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].freq,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMin,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMax,

                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].idx,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].enabled,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].freq,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].drMin,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].drMax
                            );
    }
  }

  return(true);
}

bool LoRaWANNode::applyChannelMaskFix(uint8_t chMaskCntl, uint16_t chMask) {
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("mask[%d] = 0x%04x", chMaskCntl, chMask);

  // find out how many channels have already been configured
  uint8_t idx = 0;
  for(size_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
    if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].freq > 0) {
      idx++;
    }
  }
  
  if((this->band->numTxSpans == 1 && chMaskCntl <= 5) || (this->band->numTxSpans == 2 && chMaskCntl <= 3)) {
    // select channels from first span
    LoRaWANChannel_t chnl;
    for(uint8_t i = 0; i < 16; i++) {
      uint16_t mask = 1 << i;
      if(mask & chMask) {
        uint8_t chNum = chMaskCntl * 16 + i;  // 0 through 63 or 95
        this->subBand = chNum / 8 + 1;        // save configured subband in case we must reset the channels (1-based)
        chnl.enabled = true;
        chnl.idx   = chNum;
        chnl.freq  = this->band->txSpans[0].freqStart + chNum*this->band->txSpans[0].freqStep;
        chnl.drMin = this->band->txSpans[0].drMin;
        chnl.drMax = this->band->txSpans[0].drMax;
        this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][idx++] = chnl;
      }
    }

  }
  if(this->band->numTxSpans == 1 && chMaskCntl == 6) {
    // all channels on (but we revert to user-selected subband)
    this->setupChannelsFix(this->subBand);

  }
  if(this->band->numTxSpans == 2 && chMaskCntl == 4) {
    // select channels from second span
    LoRaWANChannel_t chnl;
    for(uint8_t i = 0; i < 8; i++) {
      uint16_t mask = 1 << i;
      if(mask & chMask) {
        uint8_t chNum = chMaskCntl * 16 + i;  // 64 through 71
        chnl.enabled = true;
        chnl.idx   = chNum;
        chnl.freq  = this->band->txSpans[1].freqStart + i*this->band->txSpans[1].freqStep;
        chnl.drMin = this->band->txSpans[1].drMin;
        chnl.drMax = this->band->txSpans[1].drMax;
        this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][idx++] = chnl;
      }
    }

  }
  if(this->band->numTxSpans == 2 && chMaskCntl == 5) {
    // a '1' enables a bank of 8 + 1 channels from 1st and 2nd span respectively
    LoRaWANChannel_t chnl;
    for(uint8_t i = 0; i < 8; i++) {
      uint16_t mask = 1 << i;
      if(mask & chMask) {
        // enable bank of 8 channels from first span
        for(uint8_t j = 0; j < 8; j++) {
          uint8_t chNum = i * 8 + j;
          chnl.enabled = true;
          chnl.idx   = chNum;
          chnl.freq  = this->band->txSpans[0].freqStart + chNum*this->band->txSpans[0].freqStep;
          chnl.drMin = this->band->txSpans[0].drMin;
          chnl.drMax = this->band->txSpans[0].drMax;
          this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][idx++] = chnl;
        }
        // enable single channel from second span
        uint8_t chNum = 64 + i;
        chnl.enabled = true;
        chnl.idx   = chNum;
        chnl.freq  = this->band->txSpans[1].freqStart + i*this->band->txSpans[1].freqStep;
        chnl.drMin = this->band->txSpans[1].drMin;
        chnl.drMax = this->band->txSpans[1].drMax;
        this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][idx++] = chnl;
      }
    }

  }
  if(this->band->numTxSpans == 2 && chMaskCntl == 6) {
    // all channels on (but we revert to selected subband)
    this->setupChannelsFix(this->subBand);

    // a '1' enables a single channel from second span
    LoRaWANChannel_t chnl;
    for(uint8_t i = 0; i < 8; i++) {
      uint16_t mask = 1 << i;
      if(mask & chMask) {
        // enable single channel from second span
        uint8_t chNum = 64 + i;
        chnl.enabled = true;
        chnl.idx   = chNum;
        chnl.freq  = this->band->txSpans[1].freqStart + i*this->band->txSpans[1].freqStep;
        chnl.drMin = this->band->txSpans[1].drMin;
        chnl.drMax = this->band->txSpans[1].drMax;
        this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][idx++] = chnl;
      }
    }

  }
  if(this->band->numTxSpans == 2 && chMaskCntl == 7) {
    // all channels off (clear all channels)
    LoRaWANChannel_t chnl = RADIOLIB_LORAWAN_CHANNEL_NONE;
    for(int i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i] = chnl;
      // downlink channels are not defined so don't need to reset
    }
    idx = 0;
    // a '1' enables a single channel from second span
    for(uint8_t i = 0; i < 8; i++) {
      uint16_t mask = 1 << i;
      if(mask & chMask) {
        // enable single channel from second span
        uint8_t chNum = 64 + i;
        chnl.enabled = true;
        chnl.idx   = chNum;
        chnl.freq  = this->band->txSpans[1].freqStart + i*this->band->txSpans[1].freqStep;
        chnl.drMin = this->band->txSpans[1].drMin;
        chnl.drMax = this->band->txSpans[1].drMax;
        this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][idx++] = chnl;
      }
    }

  }

  for (int i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
    if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled) {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("UL: %3d %d %7.3f (%d - %d) | DL: %3d %d %7.3f (%d - %d)",
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].freq,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMin,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMax,

                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].idx,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].enabled,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].freq,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].drMin,
                              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].drMax
                            );
    }
  }

  return(true);
}

uint8_t LoRaWANNode::getMacPayloadLength(uint8_t cid) {
  for (LoRaWANMacSpec_t entry : MacTable) {
    // cppcheck warns here we should use std::find_if, but some platforms may not have that
    if (entry.cid == cid) { // cppcheck-suppress useStlAlgorithm
      return entry.lenDn;
    }
  }
  // no idea about the length
  return 0;
}

int16_t LoRaWANNode::getMacLinkCheckAns(uint8_t* margin, uint8_t* gwCnt) {
  uint8_t payload[RADIOLIB_LORAWAN_MAX_MAC_COMMAND_LEN_DOWN] = { 0 };
  int16_t state = deleteMacCommand(RADIOLIB_LORAWAN_LINK_CHECK_REQ, &this->commandsDown, payload);
  RADIOLIB_ASSERT(state);

  if(margin) { *margin = payload[0]; }
  if(gwCnt)  { *gwCnt  = payload[1]; }

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::getMacDeviceTimeAns(uint32_t* gpsEpoch, uint8_t* fraction, bool returnUnix) {
  uint8_t payload[RADIOLIB_LORAWAN_MAX_MAC_COMMAND_LEN_DOWN] = { 0 };
  int16_t state = deleteMacCommand(RADIOLIB_LORAWAN_MAC_DEVICE_TIME, &this->commandsDown, payload);
  RADIOLIB_ASSERT(state);

  if(gpsEpoch) { 
    *gpsEpoch = LoRaWANNode::ntoh<uint32_t>(&payload[0]); 
    if(returnUnix) {
      uint32_t unixOffset = 315964800UL - 18UL; // 18 leap seconds since GPS epoch (Jan. 6th 1980)
      *gpsEpoch += unixOffset;
    }
  }
  if(fraction) { *fraction = payload[4]; }

  return(RADIOLIB_ERR_NONE);
}

uint64_t LoRaWANNode::getDevAddr() {
  return(this->devAddr);
}

RadioLibTime_t LoRaWANNode::getLastToA() {
  return(this->lastToA);
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
                RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Occupied channel during DIFS");
                channelFreeDuringDIFS = false;
                // Channel is occupied during DIFS, hop to another.
                this->selectChannels();
                break;
            }
        }
        // Start reducing BO counter if DIFS slot was free.
        if (channelFreeDuringDIFS) {
            // Continue decrementing BO with per each CAD reporting free channel.
            while (BO > 0) {
                if (performCAD()) {
                    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Occupied channel during BO");
                    // Channel is busy during CAD, hop to another and return to DIFS state again.
                    this->selectChannels();
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

void LoRaWANNode::processAES(const uint8_t* in, size_t len, uint8_t* key, uint8_t* out, uint32_t fCnt, uint8_t dir, uint8_t ctrId, bool counter) {
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
  LoRaWANNode::hton<uint32_t>(&encBlock[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], fCnt);

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

uint16_t LoRaWANNode::checkSum16(uint8_t *key, uint16_t keyLen) {
  uint16_t checkSum = 0;
  for(uint16_t i = 0; i < keyLen; i += 2) {
    checkSum ^= ((uint16_t)key[i] << 8) | key[i + 1];
  }
  if(keyLen % 2 == 1) {
    uint16_t val = ((uint16_t)key[keyLen - 1] << 8);
    checkSum ^= val;
  }
  return(checkSum);
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

#endif
