#ifndef _KITELIB_RTTY_H
#define _KITELIB_RTTY_H

#include "TypeDef.h"
#include "PhysicalLayer.h"

#define ITA2_FIGS                                     0x1B
#define ITA2_LTRS                                     0x1F
#define ITA2_LENGTH                                   32

// ITA2 character table: - position in array corresponds to 5-bit ITA2 code
//                       - characters to the left are in letters shift, characters to the right in figures shift
//                       - characters marked 0x7F do not have ASCII equivalent
const char ITA2Table[ITA2_LENGTH][2] = {{'\0', '\0'}, {'E', '3'}, {'\n', '\n'}, {'A', '-'}, {' ', ' '}, {'S', '\''}, {'I', '8'}, {'U', '7'}, 
                                        {'\r', '\r'}, {'D', 0x05}, {'R', '4'}, {'J', '\a'}, {'N', ','}, {'F', '!'}, {'C', ':'}, {'K', '('}, 
                                        {'T', '5'}, {'Z', '+'}, {'L', ')'}, {'W', '2'}, {'H', 0x7F}, {'Y', '6'}, {'P', '0'}, {'Q', '1'}, 
                                        {'O', '9'}, {'B', '?'}, {'G', '&'}, {0x7F, 0x7F}, {'M', '.'}, {'X', '/'}, {'V', ';'}, {0x7F, 0x7F}};

class ITA2 {
  public:
    ITA2(const char* str);
    ~ITA2();
    
    size_t length();
    uint8_t* byteArr();
  
  private:
    char* _str;
    size_t _len;
    
    uint16_t getBits(char c);
};

class RTTYClient {
  public:
    RTTYClient(PhysicalLayer* phy);
    
    // basic methods
    int16_t begin(float base, uint16_t shift, uint16_t rate, uint8_t dataBits = 8, uint8_t stopBits = 1);
    void idle();
    size_t write(const char* str);
    size_t write(uint8_t* buff, size_t len);
    size_t write(uint8_t b);
    
    size_t print(ITA2 &);
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
    size_t println(ITA2 &);
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
