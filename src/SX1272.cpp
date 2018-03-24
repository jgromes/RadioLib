#include "SX1272.h"

SX1272::SX1272(Module* module) {
  _mod = module;
}

uint8_t SX1272::begin(Bandwidth bw, SpreadingFactor sf, CodingRate cr, uint16_t addrEeprom) {
  _bw = bw;
  _sf = sf;
  _cr = cr;
  
  #ifdef ESP32
    if(!EEPROM.begin(9)) {
      #ifdef DEBUG
        Serial.println("Unable to initialize EEPROM");
      #endif
      return(ERR_EEPROM_NOT_INITIALIZED);
    }
  #endif
  
  _addrEeprom = addrEeprom;
  
  bool hasAddress = false;
  for(uint16_t i = 0; i < 8; i++) {
    if(EEPROM.read(_addrEeprom + i) != 255) {
      hasAddress = true;
      break;
    }
  }
  
  if(!hasAddress) {
    randomSeed(analogRead(5));
    generateLoRaAdress();
  }
  
  #ifdef DEBUG
    Serial.print("LoRa node address string: ");
  #endif
  for(uint8_t i = 0; i < 8; i++) {
    _address[i] = EEPROM.read(i);
    #ifdef DEBUG
      Serial.print(_address[i], HEX);
      if(i < 7) {
        Serial.print(":");
      } else {
        Serial.println();
      }
    #endif
  }
  
  _mod->init(USE_SPI, INT_BOTH);
  
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    uint8_t version = _mod->SPIreadRegister(SX1272_REG_VERSION);
    if(version == 0x22) {
      flagFound = true;
    } else {
      #ifdef DEBUG
        Serial.print("SX1272 not found! (");
        Serial.print(i + 1);
        Serial.print(" of 10 tries) SX1272_REG_VERSION == ");
        
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
      Serial.println("No SX1272 found!");
    #endif
    SPI.end();
    return(ERR_CHIP_NOT_FOUND);
  }
  #ifdef DEBUG
    else {
      Serial.println("Found SX1272! (match by SX1272_REG_VERSION == 0x12)");
    }
  #endif
  
  return(config(_bw, _sf, _cr));
}

uint8_t SX1272::transmit(Packet& pack) {
  char buffer[256];
  
  for(uint8_t i = 0; i < 8; i++) {
    buffer[i] = pack.source[i];
    buffer[i+8] = pack.destination[i];
  }
  
  for(uint8_t i = 0; i < pack.length; i++) {
    buffer[i+16] = pack.data[i];
  }
  
  setMode(SX1272_STANDBY);
  
  _mod->SPIsetRegValue(SX1272_REG_DIO_MAPPING_1, SX1272_DIO0_TX_DONE, 7, 6);
  clearIRQFlags();
  
  if(pack.length > 256) {
    return(ERR_PACKET_TOO_LONG);
  }

  _mod->SPIsetRegValue(SX1272_REG_PAYLOAD_LENGTH, pack.length);
  _mod->SPIsetRegValue(SX1272_REG_FIFO_TX_BASE_ADDR, SX1272_FIFO_TX_BASE_ADDR_MAX);
  _mod->SPIsetRegValue(SX1272_REG_FIFO_ADDR_PTR, SX1272_FIFO_TX_BASE_ADDR_MAX);
  
  _mod->SPIwriteRegisterBurstStr(SX1272_REG_FIFO, buffer, pack.length);
  
  setMode(SX1272_TX);
  
  unsigned long start = millis();
  while(!_mod->getInt0State()) {
    #ifdef DEBUG
      Serial.print('.');
    #endif
  }
  
  clearIRQFlags();
  
  return(ERR_NONE);
}

