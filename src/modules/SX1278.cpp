#include "SX1278.h"

SX1278::SX1278(Module* module) {
  _mod = module;
}

uint8_t SX1278::begin(Bandwidth bw, SpreadingFactor sf, CodingRate cr, uint16_t addrEeprom) { 
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
    uint8_t version = _mod->SPIreadRegister(SX1278_REG_VERSION);
    if(version == 0x12) {
      flagFound = true;
    } else {
      #ifdef DEBUG
        Serial.print("SX1278 not found! (");
        Serial.print(i + 1);
        Serial.print(" of 10 tries) SX1278_REG_VERSION == ");
        
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
      Serial.println("No SX1278 found!");
    #endif
    SPI.end();
    return(ERR_CHIP_NOT_FOUND);
  }
  #ifdef DEBUG
    else {
      Serial.println("Found SX1278! (match by SX1278_REG_VERSION == 0x12)");
    }
  #endif
  
  return(config(_bw, _sf, _cr));
}

uint8_t SX1278::transmit(Packet& pack) {
  char buffer[256];
  
  for(uint8_t i = 0; i < 8; i++) {
    buffer[i] = pack.source[i];
    buffer[i+8] = pack.destination[i];
  }
  
  for(uint8_t i = 0; i < pack.length; i++) {
    buffer[i+16] = pack.data[i];
  }
  
  setMode(SX1278_STANDBY);
  
  _mod->SPIsetRegValue(SX1278_REG_DIO_MAPPING_1, SX1278_DIO0_TX_DONE, 7, 6);
  clearIRQFlags();
  
  if(pack.length > 256) {
    return(ERR_PACKET_TOO_LONG);
  }

  _mod->SPIsetRegValue(SX1278_REG_PAYLOAD_LENGTH, pack.length);
  _mod->SPIsetRegValue(SX1278_REG_FIFO_TX_BASE_ADDR, SX1278_FIFO_TX_BASE_ADDR_MAX);
  _mod->SPIsetRegValue(SX1278_REG_FIFO_ADDR_PTR, SX1278_FIFO_TX_BASE_ADDR_MAX);
  
  _mod->SPIwriteRegisterBurstStr(SX1278_REG_FIFO, buffer, pack.length);
  
  setMode(SX1278_TX);
  
  unsigned long start = millis();
  while(!_mod->getInt0State()) {
    #ifdef DEBUG
      Serial.print('.');
    #endif
  }
  
  clearIRQFlags();
  
  return(ERR_NONE);
}

