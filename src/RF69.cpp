#include "RF69.h"

RF69::RF69(Module* module) {
  _mod = module;
}

uint8_t RF69::begin() {
  //_mod->init(USE_SPI, INT_BOTH);
  _mod->init(USE_SPI, INT_0);
  
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    uint8_t version = _mod->SPIreadRegister(RF69_REG_VERSION);
    if(version == 0x24) {
      flagFound = true;
    } else {
      #ifdef DEBUG
        Serial.print("RF69 not found! (");
        Serial.print(i + 1);
        Serial.print(" of 10 tries) RF69_REG_VERSION == ");
        
        char buffHex[5];
        sprintf(buffHex, "0x%02X", version);
        Serial.print(buffHex);
        Serial.println();
      #endif
      delay(1000);
      i++;
    }
  }
  
  if(!flagFound) {
    #ifdef DEBUG
      Serial.println("No RF69 found!");
    #endif
    SPI.end();
    return(ERR_CHIP_NOT_FOUND);
  }
  #ifdef DEBUG
    else {
      Serial.println("Found RF69! (match by RF69_REG_VERSION == 0x12)");
    }
  #endif
  
  return(config());
}

uint8_t RF69::transmit(Packet& pack) {
  char buffer[256];
  
  for(uint8_t i = 0; i < 8; i++) {
    buffer[i] = pack.source[i];
    buffer[i+8] = pack.destination[i];
  }
  
  for(uint8_t i = 0; i < pack.length; i++) {
    buffer[i+16] = pack.data[i];
  }
  
  //_mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_2, RF69_RESTART_RX, 2, 2);
  
  setMode(RF69_STANDBY);
  
  _mod->SPIsetRegValue(RF69_REG_DIO_MAPPING_1, RF69_DIO0_PACK_PACKET_SENT, 7, 6);
  clearIRQFlags();
  
  if(pack.length > 256) {
    return(ERR_PACKET_TOO_LONG);
  }

  _mod->SPIwriteRegister(RF69_REG_FIFO, pack.length);
  _mod->SPIwriteRegisterBurstStr(RF69_REG_FIFO, buffer, pack.length);
  
  setMode(RF69_TX);
  _mod->SPIsetRegValue(RF69_REG_TEST_PA1, RF69_PA1_20_DBM);
  _mod->SPIsetRegValue(RF69_REG_TEST_PA2, RF69_PA2_20_DBM);
  
  while(!_mod->getInt0State()) {
    #ifdef DEBUG
      Serial.print('.');
    #endif
  }
  
  clearIRQFlags();
  
  return(ERR_NONE);
}

uint8_t RF69::receive(Packet& pack) {
  char buffer[256];
  
  setMode(RF69_STANDBY);
  
  //_mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_2, RF69_RESTART_RX, 2, 2);
  
  //_mod->SPIsetRegValue(RF69_REG_DIO_MAPPING_1, RF69_DIO0_PACK_PAYLOAD_READY | RF69_DIO1_PACK_TIMEOUT, 7, 4);
  _mod->SPIsetRegValue(RF69_REG_DIO_MAPPING_1, RF69_DIO0_PACK_PAYLOAD_READY, 7, 6);
  clearIRQFlags();
  
  setMode(RF69_RX);
  _mod->SPIsetRegValue(RF69_REG_TEST_PA1, RF69_PA1_NORMAL);
  _mod->SPIsetRegValue(RF69_REG_TEST_PA2, RF69_PA2_NORMAL);
  
  /*while(!_mod->getInt0State()) {
    if(_mod->getInt1State()) {
      clearIRQFlags();
      return(ERR_RX_TIMEOUT);
    }
  }*/
  unsigned long start = millis();
  while(!_mod->getInt0State()) {
    if(millis() - start > 2000) {
      clearIRQFlags();
      return(ERR_RX_TIMEOUT);
    }
  }
  
  pack.length = _mod->SPIreadRegister(RF69_REG_FIFO);
  
  _mod->SPIreadRegisterBurstStr(RF69_REG_FIFO, pack.length, buffer);
  
  clearIRQFlags();
  
  for(uint8_t i = 0; i < 8; i++) {
    pack.source[i] = buffer[i];
    pack.destination[i] = buffer[i+8];
  }
  
  for(uint8_t i = 16; i < pack.length; i++) {
    pack.data[i-16] = buffer[i];
  }
  pack.data[pack.length-16] = 0;
  
  return(ERR_NONE);
}

uint8_t RF69::sleep() {
  return(setMode(RF69_SLEEP));
}

uint8_t RF69::standby() {
  return(setMode(RF69_STANDBY));
}

