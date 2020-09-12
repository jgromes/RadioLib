#include "XBee.h"
#if !defined(RADIOLIB_EXCLUDE_XBEE)

XBee::XBee(Module* mod) {
  _mod = mod;
  _packetData[0] = '\0';
}

int16_t XBee::begin(long speed) {
  // set module properties
  _mod->baudrate = speed;
  _mod->init(RADIOLIB_USE_UART);

  // reset module
  reset();

  // empty UART buffer (garbage data)
  _mod->ATemptyBuffer();

  // try to find the XBee
  bool flagFound = false;
  uint8_t i = 0;
  while((i < 10) && !flagFound) {
    // hardware reset should return 2 modem status frames - 1st status 0x00, second status 0x06
    int16_t state = readApiFrame(0x00, 1, 2000);
    readApiFrame(0x00, 1, 2000);

    if(state == ERR_NONE) {
      flagFound = true;
    } else {
      RADIOLIB_DEBUG_PRINT(F("XBee not found! ("));
      RADIOLIB_DEBUG_PRINT(i + 1);
      RADIOLIB_DEBUG_PRINT(F(" of 10 tries) STATE == "));
      RADIOLIB_DEBUG_PRINTLN(state);
      RADIOLIB_DEBUG_PRINTLN(F("Resetting ..."));
      reset();
      Module::delay(10);
      _mod->ATemptyBuffer();
      i++;
    }
  }

  if(!flagFound) {
    RADIOLIB_DEBUG_PRINTLN(F("No XBee found!"));
    return(ERR_CMD_MODE_FAILED);
  } else {
    RADIOLIB_DEBUG_PRINTLN(F("Found XBee!"));
  }

  return(ERR_NONE);
}

void XBee::reset() {
  Module::pinMode(_mod->getRst(), OUTPUT);
  Module::digitalWrite(_mod->getRst(), LOW);
  Module::delay(1);
  Module::digitalWrite(_mod->getRst(), HIGH);
}

int16_t XBee::transmit(uint8_t* dest, const char* payload, uint8_t radius) {
  uint8_t destNetwork[] = {0xFF, 0xFE};
  return(transmit(dest, destNetwork, payload, radius));
}

int16_t XBee::transmit(uint8_t* dest, uint8_t* destNetwork, const char* payload, uint8_t radius) {
  // build the frame
  size_t payloadLen = strlen(payload);
  size_t dataLen = 8 + 2 + 1 + 1 + payloadLen;
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t cmd[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* cmd = new uint8_t[dataLen];
  #endif
  memcpy(cmd, dest, 8);
  memcpy(cmd + 8, destNetwork, 2);
  cmd[10] = radius;
  cmd[11] = 0x01;   // options: no retries
  memcpy(cmd + 12, payload, payloadLen);

  // send frame
  uint8_t frameID = _frameID++;
  sendApiFrame(XBEE_API_FRAME_ZIGBEE_TRANSMIT_REQUEST, frameID, cmd, dataLen);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] cmd;
  #endif

  // get response code
  return(readApiFrame(frameID, 5));
}

size_t XBee::available() {
  // check if there are data available in the buffer
  size_t serialBytes = _mod->ModuleSerial->available();
  if(serialBytes < 3) {
    return(0);
  }

  if(!_frameHeaderProcessed) {
    // read frame header
    uint8_t header[3];
    for(uint8_t i = 0; i < 3; i++) {
      header[i] = _mod->ModuleSerial->read();
    }

    // check if we received API frame
    if(header[0] != XBEE_API_START) {
      return(0);
    }

    // get expected frame length
    _frameLength = ((header[1] << 8) | header[2]) + 1;
    _frameHeaderProcessed = true;
  }

  // check if the header is complete
  if(serialBytes < _frameLength) {
    return(0);
  }

  #ifdef RADIOLIB_STATIC_ONLY
    char frame[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* frame = new uint8_t[_frameLength];
  #endif
  for(size_t i = 0; i < _frameLength; i++) {
    frame[i] = _mod->ModuleSerial->read();
  }

  // save packet source and data
  size_t payloadLength = _frameLength - 12;
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] _packetData;
    _packetData = new char[payloadLength];
  #endif
  memcpy(_packetData, frame + 12, payloadLength - 1);
  _packetData[payloadLength - 1] = '\0';
  memcpy(_packetSource, frame + 1, 8);

  #ifndef RADIOLIB_STATIC_ONLY
    delete[] frame;
  #endif
  _frameLength = 0;
  _frameHeaderProcessed = false;

  // return number of bytes in payload
  return(payloadLength);
}

