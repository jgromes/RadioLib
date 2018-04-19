#include "XBee.h"

XBeeApiFrame::XBeeApiFrame(uint8_t apiId, uint8_t frameId) {
  _apiId = apiId;
  _frameId = frameId;
}

XBee::XBee(Module* module) {
  _mod = module;
}

uint8_t XBee::begin(long speed) {
  // set Arduino pins
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(3, OUTPUT);
  digitalWrite(A4, LOW);
  digitalWrite(A5, LOW);
  digitalWrite(3, HIGH);
  
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
  
  delay(1000);

  return(ERR_NONE);
}

uint8_t XBee::setDestinationAddress(const char destinationAddressHigh[], const char destinationAddressLow[]) {
  // enter command mode
  DEBUG_PRINTLN_STR("Entering command mode ...");
  if(!enterCmdMode()) {
    return(ERR_CMD_MODE_FAILED);
  }
  
  // set higher address bytes
  DEBUG_PRINTLN_STR("Setting address (high) ...");
  String addressHigh = "ATDH";
  addressHigh += destinationAddressHigh;
  if(!_mod->ATsendCommand(addressHigh)) {
    return(ERR_AT_FAILED);
  }
  
  // set lower address bytes
  DEBUG_PRINTLN_STR("Setting address (low) ...");
  String addressLow = "ATDL";
  addressLow += destinationAddressLow;
  if(!_mod->ATsendCommand(addressLow)) {
    return(ERR_AT_FAILED);
  }
  
  // exit command mode
  DEBUG_PRINTLN_STR("Exiting command mode ...");
  if(!_mod->ATsendCommand("ATCN")) {
    return(ERR_AT_FAILED);
  }
  
  return(ERR_NONE);
}

uint8_t XBee::setPanId(const char panId[]) {
  // enter command mode
  DEBUG_PRINTLN_STR("Entering command mode ...");
  if(!enterCmdMode()) {
    return(ERR_CMD_MODE_FAILED);
  }
  
  // set PAN ID
  DEBUG_PRINTLN_STR("Setting PAN ID ...");
  String panIdCmd = "ATID";
  panIdCmd += panId;
  if(!_mod->ATsendCommand(panIdCmd)) {
    return(ERR_AT_FAILED);
  }
  
  // exit command mode
  DEBUG_PRINTLN_STR("Exiting command mode ...");
  if(!_mod->ATsendCommand("ATCN")) {
    return(ERR_AT_FAILED);
  }
  
  return(ERR_NONE);
}

bool XBee::enterCmdMode() {
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

/*uint8_t XBee::transmit(uint32_t destinationAddressHigh, uint32_t destinationAddressLow, const char* data, uint8_t length) {
  //build the API frame
  uint8_t frameLength = length + 12;
  uint8_t * frameData = new uint8_t[frameLength];
  
  //set the destination address
  frameData[0] = (destinationAddressHigh >> 24) & 0xFF;
  frameData[1] = (destinationAddressHigh >> 16) & 0xFF;
  frameData[2] = (destinationAddressHigh >> 8) & 0xFF;
  frameData[3] = destinationAddressHigh & 0xFF;
  
  frameData[4] = (destinationAddressLow >> 24) & 0xFF;
  frameData[5] = (destinationAddressLow >> 16) & 0xFF;
  frameData[6] = (destinationAddressLow >> 8) & 0xFF;
  frameData[7] = destinationAddressLow & 0xFF;
  
  //set the destination network address
  frameData[8] = 0xFF;
  frameData[9] = 0xFE;
  
  //set broadcast radius (number of allowed hops, 0 - maximum)
  frameData[10] = 0x00;
  
  //set the options
  frameData[11] = 0x00;
  
  //copy payload data
  for(uint8_t i = 0; i < length; i++) {
    frameData[12 + i] = (uint8_t)data[i];
  }
  
  //send the frame to XBee
  sendApiFrame(XBEE_API_FRAME_ZIGBEE_TRANSMIT_REQUEST, frameData, frameLength);
  
  //deallocate memory
  delete frameData;
  
  //wait for status frame
  readApiFrame(1000);
}*/

/*void XBee::sendApiFrame(uint8_t apiId, uint8_t* data, uint16_t length) {
  //send frame start delimiter
  _mod->ModuleSerial->write(XBEE_API_START);
  
  //send frame length (API ID, frame ID and data length)
  write(((length + 2) >> 8) & 0xFF);
  write((length + 2) & 0xFF);
  
  //send API ID
  write(apiId);
  
  //send default frame ID (value 0x00 would disable some feedback)
  write(XBEE_API_DEFAULT_FRAME_ID);
  
  //checksum is calculated from API ID, frame ID and data
  uint8_t checksum = apiId;
  checksum += XBEE_API_DEFAULT_FRAME_ID;
  
  //send the data and calculate checksum
  for(uint16_t i = 0; i < length; i++) {
    write(data[i]);
    checksum += data[i];
  }
  
  //send the checksum
  checksum = 0xFF - checksum;
  write(checksum);
}*/

/*uint8_t XBee::readApiFrame(uint16_t timeout) {
  //start the timer
  unsigned long start = millis();
  
  Serial.println("reading");
  
  
  //array to store frame length, type and ID
  uint8_t header[4];
  while(millis() - start < timeout) {
    Serial.println(_mod->ModuleSerial->available());
    //check buffer for new data
    while(_mod->ModuleSerial->available()) {
      uint8_t b = _mod->ModuleSerial->read();
      
      Serial.write(b);
      Serial.print('\t');
      Serial.println(b, HEX);
      
      if(b == XBEE_API_START) {
        //received the start character
        n = 0;
      } else {
        n++;
      }
      
      //check escaped characters
      if(b == XBEE_API_ESCAPE) {
        //wait for the next byte
        while(!_mod->ModuleSerial->available());
        
        //resolve the escaped character
        b =_mod->ModuleSerial->read();
        b = 0x20 ^ b;
      }
    }
  }
}*/

/*void XBee::write(uint8_t b) {
  if((b == XBEE_API_START) || (b == XBEE_API_ESCAPE) || (b == XBEE_API_XON) || (b == XBEE_API_XOFF)) {
    _mod->ModuleSerial->write(XBEE_API_ESCAPE);
    _mod->ModuleSerial->write(b ^ 0x20);
  } else {
    _mod->ModuleSerial->write(b);
  }
}*/
