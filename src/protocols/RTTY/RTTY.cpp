#include "RTTY.h"
#include <string.h>
#include <math.h>
#if !defined(RADIOLIB_EXCLUDE_RTTY)

ITA2String::ITA2String(char c) {
  asciiLen = 1;
  #if !defined(RADIOLIB_STATIC_ONLY)
  strAscii = new char[1];
  #endif
  strAscii[0] = c;
  ita2Len = 0;
}

ITA2String::ITA2String(const char* str) {
  asciiLen = strlen(str);
  #if !defined(RADIOLIB_STATIC_ONLY)
  strAscii = new char[asciiLen + 1];
  #endif
  strcpy(strAscii, str);
  ita2Len = 0;
}

ITA2String::~ITA2String() {
  #if !defined(RADIOLIB_STATIC_ONLY)
    delete[] strAscii;
  #endif
}

size_t ITA2String::length() {
  // length returned by this method is different than the length of ASCII-encoded strAscii
  // ITA2-encoded string length varies based on how many number and characters the string contains

  if(ita2Len == 0) {
    // ITA2 length wasn't calculated yet, call byteArr() to calculate it
    byteArr();
  }

  return(ita2Len);
}

uint8_t* ITA2String::byteArr() {
  // create temporary array 2x the string length (figures may be 3 bytes)
  #if defined(RADIOLIB_STATIC_ONLY)
    uint8_t temp[RADIOLIB_STATIC_ARRAY_SIZE*2 + 1];
  #else
    uint8_t* temp = new uint8_t[asciiLen*2 + 1];
  #endif

  size_t arrayLen = 0;
  bool flagFigure = false;
  for(size_t i = 0; i < asciiLen; i++) {
    uint16_t code = getBits(strAscii[i]);
    uint8_t shift = (code >> 5) & 0b11111;
    uint8_t character = code & 0b11111;
    // check if the code is letter or figure
    if(shift == RADIOLIB_ITA2_FIGS) {
      // check if this is the first figure in sequence
      if(!flagFigure) {
        flagFigure = true;
        temp[arrayLen++] = RADIOLIB_ITA2_FIGS;
      }

      // add the character code
      temp[arrayLen++] = character & 0b11111;

      // check the following character (skip for message end)
      if(i < (asciiLen - 1)) {
        uint16_t nextCode = getBits(strAscii[i+1]);
        uint8_t nextShift = (nextCode >> 5) & 0b11111;
        if(nextShift == RADIOLIB_ITA2_LTRS) {
          // next character is a letter, terminate figure shift
          temp[arrayLen++] = RADIOLIB_ITA2_LTRS;
          flagFigure = false;
        }
      } else {
        // reached the end of the message, terminate figure shift
        temp[arrayLen++] = RADIOLIB_ITA2_LTRS;
        flagFigure = false;
      }
    } else {
      temp[arrayLen++] = character & 0b11111;
    }
  }

  // save ITA2 string length
  ita2Len = arrayLen;

  uint8_t* arr = new uint8_t[arrayLen];
  memcpy(arr, temp, arrayLen);
  #if !defined(RADIOLIB_STATIC_ONLY)
    delete[] temp;
  #endif

  return(arr);
}

uint16_t ITA2String::getBits(char c) {
  // search ITA2 table
  uint16_t code = 0x0000;
  for(uint8_t i = 0; i < RADIOLIB_ITA2_LENGTH; i++) {
    if(RADIOLIB_NONVOLATILE_READ_BYTE(&ITA2Table[i][0]) == c) {
      // character is in letter shift
      code = (RADIOLIB_ITA2_LTRS << 5) | i;
      break;
    } else if(RADIOLIB_NONVOLATILE_READ_BYTE(&ITA2Table[i][1]) == c) {
      // character is in figures shift
      code = (RADIOLIB_ITA2_FIGS << 5) | i;
      break;
    }
  }

  return(code);
}

RTTYClient::RTTYClient(PhysicalLayer* phy) {
  phyLayer = phy;
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  audioClient = nullptr;
  #endif
}

#if !defined(RADIOLIB_EXCLUDE_AFSK)
RTTYClient::RTTYClient(AFSKClient* audio) {
  phyLayer = audio->phyLayer;
  audioClient = audio;
}
#endif