String XBee::getPacketSource() {
  char buff[17];
  sprintf(buff, "%02X%02X%02X%02X%02X%02X%02X%02X", _packetSource[0], _packetSource[1], _packetSource[2], _packetSource[3],
                                                    _packetSource[4], _packetSource[5], _packetSource[6], _packetSource[7]);
  String str(buff);
  return(str);
}

String XBee::getPacketData() {
  String str(_packetData);
  return(str);
}

int16_t XBee::setPanId(uint8_t* panId) {
  // build AT command
  uint8_t cmd[10];
  memcpy(cmd, "ID", 2);
  memcpy(cmd + 2, panId, 8);

  // send frame
  uint8_t frameID = _frameID++;
  sendApiFrame(XBEE_API_FRAME_AT_COMMAND_QUEUE, frameID, cmd, 10);

  // get response code
  int16_t state = readApiFrame(frameID, 4);
  RADIOLIB_ASSERT(state);

  // confirm changes
  return(confirmChanges());
}

XBeeSerial::XBeeSerial(Module* mod) : ISerial(mod) {

}

int16_t XBeeSerial::begin(long speed) {
  // set module properties
  char lf[3] = "\r";
  memcpy(_mod->AtLineFeed, lf, strlen(lf));
  _mod->baudrate = speed;
  _mod->init(RADIOLIB_USE_UART);

  // reset module
  reset();

  // empty UART buffer (garbage data)
  _mod->ATemptyBuffer();

  // enter command mode
  RADIOLIB_DEBUG_PRINTLN(F("Entering command mode ..."));
  if(!enterCmdMode()) {
    return(ERR_CMD_MODE_FAILED);
  }

  // test AT setup
  RADIOLIB_DEBUG_PRINTLN(F("Sending test command ..."));
  if(!_mod->ATsendCommand("AT")) {
    return(ERR_AT_FAILED);
  }

  // exit command mode
  RADIOLIB_DEBUG_PRINTLN(F("Exiting command mode ..."));
  if(!_mod->ATsendCommand("ATCN")) {
    return(ERR_AT_FAILED);
  }

  return(ERR_NONE);
}

void XBeeSerial::reset() {
  Module::pinMode(_mod->getRst(), OUTPUT);
  Module::digitalWrite(_mod->getRst(), LOW);
  Module::delay(1);
  Module::digitalWrite(_mod->getRst(), HIGH);
  Module::pinMode(_mod->getRst(), INPUT);
}

