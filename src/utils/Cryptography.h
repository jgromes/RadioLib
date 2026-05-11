#if !defined(_RADIOLIB_CRYPTOGRAPHY_H)
#define _RADIOLIB_CRYPTOGRAPHY_H

#include "../TypeDef.h"

// AES-128 constants
#define RADIOLIB_AES128_BLOCK_SIZE                              (16)
#define RADIOLIB_AES128_KEY_SIZE                                (RADIOLIB_AES128_BLOCK_SIZE)
#define RADIOLIB_AES128_N_K                                     ((RADIOLIB_AES128_BLOCK_SIZE) / sizeof(uint32_t))
#define RADIOLIB_AES128_N_B                                     (4)
#define RADIOLIB_AES128_N_R                                     (10)
#define RADIOLIB_AES128_KEY_EXP_SIZE                            (176)

typedef struct {
  uint8_t X[RADIOLIB_AES128_BLOCK_SIZE];
  uint8_t buffer[RADIOLIB_AES128_BLOCK_SIZE];
  size_t buffer_len;
  uint8_t k1[RADIOLIB_AES128_BLOCK_SIZE];
  uint8_t k2[RADIOLIB_AES128_BLOCK_SIZE];
  bool subkeys_generated;
} RadioLibCmacState;

// helper type
typedef uint8_t state_t[4][4];

/*!
  \class RadioLibAES128
  Most of the implementation here is adapted from https://github.com/kokke/tiny-AES-c
  Additional code and CMAC calculation is from https://github.com/megrxu/AES-CMAC
  \brief Class to perform AES encryption, decryption and CMAC.
*/
class RadioLibAES128 {
  public:
    /*!
      \brief Default constructor.
    */
    RadioLibAES128();

    /*!
      \brief Initialize the AES.
      If the user has a hardware with AES acceleration, this method is the interface to implement.
      By default a software-only implementation is provided in RadioLibSoftwareAES128.
      \param key AES key to use.
    */
    virtual void init(uint8_t* key) = 0;

    /*!
      \brief Perform ECB-type AES encryption.
      If the user has a hardware with AES acceleration, this method is the interface to implement.
      By default a software-only implementation is provided in RadioLibSoftwareAES128.
      \param in Input plaintext data (unpadded).
      \param len Length of the input data.
      \param out Buffer to save the output ciphertext into. It is up to the caller
      to ensure the buffer is sufficiently large to save the data!
      \returns The number of bytes saved into the output buffer.
    */
    virtual size_t encryptECB(const uint8_t* in, size_t len, uint8_t* out) = 0;
    
    /*!
      \brief Perform ECB-type AES decryption.
      If the user has a hardware with AES acceleration, this method is the interface to implement.
      By default a software-only implementation is provided in RadioLibSoftwareAES128.
      \param in Input ciphertext data.
      \param len Length of the input data.
      \param out Buffer to save the output plaintext into. It is up to the caller
      to ensure the buffer is sufficiently large to save the data!
      \returns The number of bytes saved into the output buffer.
    */
    virtual size_t decryptECB(const uint8_t* in, size_t len, uint8_t* out) = 0;

    /*!
      \brief Calculate message authentication code according to RFC4493.
      \param in Input data (unpadded).
      \param len Length of the input data.
      \param cmac Buffer to save the output MAC into. The buffer must be at least 16 bytes long!
    */
    void generateCMAC(const uint8_t* in, size_t len, uint8_t* cmac);

    /*!
      \brief Initialize the CMAC state. This must be called before any updateCMAC calls.
      \param st State to initialize.
    */
    void initCMAC(RadioLibCmacState* st);

    /*!
      \brief Update the CMAC state with a chunk of data. This can be called multiple times to process the data in chunks.
      \param st State to update.
      \param data Input data (unpadded).
      \param len Length of the input data.
    */
    void updateCMAC(RadioLibCmacState* st, const uint8_t* data, size_t len);
    
    /*!
      \brief Finalize the CMAC calculation and save the result. This must be called after all updateCMAC calls are done.
      \param st State to finalize.
      \param out Buffer to save the output MAC into. The buffer must be at least 16 bytes long!
    */
    void finishCMAC(RadioLibCmacState* st, uint8_t* out);

    /*!
      \brief Verify the received CMAC. This just calculates the CMAC again and compares the results.
      \param in Input data (unpadded).
      \param len Length of the input data.
      \param cmac CMAC to verify.
      \returns True if valid, false otherwise.
    */
    bool verifyCMAC(const uint8_t* in, size_t len, const uint8_t* cmac);
  
  private:
    void blockXor(uint8_t* dst, const uint8_t* a, const uint8_t* b);
    void blockLeftshift(uint8_t* dst, const uint8_t* src);
    void generateSubkeys(uint8_t* key1, uint8_t* key2);
};

// in cases the user does not provide their own hardware-based AES-128, use the default software implementation
#if !RADIOLIB_CUSTOM_AES128

/*!
  \class RadioLibSoftwareAES128
  Most of the implementation here is adapted from https://github.com/kokke/tiny-AES-c
  \brief Class to perform AES encryption and decryption in software only.
  Contains implementation of pure virtual methods from RadioLibAES128.
*/
class RadioLibSoftwareAES128: public RadioLibAES128 {
  public:
    /*!
      \brief Default constructor.
    */
    RadioLibSoftwareAES128();

    /*!
      \brief Initialize the AES.
      \param key AES key to use.
    */
    void init(uint8_t* key) override;

    /*!
      \brief Perform ECB-type AES encryption.
      \param in Input plaintext data (unpadded).
      \param len Length of the input data.
      \param out Buffer to save the output ciphertext into. It is up to the caller
      to ensure the buffer is sufficiently large to save the data!
      \returns The number of bytes saved into the output buffer.
    */
    size_t encryptECB(const uint8_t* in, size_t len, uint8_t* out) override;
    
    /*!
      \brief Perform ECB-type AES decryption.
      \param in Input ciphertext data.
      \param len Length of the input data.
      \param out Buffer to save the output plaintext into. It is up to the caller
      to ensure the buffer is sufficiently large to save the data!
      \returns The number of bytes saved into the output buffer.
    */
    size_t decryptECB(const uint8_t* in, size_t len, uint8_t* out) override;

  private:
    uint8_t* keyPtr = nullptr;
    uint8_t roundKey[RADIOLIB_AES128_KEY_EXP_SIZE] = { 0 };

    void keyExpansion(uint8_t* roundKey, const uint8_t* key);
    void cipher(state_t* state, uint8_t* roundKey);
    void decipher(state_t* state, uint8_t* roundKey);

    void subWord(uint8_t* word);
    void rotWord(uint8_t* word);

    void subBytes(state_t* state, const uint8_t* box);
    void shiftRows(state_t* state, bool inv);
    void mixColumns(state_t* state, bool inv);

    // cppcheck seems convinced these are nut used, which is not true
    uint8_t mul(uint8_t a, uint8_t b); // cppcheck-suppress unusedPrivateFunction
    void addRoundKey(uint8_t round, state_t* state, const uint8_t* roundKey); // cppcheck-suppress unusedPrivateFunction
};

#endif

#endif
