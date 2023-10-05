#include "LoRaWAN.h"
#include "LoRaWANBands.cpp"
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
  this->rx2 = this->band->rx2;
}

void LoRaWANNode::wipe() {
  Module* mod = this->phyLayer->getMod();
  mod->hal->wipePersistentStorage();
}

int16_t LoRaWANNode::restore() {
  // check the magic value
  Module* mod = this->phyLayer->getMod();
  if(mod->hal->getPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID) != RADIOLIB_LORAWAN_MAGIC) {
    // the magic value is not set, user will have to do perform the join procedure
    return(RADIOLIB_ERR_NETWORK_NOT_JOINED);
  }

  uint16_t nvm_table_version = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TABLE_VERSION_ID);
  // if (RADIOLIB_PERSISTENT_PARAM_LORAWAN_TABLE_VERSION > nvm_table_version) {
  //  // set default values for variables that are new or something
  // }
  (void)nvm_table_version;

  // pull all authentication keys from persistent storage
  this->devAddr = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_ADDR_ID);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_APP_S_KEY_ID), this->appSKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FNWK_SINT_KEY_ID), this->fNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_SNWK_SINT_KEY_ID), this->sNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NWK_SENC_KEY_ID), this->nwkSEncKey, RADIOLIB_AES128_BLOCK_SIZE);

  this->rev = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_VERSION);
  RADIOLIB_DEBUG_PRINTLN("LoRaWAN session: v1.%d", this->rev);
  uint8_t txDrRx2Dr = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXDR_RX2DR_ID);
  uint8_t txPwrCurMax = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXPWR_CUR_MAX_ID);
  uint8_t rx1DrOffDel = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX1_DROFF_DEL_ID);
  uint8_t rx2FreqBuf[3];
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX2FREQ_ID), rx2FreqBuf, 3);
  uint32_t rx2Freq = LoRaWANNode::ntoh<uint32_t>(&rx2FreqBuf[0], 3);
  uint8_t adrLimDel = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_LIM_DEL_ID);
  this->nbTrans = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NBTRANS_ID);

  this->rx2.drMax = (txDrRx2Dr & 0x0F) >> 0;
  this->rx1DrOffset = (rx1DrOffDel & 0xF0) >> 4;
  this->rxDelays[0] = ((rx1DrOffDel & 0x0F) >> 0) * 1000;
  if(this->rxDelays[0] == 0) {
    this->rxDelays[0] = RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS;
  }
  this->rxDelays[1] = this->rxDelays[0] + 1000;
  this->rx2.freq = (float)rx2Freq / 10000.0;
  this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = (txDrRx2Dr & 0xF0) >> 4;
  this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] + this->band->rx1DataRateBase + this->rx1DrOffset;

  uint8_t queueBuff[sizeof(LoRaWANMacCommandQueue_t)] = { 0 };
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FOPTS_ID), queueBuff, sizeof(LoRaWANMacCommandQueue_t));
  memcpy(&this->commandsUp, queueBuff, sizeof(LoRaWANMacCommandQueue_t));
  RADIOLIB_DEBUG_PRINTLN("Number of MAC commands: %d", this->commandsUp.numCommands);
  
  int16_t state = this->restoreChannels();
  RADIOLIB_ASSERT(state);

  state = this->setPhyProperties();
  RADIOLIB_ASSERT(state);

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::beginOTAA(uint64_t joinEUI, uint64_t devEUI, uint8_t* nwkKey, uint8_t* appKey, bool force) {
  // check if we actually need to send the join request
  Module* mod = this->phyLayer->getMod();
  if(!force && (mod->hal->getPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID) == RADIOLIB_LORAWAN_MAGIC)) {
    // the device has joined already, we can just pull the data from persistent storage
    return(this->restore());
  }

  // set the physical layer configuration
  int16_t state = this->setPhyProperties();
  RADIOLIB_ASSERT(state);

  // get dev nonce from persistent storage and increment it
  uint16_t devNonce = mod->hal->getPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_NONCE_ID);
  mod->hal->setPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_NONCE_ID, devNonce + 1);

  // setup uplink/downlink frequencies and datarates
  state = this->selectChannelsJR(devNonce);
  RADIOLIB_ASSERT(state);

  // configure for uplink with default configuration
  state = this->configureChannel(RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK);
  RADIOLIB_ASSERT(state);

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
  state = this->phyLayer->startReceive(0x00, 0b0000001001100010, 0b0000000000000010, 0);
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
  if((joinNoncePrev > 0) && (joinNonce <= joinNoncePrev)) {
    return(RADIOLIB_ERR_JOIN_NONCE_INVALID);
  }

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
  if(lenRx == RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN) {
    uint8_t cfList[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN] = { 0 };
    memcpy(&cfList[0], &joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_POS], RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN);
    this->setupChannels(cfList);
  } else {
    this->setupChannels(nullptr);
  }
  this->saveChannels();

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
      .repeat = 0x01 << RADIOLIB_LORAWAN_ADR_ACK_LIMIT_EXP,
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
  
  // store session configuration
  mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_VERSION, this->rev);
  uint8_t txDrRx2Dr = (this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] << 4) | this->rx2.drMax;
  uint8_t txPwrCurMax;
  uint8_t rx1DrOffDel = (this->rx1DrOffset << 4) | (this->rxDelays[0] / 1000);
  uint32_t rx2Freq = uint32_t(this->rx2.freq * 10000);
  uint8_t rx2FreqBuf[3];
  LoRaWANNode::hton<uint32_t>(&rx2FreqBuf[0], rx2Freq, 3);
  uint8_t adrLimDel = (RADIOLIB_LORAWAN_ADR_ACK_LIMIT_EXP << 4) | (RADIOLIB_LORAWAN_ADR_ACK_DELAY_EXP << 0);
  uint8_t nbTrans = (0x01 << 0);
  mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXDR_RX2DR_ID, txDrRx2Dr);
  mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXPWR_CUR_MAX_ID, txPwrCurMax);
  mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX1_DROFF_DEL_ID, rx1DrOffDel);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX2FREQ_ID), rx2FreqBuf, 3);
  mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_LIM_DEL_ID, adrLimDel);
  mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NBTRANS_ID, nbTrans);

  // save the device address & keys
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_ADDR_ID, this->devAddr);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_APP_S_KEY_ID), this->appSKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FNWK_SINT_KEY_ID), this->fNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_SNWK_SINT_KEY_ID), this->sNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NWK_SENC_KEY_ID), this->nwkSEncKey, RADIOLIB_AES128_BLOCK_SIZE);

  // save uplink parameters
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_HOME_NET_ID, homeNetId);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_JOIN_NONCE_ID, joinNonce);

  // all complete, reset all frame counters and set the magic number
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_A_FCNT_DOWN_ID, 0);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_N_FCNT_DOWN_ID, 0);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_CONF_FCNT_UP_ID, 0);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_CONF_FCNT_DOWN_ID, 0);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_FCNT_ID, 0);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID, 0);
  mod->hal->setPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID, RADIOLIB_LORAWAN_MAGIC);

  // everything written to NVM, write current table version to NVM
  mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TABLE_VERSION_ID, RADIOLIB_PERSISTENT_PARAM_LORAWAN_TABLE_VERSION);
  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::beginABP(uint32_t addr, uint8_t* nwkSKey, uint8_t* appSKey, uint8_t* fNwkSIntKey, uint8_t* sNwkSIntKey, bool force) {
  // check if we actually need to restart from a clean session
  Module* mod = this->phyLayer->getMod();
  if(!force && (mod->hal->getPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID) == RADIOLIB_LORAWAN_MAGIC)) {
    // the device has joined already, we can just pull the data from persistent storage
    return(this->restore());
  }
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
  state = this->setupChannels(nullptr);
  RADIOLIB_ASSERT(state);

  // everything written to NVM, write current version to NVM
  mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TABLE_VERSION_ID, RADIOLIB_PERSISTENT_PARAM_LORAWAN_TABLE_VERSION);

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
  bool adrEn = true;
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
  if(len > this->band->payloadLenMax[this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK]]) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // get frame counter from persistent storage
  uint32_t fcnt = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID) + 1;
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID, fcnt);
  uint32_t adrFcnt = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_FCNT_ID);
  uint8_t adrParams = mod->hal->getPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_LIM_DEL_ID);
  uint32_t adrLimit = 0x01 << ((adrParams & 0xF0) >> 4);
  uint32_t adrDelay = 0x01 << ((adrParams & 0x0F) >> 0);
  bool adrAckReq = false;
  if((fcnt - adrFcnt) == adrLimit) {
    adrAckReq = true;
    // add MAC command to queue
  } else if ((fcnt - adrFcnt) == (adrLimit + adrDelay)) {
    // set TX power to max

    // decrease DR if possible
    if(this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] > this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK].drMin) {
      this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK]--;
    } else {
      // enable all channels
    }
  }

  // configure for uplink
  this->selectChannels();
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

  // length of fopts will be added later
  uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] = 0x00;
  if(adrEn) {
    uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] |= RADIOLIB_LORAWAN_FCTRL_ADR_ENABLED;
    if(adrAckReq) {
      uplinkMsg[RADIOLIB_LORAWAN_FHDR_FCTRL_POS] |= RADIOLIB_LORAWAN_FCTRL_ADR_ACK_REQ;
    }
  }

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
      for(size_t i = 0; i < cmd.len; i++) {
        foptsBuff[idx + 1 + i] = cmd.payload[i];
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
    memcpy(queueBuff, &this->commandsUp, sizeof(LoRaWANMacCommandQueue_t));
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
  const uint32_t scanGuard = 10;
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
    RADIOLIB_DEBUG_PRINTLN("Waiting for %d ms...", waitLen);
    mod->hal->delay(waitLen);
    RADIOLIB_DEBUG_PRINTLN("Opening Rx%d window...", (i+1));

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
        
        // channel scan is finished, swap the actions
        this->phyLayer->clearChannelScanAction();
        this->phyLayer->setPacketReceivedAction(LoRaWANNodeOnDownlink);

        // start receiving
        state = this->phyLayer->startReceive(0x00, 0b0000001001100010, 0b0000000000000010, 0);   // RxSingle
        RADIOLIB_ASSERT(state);

        break;
      }
    
    }

    // check if we have a packet
    if(packetDetected) {
      RADIOLIB_DEBUG_PRINTLN("Detected a packet...");
      break;

    } else if(i == 0) {
      // nothing in the first window, configure for the second
      state = this->phyLayer->setFrequency(this->rx2.freq);
      RADIOLIB_ASSERT(state);

      DataRate_t dataRate;
      findDataRate(this->rx2.drMax, &dataRate);
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

    return(RADIOLIB_ERR_RX_TIMEOUT);
  }

  // wait for reception with some timeout
  uint32_t rxStart = mod->hal->millis();
  while(!downlinkReceived) {
    mod->hal->yield();
    // 3 seconds is maximum airtime
    if(mod->hal->millis() - rxStart >= 3000) {
      // timed out
      RADIOLIB_DEBUG_PRINTLN("Packet length: %d", this->phyLayer->getPacketLength());
      this->phyLayer->standby();
      if(!this->FSK) {
        this->phyLayer->invertIQ(false);
      }
      // return(RADIOLIB_ERR_RX_TIMEOUT);
      break;
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

  // if this is not the first downlink...
  // assume a 16-bit to 32-bit rollover if difference between counters in LSB is smaller than MAX_FCNT_GAP
  // if that isn't the case and the received fcnt is smaller or equal to the last heard fcnt, then error
  if(fcntDownPrev > 0) {
    if((fcnt16 <= fcntDownPrev) && ((0xFFFF - (uint16_t)fcntDownPrev + fcnt16) > RADIOLIB_LORAWAN_MAX_FCNT_GAP)) {
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
    }

    // write the MAC command queue to nvm for next uplink
    uint8_t queueBuff[sizeof(LoRaWANMacCommandQueue_t)];
    memcpy(queueBuff, &this->commandsUp, sizeof(LoRaWANMacCommandQueue_t));
    mod->hal->writePersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FOPTS_ID), queueBuff, sizeof(LoRaWANMacCommandQueue_t));
  }

  // a downlink was received, so reset the ADR counter to this uplink's fcnt
  uint32_t fcntUp = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_FCNT_ID, fcntUp);

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

  return(RADIOLIB_ERR_NONE);
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
  // if(this->FSK) {
  //   // for FSK, configure the channel
  //   state = this->phyLayer->setFrequency(this->band->fskFreq);
  //   RADIOLIB_ASSERT(state);
  //   DataRate_t dr;
  //   dr.fsk.bitRate = 50;
  //   dr.fsk.freqDev = 25;
  //   state = this->phyLayer->setDataRate(dr);
  //   RADIOLIB_ASSERT(state);
  //   state = this->phyLayer->setDataShaping(RADIOLIB_SHAPING_1_0);
  //   RADIOLIB_ASSERT(state);
  //   state = this->phyLayer->setEncoding(RADIOLIB_ENCODING_WHITENING);
  // }
  // RADIOLIB_ASSERT(state);
  
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

