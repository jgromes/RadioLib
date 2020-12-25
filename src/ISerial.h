#ifndef _RADIOLIB_ISERIAL_H
#define _RADIOLIB_ISERIAL_H

#include "Module.h"

/*!
  \class ISerial

  \brief Interface class for Arduino Serial. Only calls the appropriate methods for the active UART interface.
*/
class ISerial {
  public:
    explicit ISerial(Module* mod);

    void begin(long);
    void end();
    int peek();
    size_t write(uint8_t);
    int read();
    int available();
    void flush();

    #ifndef ARDUINO_ARCH_MEGAAVR
    size_t print(const __FlashStringHelper *);
    #endif
    size_t print(const String &);
    size_t print(const char[]);
    size_t print(char);
    size_t print(unsigned char, int = DEC);
    size_t print(int, int = DEC);
    size_t print(unsigned int, int = DEC);
    size_t print(long, int = DEC);
    size_t print(unsigned long, int = DEC);
    size_t print(double, int = 2);
    size_t print(const Printable&);

    #ifndef ARDUINO_ARCH_MEGAAVR
    size_t println(const __FlashStringHelper *);
    #endif
    size_t println(const String &s);
    size_t println(const char[]);
    size_t println(char);
    size_t println(unsigned char, int = DEC);
    size_t println(int, int = DEC);
    size_t println(unsigned int, int = DEC);
    size_t println(long, int = DEC);
    size_t println(unsigned long, int = DEC);
    size_t println(double, int = 2);
    size_t println(const Printable&);
    size_t println(void);

#ifndef RADIOLIB_GODMODE
  protected:
#endif
    Module* _mod;
};

#endif
