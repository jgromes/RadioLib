#include "LoRaWAN.h"
#include <string.h>

#if !RADIOLIB_EXCLUDE_LORAWAN

#if defined(RADIOLIB_EEPROM_UNSUPPORTED)
  #warning "Persistent storage not supported!"
#endif

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

LoRaWANNode::LoRaWANNode(PhysicalLayer* phy, const LoRaWANBand_t* band) {
  this->phyLayer = phy;
  this->band = band;
  this->rx2 = this->band->rx2;
  this->difsSlots = 2;
  this->backoffMax = 6;
  this->enableCSMA = false;
}

void LoRaWANNode::setCSMA(uint8_t backoffMax, uint8_t difsSlots, bool enableCSMA) {
    this->backoffMax = backoffMax;
    this->difsSlots = difsSlots;
    this->enableCSMA = enableCSMA;
}

#if !defined(RADIOLIB_EEPROM_UNSUPPORTED)
void LoRaWANNode::wipe() {
  Module* mod = this->phyLayer->getMod();
  mod->hal->wipePersistentStorage();
}

int16_t LoRaWANNode::restore() {
  // if already joined, ignore
  if(this->isJoinedFlag) {
    return(RADIOLIB_ERR_NONE);
  }

  Module* mod = this->phyLayer->getMod();

  uint8_t nvm_table_version = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TABLE_VERSION_ID);
  // if (RADIOLIB_PERSISTENT_PARAM_LORAWAN_TABLE_VERSION > nvm_table_version) {
  //  // set default values for variables that are new or something
  // }
  (void)nvm_table_version;

  // check the magic value
  if(mod->hal->getPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID) != RADIOLIB_LORAWAN_MAGIC) {
    #if RADIOLIB_DEBUG
      RADIOLIB_DEBUG_PRINTLN("magic id not set (no saved session)");
      RADIOLIB_DEBUG_PRINTLN("first 16 bytes of NVM:");
      uint8_t nvmBuff[16];
      mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(0), nvmBuff, 16);
      RADIOLIB_DEBUG_HEXDUMP(nvmBuff, 16);
    #endif
    // the magic value is not set, user will have to do perform the join procedure
    return(RADIOLIB_ERR_NETWORK_NOT_JOINED);
  }

  // pull all authentication keys from persistent storage
  this->devAddr = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_ADDR_ID);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_APP_S_KEY_ID), this->appSKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FNWK_SINT_KEY_ID), this->fNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_SNWK_SINT_KEY_ID), this->sNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NWK_SENC_KEY_ID), this->nwkSEncKey, RADIOLIB_AES128_BLOCK_SIZE);

  // get session parameters
  this->rev         = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_VERSION_ID);
  RADIOLIB_DEBUG_PRINTLN("LoRaWAN session: v1.%d", this->rev);
  this->devNonce    = mod->hal->getPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_NONCE_ID);
  this->joinNonce   = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_JOIN_NONCE_ID);
  
  // get MAC state
  uint8_t txDrRx2Dr = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXDR_RX2DR_ID);
  this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = (txDrRx2Dr >> 4) & 0x0F;
  
  this->txPwrCur = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXPWR_CUR_ID);
  
  uint8_t rx1DrOffDel = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX1_DROFF_DEL_ID);
  this->rx1DrOffset  =  (rx1DrOffDel >> 4) & 0x0F;
  this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] + this->band->rx1DataRateBase + this->rx1DrOffset;
  this->rxDelays[0]  = ((rx1DrOffDel >> 0) & 0x0F) * 1000;
  if(this->rxDelays[0] == 0) {
    this->rxDelays[0] = RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS;
  }
  this->rxDelays[1]  = this->rxDelays[0] + 1000;
  
  uint8_t rx2FreqBuf[3];
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX2FREQ_ID), rx2FreqBuf, 3);
  uint32_t rx2Freq   = LoRaWANNode::ntoh<uint32_t>(&rx2FreqBuf[0], 3);
  this->rx2.drMax    = (txDrRx2Dr >> 0) & 0x0F;
  this->rx2.freq     = (float)rx2Freq / 10000.0;
  
  uint8_t adrLimDel  = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_LIM_DEL_ID);
  this->adrLimitExp  = (adrLimDel >> 4) & 0x0F;
  this->adrDelayExp  = (adrLimDel >> 0) & 0x0F;
  
  this->nbTrans      = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NBTRANS_ID);

  this->aFcntDown    = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_A_FCNT_DOWN_ID);
  this->nFcntDown    = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_N_FCNT_DOWN_ID);
  this->confFcntUp   = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_CONF_FCNT_UP_ID);
  this->confFcntDown = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_CONF_FCNT_DOWN_ID);
  this->adrFcnt      = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_FCNT_ID);
  
  // fcntUp is stored in highly efficient wear-leveling system, so parse it as required
  this->restoreFcntUp();

  uint8_t queueBuff[sizeof(LoRaWANMacCommandQueue_t)] = { 0 };
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FOPTS_ID), queueBuff, sizeof(LoRaWANMacCommandQueue_t));
  memcpy(&this->commandsUp, queueBuff, sizeof(LoRaWANMacCommandQueue_t));
  RADIOLIB_DEBUG_PRINTLN("Number of MAC commands: %d", this->commandsUp.numCommands);
  
  int16_t state = this->restoreChannels();
  RADIOLIB_ASSERT(state);

  state = this->setPhyProperties();
  RADIOLIB_ASSERT(state);

  // full session is restored, so set joined flag
  this->isJoinedFlag = true;

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::restoreFcntUp() {
  Module* mod = this->phyLayer->getMod();

  uint8_t fcntBuffStart = mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID);
  uint8_t fcntBuffEnd = mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID + 1);
  uint8_t buffSize = fcntBuffEnd - fcntBuffStart;
  #if RADIOLIB_STATIC_ONLY
  uint8_t fcntBuff[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
  uint8_t* fcntBuff = new uint8_t[buffSize];
  #endif
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID), fcntBuff, buffSize);

  // copy the two most significant bytes from the first two bytes
  uint32_t bits_30_22 = (uint32_t)fcntBuff[0];
  uint32_t bits_22_14 = (uint32_t)fcntBuff[1];

  // the next 7 bits must be retrieved from the byte to which was written most recently
  // this is the last byte that has its state bit (most significant bit) set equal to its predecessor
  // we find the first byte that has its state bit different, and subtract one
  uint8_t idx = 2;
  uint8_t state = fcntBuff[idx] >> 7;
  for(; idx < 5; idx++) {
    if(fcntBuff[idx] >> 7 != state) {
      break;
    }
  }
  uint32_t bits_14_7 = (uint32_t)fcntBuff[idx-1] & 0x7F;

  // equally, the last 7 bits must be retrieved from the byte to which was written most recently
  // this is the last byte that has its state bit (most significant bit) set equal to its predecessor
  // we find the first byte that has its state bit different, and subtract one
  idx = 5;
  state = fcntBuff[idx] >> 7;
  for(; idx < buffSize; idx++) {
    if(fcntBuff[idx] >> 7 != state) {
      break;
    }
  }
  uint32_t bits_7_0 = (uint32_t)fcntBuff[idx-1] & 0x7F;
  #if !RADIOLIB_STATIC_ONLY
  delete[] fcntBuff;
  #endif

  this->fcntUp = (bits_30_22 << 22) | (bits_22_14 << 14) | (bits_14_7 << 7) | bits_7_0;
  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::restoreChannels() {
  const uint8_t bytesPerChannel = 5;
  const uint8_t numBytes = 2 * RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS * bytesPerChannel;
  uint8_t buffer[numBytes] = { 0 };
  Module* mod = this->phyLayer->getMod();
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FREQS_ID), buffer, numBytes);
  for(uint8_t dir = 0; dir < 2; dir++) {
    for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
      uint8_t chBuff[bytesPerChannel] = { 0 };
      memcpy(chBuff, &buffer[(dir * RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS * bytesPerChannel) + i * bytesPerChannel], bytesPerChannel);
      this->availableChannels[dir][i].enabled = (chBuff[0] & 0x80) >> 7;
      this->availableChannels[dir][i].idx = chBuff[0] & 0x7F;
      uint32_t freq = LoRaWANNode::ntoh<uint32_t>(&chBuff[1], 3);
      this->availableChannels[dir][i].freq = (float)freq/10000.0;
      this->availableChannels[dir][i].drMax = (chBuff[4] & 0xF0) >> 4;
      this->availableChannels[dir][i].drMin = (chBuff[4] & 0x0F) >> 0;
    }
  }
  return(RADIOLIB_ERR_NONE);
}
#endif

int16_t LoRaWANNode::beginOTAA(uint64_t joinEUI, uint64_t devEUI, uint8_t* nwkKey, uint8_t* appKey, uint8_t joinDr, bool force) {
  // check if we actually need to send the join request
  Module* mod = this->phyLayer->getMod();

#if !defined(RADIOLIB_EEPROM_UNSUPPORTED)
  if(!force && (mod->hal->getPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID) == RADIOLIB_LORAWAN_MAGIC)) {
    // the device has joined already, we can just pull the data from persistent storage
    RADIOLIB_DEBUG_PRINTLN("Found existing session; restoring...");
    return(this->restore());
  }
#else
  (void)force;
