#include "RTTY.h"

ITA2::ITA2(const char* str) {
  _len = strlen(str);
  _str = new char[_len];
  strcpy(_str, str);
}

ITA2::~ITA2() {
  delete[] _str;
}

size_t ITA2::length() {
  return(_len);
}

uint8_t* ITA2::byteArr() {
  // create temporary array 3x the string length (figures may be 3 bytes)
  uint8_t* temp = new uint8_t[_len*3];
  
  size_t arrayLen = 0;
  for(size_t i = 0; i < _len; i++) {
    uint16_t code = getBits(_str[i]);
    // check if the code is letter or figure
    if(code & (ITA2_FIGS << 10)) {
      temp[arrayLen] = ITA2_FIGS;
      temp[arrayLen + 1] = (code >> 5) & ITA2_LTRS;
      temp[arrayLen + 2] = ITA2_LTRS;
      arrayLen += 3;
    } else {
      temp[arrayLen] = code;
      arrayLen += 1;
    }
  }
  
  uint8_t* arr = new uint8_t[arrayLen];
  memcpy(arr, temp, arrayLen);
  delete[] temp;
  return(arr);
}

uint16_t ITA2::getBits(char c) {
  // search ITA2 table
  uint16_t code = 0x0000;
  for(uint8_t i = 0; i < ITA2_LENGTH; i++) {
    if(ITA2Table[i][0] == c) {
      // character is in letter shift
      code = i;
      break;
    } else if(ITA2Table[i][1] == c) {
      // character is in figures shift
      code = (ITA2_FIGS << 10) | (i << 5) | ITA2_LTRS;
      break;
    }
  }
  
  return(code);
}

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

void RTTYClient::idle() {
  _phy->transmitDirect();
  
  mark();
}

size_t RTTYClient::write(const char* str) {
  if(str == NULL) {
    return(0);
  }
  return(RTTYClient::write((uint8_t *)str, strlen(str)));
}

size_t RTTYClient::write(uint8_t* buff, size_t len) {
  size_t n = 0;
  for(size_t i = 0; i < len; i++) {
    n += write(buff[i]);
  }
  return(n);
}

size_t RTTYClient::write(uint8_t b) {
  space();
  
  for(uint16_t mask = 0x01; mask <= (0x01 << (_dataBits - 1)); mask <<= 1) {
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

size_t RTTYClient::print(ITA2& ita2) {
  return(RTTYClient::write(ita2.byteArr(), ita2.length()));
}

size_t RTTYClient::print(const String& str) {
  return(RTTYClient::write((uint8_t*)str.c_str(), str.length()));
}

size_t RTTYClient::print(const char str[]) {
  return(RTTYClient::write((uint8_t*)str, strlen(str)));
}

size_t RTTYClient::print(char c) {
  return(RTTYClient::write(c));
}

size_t RTTYClient::print(unsigned char b, int base) {
  return(RTTYClient::print((unsigned long)b, base));
}

size_t RTTYClient::print(int n, int base) {
  return(RTTYClient::print((long)n, base));
}

size_t RTTYClient::print(unsigned int n, int base) {
  return(RTTYClient::print((unsigned long)n, base));
}

size_t RTTYClient::print(long n, int base) {
  if(base == 0) {
    return(RTTYClient::write(n));
  } else if(base == DEC) {
    if (n < 0) {
      int t = RTTYClient::print('-');
      n = -n;
      return(RTTYClient::printNumber(n, DEC) + t);
    }
    return(RTTYClient::printNumber(n, DEC));
  } else {
    return(RTTYClient::printNumber(n, base));
  }
}

size_t RTTYClient::print(unsigned long n, int base) {
  if(base == 0) {
    return(RTTYClient::write(n));
  } else {
    return(RTTYClient::printNumber(n, base));
  }
}

size_t RTTYClient::print(double n, int digits) {
  return(RTTYClient::printFloat(n, digits));
}

size_t RTTYClient::println(void) {
  size_t n = 0;
  if(_dataBits == 5) {
    // use ITA2 line
    ITA2 lf = "\r\n";
    n = RTTYClient::print(lf);
  } else {
    // use ASCII line
    n = RTTYClient::write("\r\n");
  }
  return(n);
}

size_t RTTYClient::println(ITA2& ita2) {
  size_t n = RTTYClient::print(ita2);
  n += RTTYClient::println();
  return(n);
}

size_t RTTYClient::println(const String& str) {
  size_t n = RTTYClient::print(str);
  n += RTTYClient::println();
  return(n);
}

size_t RTTYClient::println(const char* str) {
  size_t n = RTTYClient::print(str);
  n += RTTYClient::println();
  return(n);
}

size_t RTTYClient::println(char c) {
  size_t n = RTTYClient::print(c);
  n += RTTYClient::println();
  return(n);
}

size_t RTTYClient::println(unsigned char b, int base) {
  size_t n = RTTYClient::print(b, base);
  n += RTTYClient::println();
  return(n);
}

size_t RTTYClient::println(int num, int base) {
  size_t n = RTTYClient::print(num, base);
  n += RTTYClient::println();
  return(n);
}

size_t RTTYClient::println(unsigned int num, int base) {
  size_t n = RTTYClient::print(num, base);
  n += RTTYClient::println();
  return(n);
}

size_t RTTYClient::println(long num, int base) {
  size_t n = RTTYClient::print(num, base);
  n += RTTYClient::println();
  return(n);
}

size_t RTTYClient::println(unsigned long num, int base) {
  size_t n = RTTYClient::print(num, base);
  n += RTTYClient::println();
  return(n);
}

size_t RTTYClient::println(double d, int digits) {
  size_t n = RTTYClient::print(d, digits);
  n += RTTYClient::println();
  return(n);
}

void RTTYClient::mark() {
  uint32_t start = micros();
  _phy->transmitDirect(_base + _shift);
  while(micros() - start < _bitDuration);
}

void RTTYClient::space() {
  uint32_t start = micros();
  _phy->transmitDirect(_base);
  while(micros() - start < _bitDuration);
}

size_t RTTYClient::printNumber(unsigned long n, uint8_t base) {
  char buf[8 * sizeof(long) + 1];
  char *str = &buf[sizeof(buf) - 1];

  *str = '\0';

  if(base < 2) {
    base = 10;
  }

  do {
    char c = n % base;
    n /= base;

    *--str = c < 10 ? c + '0' : c + 'A' - 10;
  } while(n);

  return(RTTYClient::write(str));
}

size_t RTTYClient::printFloat(double number, uint8_t digits)  { 
  size_t n = 0;
  
  if (isnan(number)) return RTTYClient::print("nan");
  if (isinf(number)) return RTTYClient::print("inf");
  if (number > 4294967040.0) return RTTYClient::print("ovf");  // constant determined empirically
  if (number <-4294967040.0) return RTTYClient::print("ovf");  // constant determined empirically
  
  // Handle negative numbers
  if (number < 0.0) {
     n += RTTYClient::print('-');
     number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for(uint8_t i = 0; i < digits; ++i) {
    rounding /= 10.0;
  }
  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  n += RTTYClient::print(int_part);

  // Print the decimal point, but only if there are digits beyond
  if(digits > 0) {
    n += RTTYClient::print('.'); 
  }

  // Extract digits from the remainder one at a time
  while(digits-- > 0) {
    remainder *= 10.0;
    unsigned int toPrint = (unsigned int)(remainder);
    n += RTTYClient::print(toPrint);
    remainder -= toPrint; 
  } 
  
  return n;
}
