#ifndef _KITELIB_ESP8266_H
#define _KITELIB_ESP8266_H

#include "Module.h"

class ESP8266 {
  public:
    ESP8266(Module* module);
    
    uint16_t portTdp, portUdp;
    
    // Basic methods
    uint8_t begin(long speed);
    uint8_t reset();
    uint8_t join(const char* ssid, const char* password);
    
    // HTTP methods
    uint16_t HttpGet(const char* url, String& response);
    uint16_t HttpPost(const char* url, String content, String& response, const char* contentType = "");
    
    // Transport layer methods
    uint8_t startTcp(const char* host);
    uint8_t closeTcp();
    uint8_t startUdp(const char* host);
    uint8_t closeUdp();
    uint8_t send(String data);
    String receive(uint32_t timeout = 10000);
    
  private:
    Module* _mod;
    
    uint8_t openTransportConnection(const char* host, const char* protocol, uint16_t port);
    uint8_t closeTransportConnection();
};

#endif