int16_t XBeeSerial::setDestinationAddress(const char* destinationAddressHigh, const char* destinationAddressLow) {
  // enter command mode
  RADIOLIB_DEBUG_PRINTLN(F("Entering command mode ..."));
  if(!enterCmdMode()) {
    return(ERR_CMD_MODE_FAILED);
  }

  // set higher address bytes
  RADIOLIB_DEBUG_PRINTLN(F("Setting address (high) ..."));
  #ifdef RADIOLIB_STATIC_ONLY
    char addressHigh[13];
  #else
    char* addressHigh = new char[strlen(destinationAddressHigh) + 4];
  #endif
  strcpy(addressHigh, "ATDH");
  strcat(addressHigh, destinationAddressHigh);
  bool res = _mod->ATsendCommand(addressHigh);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] addressHigh;
  #endif
  if(!res) {
    return(ERR_AT_FAILED);
  }

  // set lower address bytes
  RADIOLIB_DEBUG_PRINTLN(F("Setting address (low) ..."));
  #ifdef RADIOLIB_STATIC_ONLY
    char addressLow[13];
  #else
    char* addressLow = new char[strlen(destinationAddressLow) + 4];
  #endif
  strcpy(addressLow, "ATDL");
  strcat(addressLow, destinationAddressLow);
  res = _mod->ATsendCommand(addressLow);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] addressLow;
  #endif
  if(!res) {
    return(ERR_AT_FAILED);
  }

  // exit command mode
  RADIOLIB_DEBUG_PRINTLN(F("Exiting command mode ..."));
  if(!_mod->ATsendCommand("ATCN")) {
    return(ERR_AT_FAILED);
  }

  return(ERR_NONE);
}

int16_t XBeeSerial::setPanId(const char* panId) {
  // enter command mode
  RADIOLIB_DEBUG_PRINTLN(F("Entering command mode ..."));
  if(!enterCmdMode()) {
    return(ERR_CMD_MODE_FAILED);
  }

  // set PAN ID
  RADIOLIB_DEBUG_PRINTLN(F("Setting PAN ID ..."));
  #ifdef RADIOLIB_STATIC_ONLY
    char cmd[21];
  #else
    char* cmd = new char[strlen(panId) + 4];
  #endif
  strcpy(cmd, "ATID");
  strcat(cmd, panId);
  bool res = _mod->ATsendCommand(cmd);
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] cmd;
  #endif
  if(!res) {
    return(ERR_AT_FAILED);
  }

  // exit command mode
  RADIOLIB_DEBUG_PRINTLN(F("Exiting command mode ..."));
  if(!_mod->ATsendCommand("ATCN")) {
    return(ERR_AT_FAILED);
  }

  return(ERR_NONE);
}

bool XBeeSerial::enterCmdMode() {
  for(uint8_t i = 0; i < 10; i++) {
    Module::delay(1000);

    _mod->ModuleSerial->write('+');
    _mod->ModuleSerial->write('+');
    _mod->ModuleSerial->write('+');

    Module::delay(1000);

    if(_mod->ATgetResponse()) {
      return(true);
    } else {
      RADIOLIB_DEBUG_PRINT(F("Unable to enter command mode! ("));
      RADIOLIB_DEBUG_PRINT(i + 1);
      RADIOLIB_DEBUG_PRINTLN(F(" of 10 tries)"));

      reset();

      _mod->ATsendCommand("ATCN");
    }
  }

  RADIOLIB_DEBUG_PRINTLN(F("Terminated, check your wiring. Is AT FW uploaded?"));
  return(false);
}

int16_t XBee::confirmChanges() {
  // save changes to non-volatile memory
  uint8_t frameID = _frameID++;
  sendApiFrame(XBEE_API_FRAME_AT_COMMAND_QUEUE, frameID, "WR");

  // get response code
  int16_t state = readApiFrame(frameID, 4);
  RADIOLIB_ASSERT(state);

  // apply changes
  frameID = _frameID++;
  sendApiFrame(XBEE_API_FRAME_AT_COMMAND_QUEUE, frameID, "AC");

  // get response code
  state = readApiFrame(frameID, 4);

  return(state);
}

void XBee::sendApiFrame(uint8_t type, uint8_t id, const char* data) {
  sendApiFrame(type, id, (uint8_t*)data, strlen(data));
}

