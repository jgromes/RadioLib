#include "MockPhysicalLayer.hpp"

extern "C" int FuzzPager(const uint8_t* data, size_t size) {
  // pager works with 32-bit codewords
  if(size % 4) { return(0); }

  // create a FuzzPhysicalLayer with a mock module
  FuzzHal hal;
  FuzzPhysicalLayer phy;
  Module mod(&hal, 1, 2, 3, 4);
  phy.mod = &mod;

  // create Pager client instance using the mocked module
  PagerClient pager(&phy);
  pager.begin(434.0, 1200);

  pager.transmit(data, size, 0);

  // pass the data from fuzzer
  phy.fillDirectReceiveBuffer(data, size);

  uint8_t buff[8192];
  size_t len = 0;
  uint32_t addr;
  pager.readData(buff, &len, &addr);

  return(0);
}
