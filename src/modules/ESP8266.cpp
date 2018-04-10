#include "ESP8266.h"

ESP8266::ESP8266(Module* module) {
  portTcp = 80; // Default HTTP port (TCP application)
  portUdp = 53; // Default DNS port (UDP application)
  portMqtt = 1883;
  _mod = module;
}

uint8_t ESP8266::begin(long speed) {
  _mod->AtLineFeed = "\r\n";
  _mod->baudrate = speed;
  _mod->init(USE_UART, INT_NONE);
  _mod->ATemptyBuffer();
  if(!_mod->ATsendCommand("AT")) {
    return(ERR_AT_FAILED);
  }
  return(ERR_NONE);
}

uint8_t ESP8266::reset() {
  // send the reset command
  if(!_mod->ATsendCommand("AT+RST")) {
    return(ERR_AT_FAILED);
  }
  
  // wait for the module to start
  delay(2000);
  
  // test AT
  uint32_t start = millis();
  while (millis() - start < 3000) {
    if(!_mod->ATsendCommand("AT")) {
      delay(100);
    } else {
      return(ERR_NONE);
    }
  }
  
  return(ERR_AT_FAILED);
}

uint8_t ESP8266::join(const char* ssid, const char* password) {
  // set mode to station + soft AP
  if(!_mod->ATsendCommand("AT+CWMODE_CUR=3")) {
    return(ERR_AT_FAILED);
  }
  
  // reset the module
  uint8_t state = reset();
  if(state != ERR_NONE) {
    return(state);
  }
  
  // join AP
  String cmd = "AT+CWJAP_CUR=\"";
  cmd += ssid;
  cmd += "\",\"";
  cmd += password;
  cmd += "\"";
  if(!_mod->ATsendCommand(cmd)) {
    return(ERR_AT_FAILED);
  }
  
  // disable multiple connection mode
  if(!_mod->ATsendCommand("AT+CIPMUX=0")) {
    return(ERR_AT_FAILED);
  }
  
  return(ERR_NONE);
}

uint16_t ESP8266::HttpGet(const char* url, String& response) {
  String urlString(url);
  
  // get the host address and endpoint
  int32_t resourceIndex = urlString.indexOf("/", 7);
  if(resourceIndex == -1) {
    return(ERR_URL_MALFORMED);
  }
  String host = urlString.substring(7, resourceIndex);
  String endpoint = urlString.substring(resourceIndex);
  
  // build the GET request
  String request = "GET ";
  request += endpoint;
  request += " HTTP/1.1\r\nHost: ";
  request += host;
  request += "\r\n\r\n";
  
  // create TCP connection
  uint8_t state = startTcp(host.c_str());
  if(state != ERR_NONE) {
    return(state);
  }
  
  // send the GET request
  state = send(request);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // read the response
  String raw = receive();
  
  // close the TCP connection
  state = closeTcp();
  if(state != ERR_NONE) {
    return(state);
  }
  
  // parse the response
  int32_t numBytesIndex = raw.indexOf(":");
  if(numBytesIndex == -1) {
    return(ERR_RESPONSE_MALFORMED_AT);
  }
  response = raw.substring(numBytesIndex + 1);
  
  // return the HTTP status code
  int32_t spaceIndex = response.indexOf(" ");
  if(spaceIndex == -1) {
    return(ERR_RESPONSE_MALFORMED);
  }
  String statusString = response.substring(spaceIndex + 1, spaceIndex + 4);
  return(statusString.toInt());
}

uint16_t ESP8266::HttpPost(const char* url, String content, String& response, const char* contentType) {
  String urlString(url);
  String contentTypeString(contentType);
  
  // get the host address and endpoint
  int32_t resourceIndex = urlString.indexOf("/", 7);
  if(resourceIndex == -1) {
    return(ERR_URL_MALFORMED);
  }
  String host = urlString.substring(7, resourceIndex);
  String endpoint = urlString.substring(resourceIndex);
  
  // build the POST request
  String request = "POST ";
  request += endpoint;
  request += " HTTP/1.1\r\nHost: ";
  request += host;
  request += "\r\nContent-Type: ";
  request += contentTypeString;
  request += "\r\nContent-length: ";
  request += content.length();
  request += "\r\n\r\n";
  
  // create TCP connection
  uint8_t state = startTcp(host.c_str());
  if(state != ERR_NONE) {
    return(state);
  }
  
  // send the POST request
  state = send(request);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // close the TCP connection
  state = closeTcp();
  if(state != ERR_NONE) {
    return(state);
  }
  
  // read the response
  String raw = receive();
  
  // parse the response
  int32_t numBytesIndex = raw.indexOf(":");
  if(numBytesIndex == -1) {
    return(ERR_RESPONSE_MALFORMED_AT);
  }
  response = raw.substring(numBytesIndex + 1);
  
  // return the HTTP status code
  int32_t spaceIndex = response.indexOf(" ");
  if(spaceIndex == -1) {
    return(ERR_RESPONSE_MALFORMED);
  }
  String statusString = response.substring(spaceIndex + 1, spaceIndex + 4);
  return(statusString.toInt());
}

