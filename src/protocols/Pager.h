#ifndef _RADIOLIB_PAGER_H
#define _RADIOLIB_PAGER_H

#include "TypeDef.h"
#include "PhysicalLayer.h"

// supported encoding schemes
#define ASCII                                         0
#define BCD                                           1

#define PREAMBLE_LENGTH                               18
#define MESSAGE_BITS_LENGTH                           20

#define FREQ_SHIFT_HZ                                 4500

#define PREAMBLE_CODE_WORD                            0xAAAAAAAA
#define FRAME_SYNC_CODE_WORD                          0x7CD215D8
#define IDLE_CODE_WORD                                0x7A89C197
#define ADDRESS_CODE_WORD                             0
#define MESSAGE_CODE_WORD                             1

#define BCH_GENERATOR_POLYNOMIAL                      0b11101101001   // x^10 + x^9 + x^8 + x^6 + x^5 + x^3 + 1

class PagerClient {
  public:
    PagerClient(PhysicalLayer* phy);

    // basic methods
    int16_t begin(float base, uint16_t speed);
    int16_t transmit(String& str, uint32_t addr, uint8_t encoding = BCD);
    int16_t transmit(const char* str, uint32_t addr, uint8_t encoding = BCD);
    int16_t transmit(uint8_t* data, size_t len, uint32_t addr, uint8_t encoding = BCD);

    // TODO: add receiving + option to listen to all packets

  private:
    PhysicalLayer* _phy;

    uint32_t _base;
    uint16_t _shift;
    uint16_t _bitDuration;

    void write(uint32_t* data, size_t len);
    void write(uint32_t b);

    uint32_t encodeBCH(uint32_t data);
    uint32_t addParity(uint32_t msg);
};

#endif