#endif

  // set the physical layer configuration
  this->txPwrCur = this->band->powerMax;
  int16_t state = this->setPhyProperties();
  RADIOLIB_ASSERT(state);

  // setup uplink/downlink frequencies and datarates
  state = this->selectChannelsJR(this->devNonce, joinDr);
  RADIOLIB_ASSERT(state);

  // configure for uplink with default configuration
  state = this->configureChannel(RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK);
  RADIOLIB_ASSERT(state);

  // increment devNonce as we are sending another join-request
  this->devNonce += 1;

  // build the join-request message
  uint8_t joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_LEN];
  
  // set the packet fields
  joinRequestMsg[0] = RADIOLIB_LORAWAN_MHDR_MTYPE_JOIN_REQUEST | RADIOLIB_LORAWAN_MHDR_MAJOR_R1;
  LoRaWANNode::hton<uint64_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_JOIN_EUI_POS], joinEUI);
  LoRaWANNode::hton<uint64_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_DEV_EUI_POS], devEUI);
  LoRaWANNode::hton<uint16_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_DEV_NONCE_POS], this->devNonce);

  // add the authentication code
  uint32_t mic = this->generateMIC(joinRequestMsg, RADIOLIB_LORAWAN_JOIN_REQUEST_LEN - sizeof(uint32_t), nwkKey);
  LoRaWANNode::hton<uint32_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_LEN - sizeof(uint32_t)], mic);

  // send it
  state = this->phyLayer->transmit(joinRequestMsg, RADIOLIB_LORAWAN_JOIN_REQUEST_LEN);
  this->rxDelayStart = mod->hal->millis();
  RADIOLIB_DEBUG_PRINTLN("Join-request sent <-- Rx Delay start");
  RADIOLIB_ASSERT(state);

  // configure Rx delay for join-accept message - these are re-configured once a valid join-request is received
  this->rxDelays[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_DELAY_1_MS;
  this->rxDelays[1] = RADIOLIB_LORAWAN_JOIN_ACCEPT_DELAY_2_MS;

  // handle Rx1 and Rx2 windows - returns RADIOLIB_ERR_NONE if a downlink is received
  state = downlinkCommon();
  RADIOLIB_ASSERT(state);

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

  // get current JoinNonce from downlink and previous JoinNonce from persistent storage
  uint32_t joinNonceNew = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_NONCE_POS], 3);

  RADIOLIB_DEBUG_PRINTLN("JoinNoncePrev: %d, JoinNonce: %d", this->joinNonce, joinNonceNew);
    // JoinNonce received must be greater than the last JoinNonce heard, else error
  if((this->joinNonce > 0) && (joinNonceNew <= this->joinNonce)) {
    return(RADIOLIB_ERR_JOIN_NONCE_INVALID);
  }
  this->joinNonce = joinNonceNew;

  // check LoRaWAN revision (the MIC verification depends on this)
  uint8_t dlSettings = joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_DL_SETTINGS_POS];
  this->rev = (dlSettings & RADIOLIB_LORAWAN_JOIN_ACCEPT_R_1_1) >> 7;
  RADIOLIB_DEBUG_PRINTLN("LoRaWAN revision: 1.%d", this->rev);
  this->rx1DrOffset = (dlSettings & 0x70) >> 4;
  this->rx2.drMax = dlSettings & 0x0F;

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
    LoRaWANNode::hton<uint16_t>(&micBuff[9], this->devNonce);
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
  
  // parse other contents
  this->homeNetId = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_HOME_NET_ID_POS], 3);
  this->devAddr = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_ADDR_POS]);

  // parse Rx1 delay (and subsequently Rx2)
  this->rxDelays[0] = joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_RX_DELAY_POS]*1000;
  if(this->rxDelays[0] == 0) {
    this->rxDelays[0] = RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS;
  }
  this->rxDelays[1] = this->rxDelays[0] + 1000;

  // process CFlist if present
  if(lenRx == RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN) {
    uint8_t cfList[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN] = { 0 };
    memcpy(&cfList[0], &joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_POS], RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN);
    this->setupChannels(cfList);
  } else {
    this->setupChannels(nullptr);
  }

  // prepare buffer for key derivation
  uint8_t keyDerivationBuff[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  LoRaWANNode::hton<uint32_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_NONCE_POS], joinNonce, 3);

  // check protocol version (1.0 vs 1.1)
  if(this->rev == 1) {
    // 1.1 version, derive the keys
    LoRaWANNode::hton<uint64_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_EUI_POS], joinEUI);
    LoRaWANNode::hton<uint16_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_NONCE_POS], this->devNonce);
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
      .payload = { this->rev },
      .len = sizeof(uint8_t),
      .repeat = 0x01 << RADIOLIB_LORAWAN_ADR_ACK_LIMIT_EXP,
    };
    state = pushMacCommand(&cmd, &this->commandsUp);
    RADIOLIB_ASSERT(state);
  
  } else {
    // 1.0 version, just derive the keys
    LoRaWANNode::hton<uint32_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_HOME_NET_ID_POS], this->homeNetId, 3);
    LoRaWANNode::hton<uint16_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_ADDR_POS], this->devNonce);
    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_APP_S_KEY;
    RadioLibAES128Instance.init(nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->appSKey);

    keyDerivationBuff[0] = RADIOLIB_LORAWAN_JOIN_ACCEPT_F_NWK_S_INT_KEY;
    RadioLibAES128Instance.init(nwkKey);
    RadioLibAES128Instance.encryptECB(keyDerivationBuff, RADIOLIB_AES128_BLOCK_SIZE, this->fNwkSIntKey);

    memcpy(this->sNwkSIntKey, this->fNwkSIntKey, RADIOLIB_AES128_KEY_SIZE);
    memcpy(this->nwkSEncKey, this->fNwkSIntKey, RADIOLIB_AES128_KEY_SIZE);
  
  }

  // reset all frame counters
  this->fcntUp = 0;
  this->aFcntDown = 0;
  this->nFcntDown = 0;
  this->confFcntUp = RADIOLIB_LORAWAN_FCNT_NONE;
  this->confFcntDown = RADIOLIB_LORAWAN_FCNT_NONE;
  this->adrFcnt = 0;

#if !defined(RADIOLIB_EEPROM_UNSUPPORTED)
  // save the device address & keys as well as JoinAccept values; these are only ever set when joining
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_ADDR_ID, this->devAddr);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_APP_S_KEY_ID), this->appSKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FNWK_SINT_KEY_ID), this->fNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_SNWK_SINT_KEY_ID), this->sNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NWK_SENC_KEY_ID), this->nwkSEncKey, RADIOLIB_AES128_BLOCK_SIZE);

  // save join-request parameters
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_HOME_NET_ID, this->homeNetId);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_JOIN_NONCE_ID, this->joinNonce);

  this->saveSession();
  this->saveChannels();

  // everything written to NVM, write current table version to persistent storage and set magic number
  mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TABLE_VERSION_ID, RADIOLIB_PERSISTENT_PARAM_LORAWAN_TABLE_VERSION);
  mod->hal->setPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID, RADIOLIB_LORAWAN_MAGIC);
#endif

  this->isJoinedFlag = true;

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::beginABP(uint32_t addr, uint8_t* nwkSKey, uint8_t* appSKey, uint8_t* fNwkSIntKey, uint8_t* sNwkSIntKey, bool force) {
  
#if !defined(RADIOLIB_EEPROM_UNSUPPORTED)
  // check if we actually need to restart from a clean session
  Module* mod = this->phyLayer->getMod();
  if(!force && (mod->hal->getPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID) == RADIOLIB_LORAWAN_MAGIC)) {
    // the device has joined already, we can just pull the data from persistent storage
    RADIOLIB_DEBUG_PRINTLN("Found existing session; restoring...");
    return(this->restore());
  }
#else
  (void)force;
#endif

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
  this->txPwrCur = this->band->powerMax;
  int16_t state = this->setPhyProperties();
  RADIOLIB_ASSERT(state);

  // setup uplink/downlink frequencies and datarates
  state = this->setupChannels(nullptr);
  RADIOLIB_ASSERT(state);

  this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = (this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][0].drMax + this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][0].drMin) / 2;
  // downlink datarate is calculated using a specific uplink channel, so don't care here

#if !defined(RADIOLIB_EEPROM_UNSUPPORTED)
  // save the device address & keys
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_ADDR_ID, this->devAddr);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_APP_S_KEY_ID), this->appSKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FNWK_SINT_KEY_ID), this->fNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_SNWK_SINT_KEY_ID), this->sNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NWK_SENC_KEY_ID), this->nwkSEncKey, RADIOLIB_AES128_BLOCK_SIZE);

  this->saveSession();
  this->saveChannels();

  // everything written to NVM, write current table version to persistent storage and set magic number
  mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TABLE_VERSION_ID, RADIOLIB_PERSISTENT_PARAM_LORAWAN_TABLE_VERSION);
  mod->hal->setPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID, RADIOLIB_LORAWAN_MAGIC);
#endif

  this->isJoinedFlag = true;

  return(RADIOLIB_ERR_NONE);
}

bool LoRaWANNode::isJoined() {
  return(this->isJoinedFlag);
}

