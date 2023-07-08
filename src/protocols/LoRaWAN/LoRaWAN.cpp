#include "LoRaWAN.h"

#include <string.h>

#if !defined(RADIOLIB_EXCLUDE_LORAWAN)

// flag to indicate whether we have received a downlink
static volatile bool downlinkReceived = false;

// interrupt service routine to handle downlinks automatically
#if defined(ESP8266) || defined(ESP32)
  IRAM_ATTR
#endif
static void LoRaWANNodeOnDownlink(void) {
  downlinkReceived = true;
}

LoRaWANNode::LoRaWANNode(PhysicalLayer* phy, const LoRaWANBand_t* band) {
  this->phyLayer = phy;
  this->band = band;
}

void LoRaWANNode::wipe() {
  Module* mod = this->phyLayer->getMod();
  mod->hal->wipePersistentStorage();
}

int16_t LoRaWANNode::begin() {
  int16_t state = this->setPhyProperties();
  RADIOLIB_ASSERT(state);

  // check the magic value
  Module* mod = this->phyLayer->getMod();
  if(mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID) != RADIOLIB_LORAWAN_MAGIC) {
    // the magic value is not set, user will have to do perform the join procedure
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }

  // pull all needed information from persistent storage
  this->devAddr = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_ADDR_ID);
  RADIOLIB_DEBUG_PRINTLN("devAddr = 0x%08x", this->devAddr);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_APP_S_KEY_ID), this->appSKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FNWK_SINT_KEY_ID), this->fNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_SNWK_SINT_KEY_ID), this->sNwkSIntKey, RADIOLIB_AES128_BLOCK_SIZE);
  mod->hal->readPersistentStorage(mod->hal->getPersistentAddr(RADIOLIB_PERSISTENT_PARAM_LORAWAN_NWK_SENC_KEY_ID), this->nwkSEncKey, RADIOLIB_AES128_BLOCK_SIZE);
  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::beginOTAA(uint64_t appEUI, uint64_t devEUI, uint8_t* nwkKey, uint8_t* appKey, bool force) {
  // check if we actually need to send the join request
  Module* mod = this->phyLayer->getMod();
  if(!force && (mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID) == RADIOLIB_LORAWAN_MAGIC)) {
    // the device has joined already, we can just pull the data from persistent storage
    return(this->begin());
  }

  // set the physical layer configuration
  int16_t state = this->setPhyProperties();
  RADIOLIB_ASSERT(state);

  // get dev nonce from persistent storage and increment it
  uint16_t devNonce = mod->hal->getPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_NONCE_ID);
  mod->hal->setPersistentParameter<uint16_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_DEV_NONCE_ID, devNonce + 1);
  RADIOLIB_DEBUG_PRINTLN("devNonce = %d", devNonce);

  // build the join-request message
  uint8_t joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_LEN];
  
  // set the packet fields
  joinRequestMsg[0] = RADIOLIB_LORAWAN_MHDR_MTYPE_JOIN_REQUEST | RADIOLIB_LORAWAN_MHDR_MAJOR_R1;
  LoRaWANNode::hton<uint64_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_JOIN_EUI_POS], appEUI);
  LoRaWANNode::hton<uint64_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_DEV_EUI_POS], devEUI);
  LoRaWANNode::hton<uint16_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_DEV_NONCE_POS], devNonce);

  // add the authentication code
  uint32_t mic = this->generateMIC(joinRequestMsg, RADIOLIB_LORAWAN_JOIN_REQUEST_LEN - sizeof(uint32_t), nwkKey);
  LoRaWANNode::hton<uint32_t>(&joinRequestMsg[RADIOLIB_LORAWAN_JOIN_REQUEST_LEN - sizeof(uint32_t)], mic);

  // send it
  state = this->phyLayer->transmit(joinRequestMsg, RADIOLIB_LORAWAN_JOIN_REQUEST_LEN);
  RADIOLIB_ASSERT(state);
  
  // set the function that will be called when the reply is received
  this->phyLayer->setPacketReceivedAction(LoRaWANNodeOnDownlink);

  // downlink messages are sent with interted IQ
  state = this->phyLayer->invertIQ(true);
  RADIOLIB_ASSERT(state);

  // start receiving
  uint32_t start = mod->hal->millis();
  downlinkReceived = false;
  state = this->phyLayer->startReceive();
  RADIOLIB_ASSERT(state);

  // wait for the reply or timeout
  while(!downlinkReceived) {
    if(mod->hal->millis() - start >= RADIOLIB_LORAWAN_JOIN_ACCEPT_DELAY_2_MS + 2000) {
      downlinkReceived = false;
      this->phyLayer->invertIQ(false);
      return(RADIOLIB_ERR_RX_TIMEOUT);
    }
  }

  // we have a message, reset the IQ inversion
  downlinkReceived = false;
  this->phyLayer->clearPacketReceivedAction();
  state = this->phyLayer->invertIQ(false);
  RADIOLIB_ASSERT(state);
  
  // build the buffer for the reply data
  uint8_t joinAcceptMsgEnc[RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN];

  // check received length
  size_t lenRx = this->phyLayer->getPacketLength(true);
  if((lenRx != RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN) && (lenRx != RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN - RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN)) {
    RADIOLIB_DEBUG_PRINTLN("joinAccept reply length mismatch, expected %luB got %luB", RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN, lenRx);
    return(RADIOLIB_ERR_RX_TIMEOUT);
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
    return(RADIOLIB_ERR_RX_TIMEOUT);
  }

  // decrypt the join accept message
  // this is done by encrypting again in ECB mode
  // the first byte is the MAC header which is not encrpyted
  uint8_t joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN];
  joinAcceptMsg[0] = joinAcceptMsgEnc[0];
  RadioLibAES128Instance.init(nwkKey);
  RadioLibAES128Instance.encryptECB(&joinAcceptMsgEnc[1], RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN - 1, &joinAcceptMsg[1]);

  //Module::hexdump(joinAcceptMsg, RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN);

  // verify MIC
  if(!verifyMIC(joinAcceptMsg, lenRx, nwkKey)) {
    return(RADIOLIB_ERR_CRC_MISMATCH);
  }

  // parse the contents
  uint32_t joinNonce = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_NONCE_POS], 3);
  uint32_t homeNetId = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_HOME_NET_ID_POS], 3);
  this->devAddr = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_ADDR_POS]);
  uint8_t dlSettings = joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_DL_SETTINGS_POS];
  this->rxDelay = joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_RX_DELAY_POS];

  // process CFlist if present
  if(lenRx == RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN) {
    if(this->band->cfListType == RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES) {
      // list of frequencies
      for(uint8_t i = 0; i < 5; i++) {
        uint32_t freq = LoRaWANNode::ntoh<uint32_t>(&joinAcceptMsg[RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_POS + 3*i], 3);
        availableChannelsFreq[i] = (float)freq/10000.0;
        RADIOLIB_DEBUG_PRINTLN("Channel %d frequency = %f MHz", i, availableChannelsFreq[i]);
      }

    } else {
      // TODO list of masks
      RADIOLIB_DEBUG_PRINTLN("CFlist masks not supported (yet)");
      return(RADIOLIB_ERR_UNSUPPORTED_ENCODING);

    }
  
  }

  RADIOLIB_DEBUG_PRINTLN("joinNonce = %lu", joinNonce);
  RADIOLIB_DEBUG_PRINTLN("homeNetId = %lu", homeNetId);
  RADIOLIB_DEBUG_PRINTLN("devAddr = 0x%08x", devAddr);
  RADIOLIB_DEBUG_PRINTLN("dlSettings = 0x%02x", dlSettings);
  RADIOLIB_DEBUG_PRINTLN("rxDelay = %d", this->rxDelay);

  // prepare buffer for key derivation
  uint8_t keyDerivationBuff[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  LoRaWANNode::hton<uint32_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_NONCE_POS], joinNonce, 3);
  LoRaWANNode::hton<uint32_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_HOME_NET_ID_POS], homeNetId, 3);
  LoRaWANNode::hton<uint16_t>(&keyDerivationBuff[RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_ADDR_POS], devNonce);

  // check protocol version (1.0 vs 1.1)
  if(dlSettings & RADIOLIB_LORAWAN_JOIN_ACCEPT_R_1_1) {
    // TODO implement 1.1
    this->rev = 1;
    RADIOLIB_DEBUG_PRINTLN("LoRaWAN 1.1 not supported (yet)");
    (void)appKey;
    return(RADIOLIB_ERR_UNSUPPORTED_ENCODING);
  
  } else {
    // 1.0 version, just derive the keys
    this->rev = 0;
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

  // all complete, reset device counters and set the magic number
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID, 0);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_MAGIC_ID, RADIOLIB_LORAWAN_MAGIC);
  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::beginAPB(uint32_t addr, uint8_t net, uint8_t* nwkSKey, uint8_t* appSKey) {
  this->devAddr = (((uint32_t)net << 25) & 0xFE000000) | (addr & 0x01FFFFFF);
  memcpy(this->appSKey, appSKey, RADIOLIB_AES128_KEY_SIZE);
  memcpy(this->nwkSEncKey, nwkSKey, RADIOLIB_AES128_KEY_SIZE);
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
  RADIOLIB_CHECK_RANGE(port, 0x01, 0xDF, RADIOLIB_ERR_INVALID_PAYLOAD);

  // check maximum payload len as defiend in phy
  // TODO implement Fopts
  uint8_t foptsLen = 0;
  if(len > this->band->payloadLenMax[this->dataRate]) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // build the uplink message
  // the first 16 bytes are reserved for MIC calculation blocks
  size_t uplinkMsgLen = RADIOLIB_LORAWAN_UPLINK_LEN(len, foptsLen);
  #if defined(RADIOLIB_STATIC_ONLY)
  uint8_t uplinkMsg[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
  uint8_t* uplinkMsg = new uint8_t[uplinkMsgLen];
  #endif
  
  // set the packet fields
  uplinkMsg[RADIOLIB_LORAWAN_UPLINK_LEN_START_OFFS] = RADIOLIB_LORAWAN_MHDR_MTYPE_UNCONF_DATA_UP | RADIOLIB_LORAWAN_MHDR_MAJOR_R1;
  LoRaWANNode::hton<uint32_t>(&uplinkMsg[RADIOLIB_LORAWAN_UPLINK_DEV_ADDR_POS], this->devAddr);

  // TODO implement adaptive data rate
  uplinkMsg[RADIOLIB_LORAWAN_UPLINK_FCTRL_POS] = 0x00;

  // get frame counter from persistent storage
  Module* mod = this->phyLayer->getMod();
  uint32_t fcnt = mod->hal->getPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID);
  mod->hal->setPersistentParameter<uint32_t>(RADIOLIB_PERSISTENT_PARAM_LORAWAN_FCNT_UP_ID, fcnt + 1);
  LoRaWANNode::hton<uint16_t>(&uplinkMsg[RADIOLIB_LORAWAN_UPLINK_FCNT_POS], (uint16_t)fcnt);

  // TODO implement FOpts
  uplinkMsg[RADIOLIB_LORAWAN_UPLINK_FPORT_POS(foptsLen)] = port;

  // select encryption key based on the target port
  uint8_t* encKey = this->appSKey;
  if(port == 0) {
    encKey = this->nwkSEncKey;
  }

  // figure out how many encryption blocks are there
  size_t numBlocks = len/RADIOLIB_AES128_BLOCK_SIZE;
  if(len % RADIOLIB_AES128_BLOCK_SIZE) {
    numBlocks++;
  }

  // generate the encryption blocks
  uint8_t encBuffer[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  uint8_t encBlock[RADIOLIB_AES128_BLOCK_SIZE] = { 0 };
  encBlock[RADIOLIB_LORAWAN_BLOCK_MAGIC_POS] = RADIOLIB_LORAWAN_ENC_BLOCK_MAGIC;
  encBlock[RADIOLIB_LORAWAN_BLOCK_DIR_POS] = RADIOLIB_LORAWAN_CHANNEL_DIR_UPLINK;
  LoRaWANNode::hton<uint32_t>(&encBlock[RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS], this->devAddr);
  LoRaWANNode::hton<uint32_t>(&encBlock[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], fcnt);

  //Module::hexdump(uplinkMsg, uplinkMsgLen);

  // now encrypt the payload
  size_t remLen = len;
  for(size_t i = 0; i < numBlocks; i++) {
    encBlock[RADIOLIB_LORAWAN_ENC_BLOCK_COUNTER_POS] = i + 1;

    // encrypt the buffer
    RadioLibAES128Instance.init(encKey);
    RadioLibAES128Instance.encryptECB(encBlock, RADIOLIB_AES128_BLOCK_SIZE, encBuffer);

    // now xor the buffer with the payload
    size_t xorLen = remLen;
    if(xorLen > RADIOLIB_AES128_BLOCK_SIZE) {
      xorLen = RADIOLIB_AES128_BLOCK_SIZE;
    }
    for(uint8_t j = 0; j < xorLen; j++) {
      uplinkMsg[RADIOLIB_LORAWAN_UPLINK_PAYLOAD_POS(foptsLen) + i*RADIOLIB_AES128_BLOCK_SIZE + j] = data[i*RADIOLIB_AES128_BLOCK_SIZE + j] ^ encBuffer[j];
    }
    remLen -= xorLen;
  }

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
  block1[RADIOLIB_LORAWAN_MIC_DATA_RATE_POS] = this->dataRate;
  block1[RADIOLIB_LORAWAN_MIC_CH_INDEX_POS] = this->chIndex;

  //Module::hexdump(uplinkMsg, uplinkMsgLen);

  // calculate authentication codes
  memcpy(uplinkMsg, block1, RADIOLIB_AES128_BLOCK_SIZE);
  uint32_t micS = this->generateMIC(uplinkMsg, uplinkMsgLen - sizeof(uint32_t), this->sNwkSIntKey);
  memcpy(uplinkMsg, block0, RADIOLIB_AES128_BLOCK_SIZE);
  uint32_t micF = this->generateMIC(uplinkMsg, uplinkMsgLen - sizeof(uint32_t), this->fNwkSIntKey);

  // check LoRaWAN revision
  if(this->rev == 1) {
    uint32_t mic = ((uint32_t)(micS & 0x00FF) << 24) | ((uint32_t)(micS & 0xFF00) << 8) | ((uint32_t)(micF & 0xFF00) >> 8) | ((uint32_t)(micF & 0x00FF) << 8);
    LoRaWANNode::hton<uint32_t>(&uplinkMsg[uplinkMsgLen - sizeof(uint32_t)], mic);
  } else {
    LoRaWANNode::hton<uint32_t>(&uplinkMsg[uplinkMsgLen - sizeof(uint32_t)], micF);
  }

  //Module::hexdump(uplinkMsg, uplinkMsgLen);

  // send it (without the MIC calculation blocks)
  int16_t state = this->phyLayer->transmit(&uplinkMsg[RADIOLIB_LORAWAN_UPLINK_LEN_START_OFFS], uplinkMsgLen - RADIOLIB_LORAWAN_UPLINK_LEN_START_OFFS);
  #if !defined(RADIOLIB_STATIC_ONLY)
  delete[] uplinkMsg;
  #endif
  RADIOLIB_ASSERT(state);

  // TODO implement listening for downlinks in RX1/RX2 slots

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANNode::configureChannel(uint8_t chan, uint8_t dr) {
  // find the span based on the channel ID
  uint8_t span = 0;
  uint8_t spanChannelId = 0;
  bool found = false;
  for(uint8_t chanCtr = 0; span < this->band->numChannelSpans; span++) {
    for(; spanChannelId < this->band->defaultChannels[span].numChannels; spanChannelId++) {
      if(chanCtr >= chan) {
        found = true;
        break;
      }
      chanCtr++;
    }
    if(found) {
      break;
    }
  }

  if(!found) {
    return(RADIOLIB_ERR_INVALID_MODULATION_PARAMETERS);
  }

  this->chIndex = chan;
  RADIOLIB_DEBUG_PRINTLN("Channel span %d, channel %d", span, spanChannelId);

  // set the frequency
  float freq = this->band->defaultChannels[span].freqStart + this->band->defaultChannels[span].freqStep * (float)spanChannelId;
  RADIOLIB_DEBUG_PRINTLN("Frequency %f MHz", freq);
  int state = this->phyLayer->setFrequency(freq);
  RADIOLIB_ASSERT(state);

  // set the data rate
  uint8_t dataRateBand = this->band->defaultChannels[span].dataRates[dr];
  this->dataRate = dr;
  if(dr == RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
    // find the first usable data rate
    for(uint8_t i = 0; i < RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES; i++) {
      if(this->band->defaultChannels[span].dataRates[i] != RADIOLIB_LORAWAN_DATA_RATE_UNUSED) {
        dataRateBand = this->band->defaultChannels[span].dataRates[i];
        this->dataRate = i;
        break;
      }
    }
  }
  
  RADIOLIB_DEBUG_PRINTLN("Data rate %02x", dataRateBand);
  DataRate_t datr;
  uint8_t bw = dataRateBand & 0x03;
  switch(bw) {
    case(RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ):
      datr.lora.bandwidth = 125.0;
      break;
    case(RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ):
      datr.lora.bandwidth = 250.0;
      break;
    case(RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ):
      datr.lora.bandwidth = 500.0;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_BANDWIDTH);
  }
  
  datr.lora.spreadingFactor = ((dataRateBand & 0xF0) >> 4) + 6;
  state = this->phyLayer->setDataRate(datr);

  return(state);
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
    RADIOLIB_DEBUG_PRINTLN("MIC mismatch, expected 0x%08x, got 0x%08x", micCalculated, micReceived);
    return(false);
  }

  return(true);
}

int16_t LoRaWANNode::setPhyProperties() {
  // set the physical layer configuration
  // TODO select channel span based on channel ID
  // TODO select channel randomly
  uint8_t channelId = 0;
  int16_t state = this->configureChannel(channelId, this->band->defaultChannels[0].joinRequestDataRate);
  RADIOLIB_ASSERT(state);

  state = this->phyLayer->setOutputPower(this->band->powerMax);
  RADIOLIB_ASSERT(state);

  uint8_t syncWord = RADIOLIB_LORAWAN_LORA_SYNC_WORD;
  state = this->phyLayer->setSyncWord(&syncWord, 1);
  RADIOLIB_ASSERT(state);

  state = this->phyLayer->setPreambleLength(RADIOLIB_LORAWAN_LORA_PREAMBLE_LEN);
  return(state);
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
