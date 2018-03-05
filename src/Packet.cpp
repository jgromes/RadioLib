#include "Packet.h"

Packet::Packet(void) {
  uint8_t src[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  getLoRaAddress(src);
  
  uint8_t dest[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t dat[240];
  uint8_t len = 0;
  
  init(src, dest, dat, len);
}

Packet::Packet(const char dest[24], const char dat[240]) {
  uint8_t src[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  getLoRaAddress(src);
  
  uint8_t destTmp[8];
  for(uint8_t i = 0; i < 8; i++) {
    destTmp[i] = (parseByte(dest[3*i]) << 4) | parseByte(dest[3*i + 1]);
  }
  
  init(src, destTmp, dat);
}

Packet::Packet(uint8_t dest[8], const char dat[240]) {
  uint8_t src[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  getLoRaAddress(src);
  
  init(src, dest, dat);
}

Packet::Packet(const char src[24], const char dest[24], const char dat[240]) {
  uint8_t srcTmp[8];
  for(uint8_t i = 0; i < 8; i++) {
    srcTmp[i] = (parseByte(src[3*i]) << 4) | parseByte(src[3*i + 1]);
  }
  
  uint8_t destTmp[8];
  for(uint8_t i = 0; i < 8; i++) {
    destTmp[i] = (parseByte(dest[3*i]) << 4) | parseByte(dest[3*i + 1]);
  }
  
  init(srcTmp, destTmp, dat);
}

Packet::Packet(uint8_t src[8], uint8_t dest[8], const char dat[240]) {
  init(src, dest, dat);
}

Packet::Packet(const char dest[24], uint8_t* dat, uint8_t len) {
  uint8_t src[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  getLoRaAddress(src);
  
  uint8_t destTmp[8];
  for(uint8_t i = 0; i < 8; i++) {
    destTmp[i] = (parseByte(dest[3*i]) << 4) | parseByte(dest[3*i + 1]);
  }
  
  init(src, destTmp, dat, len);
}

Packet::Packet(uint8_t dest[8], uint8_t* dat, uint8_t len) {
  uint8_t src[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  getLoRaAddress(src);
  
  init(src, dest, dat, len);
}

Packet::Packet(const char src[24], const char dest[24], uint8_t* dat, uint8_t len) {
  uint8_t srcTmp[8];
  for(uint8_t i = 0; i < 8; i++) {
    srcTmp[i] = (parseByte(src[3*i]) << 4) | parseByte(src[3*i + 1]);
  }
  
  uint8_t destTmp[8];
  for(uint8_t i = 0; i < 8; i++) {
    destTmp[i] = (parseByte(dest[3*i]) << 4) | parseByte(dest[3*i + 1]);
  }
  
  init(srcTmp, destTmp, dat, len);
}

Packet::Packet(uint8_t src[8], uint8_t dest[8], uint8_t* dat, uint8_t len) {
  init(src, dest, dat, len);
}

void Packet::init(uint8_t src[8], uint8_t dest[8], const char dat[240]) {
  for(uint8_t i = 0; i < 8; i++) {
    source[i] = src[i];
    destination[i] = dest[i];
  }
  
  length = 0;
  for(uint8_t i = 0; i < 240; i++) {
    data[i] = dat[i];
    if(data[i] == '\0') {
      length = i + 16;
      break;
    }
  }
}

void Packet::init(uint8_t src[8], uint8_t dest[8], uint8_t* dat, uint8_t len) {
  length = len + 16;
  for(uint8_t i = 0; i < 8; i++) {
    source[i] = src[i];
    destination[i] = dest[i];
  }

  for(uint8_t i = 0; i < length; i++) {
    data[i] = dat[i];
  }
}

void Packet::getSourceStr(char src[24]) {
  for(uint8_t i = 0; i < 8; i++) {
    src[3*i] = reparseChar(source[i] >> 4);
    src[3*i+1] = reparseChar(source[i] & 0x0F);
    src[3*i+2] = ':';
  }
  src[23] = '\0';
}

void Packet::getDestinationStr(char dest[24]) {
  for(uint8_t i = 0; i < 8; i++) {
    dest[3*i] = reparseChar(destination[i] >> 4);
    dest[3*i+1] = reparseChar(destination[i] & 0x0F);
    dest[3*i+2] = ':';
  }
  dest[23] = '\0';
}

void Packet::setSourceStr(const char src[24]) {
  for(uint8_t i = 0; i < 8; i++) {
    source[i] = (parseByte(src[3*i]) << 4) | parseByte(src[3*i + 1]);
  }
}

void Packet::setDestinationStr(const char dest[24]) {
  for(uint8_t i = 0; i < 8; i++) {
    destination[i] = (parseByte(dest[3*i]) << 4) | parseByte(dest[3*i + 1]);
  }
}

void Packet::copyInto(Packet& pack) {
  for(uint8_t i = 0; i < 8; i++) {
    pack.source[i] = source[i];
    pack.destination[i] = destination[i];
  }
  strcpy(pack.data, data);
}

void Packet::getLoRaAddress(uint8_t addr[8]) {
  for(uint8_t i = 0; i < 8; i++) {
    addr[i] = EEPROM.read(i);
  }
}

uint8_t Packet::parseByte(char c) {
  if((c >= 48) && (c <= 57)) {
    return(c - 48);
  } else if((c >= 65) && (c <= 70)) {
    return(c - 55);
  } else if((c >= 97) && (c <= 102)) {
    return(c - 87);
  }
  return(0);
}

char Packet::reparseChar(uint8_t b) {
  if(b <= 9) {
    return(b + 48);
  } else if((b >= 10) && (b <= 16)) {
    return(b + 55);
  }
  return(0);
}