#if !defined(RADIOLIB_EEPROM_UNSUPPORTED)
int16_t LoRaWANNode::saveSession() {
  Module* mod = this->phyLayer->getMod();
  
  // store session configuration (MAC commands)
  if(mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_VERSION_ID) != this->rev)
     mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_VERSION_ID, this->rev);

  if(mod->hal->getPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_NONCE_ID) != this->devNonce)
     mod->hal->setPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_NONCE_ID, this->devNonce);
  
  uint8_t txDrRx2Dr = (this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] << 4) | this->rx2.drMax;
  if(mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXDR_RX2DR_ID) != txDrRx2Dr) 
     mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXDR_RX2DR_ID, txDrRx2Dr);

  if(mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXPWR_CUR_ID) != this->txPwrCur)
     mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXPWR_CUR_ID, this->txPwrCur);

  uint8_t rx1DrOffDel = (this->rx1DrOffset << 4) | (this->rxDelays[0] / 1000);
  if(mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX1_DROFF_DEL_ID) != rx1DrOffDel)
     mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX1_DROFF_DEL_ID, rx1DrOffDel);

  uint8_t rx2FreqBuf[3];
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX2FREQ_ID), rx2FreqBuf, 3);
  uint32_t rx2Freq = LoRaWANNode::ntoh<uint32_t>(&rx2FreqBuf[0], 3);
  if(rx2Freq != uint32_t(this->rx2.freq * 10000)) {
    rx2Freq = uint32_t(this->rx2.freq * 10000);
    LoRaWANNode::hton<uint32_t>(&rx2FreqBuf[0], rx2Freq, 3);
    mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX2FREQ_ID), rx2FreqBuf, 3);
  }

  uint8_t adrLimDel = (this->adrLimitExp << 4) | (this->adrDelayExp << 0);
  if(mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_LIM_DEL_ID) != adrLimDel)
     mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_LIM_DEL_ID, adrLimDel);

  if(mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NBTRANS_ID) != this->nbTrans)  
     mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NBTRANS_ID, this->nbTrans);

  // store all frame counters
  if(mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_A_FCNT_DOWN_ID) != this->aFcntDown)
     mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_A_FCNT_DOWN_ID, this->aFcntDown);
  
  if(mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_N_FCNT_DOWN_ID) != this->nFcntDown)
     mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_N_FCNT_DOWN_ID, this->nFcntDown);
  
  if(mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_CONF_FCNT_UP_ID) != this->confFcntUp)
     mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_CONF_FCNT_UP_ID, this->confFcntUp);

  if(mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_CONF_FCNT_DOWN_ID) != this->confFcntDown)
     mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_CONF_FCNT_DOWN_ID, this->confFcntDown);
  
  if(mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_FCNT_ID) != this->adrFcnt)
     mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_FCNT_ID, this->adrFcnt);
  
  // fcntUp is saved using highly efficient wear-leveling as this is by far going to be written most often
  this->saveFcntUp();

  // if there is, or was, any MAC command in the queue, overwrite with the current MAC queue
  uint8_t queueBuff[sizeof(LoRaWANMacCommandQueue_t)] = { 0 };
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FOPTS_ID), queueBuff, sizeof(LoRaWANMacCommandQueue_t));
  LoRaWANMacCommandQueue_t cmdTemp;
  memcpy(&cmdTemp, queueBuff, sizeof(LoRaWANMacCommandQueue_t));
  if(this->commandsUp.numCommands > 0 || cmdTemp.numCommands > 0) {
    memcpy(queueBuff, &this->commandsUp, sizeof(LoRaWANMacCommandQueue_t));
    mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FOPTS_ID), queueBuff, sizeof(LoRaWANMacCommandQueue_t));
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::saveFcntUp() {
  Module* mod = this->phyLayer->getMod();

  uint8_t fcntBuff[30] = { 0 };
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID), fcntBuff, 30);

  // we discard the first two bits - your flash will likely be far dead by the time you reach 2^30 uplinks
  // the first two bytes of the remaining 30 bytes are stored straight into storage without additional wear leveling
  // because they hardly ever change
  uint8_t bits_30_22 = (uint8_t)(this->fcntUp >> 22);
  if(fcntBuff[0] != bits_30_22)
    mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID, bits_30_22, 0);
  uint8_t bits_22_14 = (uint8_t)(this->fcntUp >> 14);
  if(fcntBuff[1] != bits_22_14)
    mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID, bits_22_14, 1);

  // the next 7 bits are stored into one of few indices
  // this index is indicated by the first byte that has its state (most significant bit) different from its predecessor
  // if all have an equal state, restart from the beginning
  // always flip the state bit of the byte that we write to, to indicate that this is the most recently written byte
  uint8_t idx = 2;
  uint8_t state = fcntBuff[idx] >> 7;
  for(; idx < 5; idx++) {
    if(fcntBuff[idx] >> 7 != state) {
      break;
    }
  }
  // check if the last written byte is equal to current, only rewrite if different
  uint8_t bits_14_7 = (this->fcntUp >> 7) & 0x7F;
  if((fcntBuff[idx - 1] & 0x7F) != bits_14_7) {
    // find next index to write
    idx = idx < 5 ? idx : 2;

    // flip the first bit of this byte to indicate that we just wrote here
    bits_14_7 |= (~(fcntBuff[idx] >> 7)) << 7;
    mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID, bits_14_7, idx);
  }

  // equally, the last 7 bits are stored into one of many indices
  // this index is indicated by the first byte that has its state (most significant bit) different from its predecessor
  // if all have an equal state, restart from the beginning
  // always flip the state bit of the byte that we write to, to indicate that this is the most recently written byte
  idx = 5;
  state = fcntBuff[idx] >> 7;
  for(; idx < 30; idx++) {
    if(fcntBuff[idx] >> 7 != state) {
      break;
    }
  }
  idx = idx < 30 ? idx : 5;
  uint8_t bits_7_0 = (this->fcntUp >> 0) & 0x7F;

  // flip the first bit of this byte to indicate that we just wrote here
  bits_7_0 |= (~(fcntBuff[idx] >> 7)) << 7;
  mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID, bits_7_0, idx);

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::saveChannels() {
  const uint8_t bytesPerChannel = 5;
  const uint8_t numBytes = 2 * RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS * bytesPerChannel;
  uint8_t buffer[numBytes] = { 0 };
  for(uint8_t dir = 0; dir < 2; dir++) {
    for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
      uint8_t chBuff[bytesPerChannel] = { 0 };
      chBuff[0]  = (uint8_t)this->availableChannels[dir][i].enabled << 7;
      chBuff[0] |= this->availableChannels[dir][i].idx;
      uint32_t freq = this->availableChannels[dir][i].freq*10000.0;
      LoRaWANNode::hton<uint32_t>(&chBuff[1], freq, 3);
      chBuff[4]  = this->availableChannels[dir][i].drMax << 4;
      chBuff[4] |= this->availableChannels[dir][i].drMin << 0;
      memcpy(&buffer[(dir * RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS * bytesPerChannel) + i * bytesPerChannel], chBuff, bytesPerChannel);
    }
  }
  Module* mod = this->phyLayer->getMod();
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FREQS_ID), buffer, numBytes);
  return(RADIOLIB_ERR_NONE);
}
#endif  // RADIOLIB_EEPROM_UNSUPPORTED

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t LoRaWANNode::uplink(String& str, uint8_t port, bool isConfirmed, LoRaWANEvent_t* event) {
  return(this->uplink(str.c_str(), port, isConfirmed, event));
}
#endif

int16_t LoRaWANNode::uplink(const char* str, uint8_t port, bool isConfirmed, LoRaWANEvent_t* event) {
  return(this->uplink((uint8_t*)str, strlen(str), port, isConfirmed, event));
}

