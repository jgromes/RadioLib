#include "ESP8266.h"

ESP8266::ESP8266(Module* module) {
  portHttp = 80;
  portMqtt = 1883;
  _mod = module;
}

uint8_t ESP8266::begin(long speed) {
  // set module properties
  _mod->AtLineFeed = "\r\n";
  _mod->baudrate = speed;
  _mod->init(USE_UART, INT_NONE);
  
  // empty UART buffer (garbage data)
  _mod->ATemptyBuffer();
  
  // test AT setup
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
  
  // test AT setup
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
  const char* atStr = "AT+CWJAP_CUR=\"";
  uint8_t cmdLen = strlen(atStr) + strlen(ssid) + strlen(password) + 4;
  
  char* cmd = new char[cmdLen];
  strcpy(cmd, atStr);
  strcat(cmd, ssid);
  strcat(cmd, "\",\"");
  strcat(cmd, password);
  strcat(cmd, "\"");
  
  bool res = _mod->ATsendCommand(cmd);
  delete[] cmd;
  if(!res) {
    return(ERR_AT_FAILED);
  }
  
  // disable multiple connection mode
  if(!_mod->ATsendCommand("AT+CIPMUX=0")) {
    return(ERR_AT_FAILED);
  }
  
  return(ERR_NONE);
}

uint16_t ESP8266::HttpGet(const char* url, String& response) {
  // get the host address and endpoint
  char* httpPrefix = strstr(url, "http://");
  char* endpoint;
  char* host;
  if(httpPrefix != NULL) {
    // find the host string
    char* hostStart = strchr(url, '/');
    hostStart = strchr(hostStart + 1, '/');
    char* hostEnd = strchr(hostStart + 1, '/');
    host = new char[hostEnd - hostStart];
    strncpy(host, hostStart + 1, hostEnd - hostStart - 1);
    host[hostEnd - hostStart - 1] = 0x00;
    
    // find the endpoint string
    endpoint = new char[url + strlen(url) - hostEnd + 1];
    strcpy(endpoint, hostEnd);
  } else {
    // find the host string
    char* hostEnd = strchr(url, '/');
    host = new char[hostEnd - url + 1];
    strncpy(host, url, hostEnd - url);
    host[hostEnd - url] = 0x00;
    
    // find the endpoint string
    endpoint = new char[url + strlen(url) - hostEnd + 1];
    strcpy(endpoint, hostEnd);
  }
  
  // build the GET request
  char* request = new char[strlen(endpoint) + strlen(host) + 25];
  strcpy(request, "GET ");
  strcat(request, endpoint);
  strcat(request, " HTTP/1.1\r\nHost: ");
  strcat(request, host);
  strcat(request, "\r\n\r\n");
  
  delete[] endpoint;
  
  // create TCP connection
  uint8_t state = openTransportConnection(host, "TCP", portHttp);
  delete[] host;
  if(state != ERR_NONE) {
    delete[] request;
    return(state);
  }
  
  // send the GET request
  state = send(request);
  delete[] request;
  if(state != ERR_NONE) {
    return(state);
  }
  
  delay(1000);
  
  // get the response length
  uint16_t numBytes = getNumBytes();
  if(numBytes == 0) {
    return(ERR_RESPONSE_MALFORMED_AT);
  }
  
  // read the response
  char* raw = new char[numBytes];
  size_t rawLength = receive((uint8_t*)raw);
  if(rawLength == 0) {
    delete[] raw;
    return(ERR_RESPONSE_MALFORMED);
  }
  
  // close the TCP connection
  state = closeTransportConnection();
  if(state != ERR_NONE) {
    delete[] raw;
    return(state);
  }
  
  // get the response body
  char* responseStart = strstr(raw, "\r\n");
  if(responseStart == NULL) {
    delete[] raw;
    return(ERR_RESPONSE_MALFORMED);
  }
  char* responseStr = new char[raw + rawLength - responseStart - 1];
  strncpy(responseStr, responseStart + 2, raw + rawLength - responseStart - 1);
  responseStr[raw + rawLength - responseStart - 2] = 0x00;
  response = String(responseStr);
  delete[] responseStr;
  
  // return the HTTP status code
  char* statusStart = strchr(raw, ' ');
  delete[] raw;
  if(statusStart == NULL) {
    return(ERR_RESPONSE_MALFORMED);
  }
  char statusStr[4];
  strncpy(statusStr, statusStart + 1, 3);
  statusStr[3] = 0x00;
  return(atoi(statusStr));
}

