#ifndef _KITELIB_HTTP_H
#define _KITELIB_HTTP_H

#include "TypeDef.h"
#include "TransportLayer.h"

class HTTPClient {
  public:
    HTTPClient(TransportLayer* tl, uint16_t port = 80);
    
    uint16_t get(const char* url, String& response);
    uint16_t post(const char* url, const char* content, String& response, const char* contentType = "text/plain");
  
  private:
    TransportLayer* _tl;
    uint16_t _port;
};

#endif