uint8_t ESP8266::MqttConnect(String host, String clientId, String username, String password) {
  _MqttHost = host;
  
  // encode packet length
  uint32_t len = 16 + clientId.length() + username.length() + password.length();
  /*uint8_t encoded[] = {0, 0, 0, 0};
  MqttEncodeLength(len, encoded);*/
  
  // build the CONNECT packet
  uint8_t* packet = new uint8_t[len + 2];
  packet[0] = (MQTT_CONNECT << 4) & 0xFF;
  packet[1] = len;
  /*for(uint8_t i = 1; i < 4; i++) {
    packet[i] = encoded[i];
  }*/
  
  packet[2] = 0x00;
  packet[3] = 0x04;
  packet[4] = 'M';
  packet[5] = 'Q';
  packet[6] = 'T';
  packet[7] = 'T';
  packet[8] = 0x04;        //protocol level
  packet[9] = 0b11000010;  //flags: user name + password + clean session
  packet[10] = 0x00;        //keep-alive interval MSB
  packet[11] = 0x3C;        //keep-alive interval LSB
  
  packet[12] = 0x00;
  packet[13] = clientId.length();
  for(uint8_t i = 0; i < clientId.length(); i++) {
    packet[i + 14] = (uint8_t)clientId.charAt(i);
  }
  
  packet[14 + clientId.length()] = 0x00;
  packet[15 + clientId.length()] = username.length();
  for(uint8_t i = 0; i < username.length(); i++) {
    packet[i + 16 + clientId.length()] = (uint8_t)username.charAt(i);
  }
  
  packet[16 + clientId.length() + username.length()] = 0x00;
  packet[17 + clientId.length() + username.length()] = password.length();
  for(uint8_t i = 0; i < password.length(); i++) {
    packet[i + 18 + clientId.length() + username.length()] = (uint8_t)password.charAt(i);
  }
  
  /*for(uint8_t i = 0; i < len + 2; i++) {
    Serial.print(i);
    Serial.print('\t');
    Serial.write(packet[i]);
    Serial.print("\t0x");
    Serial.println(packet[i], HEX);
  }*/
  
  // create TCP connection
  uint8_t state = openTransportConnection(_MqttHost.c_str(), "TCP", portMqtt, 7200);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // send MQTT packet
  state = send(packet, len + 2);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // read the response
  /*uint8_t response[] = {0, 0, 0, 0, 0};
  receive(response);*/
  String raw = receive();
  
  /*for(uint8_t i = 0; i < raw.length(); i++) {
    Serial.print(i);
    Serial.print('\t');
    Serial.write(raw.charAt(i));
    Serial.print("\t0x");
    Serial.println(raw.charAt(i), HEX);
  }*/
  
  // parse the response
  int32_t numBytesIndex = raw.indexOf(":");
  if(numBytesIndex == -1) {
    return(ERR_RESPONSE_MALFORMED_AT);
  }
  
  uint8_t response[] = {0, 0, 0, 0};
  for(uint8_t i = 0; i < 4; i++) {
    response[i] = raw.charAt(i + numBytesIndex + 1);
  }
  
  /*for(uint8_t i = 0; i < 4; i++) {
    Serial.print(i);
    Serial.print('\t');
    Serial.write(response[i]);
    Serial.print("\t0x");
    Serial.println(response[i], HEX);
  }*/
  if(response[3] != 0x00) {
    return(ERR_MQTT_CONNECTION_REFUSED);
  }
  
  return(ERR_NONE);
}

