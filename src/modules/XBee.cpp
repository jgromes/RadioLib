#include "XBee.h"

XBee::XBee(Module* mod) {
  _mod = mod;
  _frameID = 0x01;
  _frameLength = 0;
  _frameHeaderProcessed = false;
  _packetData = new char[0];
}

int16_t XBee::begin(long speed) {
  // set module properties
  _mod->baudrate = speed;
  _mod->init(USE_UART, INT_1);
  
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
      DEBUG_PRINT_STR("XBee not found! (");
      DEBUG_PRINT(i + 1);
      DEBUG_PRINT_STR(" of 10 tries) STATE == ");
      DEBUG_PRINTLN(state);
      DEBUG_PRINTLN_STR("Resetting ...");
      reset();
      delay(1000);
      _mod->ATemptyBuffer();
      i++;
    }
  }
  
  if(!flagFound) {
    DEBUG_PRINTLN_STR("No XBee found!");
    return(ERR_CMD_MODE_FAILED);
  } else {
    DEBUG_PRINTLN_STR("Found XBee!");
  }
  
  return(ERR_NONE);
}

void XBee::reset() {
  pinMode(_mod->getInt1(), OUTPUT);
  digitalWrite(_mod->getInt1(), LOW);
  delayMicroseconds(200);
  digitalWrite(_mod->getInt1(), HIGH);
  pinMode(_mod->getInt1(), INPUT);
}

int16_t XBee::transmit(uint8_t* dest, const char* payload, uint8_t radius) {
  uint8_t destNetwork[] = {0xFF, 0xFE};
  return(transmit(dest, destNetwork, payload, radius));
}

int16_t XBee::transmit(uint8_t* dest, uint8_t* destNetwork, const char* payload, uint8_t radius) {
  // build the frame
  size_t payloadLen = strlen(payload);
  size_t dataLen = 8 + 2 + 1 + 1 + payloadLen;
  uint8_t* cmd = new uint8_t[dataLen];
  memcpy(cmd, dest, 8);
  memcpy(cmd + 8, destNetwork, 2);
  cmd[10] = radius;
  cmd[11] = 0x01;   // options: no retries
  memcpy(cmd + 12, payload, payloadLen);
  
  // send frame
  uint8_t frameID = _frameID++;
  sendApiFrame(XBEE_API_FRAME_ZIGBEE_TRANSMIT_REQUEST, frameID, cmd, dataLen);
  delete[] cmd;
  
  // get response code
  return(readApiFrame(frameID, 5));
}

