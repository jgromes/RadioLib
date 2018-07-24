#include "XBee.h"

XBee::XBee(Module* mod) {
  _mod = mod;
  _frameID = 0x01;
  _frameLength = 0;
  _frameHeaderProcessed = false;
  _packetData = new char[0];
}

int16_t XBee::begin(long speed) {
  // set Arduino pins
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(3, OUTPUT);
  digitalWrite(A4, LOW);
  digitalWrite(A5, LOW);
  digitalWrite(3, HIGH);

  // set module properties
  _mod->baudrate = speed;
  _mod->init(USE_UART, INT_NONE);
  
  // wait for boot
  delay(2000);
  
  // empty UART buffer (garbage data)
  _mod->ATemptyBuffer();
  
  // send test frame (get baudrate setting)
  uint8_t frameID = _frameID++;
  sendApiFrame(XBEE_API_FRAME_AT_COMMAND, frameID, "BD");
  delay(20);
  
  // get response code
  return(readApiFrame(frameID, 4));
}

int16_t XBee::transmit(uint8_t* dest, const char* payload, uint8_t radius) {
  uint8_t destNetwork[] = {0xFF, 0xFE};
  return(transmit(dest, destNetwork, payload));
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
  delay(40);
  
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
  sendApiFrame(XBEE_API_FRAME_AT_COMMAND, frameID, cmd, 10);
  delay(40);
  
  // get response code
  return(readApiFrame(frameID, 4));
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
      
      pinMode(3, OUTPUT);
      delay(10);
      digitalWrite(3, HIGH);
      delay(500);
      digitalWrite(3, LOW);
      delay(500);
      pinMode(3, INPUT);
      delay(500);
      
      _mod->ATsendCommand("ATCN");
      
      if(i == 9) {
        DEBUG_PRINTLN_STR("Terminated, check your wiring. Is AT FW uploaded?");
        return(false);
      }
    }
  }
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

int16_t XBee::readApiFrame(uint8_t frameID, uint8_t codePos) {
  // get number of bytes in response
  uint16_t numBytes = getNumBytes(10000, 5);
  if(numBytes == 0) {
    return(ERR_FRAME_MALFORMED);
  }
  
  // checksum byte is not included in length field
  numBytes++;
  
  // read the response
  uint8_t* resp = new uint8_t[numBytes];
  for(uint16_t i = 0; i < numBytes; i++) {
    resp[i] = _mod->ModuleSerial->read();
  }
  
  // verify checksum 
  uint8_t checksum = 0;
  for(uint16_t i = 0; i < numBytes; i++) {
    checksum += resp[i];
  }
  if(checksum != 0xFF) {
    return(ERR_FRAME_INCORRECT_CHECKSUM);
  }
  
  // check frame ID
  if(resp[1] != frameID) {
    return(ERR_FRAME_UNEXPECTED_ID);
  }
  
  uint8_t code = resp[codePos];
  delete[] resp;
  return(code);
}

uint16_t XBee::getNumBytes(uint32_t timeout, size_t minBytes) {
  // wait for available data
  uint32_t start = millis();
  while(_mod->ModuleSerial->available() < minBytes) {
    if(millis() - start >= timeout) {
      return(0);
    }
  }
  
  // read response
  uint8_t resp[3];
  uint8_t i = 0;
  while(_mod->ModuleSerial->available() > 0) {
    uint8_t b = _mod->ModuleSerial->read();
    resp[i++] = b;
    if(i == 3) {
      break;
    }
  }
  
  return((resp[1] << 8) | resp[2]);
}
