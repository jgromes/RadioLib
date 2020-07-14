#include "MQTT.h"
#if !defined(RADIOLIB_EXCLUDE_MQTT)

MQTTClient::MQTTClient(TransportLayer* tl, uint16_t port) {
  _tl = tl;
  _port = port;
  _packetId = 1;
}

int16_t MQTTClient::connect(const char* host, const char* clientId, const char* userName, const char* password, uint16_t keepAlive, bool cleanSession, const char* willTopic, const char* willMessage) {
  // encode packet length
  size_t clientIdLen = strlen(clientId);
  size_t userNameLen = strlen(userName);
  size_t passwordLen = strlen(password);
  size_t willTopicLen = strlen(willTopic);
  size_t willMessageLen = strlen(willMessage);
  uint32_t remainingLength = 10 + (2 + clientIdLen);
  if(userNameLen > 0) {
    remainingLength += (2 + userNameLen);
  }
  if(passwordLen > 0) {
    remainingLength += (2 + passwordLen);
  }
  if((willTopicLen > 0) && (willMessageLen > 0)) {
    remainingLength += (2 + willTopicLen) + (2 + willMessageLen);
  }
  uint8_t encoded[] = {0, 0, 0, 0};
  size_t encodedBytes = encodeLength(remainingLength, encoded);

  // build the CONNECT packet
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t packet[256];
  #else
    uint8_t* packet = new uint8_t[1 + encodedBytes + remainingLength];
  #endif

  // fixed header
  packet[0] = (MQTT_CONNECT << 4)  | 0b0000;
  memcpy(packet + 1, encoded, encodedBytes);

  // variable header
  // protocol name
  size_t pos = encodedBytes + 1;
  packet[pos++] = 0x00;
  packet[pos++] = 0x04;
  memcpy(packet + pos, "MQTT", 4);
  pos += 4;

  // protocol level
  packet[pos++] = 0x04;

  // flags
  packet[pos++] = 0x00;
  if(cleanSession) {
    packet[encodedBytes + 8] |= MQTT_CONNECT_CLEAN_SESSION;
  }

  // keep alive interval in seconds
  packet[pos++] = (keepAlive & 0xFF00) >> 8;
  packet[pos++] = keepAlive & 0x00FF;

  // payload
  // clientId
  packet[pos++] = (clientIdLen & 0xFF00) >> 8;
  packet[pos++] = clientIdLen & 0x00FF;
  memcpy(packet + pos, clientId, clientIdLen);
  pos += clientIdLen;

  // will topic and message
  if((willTopicLen > 0) && (willMessageLen > 0)) {
    packet[encodedBytes + 8] |= MQTT_CONNECT_WILL_FLAG;

    packet[pos++] = (willTopicLen & 0xFF00) >> 8;
    packet[pos++] = willTopicLen & 0x00FF;
    memcpy(packet + pos, willTopic, willTopicLen);
    pos += willTopicLen;

    packet[pos++] = (willMessageLen & 0xFF00) >> 8;
    packet[pos++] = willMessageLen & 0x00FF;
    memcpy(packet + pos, willMessage, willMessageLen);
    pos += willMessageLen;
  }

  // user name
  if(userNameLen > 0) {
    packet[encodedBytes + 8] |= MQTT_CONNECT_USER_NAME_FLAG;
    packet[pos++] = (userNameLen & 0xFF00) >> 8;
    packet[pos++] = userNameLen & 0x00FF;
    memcpy(packet + pos, userName, userNameLen);
    pos += userNameLen;
  }

  // password
  if(passwordLen > 0) {
    packet[encodedBytes + 8] |= MQTT_CONNECT_PASSWORD_FLAG;
    packet[pos++] = (passwordLen & 0xFF00) >> 8;;
    packet[pos++] = passwordLen & 0x00FF;
    memcpy(packet + pos, password, passwordLen);
  }

  // create TCP connection
  int16_t state = _tl->openTransportConnection(host, "TCP", _port, keepAlive);
  if(state != ERR_NONE) {
    #ifndef RADIOLIB_STATIC_ONLY
      delete[] packet;
    #endif
    return(state);
  }

  // send MQTT packet
  state = _tl->send(packet, 1 + encodedBytes + remainingLength);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] packet;
  #endif
  if(state != ERR_NONE) {
    return(state);
  }

  // get the response length (MQTT CONNACK response has to be 4 bytes long)
  size_t numBytes = _tl->getNumBytes();
  if(numBytes != 4) {
    return(ERR_RESPONSE_MALFORMED_AT);
  }

  // read the response
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t response[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* response = new uint8_t[numBytes];
  #endif
  _tl->receive(response, numBytes);
  if((response[0] == MQTT_CONNACK << 4) && (response[1] == 2)) {
    uint8_t returnCode = response[3];
    #ifndef RADIOLIB_STATIC_ONLY
      delete[] response;
    #endif
    return(returnCode);
  }

  #ifndef RADIOLIB_STATIC_ONLY
    delete[] response;
  #endif
  return(ERR_RESPONSE_MALFORMED);
}

