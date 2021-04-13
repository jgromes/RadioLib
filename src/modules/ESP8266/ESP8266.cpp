#include "ESP8266.h"
#if !defined(RADIOLIB_EXCLUDE_ESP8266) && !defined(ESP8266)

ESP8266::ESP8266(Module* module) {
  _mod = module;
}

int16_t ESP8266::begin(long speed) {
  // set module properties
  char lf[3] = "\r\n";
  memcpy(_mod->AtLineFeed, lf, strlen(lf));
  _mod->baudrate = speed;
  _mod->init(RADIOLIB_USE_UART);

  // empty UART buffer (garbage data)
  _mod->ATemptyBuffer();

  // test AT setup
  if(!_mod->ATsendCommand("AT")) {
    return(ERR_AT_FAILED);
  }

  return(ERR_NONE);
}

int16_t ESP8266::reset() {
  // send the reset command
  if(!_mod->ATsendCommand("AT+RST")) {
    return(ERR_AT_FAILED);
  }

  // wait for the module to start
  Module::delay(2000);

  // test AT setup
  uint32_t start = Module::millis();
  while (Module::millis() - start < 3000) {
    if(!_mod->ATsendCommand("AT")) {
      Module::delay(100);
    } else {
      return(ERR_NONE);
    }
  }

  return(ERR_AT_FAILED);
}

int16_t ESP8266::join(const char* ssid, const char* password) {
  // set mode to station + soft AP
  if(!_mod->ATsendCommand("AT+CWMODE_CUR=3")) {
    return(ERR_AT_FAILED);
  }

  // build AT command
  const char* atStr = "AT+CWJAP_CUR=\"";
  #ifdef RADIOLIB_STATIC_ONLY
    char cmd[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t cmdLen = strlen(atStr) + strlen(ssid) + strlen(password) + 4;
    char* cmd = new char[cmdLen + 1];
  #endif
  strcpy(cmd, atStr);
  strcat(cmd, ssid);
  strcat(cmd, "\",\"");
  strcat(cmd, password);
  strcat(cmd, "\"");

  // send command
  bool res = _mod->ATsendCommand(cmd);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] cmd;
  #endif
  if(!res) {
    return(ERR_AT_FAILED);
  }

  // disable multiple connection mode
  if(!_mod->ATsendCommand("AT+CIPMUX=0")) {
    return(ERR_AT_FAILED);
  }

  return(ERR_NONE);
}

int16_t ESP8266::openTransportConnection(const char* host, const char* protocol, uint16_t port, uint16_t tcpKeepAlive) {
  char portStr[6];
  sprintf(portStr, "%u", port);
  char tcpKeepAliveStr[6];
  sprintf(tcpKeepAliveStr, "%u", tcpKeepAlive);

  // build AT command
  const char* atStr = "AT+CIPSTART=\"";
  #ifdef RADIOLIB_STATIC_ONLY
    char cmd[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t cmdLen = strlen(atStr) + strlen(protocol) + strlen(host) + strlen(portStr) + 5;
    if((strcmp(protocol, "TCP") == 0) && (tcpKeepAlive > 0)) {
  	  cmdLen += strlen(tcpKeepAliveStr) + 1;
    }
    char* cmd = new char[cmdLen + 1];
  #endif
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

  // send command
  bool res = _mod->ATsendCommand(cmd);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] cmd;
  #endif
  if(!res) {
    return(ERR_AT_FAILED);
  }

  return(ERR_NONE);
}

int16_t ESP8266::closeTransportConnection() {
  // send AT command
  if(!_mod->ATsendCommand("AT+CIPCLOSE")) {
    return(ERR_AT_FAILED);
    }
  return(ERR_NONE);
}

int16_t ESP8266::send(const char* data) {
  // build AT command
  char lenStr[12];
  sprintf(lenStr, "%u", (uint16_t)strlen(data));
  const char* atStr = "AT+CIPSEND=";
  #ifdef RADIOLIB_STATIC_ONLY
    char cmd[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    char* cmd = new char[strlen(atStr) + strlen(lenStr) + 1];
  #endif
  strcpy(cmd, atStr);
  strcat(cmd, lenStr);

  // send command
  bool res = _mod->ATsendCommand(cmd);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] cmd;
  #endif
  if(!res) {
    return(ERR_AT_FAILED);
  }

  // send data
  if(!_mod->ATsendCommand(data)) {
    return(ERR_AT_FAILED);
  }

  return(ERR_NONE);
}

int16_t ESP8266::send(uint8_t* data, size_t len) {
  // build AT command
  char lenStr[8];
  sprintf(lenStr, "%u", (uint16_t)len);
  const char atStr[] = "AT+CIPSEND=";
  #ifdef RADIOLIB_STATIC_ONLY
    char cmd[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    char* cmd = new char[strlen(atStr) + strlen(lenStr) + 1];
  #endif
  strcpy(cmd, atStr);
  strcat(cmd, lenStr);

  // send command
  bool res = _mod->ATsendCommand(cmd);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] cmd;
  #endif
  if(!res) {
    return(ERR_AT_FAILED);
  }

  // send data
  if(!_mod->ATsendData(data, len)) {
    return(ERR_AT_FAILED);
  }

  return(ERR_NONE);
}

size_t ESP8266::receive(uint8_t* data, size_t len, uint32_t timeout) {
  size_t i = 0;
  uint32_t start = Module::millis();

  // wait until the required number of bytes is received or until timeout
  while((Module::millis() - start < timeout) && (i < len)) {
    Module::yield();
    while(_mod->ModuleSerial->available() > 0) {
      uint8_t b = _mod->ModuleSerial->read();
      RADIOLIB_DEBUG_PRINT(b);
      data[i] = b;
      i++;
    }
  }
  return(i);
}

size_t ESP8266::getNumBytes(uint32_t timeout, size_t minBytes) {
  // wait for available data
  uint32_t start = Module::millis();
  while(_mod->ModuleSerial->available() < (int16_t)minBytes) {
    Module::yield();
    if(Module::millis() - start >= timeout) {
      return(0);
    }
  }

  // read response
  char rawStr[20];
  uint8_t i = 0;
  start = Module::millis();
  while(_mod->ModuleSerial->available() > 0) {
    Module::yield();
    char c = _mod->ModuleSerial->read();
    rawStr[i++] = c;
    if(c == ':') {
      rawStr[i++] = 0;
      break;
    }
    if(Module::millis() - start >= timeout) {
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
#endif
