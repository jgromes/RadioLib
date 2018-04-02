#include "ESP8266.h"

ESP8266::ESP8266(Module* module) {
  portTCP = 80; // Default HTTP port (TCP application)
  portUDP = 53; // Default DNS port (UDP application)
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
  uint8_t state = startTCP(host.c_str());
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
  state = closeTCP();
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
  uint8_t state = startTCP(host.c_str());
  if(state != ERR_NONE) {
    return(state);
  }
  
  // send the POST request
  state = send(request);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // close the TCP connection
  state = closeTCP();
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

uint8_t ESP8266::startTCP(const char* host) {
  openTransportConnection(host, "TCP", portTCP);
}

uint8_t ESP8266::closeTCP() {
  closeTransportConnection();
}

uint8_t ESP8266::startUDP(const char* host) {
  openTransportConnection(host, "UDP", portUDP);
}

uint8_t ESP8266::closeUDP() {
  closeTransportConnection();
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

uint8_t ESP8266::openTransportConnection(const char* host, const char* protocol, uint16_t port) {
  String cmd = "AT+CIPSTART=\"";
  cmd += protocol;
  cmd += "\",\"";
  cmd += host;
  cmd += "\",";
  cmd += port;
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
