#ifndef _KITELIB_MQTT_H
#define _KITELIB_MQTT_H

#include "TypeDef.h"
#include "TransportLayer.h"

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

class MQTTClient {
  public:
    MQTTClient(TransportLayer* tl, uint16_t port = 1883);
    
    uint8_t connect(const char* host, const char* clientId, const char* userName = "", const char* password = "", uint16_t keepAlive = 60, bool cleanSession = true, const char* willTopic = "", const char* willMessage = "");
    uint8_t disconnect();
    uint8_t publish(const char* topic, const char* message);
    uint8_t subscribe(const char* topicFilter);
    uint8_t unsubscribe(const char* topicFilter);
    uint8_t ping();
    uint8_t check(void (*func)(const char*, const char*));
    
  private:
    TransportLayer* _tl;
    
    uint16_t _port;
    uint16_t _packetId;
    
    size_t encodeLength(uint32_t len, uint8_t* encoded);
    uint32_t decodeLength(uint8_t* encoded);
};

#endif
