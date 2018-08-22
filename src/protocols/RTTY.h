#ifndef _KITELIB_RTTY_H
#define _KITELIB_RTTY_H

#include "TypeDef.h"
#include "PhysicalLayer.h"

class RTTYClient {
  public:
    RTTYClient(PhysicalLayer* phy);
    
    // basic methods
    int16_t begin(float base, uint16_t shift, uint16_t rate, uint8_t dataBits = 8, uint8_t stopBits = 1);
    void leadIn(uint16_t length);
    size_t write(const char* str);
    size_t write(uint8_t* buff, size_t len);
    size_t write(uint8_t b);
    
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
    uint16_t _shift;
    uint16_t _bitDuration;
    uint8_t _dataBits;
    uint8_t _stopBits;
    
    void mark();
    void space();
    
    size_t printNumber(unsigned long, uint8_t);
    size_t printFloat(double, uint8_t);
};

#endif
