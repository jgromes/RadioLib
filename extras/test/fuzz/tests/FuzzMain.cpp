#include <stdint.h>
#include <stddef.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  extern int FuzzLoRaWANDownlink(const uint8_t* data, size_t size);
  FuzzLoRaWANDownlink(data, size);

  extern int FuzzLoRaWANPersistBuffer(const uint8_t* data, size_t size);
  FuzzLoRaWANPersistBuffer(data, size);

  extern int FuzzLoRaWANMacCommands(const uint8_t* data, size_t size);
  FuzzLoRaWANMacCommands(data, size);

  extern int FuzzPager(const uint8_t* data, size_t size);
  FuzzPager(data, size);

  extern int FuzzCRC(const uint8_t* data, size_t size);
  FuzzCRC(data, size);

  extern int FuzzScrambler(const uint8_t* data, size_t size);
  FuzzScrambler(data, size);

  extern int FuzzAESECB(const uint8_t* data, size_t size);
  FuzzAESECB(data, size);

  extern int FuzzAESCMACStreaming(const uint8_t* data, size_t size);
  FuzzAESCMACStreaming(data, size);

  extern int FuzzConvCode(const uint8_t* data, size_t size);
  FuzzConvCode(data, size);

  extern int FuzzBCH(const uint8_t* data, size_t size);
  FuzzBCH(data, size);

  extern int FuzzReedSolomon(const uint8_t* data, size_t size);
  FuzzReedSolomon(data, size);

  extern int FuzzITA2(const uint8_t* data, size_t size);
  FuzzITA2(data, size);

  extern int FuzzMorseDecode(const uint8_t* data, size_t size);
  FuzzMorseDecode(data, size);

  extern int FuzzPagerBCD(const uint8_t* data, size_t size);
  FuzzPagerBCD(data, size);

  extern int FuzzSSDV(const uint8_t* data, size_t size);
  FuzzSSDV(data, size);

  extern int FuzzADSB(const uint8_t* data, size_t size);
  FuzzADSB(data, size);

  extern int FuzzAX25(const uint8_t* data, size_t size);
  FuzzAX25(data, size);

  extern int FuzzAPRS(const uint8_t* data, size_t size);
  FuzzAPRS(data, size);

  return(0);
}
