#ifndef _KITELIB_ESP8266_H
#define _KITELIB_ESP8266_H

#include "Module.h"

class ESP8266 {
  public:
    ESP8266(Module* module);
    
    uint8_t begin(long speed);
    uint8_t restart();
    uint8_t join(const char* ssid, const char* password);
    uint16_t HttpGet(const char* url, String& data, uint16_t port = 80);
    
    uint8_t startTCP(const char* host, uint16_t port);
    uint8_t closeTCP();
    uint8_t send(String data);
    String receive(uint32_t timeout = 10000);
    
  private:
    Module* _mod;
};

#endif
