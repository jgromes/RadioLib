#include "Morse.h"
#if !defined(RADIOLIB_EXCLUDE_MORSE)

MorseClient::MorseClient(PhysicalLayer* phy) {
  _phy = phy;
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  _audio = nullptr;
  #endif
}

#if !defined(RADIOLIB_EXCLUDE_AFSK)
MorseClient::MorseClient(AFSKClient* audio) {
  _phy = audio->_phy;
  _audio = audio;
}
#endif

int16_t MorseClient::begin(float base, uint8_t speed) {
  // calculate 24-bit frequency
  _baseHz = base;
  _base = (base * 1000000.0) / _phy->getFreqStep();

  // calculate tone period for decoding
  _basePeriod = (1000000.0f/base)/2.0f;

  // calculate symbol lengths (assumes PARIS as typical word)
  _dotLength = 1200 / speed;
  _dashLength = 3*_dotLength;
  _letterSpace = 3*_dotLength;
  _wordSpace = 4*_dotLength;

  // configure for direct mode
  return(_phy->startDirect());
}

size_t MorseClient::startSignal() {
  return(MorseClient::write('_'));
}

char MorseClient::decode(uint8_t symbol, uint8_t len) {
  // add the guard bit
  symbol |= (RADIOLIB_MORSE_DASH << len);

  // iterate over the table
  for(uint8_t i = 0; i < sizeof(MorseTable); i++) {
    uint8_t code = RADIOLIB_NONVOLATILE_READ_BYTE(&MorseTable[i]);
    if(code == symbol) {
      // match, return the index + ASCII offset
      return((char)(i + RADIOLIB_MORSE_ASCII_OFFSET));
    }
  }

  // nothing found
  return(RADIOLIB_MORSE_UNSUPORTED);
}

#if !defined(RADIOLIB_EXCLUDE_AFSK)
int MorseClient::read(byte* symbol, byte* len, float low, float high) {
  Module* mod = _phy->getMod();

  // measure pulse duration in us
  uint32_t duration = mod->pulseIn(_audio->_pin, LOW, 4*_basePeriod);

  // decide if this is a signal, or pause
  if((duration > low*_basePeriod) && (duration < high*_basePeriod)) {
    // this is a signal
    signalCounter++;
  } else if(duration == 0) {
    // this is a pause
    pauseCounter++;
  }

  // update everything
  if((pauseCounter > 0) && (signalCounter == 1)) {
    // start of dot or dash
    pauseCounter = 0;
    signalStart = mod->millis();
    uint32_t pauseLen = mod->millis() - pauseStart;

    if((pauseLen >= low*(float)_letterSpace) && (pauseLen <= high*(float)_letterSpace)) {
      return(RADIOLIB_MORSE_CHAR_COMPLETE);
    } else if(pauseLen > _wordSpace) {
      RADIOLIB_DEBUG_PRINTLN("\n<space>");
      return(RADIOLIB_MORSE_WORD_COMPLETE);
    }

  } else if((signalCounter > 0) && (pauseCounter == 1)) {
    // end of dot or dash
    signalCounter = 0;
    pauseStart = mod->millis();
    uint32_t signalLen = mod->millis() - signalStart;

    if((signalLen >= low*(float)_dotLength) && (signalLen <= high*(float)_dotLength)) {
      RADIOLIB_DEBUG_PRINT('.');
      (*symbol) |= (RADIOLIB_MORSE_DOT << (*len));
      (*len)++;
    } else if((signalLen >= low*(float)_dashLength) && (signalLen <= high*(float)_dashLength)) {
      RADIOLIB_DEBUG_PRINT('-');
      (*symbol) |= (RADIOLIB_MORSE_DASH << (*len));
      (*len)++;
    } else {
      RADIOLIB_DEBUG_PRINT("<len=");
      RADIOLIB_DEBUG_PRINT(signalLen);
      RADIOLIB_DEBUG_PRINTLN("ms>");
    }
  }

  return(RADIOLIB_MORSE_INTER_SYMBOL);
}
#endif

size_t MorseClient::write(const char* str) {
  if(str == NULL) {
    return(0);
  }

  return(MorseClient::write((uint8_t*)str, strlen(str)));
}

size_t MorseClient::write(uint8_t* buff, size_t len) {
  size_t n = 0;
  for(size_t i = 0; i < len; i++) {
    n += MorseClient::write(buff[i]);
  }
  return(n);
}

size_t MorseClient::write(uint8_t b) {
  Module* mod = _phy->getMod();

  // check unprintable ASCII characters and boundaries
  if((b < ' ') || (b == 0x60) || (b > 'z')) {
    return(0);
  }

  // inter-word pause (space)
  if(b == ' ') {
    RADIOLIB_DEBUG_PRINTLN(F("space"));
    standby();
    mod->delay(_wordSpace);
    return(1);
  }

  // get morse code from lookup table
  uint8_t code = RADIOLIB_NONVOLATILE_READ_BYTE(&MorseTable[(uint8_t)(toupper(b) - RADIOLIB_MORSE_ASCII_OFFSET)]);

  // check unsupported characters
  if(code == RADIOLIB_MORSE_UNSUPORTED) {
    return(0);
  }

  // iterate through codeword until guard bit is reached
  while(code > RADIOLIB_MORSE_GUARDBIT) {

    // send dot or dash
    if (code & RADIOLIB_MORSE_DASH) {
      RADIOLIB_DEBUG_PRINT('-');
      transmitDirect(_base, _baseHz);
      mod->delay(_dashLength);
    } else {
      RADIOLIB_DEBUG_PRINT('.');
      transmitDirect(_base, _baseHz);
      mod->delay(_dotLength);
    }

    // symbol space
    standby();
    mod->delay(_dotLength);

    // move onto the next bit
    code >>= 1;
  }

  // letter space
  standby();
  mod->delay(_letterSpace - _dotLength);
  RADIOLIB_DEBUG_PRINTLN();

  return(1);
}

