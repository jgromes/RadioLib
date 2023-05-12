#include "CRC.h"

RadioLibCRC::RadioLibCRC() {

}

uint32_t RadioLibCRC::checksum(uint8_t* buff, size_t len) {
  uint32_t crc = this->init;
  for(size_t i = 0; i < len; i+=this->size/8) {
    uint32_t window = 0;
    for(uint8_t j = 0; j < this->size/8; j++) {
      uint8_t inByte = buff[i + j];
      if(this->refIn) {
        inByte = Module::reflect(inByte, 8);
      }
      window |= (inByte << ((this->size - 8) - 8*j));
    }
    crc ^= window;
    for(size_t k = 0; k < this->size; k++) {
      if(crc & ((uint32_t)1 << (this->size - 1))) {
        crc <<= (uint32_t)1;
        crc ^= this->poly;
      } else {
        crc <<= (uint32_t)1;
      }
    }
  }
  crc ^= this->out;
  if(this->refOut) {
    crc = Module::reflect(crc, this->size);
  }
  crc &= (uint32_t)0xFFFFFFFF >> (32 - this->size);
  return(crc);
}

RadioLibCRC RadioLibCRCInstance;
