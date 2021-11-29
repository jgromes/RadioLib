#include "PhysicalLayer.h"

PhysicalLayer::PhysicalLayer(float freqStep, size_t maxPacketLength) {
  _freqStep = freqStep;
  _maxPacketLength = maxPacketLength;
  _bufferBitPos = 0;
  _bufferWritePos = 0;
}

int16_t PhysicalLayer::transmit(__FlashStringHelper* fstr, uint8_t addr) {
  // read flash string length
  size_t len = 0;
  PGM_P p = reinterpret_cast<PGM_P>(fstr);
  while(true) {
    char c = RADIOLIB_NONVOLATILE_READ_BYTE(p++);
    len++;
    if(c == '\0') {
      break;
    }
  }

  // dynamically allocate memory
  #if defined(RADIOLIB_STATIC_ONLY)
    char str[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    char* str = new char[len];
  #endif

  // copy string from flash
  p = reinterpret_cast<PGM_P>(fstr);
  for(size_t i = 0; i < len; i++) {
    str[i] = RADIOLIB_NONVOLATILE_READ_BYTE(p + i);
  }

  // transmit string
  int16_t state = transmit(str, addr);
  #if !defined(RADIOLIB_STATIC_ONLY)
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
  int16_t state = RADIOLIB_ERR_NONE;

  // read the number of actually received bytes
  size_t length = getPacketLength();

  if((len < length) && (len != 0)) {
    // user requested less bytes than were received, this is allowed (but frowned upon)
    // requests for more data than were received will only return the number of actually received bytes (unlike PhysicalLayer::receive())
    length = len;
  }

  // build a temporary buffer
  #if defined(RADIOLIB_STATIC_ONLY)
    uint8_t data[RADIOLIB_STATIC_ARRAY_SIZE + 1];
  #else
    uint8_t* data = new uint8_t[length + 1];
    if(!data) {
      return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
    }
  #endif

  // read the received data
  state = readData(data, length);

  if(state == RADIOLIB_ERR_NONE) {
    // add null terminator
    data[length] = 0;

    // initialize Arduino String class
    str = String((char*)data);
  }

  // deallocate temporary buffer
  #if !defined(RADIOLIB_STATIC_ONLY)
    delete[] data;
  #endif

  return(state);
}

int16_t PhysicalLayer::receive(String& str, size_t len) {
  int16_t state = RADIOLIB_ERR_NONE;

  // user can override the length of data to read
  size_t length = len;

  // build a temporary buffer
  #if defined(RADIOLIB_STATIC_ONLY)
    uint8_t data[RADIOLIB_STATIC_ARRAY_SIZE + 1];
  #else
    uint8_t* data = NULL;
    if(length == 0) {
      data = new uint8_t[_maxPacketLength + 1];
    } else {
      data = new uint8_t[length + 1];
    }
    if(!data) {
      return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
    }
  #endif

  // attempt packet reception
  state = receive(data, length);

  if(state == RADIOLIB_ERR_NONE) {
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
  #if !defined(RADIOLIB_STATIC_ONLY)
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
    randBuff[i] = randomByte();
  }

  // create 32-bit TRNG number
  int32_t randNum = ((int32_t)randBuff[0] << 24) | ((int32_t)randBuff[1] << 16) | ((int32_t)randBuff[2] << 8) | ((int32_t)randBuff[3]);
  if(randNum < 0) {
    randNum *= -1;
  }
  RADIOLIB_DEBUG_PRINTLN(randNum);
  return(randNum % max);
}

int32_t PhysicalLayer::random(int32_t min, int32_t max) {
  if(min >= max) {
    return(min);
  }

  return(PhysicalLayer::random(max - min) + min);
}

int16_t PhysicalLayer::startDirect() {
  // disable encodings
  int16_t state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  // disable shaping
  state = setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);

  // set frequency deviation to the lowest possible value
  state = setFrequencyDeviation(-1);
  return(state);
}

int16_t PhysicalLayer::available() {
  return(_bufferWritePos);
}

uint8_t PhysicalLayer::read() {
  if(_directSyncWordLen > 0) {
    _gotSync = false;
    _syncBuffer = 0;
  }
  _bufferWritePos--;
  return(_buffer[_bufferReadPos++]);
}

int16_t PhysicalLayer::setDirectSyncWord(uint32_t syncWord, uint8_t len) {
  if(len > 32) {
    return(RADIOLIB_ERR_INVALID_SYNC_WORD);
  }
  _directSyncWordMask = 0xFFFFFFFF >> (32 - len);
  _directSyncWordLen = len;
  _directSyncWord = syncWord;

  // override sync word matching when length is set to 0
  if(_directSyncWordLen == 0) {
    _gotSync = true;
  }

  return(RADIOLIB_ERR_NONE);
}

void PhysicalLayer::updateDirectBuffer(uint8_t bit) {
  // check sync word
  if(!_gotSync) {
    _syncBuffer <<= 1;
    _syncBuffer |= bit;
    if((_syncBuffer & _directSyncWordMask) == _directSyncWord) {
      _gotSync = true;
      _bufferWritePos = 0;
      _bufferReadPos = 0;
      _bufferBitPos = 0;
    }

  } else {
    // save the bit
    if(bit) {
      _buffer[_bufferWritePos] |= 0x01 << _bufferBitPos;
    } else {
      _buffer[_bufferWritePos] &= ~(0x01 << _bufferBitPos);
    }
    _bufferBitPos++;

    // check complete byte
    if(_bufferBitPos == 8) {
      _buffer[_bufferWritePos] = Module::flipBits(_buffer[_bufferWritePos]);
      _bufferWritePos++;
      _bufferBitPos = 0;
    }
  }
}