uint16_t ESP8266::HttpPost(const char* url, const char* content, String& response, const char* contentType) {
  // get the host address and endpoint
  char* httpPrefix = strstr(url, "http://");
  char* endpoint;
  char* host;
  if(httpPrefix != NULL) {
    // find the host string
    char* hostStart = strchr(url, '/');
    hostStart = strchr(hostStart + 1, '/');
    char* hostEnd = strchr(hostStart + 1, '/');
    host = new char[hostEnd - hostStart];
    strncpy(host, hostStart + 1, hostEnd - hostStart - 1);
    host[hostEnd - hostStart - 1] = 0x00;
    
    // find the endpoint string
    endpoint = new char[url + strlen(url) - hostEnd + 1];
    strcpy(endpoint, hostEnd);
    } else {
    // find the host string
    char* hostEnd = strchr(url, '/');
    host = new char[hostEnd - url + 1];
    strncpy(host, url, hostEnd - url);
    host[hostEnd - url] = 0x00;
    
    // find the endpoint string
    endpoint = new char[url + strlen(url) - hostEnd + 1];
    strcpy(endpoint, hostEnd);
  }
  
  // build the POST request
  char contentLengthStr[8];
  itoa(strlen(content), contentLengthStr, 10);
  char* request = new char[strlen(endpoint) + strlen(host) + strlen(contentType) + strlen(contentLengthStr) + strlen(content) + 64];
  strcpy(request, "POST ");
  strcat(request, endpoint);
  strcat(request, " HTTP/1.1\r\nHost: ");
  strcat(request, host);
  strcat(request, "\r\nContent-Type: ");
  strcat(request, contentType);
  strcat(request, "\r\nContent-length: ");
  strcat(request, contentLengthStr);
  strcat(request, "\r\n\r\n");
  strcat(request, content);
  strcat(request, "\r\n\r\n");
  
  delete[] endpoint;
  
  // create TCP connection
  uint8_t state = openTransportConnection(host, "TCP", portHttp);
  delete[] host;
  if(state != ERR_NONE) {
    return(state);
  }
  
  // send the POST request
  state = send(request);
  delete[] request;
  if(state != ERR_NONE) {
    return(state);
  }
  
  delay(2000);
  
  // get the response length
  uint16_t numBytes = getNumBytes();
  if(numBytes == 0) {
    return(ERR_RESPONSE_MALFORMED_AT);
  }
  
  // read the response
  char* raw = new char[numBytes];
  size_t rawLength = receive((uint8_t*)raw);
  if(rawLength == 0) {
    delete[] raw;
    return(ERR_RESPONSE_MALFORMED);
  }
  
  // close the TCP connection
  state = closeTransportConnection();
  if(state != ERR_NONE) {
    delete[] raw;
    return(state);
  }
  
  // get the response body
  char* responseStart = strstr(raw, "\r\n");
  if(responseStart == NULL) {
    delete[] raw;
    return(ERR_RESPONSE_MALFORMED);
  }
  char* responseStr = new char[raw + rawLength - responseStart - 1];
  strncpy(responseStr, responseStart + 2, raw + rawLength - responseStart - 1);
  responseStr[raw + rawLength - responseStart - 2] = 0x00;
  response = String(responseStr);
  delete[] responseStr;
  
  // return the HTTP status code
  char* statusStart = strchr(raw, ' ');
  delete[] raw;
  if(statusStart == NULL) {
    return(ERR_RESPONSE_MALFORMED);
  }
  char statusStr[4];
  strncpy(statusStr, statusStart + 1, 3);
  statusStr[3] = 0x00;
  return(atoi(statusStr));
}

