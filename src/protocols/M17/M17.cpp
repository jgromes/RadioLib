#include "M17.h"

#include "../../utils/CRC.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_M17

M17Client::M17Client(PhysicalLayer* phy) : FSK4Client(phy) {
  phyLayer = phy;
}

int16_t M17Client::begin(float base, char* addr) {
  int16_t state = FSK4Client::begin(base, RADIOLIB_M17_SHIFT_HZ, RADIOLIB_M17_RATE_BAUD);
  RADIOLIB_ASSERT(state);

  // FSK4: 0, 1600, 3200, 4800
  // M17: 800, 2400, -800, -2400
  int16_t offsets[] = { 800, 800, -4000, -7200 };
  FSK4Client::setCorrection(offsets);

  this->encodeAddr(addr, this->src);

  /*Module* mod = this->phyLayer->getMod();
  while(true) {
    FSK4Client::write(0);
    mod->hal->delay(1000);
    FSK4Client::write(1);
    mod->hal->delay(1000);
    FSK4Client::write(2);
    mod->hal->delay(1000);
    FSK4Client::write(3);
    mod->hal->delay(1000);
  }*/

  return(state);
}

int16_t M17Client::transmit(uint8_t* data, size_t len, char* dst) {
  uint8_t lsf[RADIOLIB_M17_LSF_MAXLEN_BYTES_ENCODED] = { 0 };
  size_t lsfLen = encodeLsf(dst, RADIOLIB_M17_LSF_MODE_PACKET | RADIOLIB_M17_LSF_DATA_TYPE_DATA | RADIOLIB_M17_LSF_ENC_NONE, lsf);

  // send preamble
  for(size_t i = 0; i < RADIOLIB_M17_PRE_LEN_BYTES; i++) {
    FSK4Client::write(RADIOLIB_M17_PRE_PATTERN_LSF);
  }
  
  // send sync burst
  FSK4Client::write(RADIOLIB_M17_SYNC_BURST_LSF >> 8);
  FSK4Client::write(RADIOLIB_M17_SYNC_BURST_LSF & 0xFF);

  // send payload
  FSK4Client::write(lsf, lsfLen);

  // dummy data
  /*for(size_t i = 0; i < 200; i++) {
    FSK4Client::write(0x00);
    FSK4Client::write(0x55);
    FSK4Client::write(0xAA);
    FSK4Client::write(0xFF);
  }*/

  // send EOT
  for(size_t i = 0; i < RADIOLIB_M17_EOT_LEN_BYTES / 2; i++) {
    FSK4Client::write(RADIOLIB_M17_EOT_PATTERN >> 8);
    FSK4Client::write(RADIOLIB_M17_EOT_PATTERN & 0xFF);
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t M17Client::encodeAddr(char* in, uint8_t* out) {
  //RADIOLIB_ASSERT_PTR(in);
  //RADIOLIB_ASSERT_PTR(out);

  // TODO check max len and encodable/reserved addresses
  uint64_t res = 0;
  size_t len = strlen(in);
  for(size_t i = 0; i < len; i++) {
    uint8_t val = 0;
    char c = in[i];
    if((c >= 'A') && (c <= 'Z')) {
      val = c - 'A' + 1;
    } else if((c >= '0') && (c <= '9')) {
      val = c - '0' + 27;
    } else if(c == '-') {
      val = 37;
    } else if(c == '/') {
      val = 38;
    } else if(c == '.') {
      val = 39;
    } else {
      return(RADIOLIB_ERR_INVALID_CALLSIGN);
    }
    res += val * pow(40, i);
  }

  // set the output
  for(size_t i = 0; i < RADIOLIB_M17_ADDR_LEN; i++) {
    out[i] = (res >> (i * 8)) & 0xFF;
  }

  return(RADIOLIB_ERR_NONE);
}

size_t M17Client::encodeLsf(char* dst, uint16_t type, uint8_t* out, uint8_t* meta, size_t metaLen) {
  if(!out) {
    return(0);
  }
  uint8_t* framePtr = out;

  // encode destination address
  this->encodeAddr(dst, framePtr);
  framePtr += RADIOLIB_M17_ADDR_LEN;
  
  // copy the source address
  memcpy(framePtr, this->src, RADIOLIB_M17_ADDR_LEN);
  framePtr += RADIOLIB_M17_ADDR_LEN;

  // set the type bits
  (*framePtr++) = (type & 0xFF00) >> 8;
  (*framePtr++) = type & 0x00FF;

  // TODO check meta + metaLen valid
  if(meta) {
    memcpy(framePtr, meta, metaLen);
  }
  framePtr += 14;

  // add CRC
  RadioLibCRCInstance.size = 16;
  RadioLibCRCInstance.poly = 0x5935;
  RadioLibCRCInstance.init = 0xFFFF;
  RadioLibCRCInstance.out = 0x0000;
  uint16_t crc16 = RadioLibCRCInstance.checksum(out, 240/8 - sizeof(uint16_t));
  (*framePtr++) = (crc16 & 0xFF00) >> 8;
  (*framePtr++) = crc16 & 0x00FF;

  // TODO add flush bits
  framePtr++;

  // TODO convolutional encoding
  framePtr+=30;

  // TODO puncturing
  framePtr-=15;

  // TODO interleaving

  // randomize
  size_t len = framePtr - out;
  randomize(out, len);
  return(len);
}

void M17Client::randomize(uint8_t* buff, size_t len) {
  if(!buff) {
    return;
  }

  for(size_t i = 0; i < len; i++) {
    buff[i] ^= m17_randomizer[this->randIndex++];
    this->randIndex %= RADIOLIB_M17_RANDOMIZER_LEN;
  }
}

#endif