void XBee::sendApiFrame(uint8_t type, uint8_t id, uint8_t* data, uint16_t length) {
  // build the API frame
  size_t frameLength = 1 + 2 + length + 1 + 2;
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t frame[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* frame = new uint8_t[frameLength];
  #endif

  frame[0] = 0x7E;                          // start delimiter
  frame[1] = ((length + 2) & 0xFF00) >> 8;  // length MSB
  frame[2] = (length + 2) & 0x00FF;         // length LSB
  frame[3] = type;                          // frame type
  frame[4] = id;                            // frame ID
  memcpy(frame + 5, data, length);          // data

  // calculate the checksum
  uint8_t checksum = 0;
  for(size_t i = 3; i < frameLength - 1; i++) {
    checksum += frame[i];
  }
  frame[5 + length] = 0xFF - checksum;

  // send the frame
  for(size_t i = 0; i < frameLength; i++) {
    _mod->ModuleSerial->write(frame[i]);
  }

  // deallocate memory
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] frame;
  #endif
}

int16_t XBee::readApiFrame(uint8_t frameID, uint8_t codePos, uint16_t timeout) {
  /// \todo modemStatus frames may be sent at any time, interfering with frame parsing. Add check to make sure this does not happen.

  // get number of bytes in response (must be enough to read the length field
  uint16_t numBytes = getNumBytes(timeout/2, 3);
  if(numBytes == 0) {
    return(ERR_FRAME_NO_RESPONSE);
  }

  // checksum byte is not included in length field
  numBytes++;

  // wait until all response bytes are available (5s timeout)
  uint32_t start = Module::millis();
  while(_mod->ModuleSerial->available() < (int16_t)numBytes) {
    Module::yield();
    if(Module::millis() - start >= timeout/2) {
      return(ERR_FRAME_MALFORMED);
    }
  }
  RADIOLIB_DEBUG_PRINT(F("frame data field length: "));
  RADIOLIB_DEBUG_PRINTLN(numBytes);

  // read the response
  #ifdef RADIOLIB_STATIC_ONLY
    uint8_t resp[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* resp = new uint8_t[numBytes];
  #endif
  for(uint16_t i = 0; i < numBytes; i++) {
    resp[i] = _mod->ModuleSerial->read();
  }

  // verify checksum
  uint8_t checksum = 0;
  for(uint16_t i = 0; i < numBytes; i++) {
    RADIOLIB_DEBUG_PRINT(resp[i], HEX);
    RADIOLIB_DEBUG_PRINT('\t');
    checksum += resp[i];
  }
  RADIOLIB_DEBUG_PRINTLN();
  if(checksum != 0xFF) {
    RADIOLIB_DEBUG_PRINTLN(checksum, HEX);
    return(ERR_FRAME_INCORRECT_CHECKSUM);
  }

  // check frame ID
  if(resp[1] != frameID) {
    RADIOLIB_DEBUG_PRINT(F("received frame ID: "));
    RADIOLIB_DEBUG_PRINTLN(resp[1]);
    RADIOLIB_DEBUG_PRINT(F("expected frame ID: "));
    RADIOLIB_DEBUG_PRINTLN(frameID);
    return(ERR_FRAME_UNEXPECTED_ID);
  }

  // codePos does not include start delimiter and frame ID
  uint8_t code = resp[codePos];
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] resp;
  #endif
  return(code);
}

uint16_t XBee::getNumBytes(uint32_t timeout, size_t minBytes) {
  // wait for available data
  uint32_t start = Module::millis();
  while((size_t)_mod->ModuleSerial->available() < minBytes) {
    Module::yield();
    if(Module::millis() - start >= timeout) {
      return(0);
    }
  }

  // read response
  uint8_t resp[3];
  uint8_t i = 0;
  RADIOLIB_DEBUG_PRINT(F("reading frame length: "));
  while(_mod->ModuleSerial->available() > 0) {
    Module::yield();
    uint8_t b = _mod->ModuleSerial->read();
    RADIOLIB_DEBUG_PRINT(b, HEX);
    RADIOLIB_DEBUG_PRINT('\t');
    resp[i++] = b;
    if(i == 3) {
      break;
    }
  }
  RADIOLIB_DEBUG_PRINTLN();

  return((resp[1] << 8) | resp[2]);
}

#endif
