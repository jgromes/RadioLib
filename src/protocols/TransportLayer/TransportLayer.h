#ifndef _RADIOLIB_TRANSPORT_LAYER_H
#define _RADIOLIB_TRANSPORT_LAYER_H

#include "../../TypeDef.h"

/*!
  \class TransportLayer

  \brief Provides common interface for protocols that run on modules with Internet connectivity, such as HTTP or MQTT.
  Because this class is used mainly as interface, all of its virtual members must be implemented in the module class.
*/
class TransportLayer {
  public:
    // constructor
    // this class is purely virtual and does not require explicit constructor

    // basic methods

    /*!
      \brief Open transport layer connection.

      \param host Host to connect to.

      \param protocol Transport protocol to use. Usually "TCP" or "UDP".

      \param port to be used for the connection.

      \param tcpKeepAlive TCP keep alive interval. Defaults to 0.

      \returns \ref status_codes
    */
    virtual int16_t openTransportConnection(const char* host, const char* protocol, uint16_t port, uint16_t tcpKeepAlive = 0) = 0;

    /*!
      \brief Close transport layer connection.

      \returns \ref status_codes
    */
    virtual int16_t closeTransportConnection() = 0;

    /*!
      \brief Send string-based data.

      \param string String data to be sent.

      \returns \ref status_codes
    */
    virtual int16_t send(const char* data) = 0;

    /*!
      \brief Send arbitrary binary data.

      \param data Data to be sent.

      \param len Number of bytes to send.

      \returns \ref status_codes
    */
    virtual int16_t send(uint8_t* data, size_t len) = 0;

    /*!
      \brief Receive data.

      \param data Pointer to array to save the received data.

      \param len Number of bytes to read.

      \param timeout Reception timeout in ms. Defaults to 10000.

      \returns \ref status_codes
    */
    virtual size_t receive(uint8_t* data, size_t len, uint32_t timeout = 10000) = 0;

    /*!
      \brief Get number of received bytes.

      \param timeout Reception timeout in ms. Defaults to 10000.

      \param minBytes Minimum required number of bytes that must be received.

      \returns Number of received bytes, or 0 on timeout.
    */
    virtual size_t getNumBytes(uint32_t timeout = 10000, size_t minBytes = 10) = 0;
};

#endif
