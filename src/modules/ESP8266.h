#ifndef _KITELIB_ESP8266_H
#define _KITELIB_ESP8266_H

#include "Module.h"

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

class ESP8266 {
  public:
    ESP8266(Module* module);
    
    // Port numbers
    uint16_t portHttp, portMqtt;
    
    // Basic methods
    uint8_t begin(long speed);
    uint8_t reset();
    uint8_t join(const char* ssid, const char* password);
    
    // HTTP methods
    uint16_t HttpGet(const char* url, String& response);
    uint16_t HttpPost(const char* url, const char* content, String& response, const char* contentType = "text/plain");
    
    // MQTT methods
    uint8_t MqttConnect(const char* host, const char* clientId, const char* userName = "", const char* password = "", uint16_t keepAlive = 60, bool cleanSession = true, const char* willTopic = "", const char* willMessage = "");
    uint8_t MqttPublish(const char* topic, const char* message);
    
    // Transport layer methods
    uint8_t openTransportConnection(const char* host, const char* protocol, uint16_t port, uint16_t tcpKeepAlive = 0);
    uint8_t closeTransportConnection();
    uint8_t send(const char* data);
    uint8_t send(uint8_t* data, uint32_t len);
    size_t receive(uint8_t* data, uint32_t timeout = 10000);
    
  private:
    Module* _mod;
    
    size_t MqttEncodeLength(uint32_t len, uint8_t* encoded);
    uint32_t MqttDecodeLength(uint8_t* encoded);
    
    uint16_t getNumBytes(uint32_t timeout = 10000);
};

#endif
