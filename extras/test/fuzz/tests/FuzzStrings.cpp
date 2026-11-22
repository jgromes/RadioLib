#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <RadioLib.h>

// the POCSAG BCD codec does not touch the radio, a null physical layer is enough
class FuzzNullPhy: public PhysicalLayer {
  public:
    Module* getMod() override { return(nullptr); }
};

// ITA2String must encode arbitrary ASCII input into valid 5-bit ITA2 codes
// (all bytes below the shift-code range, with well-defined length reporting)
extern "C" int FuzzITA2(const uint8_t* data, size_t size) {
  if(size < 1) { return(0); }

  // treat the input as an ASCII string (NUL would only truncate it, map to 'A')
  char* str = new char[size + 1];
  for(size_t i = 0; i < size; i++) {
    str[i] = (data[i] != '\0') ? (char)data[i] : 'A';
  }
  str[size] = '\0';

  ITA2String ita2(str);
  // compute the array first, length() is then served from the cached length
  uint8_t* arr = ita2.byteArr();
  size_t len = ita2.length();
  assert(arr != nullptr);
  assert(len >= size);

  // every emitted byte is a 5-bit ITA2 code (including the FIGS/LTRS shifts)
  for(size_t i = 0; i < len; i++) {
    assert(arr[i] <= 0x1F);
  }

  delete[] arr;
  delete[] str;
  return(0);
}

// MorseClient::decode is a pure table lookup over a symbol/length pair,
// it must never crash and only ever return printable ASCII or the
// unknown-symbol marker
extern "C" int FuzzMorseDecode(const uint8_t* data, size_t size) {
  if(size < 2) { return(0); }

  for(size_t i = 0; i + 1 < size; i += 2) {
    uint8_t symbol = data[i];
    uint8_t len = data[i + 1] % 8;
    char res = MorseClient::decode(symbol, len);
    assert(res >= ' ');
    assert(res <= '~');
  }
  return(0);
}

// POCSAG BCD codec: encode/decode must round-trip for every nibble value
extern "C" int FuzzPagerBCD(const uint8_t* data, size_t size) {
  static FuzzNullPhy* phy = nullptr;
  static PagerClient* pager = nullptr;
  if(!pager) {
    phy = new FuzzNullPhy();
    pager = new PagerClient(phy);
  }

  for(size_t i = 0; i < size; i++) {
    // decodeBCD is defined for all 16 POCSAG BCD nibbles
    uint8_t nibble = data[i] % 0x10;
    char decoded = pager->decodeBCD(nibble);
    assert(decoded != 0);

    // encoding the decoded character must return the original nibble
    assert(pager->encodeBCD(decoded) == nibble);
  }
  return(0);
}