size_t MorseClient::print(__FlashStringHelper* fstr) {
  PGM_P p = reinterpret_cast<PGM_P>(fstr);
  size_t n = 0;
  while(true) {
    char c = RADIOLIB_NONVOLATILE_READ_BYTE(p++);
    if(c == '\0') {
      break;
    }
    n += MorseClient::write(c);
  }
  return n;
}

size_t MorseClient::print(const String& str) {
  return(MorseClient::write((uint8_t*)str.c_str(), str.length()));
}

size_t MorseClient::print(const char* str) {
  return(MorseClient::write((uint8_t*)str, strlen(str)));
}

size_t MorseClient::print(char c) {
  return(MorseClient::write(c));
}

size_t MorseClient::print(unsigned char b, int base) {
  return(MorseClient::print((unsigned long)b, base));
}

size_t MorseClient::print(int n, int base) {
  return(MorseClient::print((long)n, base));
}

size_t MorseClient::print(unsigned int n, int base) {
  return(MorseClient::print((unsigned long)n, base));
}

size_t MorseClient::print(long n, int base) {
  if(base == 0) {
    return(MorseClient::write(n));
  } else if(base == DEC) {
    if (n < 0) {
      int t = MorseClient::print('-');
      n = -n;
      return(MorseClient::printNumber(n, DEC) + t);
    }
    return(MorseClient::printNumber(n, DEC));
  } else {
    return(MorseClient::printNumber(n, base));
  }
}

size_t MorseClient::print(unsigned long n, int base) {
  if(base == 0) {
    return(MorseClient::write(n));
  } else {
    return(MorseClient::printNumber(n, base));
  }
}

size_t MorseClient::print(double n, int digits) {
  return(MorseClient::printFloat(n, digits));
}

size_t MorseClient::println(void) {
  return(MorseClient::write('^'));
}

size_t MorseClient::println(__FlashStringHelper* fstr) {
  size_t n = MorseClient::print(fstr);
  n += MorseClient::println();
  return(n);
}

size_t MorseClient::println(const String& str) {
  size_t n = MorseClient::print(str);
  n += MorseClient::println();
  return(n);
}

size_t MorseClient::println(const char* str) {
  size_t n = MorseClient::print(str);
  n += MorseClient::println();
  return(n);
}

size_t MorseClient::println(char c) {
  size_t n = MorseClient::print(c);
  n += MorseClient::println();
  return(n);
}

size_t MorseClient::println(unsigned char b, int base) {
  size_t n = MorseClient::print(b, base);
  n += MorseClient::println();
  return(n);
}

size_t MorseClient::println(int num, int base) {
  size_t n = MorseClient::print(num, base);
  n += MorseClient::println();
  return(n);
}

size_t MorseClient::println(unsigned int num, int base) {
  size_t n = MorseClient::print(num, base);
  n += MorseClient::println();
  return(n);
}

size_t MorseClient::println(long num, int base) {
  size_t n = MorseClient::print(num, base);
  n += MorseClient::println();
  return(n);
}

size_t MorseClient::println(unsigned long num, int base) {
  size_t n = MorseClient::print(num, base);
  n += MorseClient::println();
  return(n);
}

size_t MorseClient::println(double d, int digits) {
  size_t n = MorseClient::print(d, digits);
  n += MorseClient::println();
  return(n);
}

size_t MorseClient::printNumber(unsigned long n, uint8_t base) {
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

  return(MorseClient::write(str));
}

size_t MorseClient::printFloat(double number, uint8_t digits)  {
  size_t n = 0;

  char code[] = {0x00, 0x00, 0x00, 0x00};
  if (isnan(number)) strcpy(code, "nan");
  if (isinf(number)) strcpy(code, "inf");
  if (number > 4294967040.0) strcpy(code, "ovf");  // constant determined empirically
  if (number <-4294967040.0) strcpy(code, "ovf");  // constant determined empirically

  if(code[0] != 0x00) {
    return(MorseClient::write(code));
  }

  // Handle negative numbers
  if (number < 0.0) {
    n += MorseClient::print('-');
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
  n += MorseClient::print(int_part);

  // Print the decimal point, but only if there are digits beyond
  if(digits > 0) {
    n += MorseClient::print('.');
  }

  // Extract digits from the remainder one at a time
  while(digits-- > 0) {
    remainder *= 10.0;
    unsigned int toPrint = (unsigned int)(remainder);
    n += MorseClient::print(toPrint);
    remainder -= toPrint;
  }

  return n;
}

int16_t MorseClient::transmitDirect(uint32_t freq, uint32_t freqHz) {
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  if(_audio != nullptr) {
    return(_audio->tone(freqHz));
  }
  #endif
  return(_phy->transmitDirect(freq));
}

int16_t MorseClient::standby() {
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  if(_audio != nullptr) {
    return(_audio->noTone(true));
  }
  #endif
  return(_phy->standby());
}

#endif