int16_t MQTTClient::disconnect() {
  // build the DISCONNECT packet
  uint8_t packet[2];

  // fixed header
  packet[0] = (MQTT_DISCONNECT << 4) | 0b0000;
  packet[1] = 0x00;

  // send MQTT packet
  int16_t state = _tl->send(packet, 2);
  if(state != ERR_NONE) {
    return(state);
  }

  // close tl connection
  return(_tl->closeTransportConnection());
}

int16_t MQTTClient::publish(String& topic, String& message) {
  return(MQTTClient::publish(topic.c_str(), message.c_str()));
}

int16_t MQTTClient::publish(const char* topic, const char* message) {
  // encode packet length
  size_t topicLen = strlen(topic);
  size_t messageLen = strlen(message);
  uint32_t remainingLength = (2 + topicLen) + messageLen;
  uint8_t encoded[] = {0, 0, 0, 0};
  size_t encodedBytes = encodeLength(remainingLength, encoded);

  // build the PUBLISH packet
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t packet[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* packet = new uint8_t[1 + encodedBytes + remainingLength];
  #endif

  // fixed header
  packet[0] = (MQTT_PUBLISH << 4) | 0b0000;
  memcpy(packet + 1, encoded, encodedBytes);

  // variable header
  // topic name
  size_t pos = encodedBytes + 1;
  packet[pos++] = (topicLen & 0xFF00) >> 8;
  packet[pos++] = topicLen & 0x00FF;
  memcpy(packet + pos, topic, topicLen);
  pos += topicLen;

  // packet ID

  // payload
  // message
  memcpy(packet + pos, message, messageLen);

  // send MQTT packet
  int16_t state = _tl->send(packet, 1 + encodedBytes + remainingLength);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] packet;
  #endif
  return(state);

  /// \todo implement QoS > 0 and PUBACK response checking
}