uint8_t SX1272::receive(Packet& pack) {
  char buffer[256];
  uint32_t startTime = millis();
  
  setMode(SX1272_STANDBY);
  
  _mod->SPIsetRegValue(SX1272_REG_DIO_MAPPING_1, SX1272_DIO0_RX_DONE | SX1272_DIO1_RX_TIMEOUT, 7, 4);
  clearIRQFlags();
  
  _mod->SPIsetRegValue(SX1272_REG_FIFO_RX_BASE_ADDR, SX1272_FIFO_RX_BASE_ADDR_MAX);
  _mod->SPIsetRegValue(SX1272_REG_FIFO_ADDR_PTR, SX1272_FIFO_RX_BASE_ADDR_MAX);
  
  setMode(SX1272_RXSINGLE);
  
  while(!_mod->getInt0State()) {
    if(_mod->getInt1State()) {
      clearIRQFlags();
      return(ERR_RX_TIMEOUT);
    }
  }
  
  if(_mod->SPIgetRegValue(SX1272_REG_IRQ_FLAGS, 5, 5) == SX1272_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR) {
    return(ERR_CRC_MISMATCH);
  }
  
  uint8_t headerMode = _mod->SPIgetRegValue(SX1272_REG_MODEM_CONFIG_1, 0, 0);
  if(headerMode == SX1272_HEADER_EXPL_MODE) {
    pack.length = _mod->SPIgetRegValue(SX1272_REG_RX_NB_BYTES);
  }
  
  _mod->SPIreadRegisterBurstStr(SX1272_REG_FIFO, pack.length, buffer);
  
  clearIRQFlags();
  
  for(uint8_t i = 0; i < 8; i++) {
    pack.source[i] = buffer[i];
    pack.destination[i] = buffer[i+8];
  }
  
  for(uint8_t i = 16; i < pack.length; i++) {
    pack.data[i-16] = buffer[i];
  }
  pack.data[pack.length-16] = 0;
  
  uint32_t elapsedTime = millis() - startTime;
  dataRate = (pack.length*8.0)/((float)elapsedTime/1000.0);
  lastPacketRSSI = getLastPacketRSSI();
  
  return(ERR_NONE);
}

uint8_t SX1272::sleep() {
  return(setMode(0b00000000));
}

uint8_t SX1272::standby() {
  return(setMode(0b00000001));
}

uint8_t SX1272::setBandwidth(Bandwidth bw) {
  uint8_t state = config(bw, _sf, _cr);
  if(state == ERR_NONE) {
    _bw = bw;
  }
  return(state);
}

uint8_t SX1272::setSpreadingFactor(SpreadingFactor sf) {
  uint8_t state = config(_bw, sf, _cr);
  if(state == ERR_NONE) {
    _sf = sf;
  }
  return(state);
}

uint8_t SX1272::setCodingRate(CodingRate cr) {
  uint8_t state = config(_bw, _sf, cr);
  if(state == ERR_NONE) {
    _cr = cr;
  }
  return(state);
}

void SX1272::generateLoRaAdress() {
  for(uint8_t i = _addrEeprom; i < (_addrEeprom + 8); i++) {
    EEPROM.write(i, (uint8_t)random(0, 256));
  }
}

