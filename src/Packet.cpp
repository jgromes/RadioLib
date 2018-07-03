#include "Packet.h"

Packet::Packet(void) {
  uint8_t src[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  getLoRaAddress(src);
  
  uint8_t dest[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t* dat;
  uint8_t len = 0;
  
  init(src, dest, dat, len);
}

Packet::Packet(const char* dest, const char* dat) {
  uint8_t src[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  getLoRaAddress(src);
  
  uint8_t destTmp[8];
  for(uint8_t i = 0; i < 8; i++) {
    char str[] = {dest[3*i], dest[3*i + 1]};
    destTmp[i] = strtoul(str, NULL, 16);
  }
  
  init(src, destTmp, dat);
}

Packet::Packet(uint8_t* dest, const char* dat) {
  uint8_t src[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  getLoRaAddress(src);
  
  init(src, dest, dat);
}

Packet::Packet(const char* src, const char* dest, const char* dat) {
  uint8_t srcTmp[8];
  for(uint8_t i = 0; i < 8; i++) {
    char str[] = {src[3*i], src[3*i + 1]};
    srcTmp[i] = strtoul(str, NULL, 16);
  }
  
  uint8_t destTmp[8];
  for(uint8_t i = 0; i < 8; i++) {
    char str[] = {dest[3*i], dest[3*i + 1]};
    destTmp[i] = strtoul(str, NULL, 16);
  }
  
  init(srcTmp, destTmp, dat);
}

Packet::Packet(uint8_t* src, uint8_t* dest, const char* dat) {
  init(src, dest, dat);
}

Packet::Packet(const char* dest, uint8_t* dat, uint8_t len) {
  uint8_t src[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  getLoRaAddress(src);
  
  uint8_t destTmp[8];
  for(uint8_t i = 0; i < 8; i++) {
    char str[] = {dest[3*i], dest[3*i + 1]};
    destTmp[i] = strtoul(str, NULL, 16);
  }
  
  init(src, destTmp, dat, len);
}

Packet::Packet(uint8_t* dest, uint8_t* dat, uint8_t len) {
  uint8_t src[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  getLoRaAddress(src);
  
  init(src, dest, dat, len);
}

Packet::Packet(const char* src, const char* dest, uint8_t* dat, uint8_t len) {
  uint8_t srcTmp[8];
  for(uint8_t i = 0; i < 8; i++) {
    char str[] = {src[3*i], src[3*i + 1]};
    srcTmp[i] = strtoul(str, NULL, 16);
  }
  
  uint8_t destTmp[8];
  for(uint8_t i = 0; i < 8; i++) {
    char str[] = {dest[3*i], dest[3*i + 1]};
    destTmp[i] = strtoul(str, NULL, 16);
  }
  
  init(srcTmp, destTmp, dat, len);
}

Packet::Packet(uint8_t* src, uint8_t* dest, uint8_t* dat, uint8_t len) {
  init(src, dest, dat, len);
}

void Packet::init(uint8_t* src, uint8_t* dest, const char* dat) {
  init(src, dest, (uint8_t*)dat, strlen(dat));
}

void Packet::init(uint8_t* src, uint8_t* dest, uint8_t* dat, uint8_t len) {
  memcpy(source, src, 8);
  memcpy(destination, dest, 8);
  length = len + 16;
  data = new char[len + 1];
  memcpy(data, dat, len + 1);
}

String Packet::getSourceStr() {
  char charArray[24];
  
  for(uint8_t i = 0; i < 8; i++) {
    sprintf(charArray + 3*i, "%02X", source[i]);
    charArray[3*i+2] = ':';
  }
  charArray[23] = '\0';
  
  String str(charArray);
  return(str);
}

String Packet::getDestinationStr() {
  char charArray[24];
  
  for(uint8_t i = 0; i < 8; i++) {
    sprintf(charArray + 3*i, "%02X", destination[i]);
    charArray[3*i+2] = ':';
  }
  charArray[23] = '\0';
  
  String str(charArray);
  return(str);
}

void Packet::setSourceStr(const char* src) {
  for(uint8_t i = 0; i < 8; i++) {
    char str[] = {src[3*i], src[3*i + 1]};
    source[i] = strtoul(str, NULL, 16);
  }
}

void Packet::setDestinationStr(const char* dest) {
  for(uint8_t i = 0; i < 8; i++) {
    char str[] = {dest[3*i], dest[3*i + 1]};
    destination[i] = strtoul(str, NULL, 16);
  }
}

void Packet::copyInto(Packet& pack) {
  memcpy(pack.source, source, 8);
  memcpy(pack.destination, destination, 8);
  strcpy(pack.data, data);
}

void Packet::setPacketData(char* charArray) {
  char* newData = new char[strlen(charArray)];
  length = strlen(charArray) + 16;
  strcpy(newData, charArray);
  delete[] data;
  data = newData;
}

void Packet::setPacketData(String str) {
  setPacketData((char*)str.c_str());
}

void Packet::setPacketData(float f, uint8_t decimals) {
  int i = f;
  float res = f - i;
  if (res == 0) {
    char charArray[16];
    itoa(i, charArray, 10);
    setPacketData(charArray);
  } else {
    String floatString = String(f, decimals);
    setPacketData(floatString);
  }
}

void Packet::getLoRaAddress(uint8_t* addr) {
  for(uint8_t i = 0; i < 8; i++) {
    addr[i] = EEPROM.read(i);
  }
}