int16_t LoRaWANNode::uplink(uint8_t* data, size_t len, uint8_t port, bool isConfirmed, LoRaWANEvent_t* event) {
  Module* mod = this->phyLayer->getMod();
  
  // check if the Rx windows were closed after sending the previous uplink
  // this FORCES a user to call downlink() after an uplink()
  if(this->rxDelayEnd < this->rxDelayStart) {
    // not enough time elapsed since the last uplink, we may still be in an Rx window
    return(RADIOLIB_ERR_UPLINK_UNAVAILABLE);
  }

  // check destination port
  if(port > 0xDF) {
    return(RADIOLIB_ERR_INVALID_PORT);
  }
  // port 0 is only allowed for MAC-only payloads
  if(port == RADIOLIB_LORAWAN_FPORT_MAC_COMMAND) {
    if (!this->isMACPayload) {
      return(RADIOLIB_ERR_INVALID_PORT);
    }
    // if this is MAC only payload, continue and reset for next uplink
    this->isMACPayload = false;
  }

  int16_t state = RADIOLIB_ERR_NONE;

  // check if there are some MAC commands to piggyback (only when piggybacking onto a application-frame)
  uint8_t foptsLen = 0;
  size_t foptsBufSize = 0;
  if(this->commandsUp.numCommands > 0 && port != RADIOLIB_LORAWAN_FPORT_MAC_COMMAND) {
    // there are, assume the maximum possible FOpts len for buffer allocation
    foptsLen = this->commandsUp.len;
    foptsBufSize = 15;
  }

  // check maximum payload len as defined in phy
  if(len > this->band->payloadLenMax[this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK]]) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // increase frame counter by one
  this->fcntUp += 1;

  // check if we need to do ADR stuff
  uint32_t adrLimit = 0x01 << this->adrLimitExp;
  uint32_t adrDelay = 0x01 << this->adrDelayExp;
  bool adrAckReq = false;
  if((this->fcntUp - this->adrFcnt) >= adrLimit) {
    adrAckReq = true;
  }
  // if we hit the Limit + Delay, try one of three, in order: 
  // set TxPower to max, set DR to min, enable all defined channels
  if ((this->fcntUp - this->adrFcnt) == (adrLimit + adrDelay)) {

    // set the maximum power supported by both the module and the band
    int8_t pwrPrev = this->txPwrCur;
    state = this->setTxPower(this->band->powerMax);
    RADIOLIB_ASSERT(state);

    if(this->txPwrCur == pwrPrev) {

      // failed to increase Tx power, so try to decrease the datarate
      if(this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] > this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK].drMin) {
        this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK]--;
        this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK]--;
      } else {

        // failed to decrease datarate, so enable all available channels
        for(size_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
          if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx != RADIOLIB_LORAWAN_CHANNEL_INDEX_NONE) {
            this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled = true;
            this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i].enabled = true;
          }
        }
      }

    }

    // we tried something to improve the range, so increase the ADR frame counter by 'ADR delay'
    this->adrFcnt += adrDelay;
  }

  // configure for uplink
  this->selectChannels();
  state = this->configureChannel(RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK);
  RADIOLIB_ASSERT(state);

  // build the uplink message
  // the first 16 bytes are reserved for MIC calculation blocks
  size_t uplinkMsgLen = RADIOLIB_LORAWAN_FRAME_LEN(len, foptsBufSize);
  #if RADIOLIB_STATIC_ONLY
  uint8_t uplinkMsg[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
  uint8_t* uplinkMsg = new uint8_t[uplinkMsgLen];
  #endif
  
  // set the packet fields
  if(isConfirmed) {
    uplinkMsg[RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS] = RADIOLIB_LORAWAN_MHDR_MTYPE_CONF_DATA_UP;
    this->confFcntUp = this->fcntUp;
  } else {
    uplinkMsg[RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS] = RADIOLIB_LORAWAN_MHDR_MTYPE_UNCONF_DATA_UP;
  }
  uplinkMsg[RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS] |= RADIOLIB_LORAWAN_MHDR_MAJOR_R1;
  LoRaWANNode::hton<uint32_t>(&uplinkMsg[RADIOLIB_LORAWAN_FHDR_DEV_ADDR_POS], this->devAddr);

  // length of fopts will be added later
  uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] = 0x00;
  if(this->adrEnabled) {
    uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] |= RADIOLIB_LORAWAN_FCTRL_ADR_ENABLED;
    if(adrAckReq) {
      uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] |= RADIOLIB_LORAWAN_FCTRL_ADR_ACK_REQ;
    }
  }

  // if the saved confirm-fcnt is set, set the ACK bit
  bool isConfirmingDown = false;
  if(this->confFcntDown != RADIOLIB_LORAWAN_FCNT_NONE) {
    isConfirmingDown = true;
    uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] |= RADIOLIB_LORAWAN_FCTRL_ACK;
  }

  LoRaWANNode::hton<uint16_t>(&uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCNT_POS], (uint16_t)this->fcntUp);

  // check if we have some MAC commands to append
  if(foptsLen > 0) {
    // assume maximum possible buffer size
    uint8_t foptsBuff[15];
    uint8_t* foptsPtr = foptsBuff;

    // append all MAC replies into fopts buffer
    int16_t i = 0;
    for (; i < this->commandsUp.numCommands; i++) {
      LoRaWANMacCommand_t cmd = this->commandsUp.commands[i];
      memcpy(foptsPtr, &cmd, 1 + cmd.len);
      foptsPtr += cmd.len + 1;
    }
    RADIOLIB_DEBUG_PRINTLN("Uplink MAC payload (%d commands):", this->commandsUp.numCommands);
    RADIOLIB_DEBUG_HEXDUMP(foptsBuff, foptsLen);

    // pop the commands from back to front
    for (; i >= 0; i--) {
      if(this->commandsUp.commands[i].repeat > 0) {
        this->commandsUp.commands[i].repeat--;
      } else {
        deleteMacCommand(this->commandsUp.commands[i].cid, &this->commandsUp);
      }
    }

    uplinkMsgLen = RADIOLIB_LORAWAN_FRAME_LEN(len, foptsLen);
    uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] |= foptsLen;

    // encrypt it
    processAES(foptsBuff, foptsLen, this->nwkSEncKey, &uplinkMsg[RADIOLIB_LORAWAN_FHDR_FOPTS_POS], this->fcntUp, RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK, 0x01, true);
    
  }

  // set the port
  uplinkMsg[RADIOLIB_LORAWAN_FHDR_FPORT_POS(foptsLen)] = port;

  // select encryption key based on the target port
  uint8_t* encKey = this->appSKey;
  if(port == RADIOLIB_LORAWAN_FPORT_MAC_COMMAND) {
    encKey = this->nwkSEncKey;
  }

  // encrypt the frame payload
  processAES(data, len, encKey, &uplinkMsg[RADIOLIB_LORAWAN_FRAME_PAYLOAD_POS(foptsLen)], this->fcntUp, RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK, 0x00, true);

  // create blocks for MIC calculation
  uint8_t block0[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  block0[RADIOLIB_LORAWAN_BLOCK_MAGIC_POS] = RADIOLIB_LORAWAN_MIC_BLOCK_MAGIC;
  block0[RADIOLIB_LORAWAN_BLOCK_DIR_POS] = RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK;
  LoRaWANNode::hton<uint32_t>(&block0[RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS], this->devAddr);
  LoRaWANNode::hton<uint32_t>(&block0[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], this->fcntUp);
  block0[RADIOLIB_LORAWAN_MIC_BLOCK_LEN_POS] = uplinkMsgLen - RADIOLIB_AES128_BLOCK_SIZE - sizeof(uint32_t);

  uint8_t block1[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  memcpy(block1, block0, RADIOLIB_AES128_BLOCK_SIZE);
  if(this->confFcntDown != RADIOLIB_LORAWAN_FCNT_NONE) {
    LoRaWANNode::hton<uint16_t>(&block1[RADIOLIB_LORAWAN_BLOCK_CONF_FCNT_POS], (uint16_t)this->confFcntDown);
  }
  block1[RADIOLIB_LORAWAN_MIC_DATA_RATE_POS] = this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK];
  block1[RADIOLIB_LORAWAN_MIC_CH_INDEX_POS] = this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK].idx;
  
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

  RADIOLIB_DEBUG_PRINTLN("uplinkMsg:");
  RADIOLIB_DEBUG_HEXDUMP(uplinkMsg, uplinkMsgLen);

  // perform CSMA if enabled.
  if (enableCSMA) {
    performCSMA();
  }

  // send it (without the MIC calculation blocks)
  state = this->phyLayer->transmit(&uplinkMsg[RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS], uplinkMsgLen - RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS);

  // set the timestamp so that we can measure when to start receiving
  this->rxDelayStart = mod->hal->millis();
  RADIOLIB_DEBUG_PRINTLN("Uplink sent <-- Rx Delay start");

  #if !RADIOLIB_STATIC_ONLY
  delete[] uplinkMsg;
  #endif
  RADIOLIB_ASSERT(state);
  
  // the downlink confirmation was acknowledged, so clear the counter value
  this->confFcntDown = RADIOLIB_LORAWAN_FCNT_NONE;

  // pass the extra info if requested
  if(event) {
    event->dir = RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK;
    event->confirmed = isConfirmed;
    event->confirming = isConfirmingDown;
    event->datarate = this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK];
    event->freq = currentChannels[event->dir].freq;
    event->power = this->txPwrCur;
    event->fcnt = this->fcntUp;
    event->port = port;
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::downlinkCommon() {
  Module* mod = this->phyLayer->getMod();
  const uint32_t scanGuard = 10;

  // check if there are any upcoming Rx windows
  // if the Rx1 window has already started, you're too late, because most downlinks happen in Rx1
  if(mod->hal->millis() - this->rxDelayStart > (this->rxDelays[0] - scanGuard)) {
    // if between start of Rx1 and end of Rx2, wait until Rx2 closes
    if(mod->hal->millis() - this->rxDelayStart < this->rxDelays[1]) {
      mod->hal->delay(this->rxDelays[1] + this->rxDelayStart - mod->hal->millis());
    }
    // update the end timestamp in case user got stuck between uplink and downlink
    this->rxDelayEnd = mod->hal->millis();
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

  // create the masks that are required for receiving downlinks
  uint16_t irqFlags = 0x0000;
  uint16_t irqMask = 0x0000;
  this->phyLayer->irqRxDoneRxTimeout(irqFlags, irqMask);

  this->phyLayer->setPacketReceivedAction(LoRaWANNodeOnDownlinkAction);

  // perform listening in the two Rx windows
  for(uint8_t i = 0; i < 2; i++) {
    downlinkAction = false;

    // calculate the Rx timeout
    // according to the spec, this must be at least enough time to effectively detect a preamble
    // but pad it a bit on both sides (start and end) to make sure it is wide enough
    uint32_t timeoutHost = this->phyLayer->getTimeOnAir(0) + 2*scanGuard*1000;
    uint32_t timeoutMod  = this->phyLayer->calculateRxTimeout(timeoutHost);

    // wait for the start of the Rx window
    // the waiting duration is shortened a bit to cover any possible timing errors
    uint32_t waitLen = this->rxDelays[i] - (mod->hal->millis() - this->rxDelayStart);
    if(waitLen > scanGuard) {
      waitLen -= scanGuard;
    }
    mod->hal->delay(waitLen);

    // open Rx window by starting receive with specified timeout
    state = this->phyLayer->startReceive(timeoutMod, irqFlags, irqMask, 0);
    RADIOLIB_DEBUG_PRINTLN("Opening Rx%d window (%d us timeout)... <-- Rx Delay end ", i+1, timeoutHost);
    
    // wait for the timeout to complete (and a small additional delay)
    mod->hal->delay(timeoutHost / 1000 + scanGuard / 2);
    RADIOLIB_DEBUG_PRINTLN("closing");

    // check if the IRQ bit for Rx Timeout is set
    if(!this->phyLayer->isRxTimeout()) {
      break;

    } else if(i == 0) {
      // nothing in the first window, configure for the second
      this->phyLayer->standby();
      state = this->phyLayer->setFrequency(this->rx2.freq);
      RADIOLIB_ASSERT(state);

      DataRate_t dataRate;
      findDataRate(this->rx2.drMax, &dataRate);
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

    return(RADIOLIB_ERR_RX_TIMEOUT);
  }

  // wait for the DIO to fire indicating a downlink is received
  while(!downlinkAction) {
    mod->hal->yield();
  }

  // we have a message, clear actions, go to standby and reset the IQ inversion
  this->phyLayer->standby();  // TODO check: this should be done automagically due to RxSingle?
  this->phyLayer->clearPacketReceivedAction();
  if(!this->FSK) {
    state = this->phyLayer->invertIQ(false);
    RADIOLIB_ASSERT(state);
  }

  return(RADIOLIB_ERR_NONE);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t LoRaWANNode::downlink(String& str, LoRaWANEvent_t* event) {
  int16_t state = RADIOLIB_ERR_NONE;

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

int16_t LoRaWANNode::downlink(uint8_t* data, size_t* len, LoRaWANEvent_t* event) {
  // handle Rx1 and Rx2 windows - returns RADIOLIB_ERR_NONE if a downlink is received
  int16_t state = downlinkCommon();
  RADIOLIB_ASSERT(state);

  // get the packet length
  size_t downlinkMsgLen = this->phyLayer->getPacketLength();
  RADIOLIB_DEBUG_PRINTLN("Downlink message length: %d", downlinkMsgLen);

  // check the minimum required frame length
  // an extra byte is subtracted because downlink frames may not have a port
  if(downlinkMsgLen < RADIOLIB_LORAWAN_FRAME_LEN(0, 0) - 1 - RADIOLIB_AES128_BLOCK_SIZE) {
    RADIOLIB_DEBUG_PRINTLN("Downlink message too short (%lu bytes)", downlinkMsgLen);
    return(RADIOLIB_ERR_DOWNLINK_MALFORMED);
  }

  // build the buffer for the downlink message
  // the first 16 bytes are reserved for MIC calculation block
  #if !RADIOLIB_STATIC_ONLY
    uint8_t* downlinkMsg = new uint8_t[RADIOLIB_AES128_BLOCK_SIZE + downlinkMsgLen];
  #else
    uint8_t downlinkMsg[RADIOLIB_STATIC_ARRAY_SIZE];
  #endif

  // set the MIC calculation block
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
    #if !RADIOLIB_STATIC_ONLY
      delete[] downlinkMsg;
    #endif
    return(state);
  }

  // get the frame counter and set it to the MIC calculation block
  uint16_t fcnt16 = LoRaWANNode::ntoh<uint16_t>(&downlinkMsg[RADIOLIB_LORAWAN_FHDR_FCNT_POS]);
  LoRaWANNode::hton<uint16_t>(&downlinkMsg[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], fcnt16);

  // if this downlink is confirming an uplink, its MIC was generated with the least-significant 16 bits of that fcntUp
  bool isConfirmingUp = false;
  if((downlinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] & RADIOLIB_LORAWAN_FCTRL_ACK) && (this->rev == 1)) {
    isConfirmingUp = true;
    LoRaWANNode::hton<uint16_t>(&downlinkMsg[RADIOLIB_LORAWAN_BLOCK_CONF_FCNT_POS], (uint16_t)this->confFcntUp);
  }
  
  RADIOLIB_DEBUG_PRINTLN("downlinkMsg:");
  RADIOLIB_DEBUG_HEXDUMP(downlinkMsg, RADIOLIB_AES128_BLOCK_SIZE + downlinkMsgLen);

  // calculate length of FOpts and payload
  uint8_t foptsLen = downlinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] & RADIOLIB_LORAWAN_FHDR_FOPTS_LEN_MASK;
  int payLen = downlinkMsgLen - 8 - foptsLen - sizeof(uint32_t);

  RADIOLIB_DEBUG_PRINTLN("FOpts: %02X", downlinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS]);

  // in LoRaWAN v1.1, a frame can be a network frame if there is no Application payload
  // i.e., no payload at all (empty frame or FOpts only), or MAC only payload (FPort = 0)
  bool isAppDownlink = true;
  if(payLen <= 0) {
    if(this->rev == 1) {
      isAppDownlink = false;
    }
  }
  else if(downlinkMsg[RADIOLIB_LORAWAN_FHDR_FPORT_POS(foptsLen)] == RADIOLIB_LORAWAN_FPORT_MAC_COMMAND) {
    foptsLen = payLen - 1;
    if(this->rev == 1) {
      isAppDownlink = false;
    }
  }
  RADIOLIB_DEBUG_PRINTLN("FOptsLen: %d", foptsLen);

  // check the FcntDown value (Network or Application)
  uint32_t fcntDownPrev = 0;
  if (isAppDownlink) {
    fcntDownPrev = this->aFcntDown;
  } else {
    fcntDownPrev = this->nFcntDown;
  }

  RADIOLIB_DEBUG_PRINTLN("fcnt: %d, fcntPrev: %d, isAppDownlink: %d", fcnt16, fcntDownPrev, (int)isAppDownlink);

  // if this is not the first downlink...
  // assume a 16-bit to 32-bit rollover if difference between counters in LSB is smaller than MAX_FCNT_GAP
  // if that isn't the case and the received fcnt is smaller or equal to the last heard fcnt, then error
  uint32_t fcnt32 = fcnt16;
  if(fcntDownPrev > 0) {
    if((fcnt16 <= fcntDownPrev) && ((0xFFFF - (uint16_t)fcntDownPrev + fcnt16) > RADIOLIB_LORAWAN_MAX_FCNT_GAP)) {
      #if !RADIOLIB_STATIC_ONLY
        delete[] downlinkMsg;
      #endif
      if (isAppDownlink) {
        return(RADIOLIB_ERR_A_FCNT_DOWN_INVALID);
      } else {
        return(RADIOLIB_ERR_N_FCNT_DOWN_INVALID);
      }
    } else if (fcnt16 <= fcntDownPrev) {
      uint16_t msb = (fcntDownPrev >> 16) + 1;  // assume a rollover
      fcnt32 |= ((uint32_t)msb << 16);          // add back the MSB part
    }
  }
  
  // check the MIC
  if(!verifyMIC(downlinkMsg, RADIOLIB_AES128_BLOCK_SIZE + downlinkMsgLen, this->sNwkSIntKey)) {
    #if !RADIOLIB_STATIC_ONLY
      delete[] downlinkMsg;
    #endif
    return(RADIOLIB_ERR_CRC_MISMATCH);
  }
  
  // save current fcnt to respective frame counter
  if (isAppDownlink) {
    this->aFcntDown = fcnt32;
  } else {
    this->nFcntDown = fcnt32;
  }

  // if this is a confirmed frame, save the downlink number (only app frames can be confirmed)
  bool isConfirmedDown = false;
  if((downlinkMsg[RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS] & 0xFE) == RADIOLIB_LORAWAN_MHDR_MTYPE_CONF_DATA_DOWN) {
    this->confFcntDown = this->aFcntDown;
    isConfirmedDown = true;
  }

  // check the address
  uint32_t addr = LoRaWANNode::ntoh<uint32_t>(&downlinkMsg[RADIOLIB_LORAWAN_FHDR_DEV_ADDR_POS]);
  if(addr != this->devAddr) {
    RADIOLIB_DEBUG_PRINTLN("Device address mismatch, expected 0x%08X, got 0x%08X", this->devAddr, addr);
    #if !RADIOLIB_STATIC_ONLY
      delete[] downlinkMsg;
    #endif
    return(RADIOLIB_ERR_DOWNLINK_MALFORMED);
  }

  // process FOpts (if there are any)
  if(foptsLen > 0) {
    // there are some Fopts, decrypt them
    uint8_t fopts[RADIOLIB_MAX(RADIOLIB_LORAWAN_FHDR_FOPTS_LEN_MASK, (int)foptsLen)];

    // TODO it COULD be the case that the assumed FCnt rollover is incorrect, if possible figure out a way to catch this and retry with just fcnt16
    // if there are <= 15 bytes of FOpts, they are in the FHDR, otherwise they are in the payload
    // in case of the latter, process AES is if it were a normal payload but using the NwkSEncKey
    if(foptsLen <= RADIOLIB_LORAWAN_FHDR_FOPTS_LEN_MASK) {
      uint8_t ctrId = 0x01 + isAppDownlink; // see LoRaWAN v1.1 errata
      processAES(&downlinkMsg[RADIOLIB_LORAWAN_FHDR_FOPTS_POS], (size_t)foptsLen, this->nwkSEncKey, fopts, fcnt32, RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK, ctrId, true);
    } else {
      processAES(&downlinkMsg[RADIOLIB_LORAWAN_FRAME_PAYLOAD_POS(0)], (size_t)foptsLen, this->nwkSEncKey, fopts, fcnt32, RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK, 0x00, true);
    }

    RADIOLIB_DEBUG_PRINTLN("fopts:");
    RADIOLIB_DEBUG_HEXDUMP(fopts, foptsLen);

    // process the MAC command(s)
    int8_t remLen = foptsLen;
    uint8_t* foptsPtr = fopts;
    while(remLen > 0) {
      LoRaWANMacCommand_t cmd = {
        .cid = *foptsPtr,
        .payload = { 0 },
        .len = (uint8_t)RADIOLIB_MIN((remLen - 1), 5),
        .repeat = 0,
      };
      memcpy(cmd.payload, foptsPtr + 1, cmd.len);
      RADIOLIB_DEBUG_PRINTLN("[%02X]: %02X %02X %02X %02X %02X (%d)",
                              cmd.cid, cmd.payload[0], cmd.payload[1], cmd.payload[2], cmd.payload[3], cmd.payload[4], cmd.len);

      // try to process the mac command
      // TODO how to handle incomplete commands?
      size_t processedLen = execMacCommand(&cmd) + 1;

      // processing succeeded, move in the buffer to the next command
      remLen -= processedLen;
      foptsPtr += processedLen;
      RADIOLIB_DEBUG_PRINTLN("Processed: %d, remaining: %d", processedLen, remLen);
    }

    // if FOptsLen for the next uplink is larger than can be piggybacked onto an uplink, send separate uplink
    if(this->commandsUp.len > 15) {
      size_t foptsBufSize = this->commandsUp.len;
      #if RADIOLIB_STATIC_ONLY
        uint8_t foptsBuff[RADIOLIB_STATIC_ARRAY_SIZE];
      #else
        uint8_t* foptsBuff = new uint8_t[foptsBufSize];
      #endif
      uint8_t* foptsPtr = foptsBuff;
      // append all MAC replies into fopts buffer
      int16_t i = 0;
      for (; i < this->commandsUp.numCommands; i++) {
        LoRaWANMacCommand_t cmd = this->commandsUp.commands[i];
        memcpy(foptsPtr, &cmd, 1 + cmd.len);
        foptsPtr += cmd.len + 1;
      }
      RADIOLIB_DEBUG_PRINTLN("Uplink MAC payload (%d commands):", this->commandsUp.numCommands);
      RADIOLIB_DEBUG_HEXDUMP(foptsBuff, foptsBufSize);

      // pop the commands from back to front
      for (; i >= 0; i--) {
        if(this->commandsUp.commands[i].repeat > 0) {
          this->commandsUp.commands[i].repeat--;
        } else {
          deleteMacCommand(this->commandsUp.commands[i].cid, &this->commandsUp);
        }
      }

      this->isMACPayload = true;
      this->uplink(foptsBuff, foptsBufSize, RADIOLIB_LORAWAN_FPORT_MAC_COMMAND);
      #if !RADIOLIB_STATIC_ONLY
        delete[] foptsBuff;
      #endif

      #if RADIOLIB_STATIC_ONLY
        uint8_t strDown[RADIOLIB_STATIC_ARRAY_SIZE];
      #else
        uint8_t* strDown = new uint8_t[this->band->payloadLenMax[this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK]]];
      #endif
      size_t lenDown = 0;
      state = this->downlink(strDown, &lenDown);
      #if !RADIOLIB_STATIC_ONLY
        delete[] strDown;
      #endif
      RADIOLIB_ASSERT(state);
    }

  }

  // a downlink was received, so reset the ADR counter to this uplink's fcnt
  this->adrFcnt = this->fcntUp;

  // pass the extra info if requested
  if(event) {
    event->dir = RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK;
    event->confirmed = isConfirmedDown;
    event->confirming = isConfirmingUp;
    event->datarate = this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK];
    event->freq = currentChannels[event->dir].freq;
    event->power = this->txPwrCur;
    event->fcnt = isAppDownlink ? this->aFcntDown : this->nFcntDown;
    event->port = downlinkMsg[RADIOLIB_LORAWAN_FHDR_FPORT_POS(foptsLen)];
  }

  // process Application payload (if there is any)
  if(payLen <= 0 || foptsLen > RADIOLIB_LORAWAN_FHDR_FOPTS_MAX_LEN) {
    // no payload
    *len = 0;
    #if !RADIOLIB_STATIC_ONLY
      delete[] downlinkMsg;
    #endif

    return(RADIOLIB_ERR_NONE);
  }

  // there is payload, and so there should be a port too
  // TODO pass the port?
  *len = payLen - 1;

  // TODO it COULD be the case that the assumed rollover is incorrect, then figure out a way to catch this and retry with just fcnt16
  processAES(&downlinkMsg[RADIOLIB_LORAWAN_FRAME_PAYLOAD_POS(foptsLen)], payLen - 1, this->appSKey, data, fcnt32, RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK, 0x00, true);
  
  #if !RADIOLIB_STATIC_ONLY
    delete[] downlinkMsg;
  #endif

  return(RADIOLIB_ERR_NONE);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t LoRaWANNode::sendReceive(String& strUp, uint8_t port, String& strDown, bool isConfirmed, LoRaWANEvent_t* eventUp, LoRaWANEvent_t* eventDown) {
  // send the uplink
  int16_t state = this->uplink(strUp, port, isConfirmed, eventUp);
  RADIOLIB_ASSERT(state);

  // wait for the downlink
  state = this->downlink(strDown, eventDown);
  return(state);
}
#endif

