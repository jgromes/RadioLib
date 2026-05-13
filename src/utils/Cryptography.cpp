#include "Cryptography.h"

#include <string.h>

RadioLibAES128::RadioLibAES128() {

}

/*
 * CMAC streaming API
 *
 * Usage:
 *   RadioLibCMAC_State st;
 *   RadioLibAES128_initCMACState(&RadioLibAES128Instance, &st);
 *   RadioLibAES128_updateCMACState(&RadioLibAES128Instance, &st, chunk1, len1);
 *   RadioLibAES128_updateCMACState(&RadioLibAES128Instance, &st, chunk2, len2);
 *   uint8_t mac[16];
 *   RadioLibAES128_finishCMACState(&RadioLibAES128Instance, &st, mac);
 */

void RadioLibAES128::initCMAC(RadioLibCmacState* st) {
  if(!st) {
    return;
  }
  memset(st->X, 0x00, RADIOLIB_AES128_BLOCK_SIZE);
  memset(st->buffer, 0x00, RADIOLIB_AES128_BLOCK_SIZE);
  st->buffer_len = 0;
  st->subkeys_generated = false;
}

void RadioLibAES128::updateCMAC(RadioLibCmacState* st, const uint8_t* data, size_t len) {
  if(!st || (!data && len != 0)) {
    return;
  }

  // ensure subkeys are present
  if(!st->subkeys_generated) {
    this->generateSubkeys(st->k1, st->k2);
    st->subkeys_generated = true;
  }

  uint8_t tmp[RADIOLIB_AES128_BLOCK_SIZE];
  size_t offset = 0;

  while(len > 0) {

    // fill buffer up to one full block
    size_t to_copy = RADIOLIB_AES128_BLOCK_SIZE - st->buffer_len;
    if(to_copy > len) {
      to_copy = len;
    }

    // copy the data into the buffer
    memcpy(&st->buffer[st->buffer_len], &data[offset], to_copy);
    st->buffer_len += to_copy;
    offset += to_copy;
    len -= to_copy;

    // if we now have a full block AND there is still more input remaining,
    // this block is NOT the final one, so process it.
    if(st->buffer_len == RADIOLIB_AES128_BLOCK_SIZE && len > 0) {
      this->blockXor(tmp, st->buffer, st->X);
      this->encryptECB(tmp, RADIOLIB_AES128_BLOCK_SIZE, st->X);
      st->buffer_len = 0;
    }
  }
}

void RadioLibAES128::finishCMAC(RadioLibCmacState* st, uint8_t* out) {
  if(!st || !out) {
    return;
  }

  // ensure subkeys are present
  if(!st->subkeys_generated) {
    this->generateSubkeys(st->k1, st->k2);
    st->subkeys_generated = true;
  }

  uint8_t last[RADIOLIB_AES128_BLOCK_SIZE];
  uint8_t Y[RADIOLIB_AES128_BLOCK_SIZE];

  if(st->buffer_len == RADIOLIB_AES128_BLOCK_SIZE) {
    this->blockXor(last, st->buffer, st->k1);
  } else {
    memset(last, 0x00, RADIOLIB_AES128_BLOCK_SIZE);
    if(st->buffer_len > 0) {
      memcpy(last, st->buffer, st->buffer_len);
    }
    last[st->buffer_len] = 0x80;
    this->blockXor(last, last, st->k2);
  }

  this->blockXor(Y, last, st->X);
  this->encryptECB(Y, RADIOLIB_AES128_BLOCK_SIZE, out);
}

void RadioLibAES128::generateCMAC(const uint8_t* in, size_t len, uint8_t* cmac) {
  RadioLibCmacState st;
  this->initCMAC(&st);
  this->updateCMAC(&st, in, len);
  this->finishCMAC(&st, cmac);
}

bool RadioLibAES128::verifyCMAC(const uint8_t* in, size_t len, const uint8_t* cmac) {
  uint8_t cmacReal[RADIOLIB_AES128_BLOCK_SIZE];
  this->generateCMAC(in, len, cmacReal);
  for(size_t i = 0; i < RADIOLIB_AES128_BLOCK_SIZE; i++) {
    if((cmacReal[i] != cmac[i])) {
      return(false);
    }
  }
  return(true);
}

void RadioLibAES128::blockXor(uint8_t* dst, const uint8_t* a, const uint8_t* b) {
  for(uint8_t j = 0; j < RADIOLIB_AES128_BLOCK_SIZE; j++) {
    dst[j] = a[j] ^ b[j];
  }
}

