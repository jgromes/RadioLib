#if !defined(_RADIOLIB_MQTT_H)
#define _RADIOLIB_MQTT_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_MQTT)

#include "../TransportLayer/TransportLayer.h"

// MQTT packet types
#define MQTT_CONNECT                                  0x01
#define MQTT_CONNACK                                  0x02
#define MQTT_PUBLISH                                  0x03
#define MQTT_PUBACK                                   0x04
#define MQTT_PUBREC                                   0x05
#define MQTT_PUBREL                                   0x06
#define MQTT_PUBCOMP                                  0x07
#define MQTT_SUBSCRIBE                                0x08
#define MQTT_SUBACK                                   0x09
#define MQTT_UNSUBSCRIBE                              0x0A
#define MQTT_UNSUBACK                                 0x0B
#define MQTT_PINGREQ                                  0x0C
#define MQTT_PINGRESP                                 0x0D
#define MQTT_DISCONNECT                               0x0E

// MQTT CONNECT flags
#define MQTT_CONNECT_USER_NAME_FLAG                   0b10000000
#define MQTT_CONNECT_PASSWORD_FLAG                    0b01000000
#define MQTT_CONNECT_WILL_RETAIN                      0b00100000
#define MQTT_CONNECT_WILL_FLAG                        0b00000100
#define MQTT_CONNECT_CLEAN_SESSION                    0b00000010

/*!
  \class MQTTClient

  \brief Client for simple MQTT communication.
*/
class MQTTClient {
  public:
    /*!
      \brief Default constructor.

      \param tl Pointer to the wireless module providing TransportLayer communication.
    */
    explicit MQTTClient(TransportLayer* tl, uint16_t port = 1883);

    // basic methods

    /*!
      \brief Connects to MQTT broker (/server).

      \param host URL of the MQTT broker.

      \param clientId ID of the client.

      \param username Username to be used in the connection. Defaults to empty string (no username).

      \param password Password to be used in the connection. Defaults to empty string (no password).

      \param keepAlive Connection keep-alive period in seconds. Defaults to 60.

      \param cleanSession MQTT CleanSession flag. Defaults to true.

      \param willTopic MQTT will topic. Defaults to empty string (no will topic).

      \param willMessage MQTT will message. Defaults to empty string (no will message).

      \returns \ref status_codes
    */
    int16_t connect(const char* host, const char* clientId, const char* userName = "", const char* password = "", uint16_t keepAlive = 60, bool cleanSession = true, const char* willTopic = "", const char* willMessage = "");

    /*!
      \brief Disconnect from MQTT broker.

      \returns \ref status_codes
    */
    int16_t disconnect();

    /*!
      \brief Publish MQTT message.

      \param topic MQTT topic to which the message will be published.

      \param message Message to be published.

      \returns \ref status_codes
    */
    int16_t publish(String& topic, String& message);

    /*!
      \brief Publish MQTT message.

      \param topic MQTT topic to which the message will be published.

      \param message Message to be published.

      \returns \ref status_codes
    */
    int16_t publish(const char* topic, const char* message);

    /*!
      \brief Subscribe to MQTT topic.

      \param topicFilter Topic to subscribe to.

      \returns \ref status_codes
    */
    int16_t subscribe(const char* topicFilter);

    /*!
      \brief Unsubscribe from MQTT topic.

      \param topicFilter Topic to unsubscribe from.

      \returns \ref status_codes
    */
    int16_t unsubscribe(const char* topicFilter);

    /*!
      \brief Ping MQTT broker. This method can be used to keep connection open.

      \returns \ref status_codes
    */
    int16_t ping();

    /*!
      \brief Set function to be called when checking new messages in subscribed topics.

      \returns \ref status_codes
    */
    int16_t check(void (*func)(const char*, const char*));

#ifndef RADIOLIB_GODMODE
  private:
#endif
    TransportLayer* _tl;

    uint16_t _port;
    uint16_t _packetId;

    static size_t encodeLength(uint32_t len, uint8_t* encoded);
    static uint32_t decodeLength(uint8_t* encoded, uint8_t& numBytes);
};

#endif

#endif
