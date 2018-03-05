#include "ESP8266.h"

ESP8266::ESP8266(Module* module) {
  _mod = module;
}

uint8_t ESP8266::begin(long speed) {
  _mod->baudrate = speed;
  _mod->init(USE_UART, INT_NONE);
  _mod->ATemptyBuffer();
  if(!_mod->ATsendCommand("AT")) {
    return(ERR_UNKNOWN);
  }
  return(ERR_NONE);
}

uint8_t ESP8266::restart() {
  // send the restart command
  if(!_mod->ATsendCommand("AT+RST")) {
    return(ERR_UNKNOWN);
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
  
  return(ERR_UNKNOWN);
}

uint8_t ESP8266::join(const char* ssid, const char* password) {
  // set mode to station + soft AP
  if(!_mod->ATsendCommand("AT+CWMODE_CUR=3")) {
    return(ERR_UNKNOWN);
  }
  
  // restart module
  uint8_t state = restart();
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
    return(ERR_UNKNOWN);
  }
  
  // disable multiple connection mode
  if(!_mod->ATsendCommand("AT+CIPMUX=0")) {
    return(ERR_UNKNOWN);
  }
  
  return(ERR_NONE);
}

uint16_t ESP8266::HttpGet(const char* url, String& data, uint16_t port) {
  String urlString(url);
  
  // get the host address and endpoint
  int32_t resourceIndex = urlString.indexOf("/", 7);
  if(resourceIndex == -1) {
    return(ERR_URL_MALFORMED);
  }
  String host = urlString.substring(7, resourceIndex);
  String endpoint = urlString.substring(resourceIndex);
  
  // create TCP connection
  uint8_t state = startTCP(host.c_str(), 80);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // build the GET request
  String request = "GET ";
  request += endpoint;
  request += " HTTP/1.1\r\nHost: ";
  request += host;
  request += "\r\n\r\n";
  
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
  data = raw.substring(numBytesIndex + 1);
  
  // return the HTTP status code
  int32_t spaceIndex = data.indexOf(" ");
  if(spaceIndex == -1) {
    return(ERR_RESPONSE_MALFORMED);
  }
  String statusString = data.substring(spaceIndex + 1, spaceIndex + 4);
  return(statusString.toInt());
}

uint8_t ESP8266::startTCP(const char* host, uint16_t port) {
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += host;
  cmd += "\",";
  cmd += port;
  if(!_mod->ATsendCommand(cmd)) {
    return(ERR_UNKNOWN);
  }
  return(ERR_NONE);
}

uint8_t ESP8266::closeTCP() {
  if(!_mod->ATsendCommand("AT+CIPCLOSE")) {
    return(ERR_UNKNOWN);
  }
  return(ERR_NONE);
}

uint8_t ESP8266::send(String data) {
  // send data length in bytes
  String cmd = "AT+CIPSEND=";
  cmd += data.length();
  if(!_mod->ATsendCommand(cmd)) {
    return(ERR_UNKNOWN);
  }
  
  // send data
  if(!_mod->ATsendCommand(data)) {
    return(ERR_UNKNOWN);
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

