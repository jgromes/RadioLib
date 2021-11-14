#include "Hellschreiber.h"
#if !defined(RADIOLIB_EXCLUDE_HELLSCHREIBER)

HellClient::HellClient(PhysicalLayer* phy) {
  _phy = phy;

  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  _audio = nullptr;
  #endif
}

#if !defined(RADIOLIB_EXCLUDE_AFSK)
HellClient::HellClient(AFSKClient* audio) {
  _phy = audio->_phy;
  _audio = audio;
}
#endif

int16_t HellClient::begin(float base, float rate) {
  // calculate 24-bit frequency
  _baseHz = base;
  _base = (base * 1000000.0) / _phy->getFreqStep();

  // calculate "pixel" duration
  _pixelDuration = 1000000.0/rate;

  // configure for direct mode
  return(_phy->startDirect());
}

size_t HellClient::printGlyph(uint8_t* buff) {
  // print the character
  Module* mod = _phy->getMod();
  for(uint8_t mask = 0x40; mask >= 0x01; mask >>= 1) {
    for(int8_t i = RADIOLIB_HELL_FONT_HEIGHT - 1; i >= 0; i--) {
        uint32_t start = mod->micros();
        if(buff[i] & mask) {
          transmitDirect(_base, _baseHz);
        } else {
          standby();
        }
        while(mod->micros() - start < _pixelDuration);
    }
  }

  // make sure transmitter is off
  standby();

  return(1);
}

size_t HellClient::write(const char* str) {
  if(str == NULL) {
    return(0);
  }
  return(HellClient::write((uint8_t *)str, strlen(str)));
}

size_t HellClient::write(uint8_t* buff, size_t len) {
  size_t n = 0;
  for(size_t i = 0; i < len; i++) {
    n += HellClient::write(buff[i]);
  }
  return(n);
}

size_t HellClient::write(uint8_t b) {
  // convert to position in font buffer
  uint8_t pos = b;
  if((pos >= ' ') && (pos <= '_')) {
    pos -= ' ';
  } else if((pos >= 'a') && (pos <= 'z')) {
    pos -= (2*' ');
  } else {
    return(0);
  }

  // fetch character from flash
  uint8_t buff[RADIOLIB_HELL_FONT_WIDTH];
  buff[0] = 0x00;
  for(uint8_t i = 0; i < RADIOLIB_HELL_FONT_WIDTH - 2; i++) {
    buff[i + 1] = RADIOLIB_NONVOLATILE_READ_BYTE(&HellFont[pos][i]);
  }
  buff[RADIOLIB_HELL_FONT_WIDTH - 1] = 0x00;

  // print the character
  return(printGlyph(buff));
}

size_t HellClient::print(__FlashStringHelper* fstr) {
  PGM_P p = reinterpret_cast<PGM_P>(fstr);
  size_t n = 0;
  while(true) {
    char c = RADIOLIB_NONVOLATILE_READ_BYTE(p++);
    if(c == '\0') {
      break;
    }
    n += HellClient::write(c);
  }
  return n;
}

size_t HellClient::print(const String& str) {
  return(HellClient::write((uint8_t*)str.c_str(), str.length()));
}

size_t HellClient::print(const char* str) {
  return(HellClient::write((uint8_t*)str, strlen(str)));
}

size_t HellClient::print(char c) {
  return(HellClient::write(c));
}

size_t HellClient::print(unsigned char b, int base) {
  return(HellClient::print((unsigned long)b, base));
}

size_t HellClient::print(int n, int base) {
  return(HellClient::print((long)n, base));
}

size_t HellClient::print(unsigned int n, int base) {
  return(HellClient::print((unsigned long)n, base));
}

size_t HellClient::print(long n, int base) {
  if(base == 0) {
    return(HellClient::write(n));
  } else if(base == DEC) {
    if (n < 0) {
      int t = HellClient::print('-');
      n = -n;
      return(HellClient::printNumber(n, DEC) + t);
    }
    return(HellClient::printNumber(n, DEC));
  } else {
    return(HellClient::printNumber(n, base));
  }
}

size_t HellClient::print(unsigned long n, int base) {
  if(base == 0) {
    return(HellClient::write(n));
  } else {
    return(HellClient::printNumber(n, base));
  }
}

size_t HellClient::print(double n, int digits) {
  return(HellClient::printFloat(n, digits));
}

size_t HellClient::println(void) {
  // Hellschreiber has no concept of "line ending", print one space instead
  return(HellClient::print(' '));
}

size_t HellClient::println(__FlashStringHelper* fstr) {
  size_t n = HellClient::print(fstr);
  n += HellClient::println();
  return(n);
}

size_t HellClient::println(const String& str) {
  size_t n = HellClient::print(str);
  n += HellClient::println();
  return(n);
}

size_t HellClient::println(const char* str) {
  size_t n = HellClient::print(str);
  n += HellClient::println();
  return(n);
}

size_t HellClient::println(char c) {
  size_t n = HellClient::print(c);
  n += HellClient::println();
  return(n);
}

size_t HellClient::println(unsigned char b, int base) {
  size_t n = HellClient::print(b, base);
  n += HellClient::println();
  return(n);
}

size_t HellClient::println(int num, int base) {
  size_t n = HellClient::print(num, base);
  n += HellClient::println();
  return(n);
}

size_t HellClient::println(unsigned int num, int base) {
  size_t n = HellClient::print(num, base);
  n += HellClient::println();
  return(n);
}

size_t HellClient::println(long num, int base) {
  size_t n = HellClient::print(num, base);
  n += HellClient::println();
  return(n);
}

size_t HellClient::println(unsigned long num, int base) {
  size_t n = HellClient::print(num, base);
  n += HellClient::println();
  return(n);
}

size_t HellClient::println(double d, int digits) {
  size_t n = HellClient::print(d, digits);
  n += HellClient::println();
  return(n);
}

size_t HellClient::printNumber(unsigned long n, uint8_t base) {
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

  return(HellClient::write(str));
}

size_t HellClient::printFloat(double number, uint8_t digits)  {
  size_t n = 0;

  char code[] = {0x00, 0x00, 0x00, 0x00};
  if (isnan(number)) strcpy(code, "nan");
  if (isinf(number)) strcpy(code, "inf");
  if (number > 4294967040.0) strcpy(code, "ovf");  // constant determined empirically
  if (number <-4294967040.0) strcpy(code, "ovf");  // constant determined empirically

  if(code[0] != 0x00) {
    return(HellClient::write(code));
  }

  // Handle negative numbers
  if (number < 0.0) {
    n += HellClient::print('-');
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
  n += HellClient::print(int_part);

  // Print the decimal point, but only if there are digits beyond
  if(digits > 0) {
    n += HellClient::print('.');
  }

  // Extract digits from the remainder one at a time
  while(digits-- > 0) {
    remainder *= 10.0;
    unsigned int toPrint = (unsigned int)(remainder);
    n += HellClient::print(toPrint);
    remainder -= toPrint;
  }

  return n;
}

int16_t HellClient::transmitDirect(uint32_t freq, uint32_t freqHz) {
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  if(_audio != nullptr) {
    return(_audio->tone(freqHz));
  }
  #endif
  return(_phy->transmitDirect(freq));
}

int16_t HellClient::standby() {
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  if(_audio != nullptr) {
    return(_audio->noTone());
  }
  #endif
  return(_phy->standby());
}

#endif
