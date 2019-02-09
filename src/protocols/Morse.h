#ifndef _RADIOLIB_MORSE_H
#define _RADIOLIB_MORSE_H

#include "TypeDef.h"
#include "PhysicalLayer.h"

#define MORSE_LENGTH                                  52

class MorseClient {
  public:
    MorseClient(PhysicalLayer* phy);
    
    // basic methods
    int16_t begin(float base, uint8_t speed = 20);
    size_t write(const char* str);
    size_t write(uint8_t* buff, size_t len);
    size_t write(uint8_t b);
    
    size_t startSignal();
    
    size_t print(const String &);
    size_t print(const char[]);
    size_t print(char);
    size_t print(unsigned char, int = DEC);
    size_t print(int, int = DEC);
    size_t print(unsigned int, int = DEC);
    size_t print(long, int = DEC);
    size_t print(unsigned long, int = DEC);
    size_t print(double, int = 2);
    
    size_t println(void);
    size_t println(const String &s);
    size_t println(const char[]);
    size_t println(char);
    size_t println(unsigned char, int = DEC);
    size_t println(int, int = DEC);
    size_t println(unsigned int, int = DEC);
    size_t println(long, int = DEC);
    size_t println(unsigned long, int = DEC);
    size_t println(double, int = 2);
  
  private:
    PhysicalLayer* _phy;
    uint32_t _base;
    uint16_t _dotLength;
    
    size_t printNumber(unsigned long, uint8_t);
    size_t printFloat(double, uint8_t);
};

#endif
