#include "PhysicalLayer.h"

PhysicalLayer::PhysicalLayer(float freqStep, size_t maxPacketLength) {
  _freqStep = freqStep;
  _maxPacketLength = maxPacketLength;
  _bufferBitPos = 0;
  _bufferWritePos = 0;
}

int16_t PhysicalLayer::transmit(const char* str, uint8_t addr) {
  return(transmit((uint8_t*)str, strlen(str), addr));
}

int16_t PhysicalLayer::startTransmit(const char* str, uint8_t addr) {
  return(startTransmit((uint8_t*)str, strlen(str), addr));
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
