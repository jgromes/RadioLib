#ifndef _KITELIB_ESP8266_H
#define _KITELIB_ESP8266_H

#include "Module.h"

#include "../protocols/TransportLayer.h"

class ESP8266: public TransportLayer {
  public:
    ESP8266(Module* module);
    
    // Basic methods
    uint8_t begin(long speed);
    uint8_t reset();
    uint8_t join(const char* ssid, const char* password);
    
    // Transport layer methods
    uint8_t openTransportConnection(const char* host, const char* protocol, uint16_t port, uint16_t tcpKeepAlive = 0);
    uint8_t closeTransportConnection();
    uint8_t send(const char* data);
    uint8_t send(uint8_t* data, uint32_t len);
    size_t receive(uint8_t* data, size_t len, uint32_t timeout = 10000);
    uint16_t getNumBytes(uint32_t timeout = 10000, size_t minBytes = 10);
    
  private:
    Module* _mod;
};

#endif
