#ifndef _RADIOLIB_HTTP_H
#define _RADIOLIB_HTTP_H

#include "TypeDef.h"
#include "TransportLayer.h"

class HTTPClient {
  public:
    // constructor
    HTTPClient(TransportLayer* tl, uint16_t port = 80);
    
    // basic methods
    int16_t get(const char* url, String& response);
    int16_t post(const char* url, const char* content, String& response, const char* contentType = "text/plain");
  
  private:
    TransportLayer* _tl;
    
    uint16_t _port;
};

#endif