uint8_t ESP8266::MqttPublish(String topic, String message) {
  // encode packet length
  uint32_t len = 2 + topic.length() + message.length();
  
  // build the PUBLISH packet
  uint8_t* packet = new uint8_t[len + 2];
  packet[0] = (MQTT_PUBLISH << 4) & 0xFF;
  packet[1] = len;
  
  packet[2] = 0x00;
  packet[3] = topic.length();
  for(uint8_t i = 0; i < topic.length(); i++) {
    packet[i + 4] = (uint8_t)topic.charAt(i);
  }
  
  for(uint8_t i = 0; i < message.length(); i++) {
    packet[i + 4 + topic.length()] = (uint8_t)message.charAt(i);
  }
  
  /*for(uint8_t i = 0; i < len + 2; i++) {
    Serial.print(i);
    Serial.print('\t');
    Serial.write(packet[i]);
    Serial.print("\t0x");
    Serial.println(packet[i], HEX);
  }*/
  
  // send MQTT packet
  uint8_t state = send(packet, len + 2);
  if(state != ERR_NONE) {
    return(state);
  }
  
  return(ERR_NONE);
}

uint8_t ESP8266::startTcp(const char* host, uint16_t tcpKeepAlive) {
  return(openTransportConnection(host, "TCP", portTcp, tcpKeepAlive));
}

uint8_t ESP8266::closeTcp() {
  return(closeTransportConnection());
}

uint8_t ESP8266::startUdp(const char* host) {
  return(openTransportConnection(host, "UDP", portUdp));
}

uint8_t ESP8266::closeUdp() {
  return(closeTransportConnection());
}

uint8_t ESP8266::send(String data) {
  // send data length in bytes
  String cmd = "AT+CIPSEND=";
  cmd += data.length();
  if(!_mod->ATsendCommand(cmd)) {
    return(ERR_AT_FAILED);
  }
  
  // send data
  if(!_mod->ATsendCommand(data)) {
    return(ERR_AT_FAILED);
  }
  
  return(ERR_NONE);
}

uint8_t ESP8266::send(uint8_t* data, uint32_t len) {
  // send data length in bytes
  String cmd = "AT+CIPSEND=";
  cmd += len;
  if(!_mod->ATsendCommand(cmd)) {
    return(ERR_AT_FAILED);
  }
  
  // send data
  if(!_mod->ATsendData(data, len)) {
    return(ERR_AT_FAILED);
  }
  
  return(ERR_NONE);
}

String ESP8266::receive(uint32_t timeout) {
  String data;
  uint32_t start = millis();
  while(millis() - start < timeout) {
    while(_mod->ModuleSerial->available() > 0) {
      char c = _mod->ModuleSerial->read();
      #ifdef DEBUG
        Serial.print(c);
      #endif
      data += c;
    }
  }
  return(data);
}

uint32_t ESP8266::receive(uint8_t* data, uint32_t timeout) {
  uint8_t i = 0;
  uint32_t start = millis();
  while(millis() - start < timeout) {
    while(_mod->ModuleSerial->available() > 0) {
      uint8_t b = _mod->ModuleSerial->read();
      /*Serial.write(b);
      Serial.print("\t0x");
      Serial.println(b, HEX);*/
      #ifdef DEBUG
        Serial.print(b);
      #endif
      data[i] = b;
      i++;
    }
  }
  return(i);
}

uint8_t ESP8266::openTransportConnection(const char* host, const char* protocol, uint16_t port, uint16_t tcpKeepAlive) {
  String cmd = "AT+CIPSTART=\"";
  cmd += protocol;
  cmd += "\",\"";
  cmd += host;
  cmd += "\",";
  cmd += port;
  if((protocol == "TCP") && (tcpKeepAlive > 0)) {
    cmd += ",";
    cmd += tcpKeepAlive;
  }
  if(!_mod->ATsendCommand(cmd)) {
    return(ERR_AT_FAILED);
  }
  return(ERR_NONE);
}

uint8_t ESP8266::closeTransportConnection() {
  if(!_mod->ATsendCommand("AT+CIPCLOSE")) {
    return(ERR_AT_FAILED);
    }
  return(ERR_NONE);
}

void ESP8266::MqttEncodeLength(uint32_t len, uint8_t* encoded) {
  uint8_t i = 0;
  do {
    encoded[i] = len % 128;
    len /= 128;
    if(len > 0) {
      encoded[i] |= 128;
    }
    i++;
  } while(len > 0);
}

uint32_t ESP8266::MqttDecodeLength(uint8_t* encoded) {
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
  } while((encoded[i] & 128) != 0);
  return len;
}
