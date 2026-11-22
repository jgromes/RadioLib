#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <RadioLib.h>

// arbitrary AES instance for fuzzing (the library global instance is also used
// by the LoRaWAN fuzz target, keep them separate)
static RadioLibSoftwareAES128 fuzzAes;

// AES-128 ECB encrypt/decrypt round trip: decrypting the ciphertext must
// recover the plaintext for arbitrary keys and data
extern "C" int FuzzAESECB(const uint8_t* data, size_t size) {
  // 16 bytes key + at least one full block
  if(size < 32) { return(0); }

  uint8_t key[16];
  memcpy(key, data, 16);

  const uint8_t* plain = &data[16];
  size_t len = (size - 16) & ~(size_t)(RADIOLIB_AES128_BLOCK_SIZE - 1);
  if(len == 0) { return(0); }

  uint8_t* cipher = new uint8_t[len];
  uint8_t* recovered = new uint8_t[len];

  fuzzAes.init(key);
  size_t encLen = fuzzAes.encryptECB(plain, len, cipher);
  assert(encLen == len);

  fuzzAes.init(key);
  size_t decLen = fuzzAes.decryptECB(cipher, len, recovered);
  assert(decLen == len);
  assert(memcmp(recovered, plain, len) == 0);

  delete[] cipher;
  delete[] recovered;
  return(0);
}

// streaming CMAC (init/update/finish) must produce the same MAC as the
// one-shot generateCMAC, for arbitrary chunk splittings
extern "C" int FuzzAESCMACStreaming(const uint8_t* data, size_t size) {
  // 16 bytes key + 1 byte chunk-size selector + at least 1 byte message
  if(size < 18) { return(0); }

  uint8_t key[16];
  memcpy(key, data, 16);
  size_t chunk = 1 + (data[16] % 32);

  const uint8_t* msg = &data[17];
  size_t len = size - 17;

  uint8_t oneShot[RADIOLIB_AES128_BLOCK_SIZE];
  uint8_t streamed[RADIOLIB_AES128_BLOCK_SIZE];

  fuzzAes.init(key);
  fuzzAes.generateCMAC(msg, len, oneShot);

  RadioLibCmacState st;
  fuzzAes.init(key);
  fuzzAes.initCMAC(&st);
  for(size_t off = 0; off < len; off += chunk) {
    fuzzAes.updateCMAC(&st, &msg[off], (len - off < chunk) ? len - off : chunk);
  }
  fuzzAes.finishCMAC(&st, streamed);

  assert(memcmp(oneShot, streamed, RADIOLIB_AES128_BLOCK_SIZE) == 0);
  return(0);
}
