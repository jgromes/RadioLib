#include "RTTY.h"

RTTYClient::RTTYClient(PhysicalLayer* phy) {
  _phy = phy;
}

int16_t RTTYClient::begin(float base, uint16_t shift, uint16_t rate, uint8_t dataBits, uint8_t stopBits) {
  // check supplied values
  if(shift % 61 != 0) {
    return(ERR_INVALID_RTTY_SHIFT);
  }
  
  // save configuration
  _shift = shift / 61;
  _dataBits = dataBits;
  _stopBits = stopBits;
  
  // calculate duration of 1 bit
  _bitDuration = (uint32_t)1000000/rate;
  
  // calculate 24-bit frequency
  uint32_t mult = 1;
  _base = (base * (mult << 19)) / 32.0;

  // set module frequency deviation to 0
  int16_t state = _phy->setFrequencyDeviation(0);
  
  return(state);
}

void RTTYClient::leadIn(uint16_t length) {
  _phy->directMode();
  
  mark();
  delay(length);
}

size_t RTTYClient::print(const String& str) {
  return(RTTYClient::print(str.c_str()));
}

size_t RTTYClient::print(const char* str) {
  size_t len = 0;
  for(size_t i = 0; i < strlen(str); i++) {
    len += write(str[i]);
  }
  return(len);
}

size_t RTTYClient::println(const String& str) {
  return(RTTYClient::println(str.c_str()));
}

size_t RTTYClient::println(const char* str) {
  size_t len = print(str);
  len += write('\r');
  len += write('\n');
  return(len);
}

size_t RTTYClient::write(uint8_t b) {
  space();
  
  for(uint8_t mask = 0x01; mask; mask <<= 1) {
    if(b & mask) {
      mark();
    } else {
      space();
    }
  }
  
  for(uint8_t i = 0; i < _stopBits; i++) {
    mark();
  }
  
  return(1);
}

void RTTYClient::mark() {
  uint32_t start = micros();
  _phy->directMode(_base + _shift);
  while(micros() - start < _bitDuration);
}

void RTTYClient::space() {
  uint32_t start = micros();
  _phy->directMode(_base);
  while(micros() - start < _bitDuration);
}
