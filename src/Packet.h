#ifndef _LORALIB_PACKET_H
#define _LORALIB_PACKET_H

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include <EEPROM.h>

class Packet {
  public:
    Packet(void);
    Packet(const char dest[24], const char dat[240]);
    Packet(uint8_t dest[8], const char dat[240]);
    Packet(const char src[24], const char dest[24], const char dat[240]);
    Packet(uint8_t src[8], uint8_t dest[8], const char dat[240]);
    
    Packet(const char dest[24], uint8_t* dat, uint8_t len);
    Packet(uint8_t dest[8], uint8_t* dat, uint8_t len);
    Packet(const char src[24], const char dest[24], uint8_t* dat, uint8_t len);
    Packet(uint8_t src[8], uint8_t dest[8], uint8_t* dat, uint8_t len);
    
    uint8_t source[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t destination[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    char data[240];
    uint8_t length = 0;
    
    void getSourceStr(char src[24]);
    void getDestinationStr(char dest[24]);
    
    void setSourceStr(const char src[24]);
    void setDestinationStr(const char dest[24]);
    
    void copyInto(Packet& pack);
    
  private:
    void init(uint8_t src[8], uint8_t dest[8], const char dat[240]);
    void init(uint8_t src[8], uint8_t dest[8], uint8_t* dat, uint8_t len);
    void getLoRaAddress(uint8_t addr[8]);
    
    uint8_t parseByte(char c);
    char reparseChar(uint8_t b);
};

#endif