int16_t LoRaWANNode::sendReceive(const char* strUp, uint8_t port, uint8_t* dataDown, size_t* lenDown, bool isConfirmed, LoRaWANEvent_t* eventUp, LoRaWANEvent_t* eventDown) {
  // send the uplink
  int16_t state = this->uplink(strUp, port, isConfirmed, eventUp);
  RADIOLIB_ASSERT(state);

  // wait for the downlink
  state = this->downlink(dataDown, lenDown, eventDown);
  return(state);
}

int16_t LoRaWANNode::sendReceive(uint8_t* dataUp, size_t lenUp, uint8_t port, uint8_t* dataDown, size_t* lenDown, bool isConfirmed, LoRaWANEvent_t* eventUp, LoRaWANEvent_t* eventDown) {
  // send the uplink
  int16_t state = this->uplink(dataUp, lenUp, port, isConfirmed, eventUp);
  RADIOLIB_ASSERT(state);

  // wait for the downlink
  state = this->downlink(dataDown, lenDown, eventDown);
  return(state);
}

void LoRaWANNode::setDeviceStatus(uint8_t battLevel) {
  this->battLevel = battLevel;
}

uint32_t LoRaWANNode::getFcntUp() {
  return(this->fcntUp);
}

uint32_t LoRaWANNode::getNFcntDown() {
  return(this->nFcntDown);
}

