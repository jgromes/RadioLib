#include "MockPhysicalLayer.hpp"

#include <string.h>

// local table of all supported bands
static const LoRaWANBand_t* fuzzLoRaWANBands[RADIOLIB_LORAWAN_NUM_SUPPORTED_BANDS] = {
  &EU868, &US915, &EU433, &AU915, &CN470, &AS923, &AS923_2, &AS923_3, &AS923_4, &KR920, &IN865,
};

// first byte selects the variant: bit 0 = Rx2 instead of Rx1, bit 1 = LoRaWAN 1.1 instead of 1.0.x,
// bits 2-7 = band; second byte: bits 0-6 = datarate of the receive window, bit 7 = fixed device address
// instead of the one in the frame; the rest is the received frame
extern "C" int FuzzLoRaWANDownlink(const uint8_t* data, size_t size) {
  if(size < 3) { return(0); }
  const uint8_t window = (data[0] & 0x01) ? RADIOLIB_LORAWAN_RX2 : RADIOLIB_LORAWAN_RX1;
  const bool rev11 = data[0] & 0x02;
  const LoRaWANBand_t* band = fuzzLoRaWANBands[(data[0] >> 2) % RADIOLIB_LORAWAN_NUM_SUPPORTED_BANDS];
  const uint8_t rxDr = (data[1] & 0x7F) % RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES;

  // the device address is sent in clear, so anyone in range knows it: use the one from the frame,
  // a fuzzer without a corpus rarely finds a fixed 32-bit address
  uint32_t addr = 0x12345678;
  if(!(data[1] & 0x80) && (size >= 7)) {
    addr = LoRaWANNode::ntoh<uint32_t>(&data[3]);
  }

  // initialize default software AES-128
  FuzzHal hal;
  static RadioLibSoftwareAES128 RadioLibAES128Instance;
  hal.aes128 = &RadioLibAES128Instance;

  // create a FuzzPhysicalLayer with a mock module
  FuzzPhysicalLayer phy;
  Module mod(&hal, 1, 2, 3, 4);
  phy.mod = &mod;

  // pass the input from the fuzzer
  phy.currentPacketLength = size - 2;
  phy.currentPacketData = &data[2];

  // set some default AES keys
  uint8_t defaultKey[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };

  // instantiate the selected band
  LoRaWANNode node(&phy, band);

  // configure node with keys and addresses
  if(rev11) {
    node.beginABP(addr, defaultKey, defaultKey, defaultKey, defaultKey);
  } else {
    node.beginABP(addr, NULL, NULL, defaultKey, defaultKey);
  }

  // an active node has a valid uplink datarate, DR3 exists in all bands
  // (active so that a reply to MAC commands is also attempted, like on a real node)
  node.sessionStatus = RADIOLIB_LORAWAN_SESSION_ACTIVE;
  node.channels[RADIOLIB_LORAWAN_UPLINK].dr = 3;

  // read the frame from the radio and check it, like the node does after a reception
  if(node.checkDownlink(window, rxDr) != RADIOLIB_ERR_NONE) {
    return(0);
  }

  // try to parse the downlink
  size_t outLen = 0;
  uint8_t outBuffer[256] = {0};
  LoRaWANEvent_t event;
  node.parseDownlink(outBuffer, &outLen, window, &event);

  return(0);
}

static FuzzPhysicalLayer phy;
static void FuzzLoRaWANPersistCallback(uint8_t* buf, size_t len) {
  memcpy(buf, phy.currentPacketData, len);
}

extern "C" int FuzzLoRaWANPersistBuffer(const uint8_t* data, size_t size) {
  if(size < RADIOLIB_LORAWAN_SESSION_BUF_SIZE) { return(0); }

  // initialize default software AES-128
  FuzzHal hal;
  static RadioLibSoftwareAES128 RadioLibAES128Instance;
  hal.aes128 = &RadioLibAES128Instance;

  // create a FuzzPhysicalLayer with a mock module
  Module mod(&hal, 1, 2, 3, 4);
  phy.mod = &mod;

  // pass the input from the fuzzer
  phy.currentPacketLength = size;
  phy.currentPacketData = data;

  // set some default AES keys
  uint8_t defaultKey[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };

  // instantiate EU868 band (or any other band)
  LoRaWANNode node(&phy, &EU868);

  // set persistent buffer callbacks and try to "load" a buffer
  node.setCallbackRestorePersistence(FuzzLoRaWANPersistCallback);
  node.setCallbackRestoreSession(FuzzLoRaWANPersistCallback);
  node.loadBuffers();

  // configure node with keys and addresses
  node.beginABP(0x12345678, defaultKey, defaultKey, defaultKey, defaultKey);

  return(0);
}

