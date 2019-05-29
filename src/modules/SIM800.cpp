#include "SIM800.h"

SIM800::SIM800(Module* module) {
  _mod = module;
}

int16_t SIM800::begin(long speed, const char* pin) {
  // set module properties
  _mod->AtLineFeed = "\r\n";
  _mod->baudrate = speed;
  _mod->init(USE_UART, INT_0);
  
  // empty UART buffer (garbage data)
  _mod->ATemptyBuffer();
  
  // power on
  pinMode(_mod->getInt0(), OUTPUT);
  digitalWrite(_mod->getInt0(), LOW);
  delay(1000);
  pinMode(_mod->getInt0(), INPUT);
  
  // test AT setup
  if(!_mod->ATsendCommand("AT")) {
    return(ERR_AT_FAILED);
  }
  
  // set phone functionality
  if(!_mod->ATsendCommand("AT+CFUN=1")) {
    return(ERR_AT_FAILED);
  }
  
  // set SMS message format
  if(!_mod->ATsendCommand("AT+CMFG=1")) {
    return(ERR_AT_FAILED);
  }
  
  // set PIN code
  char cmd[14];
  strcat(cmd, "AT+CPIN=\"");
  strcat(cmd, pin);
  strcat(cmd, "\"");
  if(!_mod->ATsendCommand(cmd)) {
    return(ERR_AT_FAILED);
  }
  
  return(ERR_NONE);
}

void SIM800::shutdown() {
  // power off
  pinMode(_mod->getInt0(), OUTPUT);
  digitalWrite(_mod->getInt0(), LOW);
  delay(1500);
  pinMode(_mod->getInt0(), INPUT);
}

int16_t SIM800::sendSMS(const char* num, const char* msg) {

}