uint32_t LoRaWANNode::getAFcntDown() {
  return(this->aFcntDown);
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
  int16_t state = this->setTxPower(this->txPwrCur);
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

int16_t LoRaWANNode::setupChannels(uint8_t* cfList) {
  size_t num = 0;
  RADIOLIB_DEBUG_PRINTLN("Setting up channels");
  
  // in case of frequency list-type band, copy the default TX channels into the available channels, with RX1 = TX
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
    RADIOLIB_DEBUG_PRINTLN("Dynamic band");
    // copy the default defined channels into the first slots
    for(; num < 3 && this->band->txFreqs[num].enabled; num++) {
      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num] = this->band->txFreqs[num];
      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][num] = this->band->txFreqs[num];
      RADIOLIB_DEBUG_PRINTLN("Channel UL/DL %d frequency = %f MHz", this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num].idx, this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num].freq);
    }
    // if there is a cflist present, parse its frequencies into the next five slots, with datarate range copied from default channel 0
    if(cfList != nullptr) {
      RADIOLIB_DEBUG_PRINTLN("CFList present");
      for(uint8_t i = 0; i < 5; i++, num++) {
        LoRaWANChannel_t chnl;
        chnl.enabled = true;
        chnl.idx = num;
        uint32_t freq = LoRaWANNode::ntoh<uint32_t>(&cfList[3*i], 3);
        chnl.freq = (float)freq/10000.0;
        chnl.drMin = this->band->txFreqs[0].drMin;  // drMin is equal for all channels
        chnl.drMax = this->band->txFreqs[0].drMax;  // drMax is equal for all channels
        this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num] = chnl;
        this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][num] = chnl;
        RADIOLIB_DEBUG_PRINTLN("Channel UL/DL %d frequency = %f MHz", chnl.idx, this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num].freq);
      }
    }
    for(; num < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; num++) {
      this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num] = RADIOLIB_LORAWAN_CHANNEL_NONE;
    }
    
  } else {                  // RADIOLIB_LORAWAN_BAND_FIXED
    if(cfList != nullptr) {
      uint8_t chSpan = 0;
      uint8_t chNum = 0;
      // in case of mask-type bands, copy those frequencies that are masked true into the available TX channels
      for(size_t chMaskCntl = 0; chMaskCntl < 5; chMaskCntl++) {
        uint16_t mask = LoRaWANNode::ntoh<uint16_t>(&cfList[2*chMaskCntl]);
        RADIOLIB_DEBUG_PRINTLN("mask[%d] = 0x%04x", chMaskCntl, mask);
        for(size_t i = 0; i < 16; i++) {
          // if we must roll over to next span, reset chNum and move to next channel span
          if(chNum >= this->band->txSpans[chSpan].numChannels) {
            chNum = 0;
            chSpan++;
          }

          if(mask & (1UL << i)) {
            if(chSpan >= this->band->numTxSpans) {
              RADIOLIB_DEBUG_PRINTLN("channel bitmask overrun!");
              return(RADIOLIB_ERR_UNKNOWN);
            }
            LoRaWANChannel_t chnl;
            chnl.enabled = true;
            chnl.idx   = chMaskCntl*16 + i;
            chnl.freq  = this->band->txSpans[chSpan].freqStart + chNum*this->band->txSpans[chSpan].freqStep;
            chnl.drMin = this->band->txSpans[chSpan].drMin;
            chnl.drMax = this->band->txSpans[chSpan].drMax;
            this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num] = chnl;
            // downlink channels are dynamically calculated on each uplink in selectChannels()
            RADIOLIB_DEBUG_PRINTLN("Channel UL %d frequency = %f MHz", num, this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num].freq);
            num++;
          }
          chNum++;
        }
      }
      for(; chNum < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; chNum++) {
        this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chNum] = RADIOLIB_LORAWAN_CHANNEL_NONE;
      }
    }
  }
  for (int i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
    RADIOLIB_DEBUG_PRINTLN("UL: %d %d %5.2f (%d - %d) | DL: %d %d %5.2f (%d - %d)",
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
  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::selectSubband(uint8_t idx) {
  int16_t state = this->selectSubband((idx - 1) * 8, idx * 8 - 1);
  return(state);
}

int16_t LoRaWANNode::selectSubband(uint8_t startChannel, uint8_t endChannel) {
  if(this->isJoinedFlag) {
    RADIOLIB_DEBUG_PRINTLN("There is already an active session - cannot change subband");
    return(RADIOLIB_ERR_INVALID_CHANNEL);
  }
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
    RADIOLIB_DEBUG_PRINTLN("This is a dynamic band plan which does not support subbands");
    return(RADIOLIB_ERR_INVALID_CHANNEL);
  }

  uint8_t numChannels = endChannel - startChannel + 1;
  if(startChannel > this->band->txSpans[0].numChannels) {
    RADIOLIB_DEBUG_PRINTLN("There are only %d channels available in this band", this->band->txSpans[0].numChannels);
    return(RADIOLIB_ERR_INVALID_CHANNEL);
  }
  if(startChannel + numChannels > this->band->txSpans[0].numChannels) {
    numChannels = this->band->txSpans[0].numChannels - startChannel;
    RADIOLIB_DEBUG_PRINTLN("Could only select %d channels due to end of band", numChannels);
  }
  if(numChannels > RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS) {
    numChannels = RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS;
    RADIOLIB_DEBUG_PRINTLN("Could only select %d channels due to specified limit", numChannels);
  }
  
  LoRaWANChannel_t chnl;
  for(size_t chNum = 0; chNum < numChannels; chNum++) {
    chnl.enabled = true;
    chnl.idx   = startChannel + chNum;
    chnl.freq  = this->band->txSpans[0].freqStart + chnl.idx*this->band->txSpans[0].freqStep;
    chnl.drMin = this->band->txSpans[0].drMin;
    chnl.drMax = this->band->txSpans[0].drMax;
    availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chNum] = chnl;
    // downlink channel is dynamically calculated on each uplink in selectChannels()
    RADIOLIB_DEBUG_PRINTLN("Channel UL %d frequency = %f MHz", chNum, availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][chNum].freq);
  }
  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::selectChannelsJR(uint16_t devNonce, uint8_t joinDr) {
  LoRaWANChannel_t channelUp;
  LoRaWANChannel_t channelDown;
  uint8_t drUp;
  uint8_t drDown;
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
    // count the number of available channels for a join-request (default channels + join-request channels)
    uint8_t numJRChannels = 0;
    for(size_t i = 0; i < 3; i++) {
      if(this->band->txFreqs[i].enabled) {
        numJRChannels++;
      }
      if(this->band->txJoinReq[i].enabled) {
        numJRChannels++;
      }
    }

    // cycle through the available channels (seed with devNonce)
    uint8_t channelId = devNonce % numJRChannels;

    // find the channel whose index is selected
    for(size_t i = 0; i < 3; i++) {
      if(this->band->txFreqs[i].idx == channelId) {
        channelUp = this->band->txFreqs[i];
        break;
      }
      if(this->band->txJoinReq[i].idx == channelId) {
        channelUp = this->band->txJoinReq[i];
      }
    }

    // if join datarate is user-specified and valid, select that value; otherwise use
    if(joinDr != RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
      if(joinDr >= channelUp.drMin && joinDr <= channelUp.drMax) {
        drUp = joinDr;
      } else {
        RADIOLIB_DEBUG_PRINTLN("Datarate %d is not valid (min: %d, max %d) - using default", joinDr, channelUp.drMin, channelUp.drMax);
        joinDr = RADIOLIB_LORAWAN_DATA_RATE_UNUSED;
      }
    } 
    if(joinDr == RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
      drUp = int((channelUp.drMax + channelUp.drMin) / 2);
    }

    // derive the downlink channel and datarate from the uplink channel and datarate
    channelDown = channelUp;
    drDown = getDownlinkDataRate(drUp, this->rx1DrOffset, this->band->rx1DataRateBase, channelDown.drMin, channelDown.drMax);
    
  } else {                  // RADIOLIB_LORAWAN_BAND_FIXED
    uint8_t spanID = 0;
    uint8_t channelID = 0;
    uint8_t numEnabledChannels = 0;
    // if there are any predefined channels because user selected a subband, select one of these channels
    for(; numEnabledChannels < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; numEnabledChannels++) {
      if(this->availableChannels[numEnabledChannels][RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK].enabled == false) {
        break;
      }
    }
    if(numEnabledChannels > 0) {
      uint8_t channelID = this->phyLayer->random(numEnabledChannels);
      channelUp = this->availableChannels[channelID][RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK];
      spanID = channelUp.idx / this->band->txSpans[0].numChannels;
      channelID = channelUp.idx;

    } else {  // no pre-selected subband, cycle through size-8 (or size-9) blocks
      channelUp.enabled = true;
      uint8_t numBlocks = this->band->txSpans[0].numChannels / 8;   // calculate number of 8-channel blocks
      uint8_t numBlockChannels = 8 + (this->band->numTxSpans == 2 ? 1 : 0);  // add a 9th channel if there's a second span
      uint8_t blockID = devNonce % numBlocks;                       // currently selected block (seed with devNonce)
      channelID = this->phyLayer->random(numBlockChannels);         // select randomly from these 8 or 9 channels
      RADIOLIB_DEBUG_PRINTLN("blocks: %d, channels/block: %d, blockID: %d, channelID: %d", numBlocks, numBlockChannels, blockID, channelID);

      // if channel 0-7 is selected, retrieve this channel from span 0; otherwise span 1
      if(channelID < 8) {
        spanID = 0;
        channelUp.idx = blockID * 8 + channelID;
      } else {
        spanID = 1;
        channelUp.idx = blockID;
      }
      channelUp.freq = this->band->txSpans[spanID].freqStart + channelUp.idx*this->band->txSpans[spanID].freqStep;
    }
    
    // for fixed channel plans, the user-specified datarate is ignored and span-specific value must be used
    drUp = this->band->txSpans[spanID].joinRequestDataRate;

    // derive the downlink channel and datarate from the uplink channel and datarate
    channelDown.enabled = true;
    channelDown.idx = channelID % this->band->rx1Span.numChannels;
    channelDown.freq = this->band->rx1Span.freqStart + channelDown.idx*this->band->rx1Span.freqStep;
    channelDown.drMin = this->band->rx1Span.drMin;
    channelDown.drMax = this->band->rx1Span.drMax;
    drDown = getDownlinkDataRate(drUp, this->rx1DrOffset, this->band->rx1DataRateBase, channelDown.drMin, channelDown.drMax);
    
  }
  this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK]   = channelUp;
  this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = channelDown;
  this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK]         = drUp;
  this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK]       = drDown;

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
    } else {
      break;
    }
  }
  if(numChannels == 0) {
    RADIOLIB_DEBUG_PRINTLN("There are no channels defined - are you in ABP mode with no defined subband?");
    return(RADIOLIB_ERR_INVALID_CHANNEL);
  }
  // select a random ID & channel from the list of enabled and possible channels
  uint8_t channelID = channelsEnabled[this->phyLayer->random(numChannels)];
  this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][channelID];
  
  if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
    // for dynamic bands, the downlink channel is the one matched to the uplink channel
    this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][channelID];
  
  } else {                  // RADIOLIB_LORAWAN_BAND_FIXED
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
  // find the minimum and maximum available datarates by checking the enabled uplink channels
  uint8_t drMin = RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES;
  uint8_t drMax = 0;
  for(size_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
    if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled) {
      drMin = RADIOLIB_MIN(drMin, this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMin);
      drMax = RADIOLIB_MAX(drMax, this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMax);
    }
  }
  if((drUp < drMin) || (drUp > drMax)) {
    RADIOLIB_DEBUG_PRINTLN("Cannot configure DR %d (min: %d, max: %d)", drUp, drMin, drMax);
    return(RADIOLIB_ERR_DATA_RATE_INVALID);
  }
  this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = drUp;

  RADIOLIB_DEBUG_PRINTLN("Configured DR up = %d", drUp);

  return(RADIOLIB_ERR_NONE);
}