uint8_t ESP8266::MqttConnect(const char* host, const char* clientId, const char* username, const char* password) {
  // encode packet length
  uint32_t len = strlen(clientId) + strlen(username) + strlen(password) + 16;
  uint8_t encoded[] = {0, 0, 0, 0};
  size_t encodedBytes = MqttEncodeLength(len, encoded);
  
  // build the CONNECT packet
  uint8_t* packet = new uint8_t[len + encodedBytes + 2];
  packet[0] = (MQTT_CONNECT << 4) & 0xFF;
  memcpy(packet + 1, encoded, encodedBytes);
  
  packet[encodedBytes + 1] = 0x00;
  packet[encodedBytes + 2] = 0x04;
  packet[encodedBytes + 3] = 'M';
  packet[encodedBytes + 4] = 'Q';
  packet[encodedBytes + 5] = 'T';
  packet[encodedBytes + 6] = 'T';
  
  packet[encodedBytes + 7] = 0x04;         // protocol level
  packet[encodedBytes + 8] = 0b11000010;   // flags: user name + password + clean session
  
  packet[encodedBytes + 9] = 0x00;         // keep-alive interval MSB
  packet[encodedBytes + 10] = 0x3C;        // keep-alive interval LSB
  
  packet[encodedBytes + 11] = (strlen(clientId) & 0xFF00) >> 8;
  packet[encodedBytes + 12] = strlen(clientId) & 0x00FF;
  memcpy(encodedBytes + packet + 13, clientId, strlen(clientId));
  
  packet[encodedBytes + 13 + strlen(clientId)] = (strlen(username) & 0xFF00) >> 8;
  packet[encodedBytes + 14 + strlen(clientId)] = strlen(username) & 0x00FF;
  memcpy(encodedBytes + packet + 15 + strlen(clientId), username, strlen(username));
  
  packet[encodedBytes + 15 + strlen(clientId) + strlen(username)] = (strlen(password) & 0xFF00) >> 8;;
  packet[encodedBytes + 16 + strlen(clientId) + strlen(username)] = strlen(password) & 0x00FF;
  memcpy(encodedBytes + packet + 17 + strlen(clientId) + strlen(username), password, strlen(password));
  
  // create TCP connection
  uint8_t state = openTransportConnection(host, "TCP", portMqtt, 7200);
  if(state != ERR_NONE) {
    delete[] packet;
    return(state);
  }
  
  // send MQTT packet
  state = send(packet, len + 2);
  delete[] packet;
  if(state != ERR_NONE) {
    return(state);
  }
  
  // get the response length (MQTT response has to be 4 bytes long)
  uint16_t numBytes = getNumBytes();
  if(numBytes != 4) {
    return(ERR_RESPONSE_MALFORMED_AT);
  }
  
  // read the response
  uint8_t* response = new uint8_t[numBytes];
  receive(response);
  if((response[0x00] == MQTT_CONNACK << 4) && (response[0x01] == 2)) {
    uint8_t returnCode = response[0x03];
    delete[] response;
    return(returnCode);
  }
  
  delete[] response;
  return(ERR_RESPONSE_MALFORMED);
}

uint8_t ESP8266::MqttPublish(const char* topic, const char* message) {
  // encode packet length
  uint32_t len = 2 + strlen(topic) + strlen(message);
  uint8_t encoded[] = {0, 0, 0, 0};
  size_t encodedBytes = MqttEncodeLength(len, encoded);
  
  // build the PUBLISH packet
  uint8_t* packet = new uint8_t[len + 2];
  packet[0] = (MQTT_PUBLISH << 4);
  memcpy(packet + 1, encoded, encodedBytes);
  
  packet[encodedBytes + 1] = (strlen(topic) & 0xFF00) >> 8;
  packet[encodedBytes + 2] = strlen(topic) & 0x00FF;
  memcpy(encodedBytes + packet + 3, topic, strlen(topic));
  memcpy(encodedBytes + packet + 3 + strlen(topic), message, strlen(message));
  
  // send MQTT packet
  uint8_t state = send(packet, len + 2);
  delete[] packet;
  return(state);
  
  //TODO: implement QoS > 0 and PUBACK response checking
}