void RadioLibAES128::blockLeftshift(uint8_t* dst, const uint8_t* src) {
  uint8_t ovf = 0x00;
  for(int8_t i = RADIOLIB_AES128_BLOCK_SIZE - 1; i >= 0; i--) {
    dst[i] = src[i] << 1;
    dst[i] |= ovf;
    ovf = (src[i] & 0x80) ? 1 : 0;
  }
}

void RadioLibAES128::generateSubkeys(uint8_t* key1, uint8_t* key2) {
  const uint8_t const_Zero[] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
  };

  const uint8_t const_Rb[] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x87
  };

  uint8_t L[RADIOLIB_AES128_BLOCK_SIZE];
  this->encryptECB(const_Zero, RADIOLIB_AES128_BLOCK_SIZE, L);
  this->blockLeftshift(key1, L);
  if(L[0] & 0x80) {
    this->blockXor(key1, key1, const_Rb);
  }

  this->blockLeftshift(key2, key1);
  if(key1[0] & 0x80) {
    this->blockXor(key2, key2, const_Rb);
  }
}

#if !RADIOLIB_CUSTOM_AES128

// AES lookup tables
static const uint8_t aesSbox[] RADIOLIB_NONVOLATILE = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
    0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
    0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
    0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
    0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
    0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
    0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
    0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
    0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
    0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
    0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
    0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const uint8_t aesSboxInv[] RADIOLIB_NONVOLATILE = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38,
    0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
    0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d,
    0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2,
    0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
    0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda,
    0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a,
    0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
    0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea,
    0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85,
    0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
    0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20,
    0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31,
    0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
    0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0,
    0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26,
    0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

static const uint8_t aesRcon[] = { 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };

RadioLibSoftwareAES128::RadioLibSoftwareAES128() : RadioLibAES128() {

}

void RadioLibSoftwareAES128::init(uint8_t* key) {
  this->keyPtr = key;
  this->keyExpansion(this->roundKey, key);
}

size_t RadioLibSoftwareAES128::encryptECB(const uint8_t* in, size_t len, uint8_t* out) {
  size_t num_blocks = len / RADIOLIB_AES128_BLOCK_SIZE;
  if(len % RADIOLIB_AES128_BLOCK_SIZE) {
    num_blocks++;
  }

  memset(out, 0x00, RADIOLIB_AES128_BLOCK_SIZE * num_blocks);
  memcpy(out, in, len);

  for(size_t i = 0; i < num_blocks; i++) {
    this->cipher((state_t*)(out + (RADIOLIB_AES128_BLOCK_SIZE * i)), this->roundKey);
  }

  return(num_blocks*RADIOLIB_AES128_BLOCK_SIZE);
}

size_t RadioLibSoftwareAES128::decryptECB(const uint8_t* in, size_t len, uint8_t* out) {
  size_t num_blocks = len / RADIOLIB_AES128_BLOCK_SIZE;
  if(len % RADIOLIB_AES128_BLOCK_SIZE) {
    num_blocks++;
  }

  memset(out, 0x00, RADIOLIB_AES128_BLOCK_SIZE * num_blocks);
  memcpy(out, in, len);

  for(size_t i = 0; i < num_blocks; i++) {
    this->decipher((state_t*)(out + (RADIOLIB_AES128_BLOCK_SIZE * i)), this->roundKey);
  }

  return(num_blocks*RADIOLIB_AES128_BLOCK_SIZE);
}

void RadioLibSoftwareAES128::keyExpansion(uint8_t* roundKey, const uint8_t* key) {
  uint8_t tmp[4];

  // the first round key is the key itself
  for(uint8_t i = 0; i < RADIOLIB_AES128_N_K; i++) {
    for(uint8_t j = 0; j < 4; j++) {
      roundKey[(i * 4) + j] = key[(i * 4) + j];
    }
  }

  // All other round keys are found from the previous round keys.
  for(uint8_t i = RADIOLIB_AES128_N_K; i < RADIOLIB_AES128_N_B * (RADIOLIB_AES128_N_R + 1); ++i) {
    uint8_t j = (i - 1) * 4;
    for(uint8_t k = 0; k < 4; k++) {
      tmp[k] = roundKey[j + k];
    }

    if(i % RADIOLIB_AES128_N_K == 0) {
      this->rotWord(tmp);
      this->subWord(tmp);
      tmp[0] = tmp[0] ^ aesRcon[i/RADIOLIB_AES128_N_K];
    }

    j = i * 4;
    uint8_t k = (i - RADIOLIB_AES128_N_K) * 4;
    for(uint8_t l = 0; l < 4; l++) {
      roundKey[j + l] = roundKey[k + l] ^ tmp[l];
    }
  }
}