uint8_t SX1272::config(Bandwidth bw, SpreadingFactor sf, CodingRate cr) {
  uint8_t status = ERR_NONE;
  uint8_t newBandwidth, newSpreadingFactor, newCodingRate;
  
  //check the supplied bw, cr and sf values
  switch(bw) {
    case BW_125_00_KHZ:
      newBandwidth = SX1272_BW_125_00_KHZ;
      break;
    case BW_250_00_KHZ:
      newBandwidth = SX1272_BW_250_00_KHZ;
      break;
    case BW_500_00_KHZ:
      newBandwidth = SX1272_BW_500_00_KHZ;
      break;
    default:
      return(ERR_INVALID_BANDWIDTH);
  }
  
  switch(sf) {
    case SF_6:
      newSpreadingFactor = SX1272_SF_6;
      break;
    case SF_7:
      newSpreadingFactor = SX1272_SF_7;
      break;
    case SF_8:
      newSpreadingFactor = SX1272_SF_8;
      break;
    case SF_9:
      newSpreadingFactor = SX1272_SF_9;
      break;
    case SF_10:
      newSpreadingFactor = SX1272_SF_10;
      break;
    case SF_11:
      newSpreadingFactor = SX1272_SF_11;
      break;
    case SF_12:
      newSpreadingFactor = SX1272_SF_12;
      break;
    default:
      return(ERR_INVALID_SPREADING_FACTOR);
  }
  
  switch(cr) {
    case CR_4_5:
      newCodingRate = SX1272_CR_4_5;
      break;
    case CR_4_6:
      newCodingRate = SX1272_CR_4_6;
      break;
    case CR_4_7:
      newCodingRate = SX1272_CR_4_7;
      break;
    case CR_4_8:
      newCodingRate = SX1272_CR_4_8;
      break;
    default:
      return(ERR_INVALID_CODING_RATE);
  }
  
  // set mode to SLEEP
  status = setMode(SX1272_SLEEP);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set LoRa mode
  status = _mod->SPIsetRegValue(SX1272_REG_OP_MODE, SX1272_LORA, 7, 7);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set carrier frequency
  status = _mod->SPIsetRegValue(SX1272_REG_FRF_MSB, SX1272_FRF_MSB);
  status = _mod->SPIsetRegValue(SX1272_REG_FRF_MID, SX1272_FRF_MID);
  status = _mod->SPIsetRegValue(SX1272_REG_FRF_LSB, SX1272_FRF_LSB);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // output power configuration
  status = _mod->SPIsetRegValue(SX1272_REG_PA_CONFIG, SX1272_PA_SELECT_BOOST | SX1272_OUTPUT_POWER);
  status = _mod->SPIsetRegValue(SX1272_REG_OCP, SX1272_OCP_ON | SX1272_OCP_TRIM, 5, 0);
  status = _mod->SPIsetRegValue(SX1272_REG_LNA, SX1272_LNA_GAIN_1 | SX1272_LNA_BOOST_ON);
  status = _mod->SPIsetRegValue(SX1272_REG_PA_DAC, SX1272_PA_BOOST_ON, 2, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // turn off frequency hopping
  status = _mod->SPIsetRegValue(SX1272_REG_HOP_PERIOD, SX1272_HOP_PERIOD_OFF);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // basic setting (bw, cr, sf, header mode and CRC)
  if(newSpreadingFactor == SX1272_SF_6) {
    status = _mod->SPIsetRegValue(SX1272_REG_MODEM_CONFIG_2, SX1272_SF_6 | SX1272_TX_MODE_SINGLE | SX1272_RX_CRC_MODE_OFF, 7, 2);
    status = _mod->SPIsetRegValue(SX1272_REG_MODEM_CONFIG_1, newBandwidth | newCodingRate | SX1272_HEADER_IMPL_MODE);
    status = _mod->SPIsetRegValue(SX1272_REG_DETECT_OPTIMIZE, SX1272_DETECT_OPTIMIZE_SF_6, 2, 0);
    status = _mod->SPIsetRegValue(SX1272_REG_DETECTION_THRESHOLD, SX1272_DETECTION_THRESHOLD_SF_6);
  } else {
    status = _mod->SPIsetRegValue(SX1272_REG_MODEM_CONFIG_2, newSpreadingFactor | SX1272_TX_MODE_SINGLE | SX1272_RX_CRC_MODE_ON, 7, 2);
    status = _mod->SPIsetRegValue(SX1272_REG_MODEM_CONFIG_1, newBandwidth | newCodingRate | SX1272_HEADER_EXPL_MODE);
    status = _mod->SPIsetRegValue(SX1272_REG_DETECT_OPTIMIZE, SX1272_DETECT_OPTIMIZE_SF_7_12, 2, 0);
    status = _mod->SPIsetRegValue(SX1272_REG_DETECTION_THRESHOLD, SX1272_DETECTION_THRESHOLD_SF_7_12);
  }
  
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set default preamble length
  status = _mod->SPIsetRegValue(SX1272_REG_PREAMBLE_MSB, SX1272_PREAMBLE_LENGTH_MSB);
  status = _mod->SPIsetRegValue(SX1272_REG_PREAMBLE_LSB, SX1272_PREAMBLE_LENGTH_LSB);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set mode to STANDBY
  status = setMode(SX1272_STANDBY);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // save the new settings
  _bw = bw;
  _sf = sf;
  _cr = cr;
  
  return(ERR_NONE);
}

uint8_t SX1272::setMode(uint8_t mode) {
  _mod->SPIsetRegValue(SX1272_REG_OP_MODE, mode, 2, 0);
  return(ERR_NONE);
}

void SX1272::clearIRQFlags() {
  _mod->SPIwriteRegister(SX1272_REG_IRQ_FLAGS, 0b11111111);
}

int8_t SX1272::getLastPacketRSSI() {
  return(-164 + _mod->SPIgetRegValue(SX1272_REG_PKT_RSSI_VALUE));
}