int16_t RTTYClient::begin(float base, uint32_t shift, uint16_t rate, uint8_t enc, uint8_t stopBits) {
  // save configuration
  encoding = enc;
  stopBitsNum = stopBits;
  baseFreqHz = base;
  shiftFreqHz = shift;

  switch(encoding) {
    case RADIOLIB_ASCII:
      dataBitsNum = 7;
      break;
    case RADIOLIB_ASCII_EXTENDED:
      dataBitsNum = 8;
      break;
    case RADIOLIB_ITA2:
      dataBitsNum = 5;
      break;
    default:
      return(RADIOLIB_ERR_UNSUPPORTED_ENCODING);
  }

  // calculate duration of 1 bit
  bitDuration = (uint32_t)1000000/rate;

  // calculate module carrier frequency resolution
  uint32_t step = round(phyLayer->getFreqStep());

  // check minimum shift value
  if(shift < step / 2) {
    return(RADIOLIB_ERR_INVALID_RTTY_SHIFT);
  }

  // round shift to multiples of frequency step size
  if(shift % step < (step / 2)) {
    shiftFreq = shift / step;
  } else {
    shiftFreq = (shift / step) + 1;
  }

  // calculate 24-bit frequency
  baseFreq = (base * 1000000.0) / phyLayer->getFreqStep();

  // configure for direct mode
  return(phyLayer->startDirect());
}

void RTTYClient::idle() {
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
    n += RTTYClient::write(buff[i]);
  }
  return(n);
}

