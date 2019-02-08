#ifndef _RADIOLIB_ESP8266_H
#define _RADIOLIB_ESP8266_H

#include "Module.h"

#include "../protocols/TransportLayer.h"

class ESP8266: public TransportLayer {
  public:
    // constructor
    ESP8266(Module* module);
    
    // basic methods
    int16_t begin(long speed);
    int16_t reset();
    int16_t join(const char* ssid, const char* password);
    
    // transport layer methods (implementations of purely virtual methods in TransportMethod class)
    int16_t openTransportConnection(const char* host, const char* protocol, uint16_t port, uint16_t tcpKeepAlive = 0);
    int16_t closeTransportConnection();
    int16_t send(const char* data);
    int16_t send(uint8_t* data, uint32_t len);
    size_t receive(uint8_t* data, size_t len, uint32_t timeout = 10000);
    size_t getNumBytes(uint32_t timeout = 10000, size_t minBytes = 10);
    
  private:
    Module* _mod;
};

#endif