void LoRaWANNode::setADR(bool enable) {
  this->adrEnabled = enable;
}

int16_t LoRaWANNode::setTxPower(int8_t txPower) {
  int16_t state = RADIOLIB_ERR_INVALID_OUTPUT_POWER;
  while(state == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    // go from the highest power and lower it until we hit one supported by the module
    state = this->phyLayer->setOutputPower(txPower--);
  }
  if(state == RADIOLIB_ERR_NONE) {
    txPower++;
    this->txPwrCur = txPower;
  }
  return(state);
}

int16_t LoRaWANNode::findDataRate(uint8_t dr, DataRate_t* dataRate) {
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
    RADIOLIB_DEBUG_PRINTLN("DR %d: LORA (SF: %d, BW: %f, CR: %d)", 
                            dataRateBand, dataRate->lora.spreadingFactor, dataRate->lora.bandwidth, dataRate->lora.codingRate);
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::configureChannel(uint8_t dir) {
  // set the frequency
  RADIOLIB_DEBUG_PRINTLN("");
  RADIOLIB_DEBUG_PRINTLN("Channel frequency %cL = %f MHz", dir ? 'D' : 'U', this->currentChannels[dir].freq);
  int state = this->phyLayer->setFrequency(this->currentChannels[dir].freq);
  RADIOLIB_ASSERT(state);

  // if this channel is an FSK channel, toggle the FSK switch
  if(this->band->dataRates[this->dataRates[dir]] == RADIOLIB_LORAWAN_DATA_RATE_FSK_50_K) {
    this->FSK = true;
  } else {
    this->FSK = false;
  }

  DataRate_t dr;
  findDataRate(this->dataRates[dir], &dr);
  state = this->phyLayer->setDataRate(dr);
  RADIOLIB_ASSERT(state);

  if(this->FSK) {
    state = this->phyLayer->setDataShaping(RADIOLIB_SHAPING_1_0);
    RADIOLIB_ASSERT(state);
    state = this->phyLayer->setEncoding(RADIOLIB_ENCODING_WHITENING);
  }

  return(state);
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

int16_t LoRaWANNode::deleteMacCommand(uint8_t cid, LoRaWANMacCommandQueue_t* queue) {
  if(queue->numCommands == 0) {
    return(RADIOLIB_ERR_COMMAND_QUEUE_EMPTY);
  }

  for(size_t index = 0; index < queue->numCommands; index++) {
    if(queue->commands[index].cid == cid) {
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
      // TODO all these configuration should only be set if all ACKs are set, otherwise retain previous state (per spec)
      uint8_t drUp = (cmd->payload[0] & 0xF0) >> 4;
      uint8_t txPower = cmd->payload[0] & 0x0F;
      uint16_t chMask = LoRaWANNode::ntoh<uint16_t>(&cmd->payload[1]);
      uint8_t chMaskCntl = (cmd->payload[3] & 0x70) >> 4;
      uint8_t nbTrans = cmd->payload[3] & 0x0F;
      RADIOLIB_DEBUG_PRINTLN("ADR REQ: dataRate = %d, txPower = %d, chMask = 0x%04x, chMaskCntl = %02x, nbTrans = %d", drUp, txPower, chMask, chMaskCntl, nbTrans);

      // apply the configuration
      uint8_t drAck = 0;
      if(drUp == 0x0F) {
        drAck = 1;
      } else if (this->band->dataRates[drUp] != RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
        uint8_t drDown = getDownlinkDataRate(drUp, this->rx1DrOffset, this->band->rx1DataRateBase,
                                             this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].drMin, this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].drMax);
        this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = drUp;
        this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = drDown;
        drAck = 1;
      } 

      // try to apply the power configuration
      uint8_t pwrAck = 0;
      if(txPower == 0x0F) {
        pwrAck = 1;

      } else {
        int8_t pwr = this->band->powerMax - 2*txPower;
        int16_t state = this->setTxPower(pwr);
        // only acknowledge if the requested datarate was succesfully configured
        if((state == RADIOLIB_ERR_NONE) && (this->txPwrCur == pwr)) {
          pwrAck = 1;
        }

      }

      uint8_t chMaskAck = 1;
      if(this->band->bandType == RADIOLIB_LORAWAN_BAND_DYNAMIC) {
        for(size_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
          if(chMaskCntl == 0) {
            // if chMaskCntl == 0, apply the mask by looking at each channel bit
            RADIOLIB_DEBUG_PRINTLN("ADR channel %d: %d --> %d", this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx, this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled, (chMask >> i) & 0x01);
            if(chMask & (1UL << i)) {
              // if it should be enabled but is not currently defined, stop immediately
              if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx == RADIOLIB_LORAWAN_CHANNEL_INDEX_NONE) {
                chMaskAck = 0;
                break;
              }
              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled = true;
            } else {
              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled = false;
            }

          } else if(chMaskCntl == 6) {
            // if chMaskCntl == 6, enable all defined channels
            if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx != RADIOLIB_LORAWAN_CHANNEL_INDEX_NONE) {
              this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled = true;
            }
          }
          
        }
      } else {  // RADIOLIB_LORAWAN_BAND_FIXED
        // delete any prior ADR responses from the uplink queue, but do not care if none is present yet
        (void)deleteMacCommand(RADIOLIB_LORAWAN_MAC_CMD_LINK_ADR, &this->commandsUp);
        RADIOLIB_DEBUG_PRINTLN("mask[%d] = 0x%04x", chMaskCntl, chMask);
        uint8_t num = 0;
        uint8_t chNum = chMaskCntl*16;
        uint8_t chSpan = 0;
        for(size_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
          RADIOLIB_DEBUG_PRINTLN("chNum: %d, chSpan: %d, i: %d, mask: %d", chNum, chSpan, i, chMask & (1UL << i));
          // if we must roll over to next span, reset chNum and move to next channel span
          if(chNum >= this->band->txSpans[chSpan].numChannels) {
            chNum = 0;
            chSpan++;
          }

          if(chMask & (1UL << i)) {
            if(chSpan >= this->band->numTxSpans) {
              RADIOLIB_DEBUG_PRINTLN("channel bitmask overrun!");
              return(RADIOLIB_ERR_UNKNOWN);
            }
            LoRaWANChannel_t chnl;
            chnl.enabled = true;
            chnl.idx   = chMaskCntl*16 + i;
            chnl.freq  = this->band->txSpans[chSpan].freqStart + chNum*this->band->txSpans[chSpan].freqStep;
            chnl.drMin = this->band->txSpans[chSpan].drMin;
            chnl.drMax = this->band->txSpans[chSpan].drMax;
            availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num] = chnl;
            // downlink channels are dynamically calculated on each uplink in selectChannels()
            RADIOLIB_DEBUG_PRINTLN("Channel UL %d (%d) frequency = %f MHz", num, chnl.idx, availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num].freq);
            num++;
          }
          chNum++;
        }
      }
      // TODO should we actually save the channels because the masks may have changed stuff?
      // this may wear the storage quickly on more mobile devices / changing RF environment

      this->nbTrans = nbTrans;

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
      this->rx1DrOffset = (cmd->payload[0] & 0x70) >> 4;
      uint8_t rx1OffsAck = 1;
      this->rx2.drMax = cmd->payload[0] & 0x0F;
      uint8_t rx2Ack = 1;
      uint32_t freqRaw = LoRaWANNode::ntoh<uint32_t>(&cmd->payload[1], 3);
      this->rx2.freq = (float)freqRaw/10000.0;
      RADIOLIB_DEBUG_PRINTLN("Rx param REQ: rx1DrOffset = %d, rx2DataRate = %d, freq = %f", this->rx1DrOffset, this->rx2.drMax, this->rx2.freq);
      
      // apply the configuration
      uint8_t chanAck = 0;
      if(this->phyLayer->setFrequency(this->rx2.freq) == RADIOLIB_ERR_NONE) {
        chanAck = 1;
        this->phyLayer->setFrequency(this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].freq);
      }

      // TODO this should be sent repeatedly until the next downlink
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
      RADIOLIB_DEBUG_PRINTLN("DevStatus ANS: status = 0x%02x%02x", cmd->payload[0], cmd->payload[1]);
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
      RADIOLIB_DEBUG_PRINTLN("New channel: index = %d, freq = %f MHz, maxDr = %d, minDr = %d", chIndex, freq, maxDr, minDr);
      uint8_t newChAck = 0;
      uint8_t freqAck = 0;
      for(int i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
        // find first empty channel and configure this as the new channel
        if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx == RADIOLIB_LORAWAN_CHANNEL_INDEX_NONE) {
          this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled = true;
          this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx   = chIndex;
          this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].freq  = freq;
          this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMin = minDr;
          this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].drMax = maxDr;
          
          // downlink channel is identical to uplink channel
          this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][i] = this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i];
          newChAck = 1;
          
          // check if the frequency is possible
          if(this->phyLayer->setFrequency(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].freq) == RADIOLIB_ERR_NONE) {
            freqAck = 1;
            this->phyLayer->setFrequency(this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].freq);
          }

          break;
        }
      }

