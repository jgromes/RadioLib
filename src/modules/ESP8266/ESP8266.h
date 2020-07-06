#if !defined(_RADIOLIB_ESP8266_H) && !defined(RADIOLIB_EXCLUDE_ESP8266) && !defined(ESP8266)
#define _RADIOLIB_ESP8266_H

#include "../../TypeDef.h"
#include "../../Module.h"

#include "../../protocols/TransportLayer/TransportLayer.h"

/*!
  \class ESP8266

  \brief Control class for %ESP8266 module. Implements TransportLayer methods.
*/
class ESP8266: public TransportLayer {
  public:
    /*!
      \brief Default constructor.

      \param mod Instance of Module that will be used to communicate with the radio.
    */
    ESP8266(Module* module);

    // basic methods

    /*!
      \brief Initialization method.

      \param speed Baud rate to use for UART interface.

      \returns \ref status_codes
    */
    int16_t begin(long speed);

    /*!
      \brief Resets module using AT command.

      \returns \ref status_codes
    */
    int16_t reset();

    /*!
      \brief Joins access point.

      \param ssid Access point SSID.

      \param password Access point password.
    */
    int16_t join(const char* ssid, const char* password);

    // transport layer methods (implementations of purely virtual methods in TransportLayer class)
    int16_t openTransportConnection(const char* host, const char* protocol, uint16_t port, uint16_t tcpKeepAlive = 0) override;
    int16_t closeTransportConnection() override;
    int16_t send(const char* data) override;
    int16_t send(uint8_t* data, size_t len) override;
    size_t receive(uint8_t* data, size_t len, uint32_t timeout = 10000) override;
    size_t getNumBytes(uint32_t timeout = 10000, size_t minBytes = 10) override;

#ifndef RADIOLIB_GODMODE
  private:
#endif
    Module* _mod;
};

#endif