uint8_t ESP8266::send(const char* data) {
  // build AT command
  char lenStr[8];
  itoa(strlen(data), lenStr, 10);
  const char* atStr = "AT+CIPSEND=";
  char* cmd = new char[strlen(atStr) + strlen(lenStr)];
  strcpy(cmd, atStr);
  strcat(cmd, lenStr);
  
  // send data length in bytes
  bool res = _mod->ATsendCommand(cmd);
  delete[] cmd;
  if(!res) {
    return(ERR_AT_FAILED);
  }
  
  // send data
  if(!_mod->ATsendCommand(data)) {
    return(ERR_AT_FAILED);
  }
  
  return(ERR_NONE);
}

uint8_t ESP8266::send(uint8_t* data, uint32_t len) {
  // build AT command
  char lenStr[8];
  itoa(len, lenStr, 10);
  const char atStr[] = "AT+CIPSEND=";
  char* cmd = new char[strlen(atStr) + strlen(lenStr)];
  strcpy(cmd, atStr);
  strcat(cmd, lenStr);
  
  // send command and data length in bytes
  bool res = _mod->ATsendCommand(cmd);
  delete[] cmd;
  if(!res) {
    return(ERR_AT_FAILED);
  }
  
  // send data
  if(!_mod->ATsendData(data, len)) {
    return(ERR_AT_FAILED);
  }
  
  return(ERR_NONE);
}

size_t ESP8266::receive(uint8_t* data, uint32_t timeout) {
  size_t i = 0;
  uint32_t start = millis();
  while(millis() - start < timeout) {
    while(_mod->ModuleSerial->available() > 0) {
      uint8_t b = _mod->ModuleSerial->read();
      DEBUG_PRINT(b);
      data[i] = b;
      i++;
    }
  }
  return(i);
}

uint8_t ESP8266::openTransportConnection(const char* host, const char* protocol, uint16_t port, uint16_t tcpKeepAlive) {
  char portStr[6];
  itoa(port, portStr, 10);
  char tcpKeepAliveStr[6];
  itoa(tcpKeepAlive, tcpKeepAliveStr, 10);
  
  const char* atStr = "AT+CIPSTART=\"";
  uint8_t cmdLen = strlen(atStr) + strlen(protocol) + strlen(host) + strlen(portStr) + 5;
  
  if((strcmp(protocol, "TCP") == 0) && (tcpKeepAlive > 0)) {
	  cmdLen += strlen(tcpKeepAliveStr) + 1;
  }
  
  char* cmd = new char[cmdLen];
  strcpy(cmd, atStr);
  strcat(cmd, protocol);
  strcat(cmd, "\",\"");
  strcat(cmd, host);
  strcat(cmd, "\",");
  strcat(cmd, portStr);
  
  if((strcmp(protocol, "TCP") == 0) && (tcpKeepAlive > 0)) {
    strcat(cmd, ",");
    strcat(cmd, tcpKeepAliveStr);
  }
  
  bool res = _mod->ATsendCommand(cmd);
  delete[] cmd;
  if(!res) {
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

size_t ESP8266::MqttEncodeLength(uint32_t len, uint8_t* encoded) {
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

uint16_t ESP8266::getNumBytes(uint32_t timeout) {
  // wait for available data
  uint32_t start = millis();
  while(_mod->ModuleSerial->available() < 10) {
    if(millis() - start >= timeout) {
      return(0);
    }
  }
  
  // read response
  char rawStr[20];
  uint8_t i = 0;
  start = millis();
  while(_mod->ModuleSerial->available() > 0) {
    char c = _mod->ModuleSerial->read();
    rawStr[i++] = c;
    if(c == ':') {
      rawStr[i++] = 0;
      break;
    }
    if(millis() - start >= timeout) {
      rawStr[i++] = 0;
      break;
    }
  }
  
  // get the number of bytes in response
  char* pch = strtok(rawStr, ",:");
  if(pch == NULL) {
    return(0);
  }
  
  pch = strtok(NULL, ",:");
  if(pch == NULL) {
    return(0);
  }
  
  return(atoi(pch));
}