#if !defined(RADIOLIB_EEPROM_UNSUPPORTED)
      // update saved frequencies
      this->saveChannels();
#endif

      // send the reply
      cmd->len = 1;
      cmd->payload[0] = (newChAck << 1) | (freqAck << 0);

      pushMacCommand(cmd, &this->commandsUp);

      return(5);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_DL_CHANNEL): {
      // get the configuration
      uint8_t chIndex = cmd->payload[0];
      uint32_t freqRaw = LoRaWANNode::ntoh<uint32_t>(&cmd->payload[1], 3);
      float freq = (float)freqRaw/10000.0;
      RADIOLIB_DEBUG_PRINTLN("DL channel: index = %d, freq = %f MHz", chIndex, freq);
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
      
#if !defined(RADIOLIB_EEPROM_UNSUPPORTED)
      // update saved frequencies
      this->saveChannels();
#endif      

      // send the reply
      cmd->len = 1;
      cmd->payload[0] = (freqUlAck << 1) | (freqDlAck << 0);

      pushMacCommand(cmd, &this->commandsUp);

      return(4);
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
      this->adrLimitExp = (cmd->payload[0] & 0xF0) >> 4;
      this->adrDelayExp = cmd->payload[0] & 0x0F;
      RADIOLIB_DEBUG_PRINTLN("ADR param setup: limitExp = %d, delayExp = %d", this->adrLimitExp, this->adrDelayExp);

      return(1);
    } break;

    case(RADIOLIB_LORAWAN_MAC_CMD_DEVICE_TIME): {
      // TODO implement this - sent by gateway as reply to node request
      uint32_t gpsEpoch = LoRaWANNode::ntoh<uint32_t>(&cmd->payload[0]);
      uint8_t fraction = cmd->payload[4];
      RADIOLIB_DEBUG_PRINTLN("Network time: gpsEpoch = %d s, delayExp = %f", gpsEpoch, (float)fraction/256.0f);
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
                this->selectChannels();
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

#endif