int16_t LoRaWANNode::setupChannels(uint8_t* cfList) {
  uint8_t num = 0;
  LoRaWANChannel_t chnl;
  
  // in case of frequency list-type band, copy the default TX channels into the available channels, with RX1 = TX
  if(this->band->cfListType == RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES) {
    for(uint8_t i = 0; i < 3; i++) {
      chnl = this->band->txFreqs[i];
      if(chnl.enabled) {
        availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num] = chnl;
        availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][num] = chnl;
        RADIOLIB_DEBUG_PRINTLN("Channel UL/DL %d frequency = %f MHz", num, availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num].freq);
        num++;
      }
    }
    // if(cfList != nullptr) {
    //   for(uint8_t i = 0; i < 5; i++) {
    //     chnl.enabled = true;
    //     chnl.idx = num;
    //     uint32_t freq = LoRaWANNode::ntoh<uint32_t>(&cfList[3*i], 3);
    //     chnl.freq = (float)freq/10000.0;
    //     chnl.drMin = this->band->txFreqs[0].drMin;  // drMin is equal for all channels
    //     chnl.drMax = this->band->txFreqs[0].drMax;  // drMax is equal for all channels
    //     availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num] = chnl;
    //     availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][num] = chnl;
    //     RADIOLIB_DEBUG_PRINTLN("Channel UL/DL %d frequency = %f MHz", num, availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num].freq);
    //     num++;
    //   }
    // }
  } else {                  // RADIOLIB_LORAWAN_CFLIST_TYPE_MASK
    uint8_t chSpan = 0;
    uint8_t chNum = 0;
    // in case of mask-type bands, copy those frequencies that are masked true into the available TX channels
    for(uint8_t i = 0; i < 5; i++) {
      uint16_t mask = LoRaWANNode::ntoh<uint16_t>(&cfList[2*i]);
      RADIOLIB_DEBUG_PRINTLN("mask[%d] = 0x%04x", i, mask);
      for(uint8_t j = 0; j < 16; j++) {
        // if we must roll over to next span, reset chNum and move to next channel span
        if(chNum >= this->band->txSpans[chSpan].numChannels) {
          chNum = 0;
          chSpan++;
        }

        if(mask & (1UL << j)) {
          if(chSpan >= this->band->numTxSpans || chNum >= this->band->txSpans[chSpan].numChannels) {
            RADIOLIB_DEBUG_PRINTLN("channel bitmask overrun!");
            return(RADIOLIB_ERR_UNKNOWN);
          }
          chnl.enabled = true;
          chnl.idx   = i*16 + j;
          chnl.freq  = this->band->txSpans[chSpan].freqStart + chNum*this->band->txSpans[chSpan].freqStep;
          chnl.drMin = this->band->txSpans[chSpan].drMin;
          chnl.drMax = this->band->txSpans[chSpan].drMax;
          availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num] = chnl;
          RADIOLIB_DEBUG_PRINTLN("Channel UL %d frequency = %f MHz", num, availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][num].freq);
          num++;
        }
        chNum++;
      }
    }
  }
  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::selectChannelsJR(uint16_t devNonce) {
  LoRaWANChannel_t channelUp;
  LoRaWANChannel_t channelDn;
  if(this->band->cfListType == RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES) {
    // count the number of available channels for a join-request
    uint8_t numJRChannels = 0;
    for(uint8_t i = 0; i < 3; i++) {
      if(this->band->txFreqs[i].idx != RADIOLIB_LORAWAN_CHANNEL_INDEX_NONE) {
        numJRChannels++;
      }
      if(this->band->txJoinReq[i].idx != RADIOLIB_LORAWAN_CHANNEL_INDEX_NONE) {
        numJRChannels++;
      }
    }
    uint8_t channelId = devNonce % numJRChannels;                 // cycle through channels (seed with devNonce)
    if(channelId < 3) {
      channelUp = this->band->txFreqs[channelId];
    } else {
      channelUp = this->band->txJoinReq[channelId - 3];
    }
    channelDn = channelUp;                                        // RX1 is equal to TX

    // configure data rates for TX and RX1: for TX the (floored) average of min and max; for RX1 identical with base offset
    this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = int((channelUp.drMax + channelUp.drMin) / 2);
    // TODO check bounds
    this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = getDownlinkDataRate(this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK], 
                                                                                 this->rx1DrOffset, this->band->rx1DataRateBase, channelDn.drMin, channelDn.drMax);
  } else {                  // RADIOLIB_LORAWAN_CFLIST_TYPE_MASK
    channelUp.enabled = true;
    uint8_t numBlocks = this->band->txSpans[0].numChannels / 8;   // calculate number of 8-channel blocks
    uint8_t numBlockChannels = 8 + this->band->txSpans[1].numChannels > 0 ? 1 : 0;  // add a 9th channel if there's a second span
    uint8_t blockID = devNonce % numBlocks;                       // currently selected block (seed with devNonce)
    uint8_t channelID = this->phyLayer->random(numBlockChannels); // select randomly from these 8 or 9 channels
    uint8_t spanID;
    if(channelID < 8) {
      spanID = 0;
      channelUp.idx = blockID * numBlockChannels + channelID;
    } else {
      spanID = 1;
      channelUp.idx = blockID;
    }
    channelUp.freq = this->band->txSpans[spanID].freqStart + channelUp.idx*this->band->txSpans[spanID].freqStep;

    channelDn.idx = blockID % this->band->rx1Span.numChannels;
    channelDn.freq = this->band->rx1Span.freqStart + channelDn.idx*this->band->rx1Span.freqStep;
    
    // configure data rates for TX and RX1: for TX the specified value for this band; for RX1 identical with base offset
    this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = this->band->txSpans[spanID].joinRequestDataRate;
    // TODO check bounds
    this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = getDownlinkDataRate(this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK], 
                                                                                 this->rx1DrOffset, this->band->rx1DataRateBase, channelDn.drMin, channelDn.drMax);
  }
  this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK]   = channelUp;
  this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = channelDn;

  Module* mod = this->phyLayer->getMod();
  uint8_t txDrRx2Dr = (this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] << 4) | this->rx2.drMax;
  mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXDR_RX2DR_ID, txDrRx2Dr);
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
  // select a random ID & channel from the list of enabled and possible channels
  uint8_t channelID = channelsEnabled[this->phyLayer->random(numChannels)];
  this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][channelID];
  
  // in case of frequency list-type band, downlink is equal to uplink, otherwise retrieve `modulo` numChannels
  if(this->band->cfListType == RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES) {
    this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK][channelID];
  } else {                  // RADIOLIB_LORAWAN_CFLIST_TYPE_MASK
    LoRaWANChannel_t channelDn;
    channelDn.enabled = true;
    channelDn.idx = this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK].idx % this->band->rx1Span.numChannels;
    channelDn.freq = this->band->rx1Span.freqStart + channelDn.idx*this->band->rx1Span.freqStep;
    channelDn.drMin = this->band->rx1Span.drMin;
    channelDn.drMax = this->band->rx1Span.drMax;
    this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = channelDn;
  }
  return(RADIOLIB_ERR_NONE);
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
  RADIOLIB_DEBUG_PRINTLN("Datarate index: %d", this->dataRates[dir]);
  int state = this->phyLayer->setFrequency(this->currentChannels[dir].freq);
  RADIOLIB_ASSERT(state);

  // set the data rate
  DataRate_t dataRate;
  findDataRate(this->dataRates[dir], &dataRate);

  state = this->phyLayer->setDataRate(dataRate);
  return(state);
}

