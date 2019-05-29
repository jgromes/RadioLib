#include "PSK.h"

VaricodeString::VaricodeString(char c) {
  _len = 1;
  _str = new char[1];
  _str[0] = c;
}

VaricodeString::VaricodeString(const char* str) {
  _len = strlen(str);
  _str = new char[_len];
  strcpy(_str, str);
}

VaricodeString::~VaricodeString() {
  delete[] _str;
}

size_t VaricodeString::length() {
  return(_len);
}

uint16_t* VaricodeString::byteArr() {
  uint16_t* arr = new uint16_t[_len];
  for(size_t i = 0; i < _len; i++) {
    arr[i] = VaricodeTable[(uint8_t)_str[i]];
  }
  return(arr);
}

PSKClient::PSKClient(PhysicalLayer* phy) {
  _phy = phy;
}

int16_t PSKClient::begin(int pin, float carrier, float rate, uint16_t audioFreq) {
  pinMode(pin, OUTPUT);

  // save configuration
  _pin = pin;
  _audioFreq = audioFreq;

  // calculate duration of 1 bit
  _bitDuration = (uint32_t)(1000000.0/rate);

  // calculate 24-bit frequency
  uint32_t mult = 1;
  _carrier = (carrier * (mult << _phy->getDivExponent())) / _phy->getCrystalFreq();

  // set FSK frequency deviation to 0
  int16_t state = _phy->setFrequencyDeviation(0);

  return(state);
}

size_t PSKClient::write(uint16_t* buff, size_t len) {
  size_t n = 0;
  for(size_t i = 0; i < len; i++) {
    n += PSKClient::write(buff[i]);
  }
  return(n);
}

size_t PSKClient::write(uint16_t code) {
  // get number of bits in character code
  uint8_t dataBits = 10;
  for(dataBits = 10; dataBits > 0; dataBits--) {
    if(code & (0x0001 << dataBits)) {
      break;
    }
  }

  // send code
  for(uint16_t mask = 0x01; mask <= (uint16_t)(0x01 << (dataBits - 1)); mask <<= 1) {
    if(code & mask) {
      mark();
    } else {
      space();
    }
  }

  // character end
  space();
  space();

  return(1);
}

size_t PSKClient::print(VaricodeString& var) {
  uint16_t* arr = var.byteArr();
  size_t n = PSKClient::write(arr, var.length());
  delete[] arr;
  return(n);
}

size_t PSKClient::print(const char str[]) {
  VaricodeString var = str;
  return(PSKClient::print(var));
}

size_t PSKClient::println() {
  VaricodeString lf = "\r\n";
  return(PSKClient::print(lf));
}

size_t PSKClient::println(VaricodeString& var) {
  size_t n = PSKClient::print(var);
  n += PSKClient::println();
  return(n);
}

size_t PSKClient::println(const char str[]) {
  size_t n = PSKClient::print(str);
  n += PSKClient::println();
  return(n);
}

void PSKClient::mark() {
  // do not perform phase change
  uint32_t start = micros();
  while(micros() - start < _bitDuration);
}

void PSKClient::space() {
  // change phase by 180 degrees
  uint32_t start = micros();
  // TODO: flip phase here
  while(micros() - start < _bitDuration);
}
