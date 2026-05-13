#include "ConvCode.h"
#include <string.h>

// each 32-bit word stores 8 values, one per each nibble
static const uint32_t ConvCodeTable1_3[16] = {
  0x07347043, 0x61521625, 0x16256152, 0x70430734,
  0x43703407, 0x25165261, 0x52612516, 0x34074370,
  0x70430734, 0x16256152, 0x61521625, 0x07347043,
  0x34074370, 0x52612516, 0x25165261, 0x43703407,
};

static const uint32_t ConvCodeTable1_2[4] = { 
  0x03122130, 0x21300312, 0x30211203, 0x12033021,
};

RadioLibConvCode::RadioLibConvCode() {

}

void RadioLibConvCode::begin(uint8_t rt) {
  this->enc_state = 0;
  this->rate = rt;
}

int16_t RadioLibConvCode::encode(const uint8_t* in, size_t in_bits, uint8_t* out, size_t* out_bits) {
  if(!in || !out) {
    return(RADIOLIB_ERR_UNKNOWN);
  }

  size_t ind_bit;
  uint16_t data_out_bitcount = 0;
  uint32_t bin_out_word = 0;

  // iterate over the provided bits
  for(ind_bit = 0; ind_bit < in_bits; ind_bit++) {
    uint8_t cur_bit = GET_BIT_IN_ARRAY_LSB(in, ind_bit);
    const uint32_t* lut_ptr = (this->rate == 2) ? ConvCodeTable1_2 : ConvCodeTable1_3;
    uint8_t word_pos = this->enc_state / 4;
    uint8_t byte_pos = (3 - (this->enc_state % 4)) * 8;
    uint8_t nibble_pos = (1 - cur_bit) * 4;
    uint8_t g1g0 = (lut_ptr[word_pos] >> (byte_pos + nibble_pos)) & 0x0F;

    uint8_t mod = this->rate == 2 ? 16 : 64;
    this->enc_state = (this->enc_state * 2 + cur_bit) % mod;
    bin_out_word |= (g1g0 << ((7 - (ind_bit % 8)) * this->rate));
    if(ind_bit % 8 == 7) {
      if(this->rate == 3) {
        *out++ = (uint8_t)(bin_out_word >> 16);
      }
      *out++ = (uint8_t)(bin_out_word >> 8);
      *out++ = (uint8_t)bin_out_word;
      bin_out_word  = 0;
    }
    data_out_bitcount += this->rate;
  }

  if(ind_bit % 8) {
    if(this->rate == 3) {
      *out++ = (uint8_t)(bin_out_word >> 16);
    }
    *out++ = (uint8_t)(bin_out_word >> 8);
    *out++ = (uint8_t)bin_out_word;
  }

  if(out_bits) { *out_bits = data_out_bitcount; }

  return(RADIOLIB_ERR_NONE);
}
