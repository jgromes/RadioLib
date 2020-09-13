#include "PhysicalLayer.h"

PhysicalLayer::PhysicalLayer(float freqStep, size_t maxPacketLength) {
  _freqStep = freqStep;
  _maxPacketLength = maxPacketLength;
}

int16_t PhysicalLayer::transmit(__FlashStringHelper* fstr, uint8_t addr) {
  // read flash string length
  size_t len = 0;
  PGM_P p = reinterpret_cast<PGM_P>(fstr);
  while(true) {
    char c = RADIOLIB_PROGMEM_READ_BYTE(p++);
    len++;
    if(c == '\0') {
      break;
    }
  }

  // dynamically allocate memory
  #ifdef RADIOLIB_STATIC_ONLY
    char str[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    char* str = new char[len];
  #endif

  // copy string from flash
  p = reinterpret_cast<PGM_P>(fstr);
  for(size_t i = 0; i < len; i++) {
    str[i] = RADIOLIB_PROGMEM_READ_BYTE(p + i);
  }

  // transmit string
  int16_t state = transmit(str, addr);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] str;
  #endif
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
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t data[RADIOLIB_STATIC_ARRAY_SIZE + 1];
  #else
    uint8_t* data = new uint8_t[length + 1];
    if(!data) {
      return(ERR_MEMORY_ALLOCATION_FAILED);
    }
  #endif

  // read the received data
  state = readData(data, length);

  if(state == ERR_NONE) {
    // add null terminator
    data[length] = 0;

    // initialize Arduino String class
    str = String((char*)data);
  }

  // deallocate temporary buffer
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] data;
  #endif

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
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t data[RADIOLIB_STATIC_ARRAY_SIZE + 1];
  #else
    uint8_t* data = new uint8_t[length + 1];
    if(!data) {
      return(ERR_MEMORY_ALLOCATION_FAILED);
    }
  #endif

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
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] data;
  #endif

  return(state);
}

float PhysicalLayer::getFreqStep() const {
  return(_freqStep);
}

int32_t PhysicalLayer::random(int32_t max) {
  if(max == 0) {
    return(0);
  }

  // get random bytes from the radio
  uint8_t randBuff[4];
  for(uint8_t i = 0; i < 4; i++) {
    randBuff[i] = random();
  }

  // create 32-bit TRNG number
  int32_t randNum = ((int32_t)randBuff[0] << 24) | ((int32_t)randBuff[1] << 16) | ((int32_t)randBuff[2] << 8) | ((int32_t)randBuff[3]);
  return(randNum % max);
}

int32_t PhysicalLayer::random(int32_t min, int32_t max) {
  if(min >= max) {
    return(min);
  }

  return(PhysicalLayer::random(max - min) + min);
}
