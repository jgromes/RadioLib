#include "APRS.h"

APRSClient::APRSClient(AX25Client* ax) {
  _ax = ax;
}

int16_t APRSClient::begin(char symbol, bool alt) {
  RADIOLIB_CHECK_RANGE(symbol, ' ', '}', RADIOLIB_ERR_INVALID_SYMBOL);
  _symbol = symbol;

  if(alt) {
    _table = '\\';
  } else {
    _table = '/';
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t APRSClient::sendPosition(char* destCallsign, uint8_t destSSID, char* lat, char* lon, char* msg, char* time) {
  #if !defined(RADIOLIB_STATIC_ONLY)
    size_t len = 1 + strlen(lat) + 1 + strlen(lon);
    if(msg != NULL) {
      len += 1 + strlen(msg);
    }
    if(time != NULL) {
      len += strlen(time);
    }
    char* info = new char[len];
  #else
    char info[RADIOLIB_STATIC_ARRAY_SIZE];
  #endif

  // build the info field
  if((msg == NULL) && (time == NULL)) {
    // no message, no timestamp
    sprintf(info, RADIOLIB_APRS_DATA_TYPE_POSITION_NO_TIME_NO_MSG "%s%c%s%c", lat, _table, lon, _symbol);
  } else if((msg != NULL) && (time == NULL)) {
    // message, no timestamp
    sprintf(info, RADIOLIB_APRS_DATA_TYPE_POSITION_NO_TIME_MSG "%s%c%s%c%s", lat, _table, lon, _symbol, msg);
  } else if((msg == NULL) && (time != NULL)) {
    // timestamp, no message
    sprintf(info, RADIOLIB_APRS_DATA_TYPE_POSITION_TIME_NO_MSG "%s%s%c%s%c", time, lat, _table, lon, _symbol);
  } else {
    // timestamp and message
    sprintf(info, RADIOLIB_APRS_DATA_TYPE_POSITION_TIME_MSG "%s%s%c%s%c%s", time, lat, _table, lon, _symbol, msg);
  }

  // send the frame
  int16_t state = sendFrame(destCallsign, destSSID, info);
  #if !defined(RADIOLIB_STATIC_ONLY)
    delete[] info;
  #endif
  return(state);
}

int16_t APRSClient::sendFrame(char* destCallsign, uint8_t destSSID, char* info) {
  // get AX.25 callsign
  char srcCallsign[RADIOLIB_AX25_MAX_CALLSIGN_LEN + 1];
  _ax->getCallsign(srcCallsign);

  AX25Frame frameUI(destCallsign, destSSID, srcCallsign, _ax->getSSID(), RADIOLIB_AX25_CONTROL_U_UNNUMBERED_INFORMATION |
                    RADIOLIB_AX25_CONTROL_POLL_FINAL_DISABLED | RADIOLIB_AX25_CONTROL_UNNUMBERED_FRAME,
                    RADIOLIB_AX25_PID_NO_LAYER_3, (const char*)info);

  return(_ax->sendFrame(&frameUI));
}
