#if !defined(_RADIOLIB_HTTP_H)
#define _RADIOLIB_HTTP_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_HTTP)

#include "../TransportLayer/TransportLayer.h"

/*!
  \class HTTPClient

  \brief Client for simple HTTP communication.
*/
class HTTPClient {
  public:
    /*!
      \brief Default constructor.

      \param tl Pointer to the wireless module providing TransportLayer communication.

      \param port Port to be used for HTTP. Defaults to 80.
    */
    explicit HTTPClient(TransportLayer* tl, uint16_t port = 80);

    /*!
      \brief Sends HTTP GET request.

      \param url URL to send the request to.

      \param response Arduino String object that will save the response.

      \returns \ref status_codes
    */
    int16_t get(String& url, String& response);

    /*!
      \brief Sends HTTP GET request.

      \param url URL to send the request to.

      \param response Arduino String object that will save the response.

      \returns \ref status_codes
    */
    int16_t get(const char* url, String& response);

    /*!
      \brief Sends HTTP POST request.

      \param url URL to send the request to.

      \param content Request content.

      \param response Arduino String object that will save the response.

      \param contentType MIME type of request content. Defaults to "text/plain".

      \returns \ref status_codes
    */
    int16_t post(const char* url, const char* content, String& response, const char* contentType = "text/plain");

#ifndef RADIOLIB_GODMODE
  private:
#endif
    TransportLayer* _tl;

    uint16_t _port;
};

#endif

#endif
