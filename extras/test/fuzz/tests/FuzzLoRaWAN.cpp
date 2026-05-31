#include "MockPhysicalLayer.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if(size < 10) { return(0); }

  // initialize default software AES-128
  FuzzHal hal;
  static RadioLibSoftwareAES128 RadioLibAES128Instance;
  hal.aes128 = &RadioLibAES128Instance;

  // create a FuzzPhysicalLayer with a mock module
  FuzzPhysicalLayer phy;
  Module mod(&hal, 1, 2, 3, 4);
  phy.mod = &mod;

  // pass the input from the fuzzer
  phy.currentPacketLength = size;
  phy.currentPacketData = data;

  // set some default AES keys
  uint8_t defaultKey[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };

  // instantiate EU868 band (or any other band)
  LoRaWANNode node(&phy, &EU868);

  // configure node with keys and addresses
  node.beginABP(0x12345678, defaultKey, defaultKey, defaultKey, defaultKey);

  // try to parse the downlink
  size_t outLen = 0;
  uint8_t outBuffer[256] = {0};
  LoRaWANEvent_t event;
  node.parseDownlink(outBuffer, &outLen, RADIOLIB_LORAWAN_RX1, &event);

  return(0);
}

