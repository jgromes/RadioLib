#include "HTTP.h"
#if !defined(RADIOLIB_EXCLUDE_HTTP)

HTTPClient::HTTPClient(TransportLayer* tl, uint16_t port) {
  _tl = tl;
  _port = port;
}

int16_t HTTPClient::get(String& url, String& response) {
  return(HTTPClient::get(url.c_str(), response));
}

int16_t HTTPClient::get(const char* url, String& response) {
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
    host[hostEnd - hostStart - 1] = '\0';

    // find the endpoint string
    endpoint = new char[url + strlen(url) - hostEnd + 1];
    strcpy(endpoint, hostEnd);
  } else {
    // find the host string
    char* hostEnd = strchr(url, '/');
    host = new char[hostEnd - url + 1];
    strncpy(host, url, hostEnd - url);
    host[hostEnd - url] = '\0';

    // find the endpoint string
    endpoint = new char[url + strlen(url) - hostEnd + 1];
    strcpy(endpoint, hostEnd);
  }

  // build the GET request
  char* request = new char[strlen(endpoint) + strlen(host) + 25 + 1];
  strcpy(request, "GET ");
  strcat(request, endpoint);
  strcat(request, " HTTP/1.1\r\nHost: ");
  strcat(request, host);
  strcat(request, "\r\n\r\n");

  delete[] endpoint;

  // create TCP connection
  int16_t state = _tl->openTransportConnection(host, "TCP", _port);
  delete[] host;
  if(state != ERR_NONE) {
    delete[] request;
    return(state);
  }

  // send the GET request
  state = _tl->send(request);
  delete[] request;
  if(state != ERR_NONE) {
    return(state);
  }

  // get the response length
  size_t numBytes = _tl->getNumBytes();
  if(numBytes == 0) {
    return(ERR_RESPONSE_MALFORMED_AT);
  }

  // read the response
  char* raw = new char[numBytes + 1];
  size_t rawLength = _tl->receive((uint8_t*)raw, numBytes);
  if(rawLength == 0) {
    delete[] raw;
    return(ERR_RESPONSE_MALFORMED);
  }

  // close the tl connection
  state = _tl->closeTransportConnection();
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
  char* responseStr = new char[raw + rawLength - responseStart - 1 + 1];
  strncpy(responseStr, responseStart + 2, raw + rawLength - responseStart - 1);
  responseStr[raw + rawLength - responseStart - 2] = '\0';
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

int16_t HTTPClient::post(const char* url, const char* content, String& response, const char* contentType) {
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
    host[hostEnd - hostStart - 1] = '\0';

    // find the endpoint string
    endpoint = new char[url + strlen(url) - hostEnd + 1];
    strcpy(endpoint, hostEnd);
    } else {
    // find the host string
    char* hostEnd = strchr(url, '/');
    host = new char[hostEnd - url + 1];
    strncpy(host, url, hostEnd - url);
    host[hostEnd - url] = '\0';

    // find the endpoint string
    endpoint = new char[url + strlen(url) - hostEnd + 1];
    strcpy(endpoint, hostEnd);
  }

  // build the POST request
  char contentLengthStr[12];
  sprintf(contentLengthStr, "%u", (uint16_t)strlen(content));
  char* request = new char[strlen(endpoint) + strlen(host) + strlen(contentType) + strlen(contentLengthStr) + strlen(content) + 64 + 1];
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
  int16_t state = _tl->openTransportConnection(host, "TCP", _port);
  delete[] host;
  if(state != ERR_NONE) {
    delete[] request;
    return(state);
  }

  // send the POST request
  state = _tl->send(request);
  delete[] request;
  if(state != ERR_NONE) {
    return(state);
  }

  // get the response length
  size_t numBytes = _tl->getNumBytes();
  if(numBytes == 0) {
    return(ERR_RESPONSE_MALFORMED_AT);
  }

  // read the response
  char* raw = new char[numBytes];
  size_t rawLength = _tl->receive((uint8_t*)raw, numBytes);
  if(rawLength == 0) {
    delete[] raw;
    return(ERR_RESPONSE_MALFORMED);
  }

  // close the tl connection
  state = _tl->closeTransportConnection();
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

#endif