void RadioLibSoftwareAES128::cipher(state_t* state, uint8_t* roundKey) {
  this->addRoundKey(0, state, roundKey);
  for(uint8_t round = 1; round < RADIOLIB_AES128_N_R; round++) {
    this->subBytes(state, aesSbox);
    this->shiftRows(state, false);
    this->mixColumns(state, false);
    this->addRoundKey(round, state, roundKey);
  }

  this->subBytes(state, aesSbox);
  this->shiftRows(state, false);
  this->addRoundKey(RADIOLIB_AES128_N_R, state, roundKey);
}


void RadioLibSoftwareAES128::decipher(state_t* state, uint8_t* roundKey) {
  this->addRoundKey(RADIOLIB_AES128_N_R, state, roundKey);
  for(uint8_t round = RADIOLIB_AES128_N_R - 1; round > 0; --round) {
    this->shiftRows(state, true);
    this->subBytes(state, aesSboxInv);
    this->addRoundKey(round, state, roundKey);
    this->mixColumns(state, true);
  }

  this->shiftRows(state, true);
  this->subBytes(state, aesSboxInv);
  this->addRoundKey(0, state, roundKey);
}

void RadioLibSoftwareAES128::subWord(uint8_t* word) {
  for(size_t i = 0; i < 4; i++) {
    uint8_t* ptr = const_cast<uint8_t*>(&aesSbox[word[i]]);
    word[i] = RADIOLIB_NONVOLATILE_READ_BYTE(ptr);
  }
}

void RadioLibSoftwareAES128::rotWord(uint8_t* word) {
  uint8_t tmp[4];
  memcpy(tmp, word, 4);
  for(size_t i = 0; i < 4; i++) {
    word[i] = tmp[(i + 1) % 4];
  }
}

void RadioLibSoftwareAES128::subBytes(state_t* state, const uint8_t* box) {
  for(size_t row = 0; row < 4; row++) {
    for(size_t col = 0; col < 4; col++) {
      uint8_t* ptr = const_cast<uint8_t*>(&box[(*state)[col][row]]);
      (*state)[col][row] = RADIOLIB_NONVOLATILE_READ_BYTE(ptr);
    }
  }
}

void RadioLibSoftwareAES128::shiftRows(state_t* state, bool inv) {
  uint8_t tmp[4];
  for(size_t row = 1; row < 4; row++) {
    for(size_t col = 0; col < 4; col++) {
      if(!inv) {
        tmp[col] = (*state)[(row + col) % 4][row];
      } else {
        tmp[(row + col) % 4] = (*state)[col][row];
      }
    }
    for(size_t col = 0; col < 4; col++) {
      (*state)[col][row] = tmp[col];
    }
  }
}

void RadioLibSoftwareAES128::mixColumns(state_t* state, bool inv) {
  uint8_t tmp[4];
  uint8_t matmul[][4] = {
    0x02, 0x03, 0x01, 0x01,
    0x01, 0x02, 0x03, 0x01,
    0x01, 0x01, 0x02, 0x03,
    0x03, 0x01, 0x01, 0x02
  };
  if(inv) {
    uint8_t matmul_inv[][4] = {
      0x0e, 0x0b, 0x0d, 0x09,
      0x09, 0x0e, 0x0b, 0x0d,
      0x0d, 0x09, 0x0e, 0x0b,
      0x0b, 0x0d, 0x09, 0x0e
    };
    memcpy(matmul, matmul_inv, sizeof(matmul_inv));
  }

  for(size_t col = 0; col < 4; col++) {
    for(size_t row = 0; row < 4; row++) {
      tmp[row] = (*state)[col][row];
    }
    for(size_t i = 0; i < 4; i++) {
      (*state)[col][i] = 0x00;
      for(size_t j = 0; j < 4; j++) {
        (*state)[col][i] ^= mul(matmul[i][j], tmp[j]);
      }
    }
  }
}

uint8_t RadioLibSoftwareAES128::mul(uint8_t a, uint8_t b) {
  uint8_t sb[4];
  uint8_t out = 0;
  sb[0] = b;
  for(size_t i = 1; i < 4; i++) {
    sb[i] = sb[i - 1] << 1;
    if (sb[i - 1] & 0x80) {
      sb[i] ^= 0x1b;
    }
  }
  for(size_t i = 0; i < 4; i++) {
    if(a >> i & 0x01) {
      out ^= sb[i];
    }
  }
  return(out);
}

void RadioLibSoftwareAES128::addRoundKey(uint8_t round, state_t* state, const uint8_t* roundKey) {
  for(size_t row = 0; row < 4; row++) {
    for(size_t col = 0; col < 4; col++) {
      (*state)[row][col] ^= roundKey[(round * RADIOLIB_AES128_N_B * 4) + (row * RADIOLIB_AES128_N_B) + col];
    }
  }
}

#endif