uint8_t RF69::config() {
  uint8_t status = ERR_NONE;
  
  //set mode to STANDBY
  status = setMode(RF69_STANDBY);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set operation modes
  status = _mod->SPIsetRegValue(RF69_REG_OP_MODE, RF69_SEQUENCER_ON | RF69_LISTEN_OFF, 7, 6);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //enable over-current protection
  status = _mod->SPIsetRegValue(RF69_REG_OCP, RF69_OCP_ON, 4, 4);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set data mode and modulation type
  status = _mod->SPIsetRegValue(RF69_REG_DATA_MODUL, RF69_PACKET_MODE | RF69_FSK, 6, 3);
  status = _mod->SPIsetRegValue(RF69_REG_DATA_MODUL, RF69_NO_SHAPING, 1, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set bit rate (4.8 kbps by default)
  status = _mod->SPIsetRegValue(RF69_REG_BITRATE_MSB, RF69_BITRATE_MSB, 7, 0);
  status = _mod->SPIsetRegValue(RF69_REG_BITRATE_LSB, RF69_BITRATE_LSB, 7, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set allowed frequency deviation (5 kHz by default)
  status = _mod->SPIsetRegValue(RF69_REG_FDEV_MSB, RF69_FDEV_MSB, 5, 0);
  status = _mod->SPIsetRegValue(RF69_REG_FDEV_LSB, RF69_FDEV_LSB, 7, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set carrier frequency
  status = _mod->SPIsetRegValue(RF69_REG_FRF_MSB, RF69_FRF_MSB, 7, 0);
  status = _mod->SPIsetRegValue(RF69_REG_FRF_MID, RF69_FRF_MID, 7, 0);
  status = _mod->SPIsetRegValue(RF69_REG_FRF_LSB, RF69_FRF_LSB, 7, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set Rx bandwidth
  status = _mod->SPIsetRegValue(RF69_REG_RX_BW, RF69_DCC_FREQ | RF69_RX_BW_MANT_16 | RF69_RX_BW_EXP, 7, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set RSSI threshold
  status = _mod->SPIsetRegValue(RF69_REG_RSSI_THRESH, RF69_RSSI_THRESHOLD, 7, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //reset FIFO flags
  status = _mod->SPIsetRegValue(RF69_REG_IRQ_FLAGS_2, RF69_IRQ_FIFO_OVERRUN, 4, 4);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //disable ClkOut on DIO5
  status = _mod->SPIsetRegValue(RF69_REG_DIO_MAPPING_2, RF69_CLK_OUT_OFF, 2, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set synchronization
  status = _mod->SPIsetRegValue(RF69_REG_SYNC_CONFIG, RF69_SYNC_ON | RF69_FIFO_FILL_CONDITION_SYNC | RF69_SYNC_SIZE | RF69_SYNC_TOL, 7, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set sync word
  status = _mod->SPIsetRegValue(RF69_REG_SYNC_VALUE_1, 0x2D, 7, 0);
  status = _mod->SPIsetRegValue(RF69_REG_SYNC_VALUE_2, 100, 7, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set packet configuration and disable encryption
  status = _mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_1, RF69_PACKET_FORMAT_VARIABLE | RF69_DC_FREE_NONE | RF69_CRC_ON | RF69_CRC_AUTOCLEAR_ON | RF69_ADDRESS_FILTERING_OFF, 7, 1);
  status = _mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_2, RF69_INTER_PACKET_RX_DELAY, 7, 4);
  status = _mod->SPIsetRegValue(RF69_REG_PACKET_CONFIG_2, RF69_AUTO_RX_RESTART_ON | RF69_AES_OFF, 1, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set payload length
  status = _mod->SPIsetRegValue(RF69_REG_PAYLOAD_LENGTH, RF69_PAYLOAD_LENGTH, 7, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set FIFO threshold
  status = _mod->SPIsetRegValue(RF69_REG_FIFO_THRESH, RF69_TX_START_CONDITION_FIFO_NOT_EMPTY | RF69_FIFO_THRESHOLD, 7, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set output power
  status = _mod->SPIsetRegValue(RF69_REG_PA_LEVEL, RF69_PA0_ON | RF69_PA1_OFF | RF69_PA2_OFF | RF69_OUTPUT_POWER, 7, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //set Rx timeouts
  //status = _mod->SPIsetRegValue(RF69_REG_RX_TIMEOUT_1, RF69_TIMEOUT_RX_START, 7, 0);
  status = _mod->SPIsetRegValue(RF69_REG_RX_TIMEOUT_1, RF69_TIMEOUT_RX_START_OFF, 7, 0);
  //status = _mod->SPIsetRegValue(RF69_REG_RX_TIMEOUT_2, RF69_TIMEOUT_RSSI_THRESH, 7, 0);
  status = _mod->SPIsetRegValue(RF69_REG_RX_TIMEOUT_2, RF69_TIMEOUT_RSSI_THRESH_OFF, 7, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  //enable improved fading margin
  status = _mod->SPIsetRegValue(RF69_REG_TEST_DAGC, RF69_CONTINUOUS_DAGC_LOW_BETA_OFF, 7, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  return(ERR_NONE);
}

uint8_t RF69::setMode(uint8_t mode) {
  _mod->SPIsetRegValue(RF69_REG_OP_MODE, mode, 4, 2);
  return(ERR_NONE);
}

void RF69::clearIRQFlags() {
  _mod->SPIwriteRegister(RF69_REG_IRQ_FLAGS_1, 0b11111111);
  _mod->SPIwriteRegister(RF69_REG_IRQ_FLAGS_2, 0b11111111);
}
