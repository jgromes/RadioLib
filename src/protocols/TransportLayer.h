#ifndef _KITELIB_TRANSPORT_LAYER_H
#define _KITELIB_TRANSPORT_LAYER_H

#include "TypeDef.h"

class TransportLayer {
  public:
    // constructor
    // this class is purely virtual and does not require explicit constructor
    
    // basic methods
    virtual int16_t openTransportConnection(const char* host, const char* protocol, uint16_t port, uint16_t tcpKeepAlive = 0) = 0;
    virtual int16_t closeTransportConnection() = 0;
    virtual int16_t send(const char* data) = 0;
    virtual int16_t send(uint8_t* data, uint32_t len) = 0;
    virtual size_t receive(uint8_t* data, size_t len, uint32_t timeout = 10000) = 0;
    virtual size_t getNumBytes(uint32_t timeout = 10000, size_t minBytes = 10) = 0;
};

#endif