size_t RTTYClient::write(uint8_t b) {
  space();

  uint16_t maxDataMask = 0x01 << (dataBitsNum - 1);
  for(uint16_t mask = 0x01; mask <= maxDataMask; mask <<= 1) {
    if(b & mask) {
      mark();
    } else {
      space();
    }
  }

  for(uint8_t i = 0; i < stopBitsNum; i++) {
    mark();
  }

  return(1);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
size_t RTTYClient::print(__FlashStringHelper* fstr) {
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

  size_t n = 0;
  if(encoding == RADIOLIB_ITA2) {
    ITA2String ita2 = ITA2String(str);
    n = RTTYClient::print(ita2);
  } else if((encoding == RADIOLIB_ASCII) || (encoding == RADIOLIB_ASCII_EXTENDED)) {
    n = RTTYClient::write((uint8_t*)str, len);
  }
  #if !defined(RADIOLIB_STATIC_ONLY)
    delete[] str;
  #endif
  return(n);
}

size_t RTTYClient::print(const String& str) {
  size_t n = 0;
  if(encoding == RADIOLIB_ITA2) {
    ITA2String ita2 = ITA2String(str.c_str());
    n = RTTYClient::print(ita2);
  } else if((encoding == RADIOLIB_ASCII) || (encoding == RADIOLIB_ASCII_EXTENDED)) {
    n = RTTYClient::write((uint8_t*)str.c_str(), str.length());
  }
  return(n);
}
#endif

size_t RTTYClient::print(ITA2String& ita2) {
  uint8_t* arr = ita2.byteArr();
  size_t n = RTTYClient::write(arr, ita2.length());
  delete[] arr;
  return(n);
}

size_t RTTYClient::print(const char str[]) {
  size_t n = 0;
  if(encoding == RADIOLIB_ITA2) {
    ITA2String ita2 = ITA2String(str);
    n = RTTYClient::print(ita2);
  } else if((encoding == RADIOLIB_ASCII) || (encoding == RADIOLIB_ASCII_EXTENDED)) {
    n = RTTYClient::write((uint8_t*)str, strlen(str));
  }
  return(n);
}

size_t RTTYClient::print(char c) {
  size_t n = 0;
  if(encoding == RADIOLIB_ITA2) {
    ITA2String ita2 = ITA2String(c);
    n = RTTYClient::print(ita2);
  } else if((encoding == RADIOLIB_ASCII) || (encoding == RADIOLIB_ASCII_EXTENDED)) {
    n = RTTYClient::write(c);
  }
  return(n);
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
  if(encoding == RADIOLIB_ITA2) {
    ITA2String lf = ITA2String("\r\n");
    n = RTTYClient::print(lf);
  } else if((encoding == RADIOLIB_ASCII) || (encoding == RADIOLIB_ASCII_EXTENDED)) {
    n = RTTYClient::write("\r\n");
  }
  return(n);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
size_t RTTYClient::println(__FlashStringHelper* fstr) {
  size_t n = RTTYClient::print(fstr);
  n += RTTYClient::println();
  return(n);
}

size_t RTTYClient::println(const String& str) {
  size_t n = RTTYClient::print(str);
  n += RTTYClient::println();
  return(n);
}
#endif

size_t RTTYClient::println(ITA2String& ita2) {
  size_t n = RTTYClient::print(ita2);
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
  Module* mod = phyLayer->getMod();
  uint32_t start = mod->hal->micros();
  transmitDirect(baseFreq + shiftFreq, baseFreqHz + shiftFreqHz);
  mod->waitForMicroseconds(start, bitDuration);
}

void RTTYClient::space() {
  Module* mod = phyLayer->getMod();
  uint32_t start = mod->hal->micros();
  transmitDirect(baseFreq, baseFreqHz);
  mod->waitForMicroseconds(start, bitDuration);
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

  size_t l = 0;
  if(encoding == RADIOLIB_ITA2) {
    ITA2String ita2 = ITA2String(str);
    uint8_t* arr = ita2.byteArr();
    l = RTTYClient::write(arr, ita2.length());
    delete[] arr;
  } else if((encoding == RADIOLIB_ASCII) || (encoding == RADIOLIB_ASCII_EXTENDED)) {
    l = RTTYClient::write(str);
  }

  return(l);
}

/// \todo improve ITA2 float print speed (characters are sent one at a time)
size_t RTTYClient::printFloat(double number, uint8_t digits)  {
  size_t n = 0;

  char code[] = {0x00, 0x00, 0x00, 0x00};
  if (isnan(number)) strcpy(code, "nan");
  if (isinf(number)) strcpy(code, "inf");
  if (number > 4294967040.0) strcpy(code, "ovf");  // constant determined empirically
  if (number <-4294967040.0) strcpy(code, "ovf");  // constant determined empirically

  if(code[0] != 0x00) {
    if(encoding == RADIOLIB_ITA2) {
      ITA2String ita2 = ITA2String(code);
      uint8_t* arr = ita2.byteArr();
      n = RTTYClient::write(arr, ita2.length());
      delete[] arr;
      return(n);
    } else if((encoding == RADIOLIB_ASCII) || (encoding == RADIOLIB_ASCII_EXTENDED)) {
      return(RTTYClient::write(code));
    }
  }

  // Handle negative numbers
  if (number < 0.0) {
    if(encoding == RADIOLIB_ITA2) {
      ITA2String ita2 = ITA2String("-");
      uint8_t* arr = ita2.byteArr();
      n += RTTYClient::write(arr, ita2.length());
      delete[] arr;
    } else if((encoding == RADIOLIB_ASCII) || (encoding == RADIOLIB_ASCII_EXTENDED)) {
      n += RTTYClient::print('-');
    }
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
    if(encoding == RADIOLIB_ITA2) {
      ITA2String ita2 = ITA2String(".");
      uint8_t* arr = ita2.byteArr();
      n += RTTYClient::write(arr, ita2.length());
      delete[] arr;
    } else if((encoding == RADIOLIB_ASCII) || (encoding == RADIOLIB_ASCII_EXTENDED)) {
      n += RTTYClient::print('.');
    }
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

int16_t RTTYClient::transmitDirect(uint32_t freq, uint32_t freqHz) {
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  if(audioClient != nullptr) {
    return(audioClient->tone(freqHz));
  }
  #endif
  return(phyLayer->transmitDirect(freq));
}

int16_t RTTYClient::standby() {
  // ensure everything is stopped in interrupt timing mode
  Module* mod = phyLayer->getMod();
  mod->waitForMicroseconds(0, 0);
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  if(audioClient != nullptr) {
    return(audioClient->noTone());
  }
  #endif
  return(phyLayer->standby());
}

#endif
