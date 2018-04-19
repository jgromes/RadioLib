#ifndef _KITELIB_ESP8266_H
#define _KITELIB_ESP8266_H

#include "Module.h"

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

class ESP8266 {
  public:
    ESP8266(Module* module);
    
    // Port numbers
    uint16_t portTcp, portUdp, portMqtt;
    
    // Basic methods
    uint8_t begin(long speed);
    uint8_t reset();
    uint8_t join(const char* ssid, const char* password);
    
    // HTTP methods
    uint16_t HttpGet(const char* url, String& response);
    uint16_t HttpPost(const char* url, String content, String& response, const char* contentType = "");
    
    // MQTT methods
    uint8_t MqttConnect(String host, String clientId, String username, String password);
    uint8_t MqttPublish(String topic, String message);
    
    // Transport layer methods
    uint8_t startTcp(const char* host, uint16_t tcpKeepAlive = 0);
    uint8_t closeTcp();
    uint8_t startUdp(const char* host);
    uint8_t closeUdp();
    uint8_t send(String data);
    uint8_t send(uint8_t* data, uint32_t len);
    String receive(uint32_t timeout = 10000);
    uint32_t receive(uint8_t* data, uint32_t timeout = 10000);
    
  private:
    Module* _mod;
    
    uint8_t openTransportConnection(const char* host, const char* protocol, uint16_t port, uint16_t tcpKeepAlive = 0);
    uint8_t closeTransportConnection();
    
    String _MqttHost;
    void MqttEncodeLength(uint32_t len, uint8_t* encoded);
    uint32_t MqttDecodeLength(uint8_t* encoded);
};

#endif
