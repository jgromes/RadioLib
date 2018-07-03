#ifndef _KITELIB_PACKET_H
#define _KITELIB_PACKET_H

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include <EEPROM.h>

class Packet {
  public:
    Packet(void);
    Packet(const char* dest, const char* dat);
    Packet(uint8_t* dest, const char* dat);
    Packet(const char* src, const char* dest, const char* dat);
    Packet(uint8_t* src, uint8_t* dest, const char* dat);
    
    Packet(const char* dest, uint8_t* dat, uint8_t len);
    Packet(uint8_t* dest, uint8_t* dat, uint8_t len);
    Packet(const char* src, const char* dest, uint8_t* dat, uint8_t len);
    Packet(uint8_t* src, uint8_t* dest, uint8_t* dat, uint8_t len);
    
    uint8_t source[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t destination[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    char* data;
    uint8_t length = 0;
    
    String getSourceStr();
    String getDestinationStr();
    
    void setSourceStr(const char* src);
    void setDestinationStr(const char* dest);
    
    void copyInto(Packet& pack);
    
    void setPacketData(char* charArray);
    void setPacketData(String str);
    void setPacketData(float f, uint8_t decimals = 3);
    
  private:
    void init(uint8_t* src, uint8_t* dest, const char* dat);
    void init(uint8_t* src, uint8_t* dest, uint8_t* dat, uint8_t len);
    void getLoRaAddress(uint8_t* addr);
};

#endif