size_t XBee::available() {
  // check if there are data available in the buffer
  size_t serialBytes = _mod->ModuleSerial->available();
  if(serialBytes < 3) {
    return(0);
  } 
  
  uint8_t header[3];
  if(!_frameHeaderProcessed) {
    // read frame header
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
  
  uint8_t* frame = new uint8_t[_frameLength];  //24
  for(size_t i = 0; i < _frameLength; i++) {
    frame[i] = _mod->ModuleSerial->read();
  }
  
  // save packet source and data
  size_t payloadLength = _frameLength - 12;
  delete[] _packetData;
  _packetData = new char[payloadLength];
  memcpy(_packetData, frame + 12, payloadLength - 1);
  _packetData[payloadLength - 1] = '\0';
  memcpy(_packetSource, frame + 1, 8);
  
  delete[] frame;
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
  if(state != ERR_NONE) {
    return(state);
  }
  
  // confirm changes
  return(confirmChanges());
}

XBeeSerial::XBeeSerial(Module* mod) : ISerial(mod) {
  
}

int16_t XBeeSerial::begin(long speed) {
  // set module properties
  _mod->AtLineFeed = "\r";
  _mod->baudrate = speed;
  _mod->init(USE_UART, INT_NONE);
  
  // empty UART buffer (garbage data)
  _mod->ATemptyBuffer();
  
  // enter command mode
  DEBUG_PRINTLN_STR("Entering command mode ...");
  if(!enterCmdMode()) {
    return(ERR_CMD_MODE_FAILED);
  }
  
  // test AT setup
  DEBUG_PRINTLN_STR("Sending test command ...");
  if(!_mod->ATsendCommand("AT")) {
    return(ERR_AT_FAILED);
  }
  
  // exit command mode
  DEBUG_PRINTLN_STR("Exiting command mode ...");
  if(!_mod->ATsendCommand("ATCN")) {
    return(ERR_AT_FAILED);
  }

  return(ERR_NONE);
}

void XBeeSerial::reset() {
  pinMode(_mod->getInt1(), OUTPUT);
  digitalWrite(_mod->getInt1(), LOW);
  delayMicroseconds(200);
  digitalWrite(_mod->getInt1(), HIGH);
  pinMode(_mod->getInt1(), INPUT);
}

int16_t XBeeSerial::setDestinationAddress(const char* destinationAddressHigh, const char* destinationAddressLow) {
  // enter command mode
  DEBUG_PRINTLN_STR("Entering command mode ...");
  if(!enterCmdMode()) {
    return(ERR_CMD_MODE_FAILED);
  }
  
  // set higher address bytes
  DEBUG_PRINTLN_STR("Setting address (high) ...");
  char* addressHigh = new char[strlen(destinationAddressHigh) + 4];
  strcpy(addressHigh, "ATDH");
  strcat(addressHigh, destinationAddressHigh);
  bool res = _mod->ATsendCommand(addressHigh);
  delete[] addressHigh;
  if(!res) {
    return(ERR_AT_FAILED);
  }
  
  // set lower address bytes
  DEBUG_PRINTLN_STR("Setting address (low) ...");
  char* addressLow = new char[strlen(destinationAddressLow) + 4];
  strcpy(addressLow, "ATDL");
  strcat(addressLow, destinationAddressLow);
  res = _mod->ATsendCommand(addressLow);
  delete[] addressLow;
  if(!res) {
    return(ERR_AT_FAILED);
  }
  
  // exit command mode
  DEBUG_PRINTLN_STR("Exiting command mode ...");
  if(!_mod->ATsendCommand("ATCN")) {
    return(ERR_AT_FAILED);
  }
  
  return(ERR_NONE);
}

int16_t XBeeSerial::setPanId(const char* panId) {
  // enter command mode
  DEBUG_PRINTLN_STR("Entering command mode ...");
  if(!enterCmdMode()) {
    return(ERR_CMD_MODE_FAILED);
  }
  
  // set PAN ID
  DEBUG_PRINTLN_STR("Setting PAN ID ...");
  char* cmd = new char[strlen(panId) + 4];
  strcpy(cmd, "ATID");
  strcat(cmd, panId);
  bool res = _mod->ATsendCommand(cmd);
  delete[] cmd;
  if(!res) {
    return(ERR_AT_FAILED);
  }
  
  // exit command mode
  DEBUG_PRINTLN_STR("Exiting command mode ...");
  if(!_mod->ATsendCommand("ATCN")) {
    return(ERR_AT_FAILED);
  }
  
  return(ERR_NONE);
}

bool XBeeSerial::enterCmdMode() {
  for(uint8_t i = 0; i < 10; i++) {
    delay(1000);
    
    _mod->ModuleSerial->write('+');
    _mod->ModuleSerial->write('+');
    _mod->ModuleSerial->write('+');
    
    delay(1000);
    
    if(_mod->ATgetResponse()) {
      return(true);
    } else {
      DEBUG_PRINT_STR("Unable to enter command mode! (");
      DEBUG_PRINT(i + 1);
      DEBUG_PRINTLN_STR(" of 10 tries)");
      
      reset();
      
      _mod->ATsendCommand("ATCN");
    }
  }
  
  DEBUG_PRINTLN_STR("Terminated, check your wiring. Is AT FW uploaded?");
  return(false);
}

int16_t XBee::confirmChanges() {
  // save changes to non-volatile memory
  uint8_t frameID = _frameID++;
  sendApiFrame(XBEE_API_FRAME_AT_COMMAND_QUEUE, frameID, "WR");
  
  // get response code
  int16_t state = readApiFrame(frameID, 4);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // apply changes
  frameID = _frameID++;
  sendApiFrame(XBEE_API_FRAME_AT_COMMAND_QUEUE, frameID, "AC");
  
  // get response code
  state = readApiFrame(frameID, 4);
  if(state != ERR_NONE) {
    return(state);
  }
  
  return(state);
}

void XBee::sendApiFrame(uint8_t type, uint8_t id, const char* data) {
  sendApiFrame(type, id, (uint8_t*)data, strlen(data));
}

void XBee::sendApiFrame(uint8_t type, uint8_t id, uint8_t* data, uint16_t length) {
  // build the API frame
  size_t frameLength = 1 + 2 + length + 1 + 2;
  uint8_t* frame = new uint8_t[frameLength];
  
  frame[0] = 0x7E;                          // start delimiter
  frame[1] = ((length + 2) & 0xFF00) >> 8;  // length MSB
  frame[2] = (length + 2) & 0x00FF;         // length LSB
  frame[3] = type;                          // frame type
  frame[4] = id;                            // frame ID
  memcpy(frame + 5, data, length);          // data
  
  // calculate the checksum
  uint8_t checksum = 0;
  for(uint16_t i = 3; i < frameLength - 1; i++) {
    checksum += frame[i];
  }
  frame[5 + length] = 0xFF - checksum;
  
  // send the frame
  for(uint16_t i = 0; i < frameLength; i++) {
    _mod->ModuleSerial->write(frame[i]);
  }
  
  // deallocate memory
  delete[] frame;
}

int16_t XBee::readApiFrame(uint8_t frameID, uint8_t codePos, uint16_t timeout) {
  // TODO: modemStatus frames may be sent at any time, interfering with frame parsing. Add check to make sure this does not happen.
  
  // get number of bytes in response (must be enough to read the length field
  uint16_t numBytes = getNumBytes(timeout/2, 3);
  if(numBytes == 0) {
    return(ERR_FRAME_NO_RESPONSE);
  }
  
  // checksum byte is not included in length field
  numBytes++;
  
  // wait until all response bytes are available (5s timeout)
  uint32_t start = millis();
  while(_mod->ModuleSerial->available() < (int16_t)numBytes) {
    if(millis() - start >= timeout/2) {
      return(ERR_FRAME_MALFORMED);
    }
  }
  DEBUG_PRINT_STR("frame data field length: ");
  DEBUG_PRINTLN(numBytes);
  
  // read the response
  uint8_t* resp = new uint8_t[numBytes];
  for(uint16_t i = 0; i < numBytes; i++) {
    resp[i] = _mod->ModuleSerial->read();
  }
  
  // verify checksum 
  uint8_t checksum = 0;
  for(uint16_t i = 0; i < numBytes; i++) {
    DEBUG_PRINT_HEX(resp[i]);
    DEBUG_PRINT_STR("\t");
    checksum += resp[i];
  }
  DEBUG_PRINTLN();
  if(checksum != 0xFF) {
    DEBUG_PRINTLN_HEX(checksum);
    return(ERR_FRAME_INCORRECT_CHECKSUM);
  }
  
  // check frame ID
  if(resp[1] != frameID) {
    DEBUG_PRINT_STR("received frame ID: ");
    DEBUG_PRINTLN(resp[1]);
    DEBUG_PRINT_STR("expected frame ID: ");
    DEBUG_PRINTLN(frameID);
    return(ERR_FRAME_UNEXPECTED_ID);
  }
  
  // codePos does not include start delimiter and frame ID
  uint8_t code = resp[codePos];
  delete[] resp;
  return(code);
}

uint16_t XBee::getNumBytes(uint32_t timeout, size_t minBytes) {
  // wait for available data
  uint32_t start = millis();
  while((size_t)_mod->ModuleSerial->available() < minBytes) {
    if(millis() - start >= timeout) {
      return(0);
    }
  }
  
  // read response
  uint8_t resp[3];
  uint8_t i = 0;
  DEBUG_PRINT_STR("reading frame length: ");
  while(_mod->ModuleSerial->available() > 0) {
    uint8_t b = _mod->ModuleSerial->read();
    DEBUG_PRINT_HEX(b);
    DEBUG_PRINT_STR("\t");
    resp[i++] = b;
    if(i == 3) {
      break;
    }
  }
  DEBUG_PRINTLN();
  
  return((resp[1] << 8) | resp[2]);
}