int16_t LoRaWANNode::saveChannels() {
  uint8_t bytesPerChannel = 5;
  uint8_t numBytes = 2 * RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS * bytesPerChannel;
  uint8_t buffer[numBytes];
  for(uint8_t dir = 0; dir < 2; dir++) {
    for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
      uint8_t chBuff[5] = { 0 };
      chBuff[0] |= (uint8_t)this->availableChannels[dir][i].enabled << 7;
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

int16_t LoRaWANNode::restoreChannels() {
  uint8_t bytesPerChannel = 5;
  uint8_t numBytes = 2 * RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS * bytesPerChannel;
  uint8_t buffer[numBytes];
  Module* mod = this->phyLayer->getMod();
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FREQS_ID), buffer, numBytes);
  for(uint8_t dir = 0; dir < 2; dir++) {
    for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
      uint8_t chBuff[5] = { 0 };
      memcpy(chBuff, &buffer[(dir * RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS * bytesPerChannel) + i * bytesPerChannel], bytesPerChannel);
      this->availableChannels[dir][i].enabled = (chBuff[0] & 0x80) >> 7;
      this->availableChannels[dir][i].idx = chBuff[0] & 0x7F;
      uint32_t freq = LoRaWANNode::ntoh<uint32_t>(&chBuff[1], 3);
      this->availableChannels[dir][i].freq = (float)freq/10000.0;
      this->availableChannels[dir][i].drMax = (chBuff[0] & 0xF0) >> 4;
      this->availableChannels[dir][i].drMin = (chBuff[0] & 0x0F) >> 0;
    }
  }
  return(RADIOLIB_ERR_NONE);
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
  Module* mod = this->phyLayer->getMod();

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
      if(dr == 0x0F) {
        drAck = 1;
      } else if (this->band->dataRates[dr] != RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
        this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] = dr;
        this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = dr + this->band->rx1DataRateBase - this->rx1DrOffset;
        if(this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] < this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].drMin) {
          this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].drMin;
        } else if(this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] > this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].drMax) {
          this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK] = this->currentChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_DOWNLINK].drMax;
        }

        uint8_t txDrRx2Dr = (this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] << 4) | this->rx2.drMax;
        mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXDR_RX2DR_ID, txDrRx2Dr);
        drAck = 1;
      } 

      // try to apply the power configuration
      uint8_t pwrAck = 0;
      if(txPower == 0x0F) {
        pwrAck = 1;
      } else {
        int8_t pwr = this->band->powerMax - 2*txPower;
        if(this->phyLayer->setOutputPower(pwr) == RADIOLIB_ERR_NONE) {
          RADIOLIB_DEBUG_PRINTLN("ADR set pwr = %d", pwr);
          pwrAck = 1;
        }
      }

      this->nbTrans = nbTrans;
      // TODO implement channel mask
      uint8_t chMaskAck = 1;
      (void)chMask;
      (void)chMaskCntl;
      if(this->band->cfListType == RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES) {
        for(uint8_t i = 0; i < 16; i++) {
          // check if this channel ID should be enabled
          if(chMask & (1UL << i)) {
            // if it should be enabled but is not currently defined, stop immediately
            if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled == false) {
              chMaskAck = 0;
              break;
            }
          } else {
            this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled = false;
          }
        }
      } else {

      }

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

      // update saved values
      uint8_t txDrRx2Dr = (this->dataRates[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK] << 4) | this->rx2.drMax;
      uint8_t rx1DrOffDel = (this->rx1DrOffset << 4) | (this->rxDelays[0] / 1000);
      mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_TXDR_RX2DR_ID, txDrRx2Dr);
      mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX1_DROFF_DEL_ID, rx1DrOffDel);

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
      RADIOLIB_DEBUG_PRINTLN("New channel: index = %d, freq = %f MHz, maxDr = %d, minDr = %d", chIndex, freq, maxDr, minDr);
      uint8_t newChAck = 0;
      uint8_t freqAck = 0;
      for(int i = 0; i < RADIOLIB_LORAWAN_NUM_AVAILABLE_CHANNELS; i++) {
        // find first empty channel and configure this as the new channel
        if(this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx == 0) {
          this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].enabled = true;
          this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].idx = chIndex;
          this->availableChannels[RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK][i].freq = freq;
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

      // update saved frequencies
      this->saveChannels();

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
      
      // update saved frequencies
      this->saveChannels();
      
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

      // update saved values
      uint8_t rx1DrOffDel = (this->rx1DrOffset << 4) | (this->rxDelays[0] / 1000);
      mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_RX1_DROFF_DEL_ID, rx1DrOffDel);

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
      uint8_t limitExp = (cmd->payload[0] & 0xF0) >> 4;
      uint8_t delayExp = cmd->payload[0] & 0x0F;
      RADIOLIB_DEBUG_PRINTLN("ADR param setup: limitExp = %d, delayExp = %d", limitExp, delayExp);
      
      // update saved values
      mod->hal->setPersistentParameter<uint8_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_ADR_LIM_DEL_ID, cmd->payload[0]);

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
