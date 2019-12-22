#include "Morse.h"

// structure to save data about character Morse code
/*!
  \cond RADIOLIB_DOXYGEN_HIDDEN
*/

//----------------------------------------------------------------------------------------
// In the below array we represent dots as 0 and dashes as 1, using a further bit as 
// 'guard' who tell us when we must stop in decoding our symbols.
// Some bytes are zeroed because there isn't a morse code representation of that char.
// The ascii code of the letter is the search index into the array for the correspondent 
// morse code representation.
//----------------------------------------------------------------------------------------
const uint8_t morse_lookuptable[64] PROGMEM = {    
    0b00,       // space 0x00
    0b1110101,  // ! 117
    0b1010010,  // " 82
    0b00,       // # (doesn't exists)<--
    0b11001000, // $ 200
    0b00,       // % (doesn't exists)<--
    0b100010,   // & 34
    0b1011110,  // ' 94
    0b101101,   // ( 45
    0b1101101,  // ) 109
    0b00,       // * (doesn't exists)<--
    0b101010,   // + 42
    0b1110011,  // , 115
    0b1100001,  // - 97
    0b1101010,  // . 106
    0b101001,   // / 41
    0b111111,   // 0 63
    0b111110,   // 1 62
    0b111100,   // 2 60
    0b111000,   // 3 56
    0b110000,   // 4 48
    0b100000,   // 5 32
    0b100001,   // 6 33
    0b100011,   // 7 35
    0b100111,   // 8 39
    0b101111,   // 9 47
    0b1000111,  // : 71
    0b1101010,  // ; 106
    0b00,       // < (doesn't exists)<--
    0b110001,   // = 49
    0b00,       // > (doesn't exists)<--
    0b1001100,  // ? 76
    0b1010110,  // @ 86
    0b110,      // A 6 
    0b10001,    // B 17
    0b10101,    // C 21
    0b1001,     // D 9 
    0b10,       // E 2 
    0b10100,    // F 20
    0b1011,     // G 11
    0b10000,    // H 16
    0b100,      // I 4 
    0b11110,    // J 30
    0b1101,     // K 13
    0b10010,    // L 18
    0b111,      // M 7 
    0b101,      // N 5 
    0b1111,     // O 15
    0b10110,    // P 22
    0b11011,    // Q 27
    0b1010,     // R 10
    0b1000,     // S 8 
    0b11,       // T 3 
    0b1100,     // U 12
    0b11000,    // V 24
    0b1110,     // W 14
    0b11001,    // X 25
    0b11101,    // Y 29
    0b10011,    // Z 19
    0b00,       // [ (doesn't exists)<--
    0b00,       // \ (doesn't exists)<--
    0b00,       // ] (doesn't exists)<--
    0b00,       // ^ (doesn't exists)<--
    0b1101100   // _ 108
};

MorseClient::MorseClient(PhysicalLayer* phy) {
  _phy = phy;
}

int16_t MorseClient::begin(float base, uint8_t speed) {
  // calculate 24-bit frequency
  _base = (base * (uint32_t(1) << _phy->getDivExponent())) / _phy->getCrystalFreq();

  // calculate dot length (assumes PARIS as typical word)
  _dotLength = 1200 / speed;

  // set module frequency deviation to 0
  int16_t state = _phy->setFrequencyDeviation(0);

  return(state);
}

size_t MorseClient::startSignal() {
  return(MorseClient::write(0x01));
}

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
  
  //Note: using toupper() you always find a char, because all possible chars are included in this array!
  uint8_t ditdah_code = 0x00;

  if (b == ' ') {
    RADIOLIB_DEBUG_PRINTLN('space'); // inter-words pause (space)
    // symbol space
    _phy->standby();
    delay(_dotLength * 3);
  }

  ditdah_code = morse_lookuptable[(uint8_t)(toupper(b) - 32)]; // retrieve morse code from lookuptable
  
  //do it until 'guardbit' is reached
  while (ditdah_code > GUARDBIT) { 
    if (ditdah_code & DAH) {
      RADIOLIB_DEBUG_PRINT('-'); // tx a 'dah'
      _phy->transmitDirect(_base);
      delay(_dotLength * 3);
    }
    else{
      RADIOLIB_DEBUG_PRINT('.'); // tx a 'dit'
      _phy->transmitDirect(_base);
      delay(_dotLength);
    } 
    ditdah_code >>= 1; // analyze next bit
  }
 
  // letter space
  RADIOLIB_DEBUG_PRINTLN();
  _phy->standby();
  delay(_dotLength);

 return(0);
}

size_t MorseClient::print(__FlashStringHelper* fstr) {
  PGM_P p = reinterpret_cast<PGM_P>(fstr);
  size_t n = 0;
  while(true) {
    char c = pgm_read_byte(p++);
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
  return(MorseClient::write(0x02));
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
