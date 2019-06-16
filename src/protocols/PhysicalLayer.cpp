#include "PhysicalLayer.h"

PhysicalLayer::PhysicalLayer(float crysFreq, uint8_t divExp, size_t maxPacketLength) {
  _crystalFreq = crysFreq;
  _divExponent = divExp;
  _maxPacketLength = maxPacketLength;
}

int16_t PhysicalLayer::transmit(__FlashStringHelper* fstr, uint8_t addr) {
  // read flash string length
  size_t len = 0;
  PGM_P p = reinterpret_cast<PGM_P>(fstr);
  while(true) {
    char c = pgm_read_byte(p++);
    len++;
    if(c == '\0') {
      break;
    }
  }

  // dynamically allocate memory
  char* str = new char[len];

  // copy string from flash
  p = reinterpret_cast<PGM_P>(fstr);
  for(size_t i = 0; i < len; i++) {
    str[i] = pgm_read_byte(p + i);
  }

  // transmit string
  int16_t state = transmit(str, addr);
  delete[] str;
  return(state);
}

int16_t PhysicalLayer::transmit(String& str, uint8_t addr) {
  return(transmit(str.c_str(), addr));
}

int16_t PhysicalLayer::transmit(const char* str, uint8_t addr) {
  return(transmit((uint8_t*)str, strlen(str), addr));
}

int16_t PhysicalLayer::startTransmit(String& str, uint8_t addr) {
  return(startTransmit(str.c_str(), addr));
}

int16_t PhysicalLayer::startTransmit(const char* str, uint8_t addr) {
  return(startTransmit((uint8_t*)str, strlen(str), addr));
}

int16_t PhysicalLayer::readData(String& str, size_t len) {
  int16_t state = ERR_NONE;

  // read the number of actually received bytes
  size_t length = getPacketLength();

  if((len < length) && (len != 0)) {
    // user requested less bytes than were received, this is allowed (but frowned upon)
    // requests for more data than were received will only return the number of actually received bytes (unlike PhysicalLayer::receive())
    length = len;
  }

  // build a temporary buffer
  uint8_t* data = new uint8_t[length + 1];
  if(!data) {
    return(ERR_MEMORY_ALLOCATION_FAILED);
  }

  // read the received data
  state = readData(data, length);

  if(state == ERR_NONE) {
    // add null terminator
    data[length] = 0;

    // initialize Arduino String class
    str = String((char*)data);
  }

  // deallocate temporary buffer
  delete[] data;

  return(state);
}

int16_t PhysicalLayer::receive(String& str, size_t len) {
  int16_t state = ERR_NONE;

  // user can override the length of data to read
  size_t length = len;

  if(len == 0) {
    // unknown packet length, set to maximum
    length = _maxPacketLength;
  }

  // build a temporary buffer
  uint8_t* data = new uint8_t[length + 1];
  if(!data) {
    return(ERR_MEMORY_ALLOCATION_FAILED);
  }

  // attempt packet reception
  state = receive(data, length);

  if(state == ERR_NONE) {
    // read the number of actually received bytes (for unknown packets)
    if(len == 0) {
      length = getPacketLength(false);
    }

    // add null terminator
    data[length] = 0;

    // initialize Arduino String class
    str = String((char*)data);
  }

  // deallocate temporary buffer
  delete[] data;

  return(state);
}

float PhysicalLayer::getCrystalFreq() {
  return(_crystalFreq);
}

uint8_t PhysicalLayer::getDivExponent() {
  return(_divExponent);
}