int16_t MQTTClient::subscribe(const char* topicFilter) {
  // encode packet length
  size_t topicFilterLen = strlen(topicFilter);
  uint32_t remainingLength = 2 + (2 + topicFilterLen + 1);
  uint8_t encoded[] = {0, 0, 0, 0};
  size_t encodedBytes = encodeLength(remainingLength, encoded);

  // build the SUBSCRIBE packet
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t packet[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* packet = new uint8_t[1 + encodedBytes + remainingLength];
  #endif

  // fixed header
  packet[0] = (MQTT_SUBSCRIBE << 4) | 0b0010;
  memcpy(packet + 1, encoded, encodedBytes);

  // variable header
  // packet ID
  size_t pos = encodedBytes + 1;
  uint16_t packetId = _packetId++;
  packet[pos++] = (packetId & 0xFF00) >> 8;
  packet[pos++] = packetId & 0x00FF;

  // payload
  // topic filter
  packet[pos++] = (topicFilterLen & 0xFF00) >> 8;;
  packet[pos++] = topicFilterLen & 0x00FF;
  memcpy(packet + pos, topicFilter, topicFilterLen);
  pos += topicFilterLen;
  packet[pos++] = 0x00; // QoS 0

  // send MQTT packet
  int16_t state = _tl->send(packet, 1 + encodedBytes + remainingLength);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] packet;
  #endif
  if(state != ERR_NONE) {
    return(state);
  }

  // get the response length (MQTT SUBACK response has to be 5 bytes long for single subscription)
  size_t numBytes = _tl->getNumBytes();
  if(numBytes != 5) {
    return(ERR_RESPONSE_MALFORMED_AT);
  }

  // read the response
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t response[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* response = new uint8_t[numBytes];
  #endif
  _tl->receive(response, numBytes);
  if((response[0] == MQTT_SUBACK << 4) && (response[1] == 3)) {
    // check packet ID
    uint16_t receivedId = response[3] | response[2] << 8;
    int16_t returnCode = response[4];
    #ifndef RADIOLIB_STATIC_ONLY
      delete[] response;
    #endif
    if(receivedId != packetId) {
      return(ERR_MQTT_UNEXPECTED_PACKET_ID);
    }
    return(returnCode);
  }

  #ifndef RADIOLIB_STATIC_ONLY
    delete[] response;
  #endif
  return(ERR_RESPONSE_MALFORMED);
}

int16_t MQTTClient::unsubscribe(const char* topicFilter) {
  // encode packet length
  size_t topicFilterLen = strlen(topicFilter);
  uint32_t remainingLength = 2 + (2 + topicFilterLen);
  uint8_t encoded[] = {0, 0, 0, 0};
  size_t encodedBytes = encodeLength(remainingLength, encoded);

  // build the UNSUBSCRIBE packet
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t packet[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* packet = new uint8_t[1 + encodedBytes + remainingLength];
  #endif

  // fixed header
  packet[0] = (MQTT_UNSUBSCRIBE << 4) | 0b0010;
  memcpy(packet + 1, encoded, encodedBytes);

  // variable header
  // packet ID
  size_t pos = encodedBytes + 1;
  uint16_t packetId = _packetId++;
  packet[pos++] = (packetId & 0xFF00) >> 8;
  packet[pos++] = packetId & 0x00FF;

  // payload
  // topic filter
  packet[pos++] = (topicFilterLen & 0xFF00) >> 8;;
  packet[pos++] = topicFilterLen & 0x00FF;
  memcpy(packet + pos, topicFilter, topicFilterLen);

  // send MQTT packet
  int16_t state = _tl->send(packet, 1 + encodedBytes + remainingLength);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] packet;
  #endif
  if(state != ERR_NONE) {
    return(state);
  }

  // get the response length (MQTT UNSUBACK response has to be 4 bytes long)
  size_t numBytes = _tl->getNumBytes();
  if(numBytes != 4) {
    return(ERR_RESPONSE_MALFORMED_AT);
  }

  // read the response
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t response[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* response = new uint8_t[numBytes];
  #endif
  _tl->receive(response, numBytes);
  if((response[0] == MQTT_UNSUBACK << 4) && (response[1] == 2)) {
    // check packet ID
    uint16_t receivedId = response[3] | response[2] << 8;
    #ifndef RADIOLIB_STATIC_ONLY
      delete[] response;
    #endif
    if(receivedId != packetId) {
      return(ERR_MQTT_UNEXPECTED_PACKET_ID);
    }
    return(ERR_NONE);
  }

  #ifndef RADIOLIB_STATIC_ONLY
    delete[] response;
  #endif
  return(ERR_RESPONSE_MALFORMED);
}

int16_t MQTTClient::ping() {
  // build the PINGREQ packet
  uint8_t packet[2];

  // fixed header
  packet[0] = (MQTT_PINGREQ << 4) | 0b0000;
  packet[1] = 0x00;

  // send MQTT packet
  int16_t state = _tl->send(packet, 2);
  if(state != ERR_NONE) {
    return(state);
  }

  // get the response length (MQTT PINGRESP response has to be 2 bytes long)
  size_t numBytes = _tl->getNumBytes();
  if(numBytes != 2) {
    return(ERR_RESPONSE_MALFORMED_AT);
  }

  // read the response
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t response[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* response = new uint8_t[numBytes];
  #endif
  _tl->receive(response, numBytes);
  if((response[0] == MQTT_PINGRESP << 4) && (response[1] == 0)) {
    #ifndef RADIOLIB_STATIC_ONLY
      delete[] response;
    #endif
    return(ERR_NONE);
  }

  #ifndef RADIOLIB_STATIC_ONLY
    delete[] response;
  #endif
  return(ERR_RESPONSE_MALFORMED);
}

int16_t MQTTClient::check(void (*func)(const char*, const char*)) {
  // ping the server
  int16_t state = ping();
  if(state != ERR_NONE) {
    return(state);
  }

  // check new data
  size_t numBytes = _tl->getNumBytes();
  if(numBytes == 0) {
    return(ERR_MQTT_NO_NEW_PACKET_AVAILABLE);
  }

  // read the PUBLISH packet from server
  uint8_t* dataIn = new uint8_t[numBytes];
  _tl->receive(dataIn, numBytes);
  if(dataIn[0] == MQTT_PUBLISH << 4) {
    uint8_t remLenFieldLen = 0;
    uint32_t remainingLength = decodeLength(dataIn + 1, remLenFieldLen);

    // get the topic
    size_t topicLength = dataIn[remLenFieldLen + 2] | dataIn[remLenFieldLen + 1] << 8;
    char* topic = new char[topicLength + 1];
    memcpy(topic, dataIn + 4, topicLength);
    topic[topicLength] = 0x00;

    // get the message
    size_t messageLength = remainingLength - topicLength - 2;
    char* message = new char[messageLength + 1];
    memcpy(message, dataIn + remLenFieldLen + 3 + topicLength, messageLength);
    message[messageLength] = 0x00;

    // execute the callback function provided by user
    func(topic, message);

    delete[] topic;
    delete[] message;
    delete[] dataIn;
    return(ERR_NONE);
  }
  delete[] dataIn;

  return(ERR_MQTT_NO_NEW_PACKET_AVAILABLE);
}

size_t MQTTClient::encodeLength(uint32_t len, uint8_t* encoded) {
  // algorithm to encode packet length as per MQTT specification 3.1.1
  size_t i = 0;
  do {
    encoded[i] = len % 128;
    len /= 128;
    if(len > 0) {
      encoded[i] |= 128;
    }
    i++;
  } while(len > 0);
  return(i);
}

uint32_t MQTTClient::decodeLength(uint8_t* encoded, uint8_t& numBytes) {
  // algorithm to decode packet length as per MQTT specification 3.1.1
  uint32_t mult = 1;
  uint32_t len = 0;
  uint8_t i = 0;
  do {
    len += (encoded[i] & 127) * mult;
    mult *= 128;
    if(mult > 2097152) {
      // malformed remaining length
      return(0);
    }
  } while((encoded[i++] & 128) != 0);
  numBytes = i;
  return len;
}

#endif