uint8_t SX1278::receive(Packet& pack) {
  char buffer[256];
  uint32_t startTime = millis();
  
  setMode(SX1278_STANDBY);
  
  _mod->SPIsetRegValue(SX1278_REG_DIO_MAPPING_1, SX1278_DIO0_RX_DONE | SX1278_DIO1_RX_TIMEOUT, 7, 4);
  clearIRQFlags();
  
  _mod->SPIsetRegValue(SX1278_REG_FIFO_RX_BASE_ADDR, SX1278_FIFO_RX_BASE_ADDR_MAX);
  _mod->SPIsetRegValue(SX1278_REG_FIFO_ADDR_PTR, SX1278_FIFO_RX_BASE_ADDR_MAX);
  
  setMode(SX1278_RXSINGLE);
  
  while(!_mod->getInt0State()) {
    if(_mod->getInt1State()) {
      clearIRQFlags();
      return(ERR_RX_TIMEOUT);
    }
  }
  
  if(_mod->SPIgetRegValue(SX1278_REG_IRQ_FLAGS, 5, 5) == SX1278_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR) {
    return(ERR_CRC_MISMATCH);
  }
  
  uint8_t headerMode = _mod->SPIgetRegValue(SX1278_REG_MODEM_CONFIG_1, 0, 0);
  if(headerMode == SX1278_HEADER_EXPL_MODE) {
    pack.length = _mod->SPIgetRegValue(SX1278_REG_RX_NB_BYTES);
  }
  
  _mod->SPIreadRegisterBurstStr(SX1278_REG_FIFO, pack.length, buffer);
  
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

uint8_t SX1278::sleep() {
  return(setMode(0b00000000));
}

uint8_t SX1278::standby() {
  return(setMode(0b00000001));
}

uint8_t SX1278::setBandwidth(Bandwidth bw) {
  uint8_t state = config(bw, _sf, _cr);
  if(state == ERR_NONE) {
    _bw = bw;
  }
  return(state);
}

uint8_t SX1278::setSpreadingFactor(SpreadingFactor sf) {
  uint8_t state = config(_bw, sf, _cr);
  if(state == ERR_NONE) {
    _sf = sf;
  }
  return(state);
}

uint8_t SX1278::setCodingRate(CodingRate cr) {
  uint8_t state = config(_bw, _sf, cr);
  if(state == ERR_NONE) {
    _cr = cr;
  }
  return(state);
}

void SX1278::generateLoRaAdress() {
  for(uint8_t i = _addrEeprom; i < (_addrEeprom + 8); i++) {
    EEPROM.write(i, (uint8_t)random(0, 256));
  }
}

uint8_t SX1278::config(Bandwidth bw, SpreadingFactor sf, CodingRate cr) {
  uint8_t status = ERR_NONE;
  uint8_t newBandwidth, newSpreadingFactor, newCodingRate;
  
  //check the supplied bw, cr and sf values
  switch(bw) {
    case BW_7_80_KHZ:
      newBandwidth = SX1278_BW_7_80_KHZ;
      break;
    case BW_10_40_KHZ:
      newBandwidth = SX1278_BW_10_40_KHZ;
      break;
    case BW_15_60_KHZ:
      newBandwidth = SX1278_BW_15_60_KHZ;
      break;
    case BW_20_80_KHZ:
      newBandwidth = SX1278_BW_20_80_KHZ;
      break;
    case BW_31_25_KHZ:
      newBandwidth = SX1278_BW_31_25_KHZ;
      break;
      case BW_41_70_KHZ:
      newBandwidth = SX1278_BW_41_70_KHZ;
      break;
    case BW_62_50_KHZ:
      newBandwidth = SX1278_BW_62_50_KHZ;
      break;
    case BW_125_00_KHZ:
      newBandwidth = SX1278_BW_125_00_KHZ;
      break;
    case BW_250_00_KHZ:
      newBandwidth = SX1278_BW_250_00_KHZ;
      break;
    case BW_500_00_KHZ:
      newBandwidth = SX1278_BW_500_00_KHZ;
      break;
    default:
      return(ERR_INVALID_BANDWIDTH);
  }
  
  switch(sf) {
    case SF_6:
      newSpreadingFactor = SX1278_SF_6;
      break;
    case SF_7:
      newSpreadingFactor = SX1278_SF_7;
      break;
    case SF_8:
      newSpreadingFactor = SX1278_SF_8;
      break;
    case SF_9:
      newSpreadingFactor = SX1278_SF_9;
      break;
    case SF_10:
      newSpreadingFactor = SX1278_SF_10;
      break;
    case SF_11:
      newSpreadingFactor = SX1278_SF_11;
      break;
    case SF_12:
      newSpreadingFactor = SX1278_SF_12;
      break;
    default:
      return(ERR_INVALID_SPREADING_FACTOR);
  }
  
  switch(cr) {
    case CR_4_5:
      newCodingRate = SX1278_CR_4_5;
      break;
    case CR_4_6:
      newCodingRate = SX1278_CR_4_6;
      break;
    case CR_4_7:
      newCodingRate = SX1278_CR_4_7;
      break;
    case CR_4_8:
      newCodingRate = SX1278_CR_4_8;
      break;
    default:
      return(ERR_INVALID_CODING_RATE);
  }
  
  // set mode to SLEEP
  status = setMode(SX1278_SLEEP);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set LoRa mode
  status = _mod->SPIsetRegValue(SX1278_REG_OP_MODE, SX1278_LORA, 7, 7);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set carrier frequency
  status = _mod->SPIsetRegValue(SX1278_REG_FRF_MSB, SX1278_FRF_MSB);
  status = _mod->SPIsetRegValue(SX1278_REG_FRF_MID, SX1278_FRF_MID);
  status = _mod->SPIsetRegValue(SX1278_REG_FRF_LSB, SX1278_FRF_LSB);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // output power configuration
  status = _mod->SPIsetRegValue(SX1278_REG_PA_CONFIG, SX1278_PA_SELECT_BOOST | SX1278_MAX_POWER | SX1278_OUTPUT_POWER);
  status = _mod->SPIsetRegValue(SX1278_REG_OCP, SX1278_OCP_ON | SX1278_OCP_TRIM, 5, 0);
  status = _mod->SPIsetRegValue(SX1278_REG_LNA, SX1278_LNA_GAIN_1 | SX1278_LNA_BOOST_HF_ON);
  status = _mod->SPIsetRegValue(SX1278_REG_PA_DAC, SX1278_PA_BOOST_ON, 2, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // turn off frequency hopping
  status = _mod->SPIsetRegValue(SX1278_REG_HOP_PERIOD, SX1278_HOP_PERIOD_OFF);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // basic setting (bw, cr, sf, header mode and CRC)
  if(newSpreadingFactor == SX1278_SF_6) {
    status = _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_2, SX1278_SF_6 | SX1278_TX_MODE_SINGLE | SX1278_RX_CRC_MODE_OFF, 7, 2);
    status = _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_1, newBandwidth | newCodingRate | SX1278_HEADER_IMPL_MODE);
    status = _mod->SPIsetRegValue(SX1278_REG_DETECT_OPTIMIZE, SX1278_DETECT_OPTIMIZE_SF_6, 2, 0);
    status = _mod->SPIsetRegValue(SX1278_REG_DETECTION_THRESHOLD, SX1278_DETECTION_THRESHOLD_SF_6);
  } else {
    status = _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_2, newSpreadingFactor | SX1278_TX_MODE_SINGLE | SX1278_RX_CRC_MODE_ON, 7, 2);
    status = _mod->SPIsetRegValue(SX1278_REG_MODEM_CONFIG_1, newBandwidth | newCodingRate | SX1278_HEADER_EXPL_MODE);
    status = _mod->SPIsetRegValue(SX1278_REG_DETECT_OPTIMIZE, SX1278_DETECT_OPTIMIZE_SF_7_12, 2, 0);
    status = _mod->SPIsetRegValue(SX1278_REG_DETECTION_THRESHOLD, SX1278_DETECTION_THRESHOLD_SF_7_12);
  }
  
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set default preamble length
  status = _mod->SPIsetRegValue(SX1278_REG_PREAMBLE_MSB, SX1278_PREAMBLE_LENGTH_MSB);
  status = _mod->SPIsetRegValue(SX1278_REG_PREAMBLE_LSB, SX1278_PREAMBLE_LENGTH_LSB);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set mode to STANDBY
  status = setMode(SX1278_STANDBY);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // save the new settings
  _bw = bw;
  _sf = sf;
  _cr = cr;
  
  return(ERR_NONE);
}

uint8_t SX1278::setMode(uint8_t mode) {
  _mod->SPIsetRegValue(SX1278_REG_OP_MODE, mode, 2, 0);
  return(ERR_NONE);
}

void SX1278::clearIRQFlags() {
  _mod->SPIwriteRegister(SX1278_REG_IRQ_FLAGS, 0b11111111);
}

int8_t SX1278::getLastPacketRSSI() {
  return(-164 + _mod->SPIgetRegValue(SX1278_REG_PKT_RSSI_VALUE));
}