// build a downlink with a valid MIC, so that the MAC commands get processed
static void FuzzLoRaWANBuildDownlink(LoRaWANNode& node, const uint8_t* mac, size_t macLen, bool fopts) {
  const uint32_t addr = node.devAddr;
  const uint16_t fcnt = 1;
  uint8_t* b = node.bufferFrame;
  memset(b, 0, sizeof(node.bufferFrame));
  size_t p = RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS;
  b[p++] = RADIOLIB_LORAWAN_MHDR_MTYPE_UNCONF_DATA_DOWN;
  LoRaWANNode::hton<uint32_t>(&b[p], addr);
  p += 4;
  b[p++] = fopts ? (uint8_t)macLen : 0;
  LoRaWANNode::hton<uint16_t>(&b[p], fcnt);
  p += 2;
  if(fopts) {
    // LoRaWAN 1.1 encrypts FOpts, 1.0.x sends them in clear
    if(node.rev == 1) {
      node.processAES(mac, macLen, node.nwkSEncKey, &b[p], addr, fcnt, RADIOLIB_LORAWAN_DOWNLINK, 0x01, true);
    } else {
      memcpy(&b[p], mac, macLen);
    }
    p += macLen;
  } else {
    // FPort 0, MAC commands in the payload
    b[p++] = 0;
    node.processAES(mac, macLen, node.nwkSEncKey, &b[p], addr, fcnt, RADIOLIB_LORAWAN_DOWNLINK, 0x00, true);
    p += macLen;
  }
  node.frameLength = (uint8_t)(p - RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + sizeof(uint32_t));

  // MIC block the same way as parseDownlink builds it
  b[RADIOLIB_LORAWAN_BLOCK_MAGIC_POS] = RADIOLIB_LORAWAN_MIC_BLOCK_MAGIC;
  b[RADIOLIB_LORAWAN_BLOCK_DIR_POS] = RADIOLIB_LORAWAN_DOWNLINK;
  LoRaWANNode::hton<uint32_t>(&b[RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS], addr);
  LoRaWANNode::hton<uint32_t>(&b[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], fcnt);
  b[RADIOLIB_LORAWAN_MIC_BLOCK_LEN_POS] = node.frameLength - sizeof(uint32_t);
  uint32_t mic = node.generateMIC(b, RADIOLIB_AES128_BLOCK_SIZE + node.frameLength - sizeof(uint32_t), node.sNwkSIntKey);
  LoRaWANNode::hton<uint32_t>(&b[RADIOLIB_AES128_BLOCK_SIZE + node.frameLength - sizeof(uint32_t)], mic);
}

// FuzzLoRaWANDownlink never gets past the MIC check, so it does not reach the MAC commands
// first byte selects the variant: bit 0 = FOpts instead of FPort 0, bit 1 = LoRaWAN 1.1 instead of 1.0.x,
// bits 2-7 = band, the rest is the MAC command payload
extern "C" int FuzzLoRaWANMacCommands(const uint8_t* data, size_t size) {
  if(size < 2) { return(0); }
  const bool fopts = data[0] & 0x01;
  const bool rev11 = data[0] & 0x02;
  const LoRaWANBand_t* band = fuzzLoRaWANBands[(data[0] >> 2) % RADIOLIB_LORAWAN_NUM_SUPPORTED_BANDS];
  const uint8_t* mac = &data[1];
  size_t macLen = size - 1;
  const size_t maxLen = fopts ? RADIOLIB_LORAWAN_FHDR_FOPTS_MAX_LEN : RADIOLIB_LORAWAN_MAX_PAYLOAD_SIZE;
  if(macLen > maxLen) { macLen = maxLen; }

  // initialize default software AES-128
  FuzzHal hal;
  static RadioLibSoftwareAES128 RadioLibAES128Instance;
  hal.aes128 = &RadioLibAES128Instance;

  // create a FuzzPhysicalLayer with a mock module
  FuzzPhysicalLayer phy;
  Module mod(&hal, 1, 2, 3, 4);
  phy.mod = &mod;

  // set some default AES keys
  uint8_t defaultKey[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };

  // instantiate the selected band
  LoRaWANNode node(&phy, band);

  // configure node with keys and addresses
  if(rev11) {
    node.beginABP(0x12345678, defaultKey, defaultKey, defaultKey, defaultKey);
  } else {
    node.beginABP(0x12345678, NULL, NULL, defaultKey, defaultKey);
  }

  // an active node has a valid uplink datarate, DR3 exists in all bands
  node.channels[RADIOLIB_LORAWAN_UPLINK].dr = 3;

  // keep a copy of the session keys and addresses
  const uint64_t joinEUI = node.joinEUI;
  const uint64_t devEUI = node.devEUI;
  const uint32_t devAddr = node.devAddr;
  uint8_t keys[4][RADIOLIB_AES128_KEY_SIZE];
  memcpy(keys[0], node.nwkKey, sizeof(keys[0]));
  memcpy(keys[1], node.appKey, sizeof(keys[1]));
  memcpy(keys[2], node.appSKey, sizeof(keys[2]));
  memcpy(keys[3], node.nwkSEncKey, sizeof(keys[3]));

  // try to parse the downlink
  FuzzLoRaWANBuildDownlink(node, mac, macLen, fopts);
  size_t outLen = 0;
  uint8_t outBuffer[256] = {0};
  LoRaWANEvent_t event;
  node.parseDownlink(outBuffer, &outLen, RADIOLIB_LORAWAN_RX1, &event);

  // MAC commands must not change the session keys or addresses
  if((node.joinEUI != joinEUI) || (node.devEUI != devEUI) || (node.devAddr != devAddr) ||
     memcmp(keys[0], node.nwkKey, sizeof(keys[0])) || memcmp(keys[1], node.appKey, sizeof(keys[1])) ||
     memcmp(keys[2], node.appSKey, sizeof(keys[2])) || memcmp(keys[3], node.nwkSEncKey, sizeof(keys[3])) ||
     (node.fOptsDownLen > RADIOLIB_LORAWAN_FHDR_FOPTS_MAX_LEN)) {
    abort();
  }

  // read back the stored answers
  uint8_t margin = 0;
  uint8_t gwCnt = 0;
  (void)node.getMacLinkCheckAns(&margin, &gwCnt);
  uint32_t ts = 0;
  uint16_t fraction = 0;
  (void)node.getMacDeviceTimeAns(&ts, &fraction, false);

  return(0);
}

