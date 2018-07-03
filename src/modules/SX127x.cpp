#include "SX127x.h"

SX127x::SX127x(Module* mod) {
  _mod = mod;
}

uint8_t SX127x::begin(float freq, uint32_t bw, uint8_t sf, uint8_t cr, uint8_t syncWord, uint16_t addrEeprom) {
  // copy LoRa modem settings
  _freq = freq;
  _bw = bw;
  _sf = sf;
  _cr = cr;
  _syncWord = syncWord;

  // ESP32-only: initialize  EEPROM
  #ifdef ESP32
    if(!EEPROM.begin(9)) {
      DEBUG_PRINTLN_STR("Unable to initialize EEPROM");
      return(ERR_EEPROM_NOT_INITIALIZED);
    }
  #endif
  
  // copy EEPROM start address
  _addrEeprom = addrEeprom;
  
  // check if the node has address
  bool hasAddress = false;
  for(uint16_t i = 0; i < 8; i++) {
    if(EEPROM.read(_addrEeprom + i) != 255) {
      hasAddress = true;
      break;
    }
  }
  
  // generate new address
  if(!hasAddress) {
    randomSeed(analogRead(5));
    generateNodeAdress();
  }
  
  DEBUG_PRINTLN_STR("LoRa node address string: ");
  for(uint8_t i = 0; i < 8; i++) {
    _address[i] = EEPROM.read(i);
    #ifdef KITELIB_DEBUG
      Serial.print(_address[i], HEX);
      if(i < 7) {
        Serial.print(":");
      } else {
        Serial.println();
      }
    #endif
  }
  
  // set module properties
  _mod->init(USE_SPI, INT_BOTH);
  
  // try to find the SX127x chip
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    uint8_t version = _mod->SPIreadRegister(SX127X_REG_VERSION);
    if(version == 0x12) {
      flagFound = true;
    } else {
      #ifdef KITELIB_DEBUG
        Serial.print("SX127x not found! (");
        Serial.print(i + 1);
        Serial.print(" of 10 tries) SX127X_REG_VERSION == ");
        
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
    DEBUG_PRINTLN_STR("No SX127x found!");
    SPI.end();
    return(ERR_CHIP_NOT_FOUND);
  } else {
    DEBUG_PRINTLN_STR("Found SX127x! (match by SX127X_REG_VERSION == 0x12)");
  }
  
  return(ERR_NONE);
}

uint8_t SX127x::transmit(Packet& pack) {
  // check packet length
  if(pack.length >= 256) {
    return(ERR_PACKET_TOO_LONG);
  }

  // calculate timeout
  uint16_t base = 1;
  float symbolLength = (float)(base << _sf) / (float)_bw;
  float de = 0;
  if(symbolLength >= 0.016) {
    de = 1;
  }
  float ih = (float)_mod->SPIgetRegValue(SX127X_REG_MODEM_CONFIG_1, 0, 0);
  float crc = (float)(_mod->SPIgetRegValue(SX127X_REG_MODEM_CONFIG_2, 2, 2) >> 2);
  float n_pre = (float)_mod->SPIgetRegValue(SX127X_REG_PREAMBLE_LSB);
  float n_pay = 8.0 + max(ceil((8.0 * (float)pack.length - 4.0 * (float)_sf + 28.0 + 16.0 * crc - 20.0 * ih)/(4.0 * (float)_sf - 8.0 * de)) * (float)_cr, 0);
  uint32_t timeout = ceil(symbolLength * (n_pre + n_pay + 4.25) * 1000.0);
  
  // write packet to FIFO
  setMode(SX127X_STANDBY);
  
  _mod->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_TX_DONE, 7, 6);
  clearIRQFlags();

  _mod->SPIsetRegValue(SX127X_REG_PAYLOAD_LENGTH, pack.length);
  _mod->SPIsetRegValue(SX127X_REG_FIFO_TX_BASE_ADDR, SX127X_FIFO_TX_BASE_ADDR_MAX);
  _mod->SPIsetRegValue(SX127X_REG_FIFO_ADDR_PTR, SX127X_FIFO_TX_BASE_ADDR_MAX);
  
  _mod->SPIwriteRegisterBurstStr(SX127X_REG_FIFO, pack.source, 8);
  _mod->SPIwriteRegisterBurstStr(SX127X_REG_FIFO, pack.destination, 8);
  _mod->SPIwriteRegisterBurstStr(SX127X_REG_FIFO, pack.data, pack.length - 16);
  
  // start transmission
  setMode(SX127X_TX);
  
  // check for timeout
  uint32_t start = millis();
  while(!_mod->getInt0State()) {
    if(millis() - start > timeout) {
      clearIRQFlags();
      return(ERR_TX_TIMEOUT);
    }
  }
  
  clearIRQFlags();
  
  return(ERR_NONE);
}

uint8_t SX127x::receive(Packet& pack) {
  // prepare for packet reception
  setMode(SX127X_STANDBY);
  
  _mod->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_RX_DONE | SX127X_DIO1_RX_TIMEOUT, 7, 4);
  clearIRQFlags();
  
  _mod->SPIsetRegValue(SX127X_REG_FIFO_RX_BASE_ADDR, SX127X_FIFO_RX_BASE_ADDR_MAX);
  _mod->SPIsetRegValue(SX127X_REG_FIFO_ADDR_PTR, SX127X_FIFO_RX_BASE_ADDR_MAX);
  
  // start receiving
  setMode(SX127X_RXSINGLE);
  
  uint32_t start = millis();
  while(!_mod->getInt0State()) {
    if(_mod->getInt1State()) {
      clearIRQFlags();
      return(ERR_RX_TIMEOUT);
    }
  }
  uint32_t elapsed = millis() - start;
  
  if(_mod->SPIgetRegValue(SX127X_REG_IRQ_FLAGS, 5, 5) == SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR) {
    return(ERR_CRC_MISMATCH);
  }
  
  if(_sf != 6) {
    pack.length = _mod->SPIgetRegValue(SX127X_REG_RX_NB_BYTES);
  }
  
  _mod->SPIreadRegisterBurstStr(SX127X_REG_FIFO, 8, pack.source);
  _mod->SPIreadRegisterBurstStr(SX127X_REG_FIFO, 8, pack.destination);
  
  delete[] pack.data;
  pack.data = new char[pack.length - 15];
  _mod->SPIreadRegisterBurstStr(SX127X_REG_FIFO, pack.length - 16, pack.data);
  pack.data[pack.length - 16] = 0;
  
  dataRate = (pack.length*8.0)/((float)elapsed/1000.0);
  lastPacketRSSI = -157 + _mod->SPIgetRegValue(SX127X_REG_PKT_RSSI_VALUE);
  int8_t rawSNR = (int8_t)_mod->SPIgetRegValue(SX127X_REG_PKT_SNR_VALUE);
  lastPacketSNR = rawSNR / 4.0;
  
  clearIRQFlags();
  
  return(ERR_NONE);
}

uint8_t SX127x::scanChannel() {
  setMode(SX127X_STANDBY);
  
  _mod->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_CAD_DONE | SX127X_DIO1_CAD_DETECTED, 7, 4);
  clearIRQFlags();
  
  setMode(SX127X_CAD);
  
  while(!_mod->getInt0State()) {
    if(_mod->getInt1State()) {
      clearIRQFlags();
      return(PREAMBLE_DETECTED);
    }
  }
  
  clearIRQFlags();
  return(CHANNEL_FREE);
}

uint8_t SX127x::sleep() {
  return(setMode(SX127X_SLEEP));
}

uint8_t SX127x::standby() {
  return(setMode(SX127X_STANDBY));
}

uint8_t SX127x::setFrequency(float freq) {
  uint8_t state = config(_bw, _sf, _cr, freq, _syncWord);
  if(state == ERR_NONE) {
    _freq = freq;
  }
  return(state);
}

uint8_t SX127x::setSyncWord(uint8_t syncWord) {
  uint8_t state = config(_bw, _sf, _cr, _freq, syncWord);
  if(state == ERR_NONE) {
    _syncWord = syncWord;
  }
  return(state);
}

uint8_t SX127x::setOutputPower(int8_t power) {
  setMode(SX127X_STANDBY);

  if((power < 2) || (power > 17)) {
    return(ERR_INVALID_OUTPUT_POWER);
  }
  
  return(_mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, power - 2, 3, 0));
}

uint8_t SX127x::config(uint8_t bw, uint8_t sf, uint8_t cr, float freq, uint8_t syncWord) {
  uint8_t status = ERR_NONE;
  
  // set mode to SLEEP
  status = setMode(SX127X_SLEEP);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set LoRa mode
  status = _mod->SPIsetRegValue(SX127X_REG_OP_MODE, SX127X_LORA, 7, 7);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set carrier frequency
  uint32_t base = 1;
  uint32_t FRF = (freq * (base << 19)) / 32.0;
  status = _mod->SPIsetRegValue(SX127X_REG_FRF_MSB, (FRF & 0xFF0000) >> 16);
  status = _mod->SPIsetRegValue(SX127X_REG_FRF_MID, (FRF & 0x00FF00) >> 8);
  status = _mod->SPIsetRegValue(SX127X_REG_FRF_LSB, FRF & 0x0000FF);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // output power configuration
  status = _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX127X_PA_SELECT_BOOST | SX127X_OUTPUT_POWER);
  status = _mod->SPIsetRegValue(SX127X_REG_OCP, SX127X_OCP_ON | SX127X_OCP_TRIM, 5, 0);
  status = _mod->SPIsetRegValue(SX127X_REG_LNA, SX127X_LNA_GAIN_1 | SX127X_LNA_BOOST_ON);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // turn off frequency hopping
  status = _mod->SPIsetRegValue(SX127X_REG_HOP_PERIOD, SX127X_HOP_PERIOD_OFF);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // basic setting (bw, cr, sf, header mode and CRC)
  if(sf == SX127X_SF_6) {
    status = _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, SX127X_SF_6 | SX127X_TX_MODE_SINGLE, 7, 3);
    status = _mod->SPIsetRegValue(SX127X_REG_DETECT_OPTIMIZE, SX127X_DETECT_OPTIMIZE_SF_6, 2, 0);
    status = _mod->SPIsetRegValue(SX127X_REG_DETECTION_THRESHOLD, SX127X_DETECTION_THRESHOLD_SF_6);
  } else {
    status = _mod->SPIsetRegValue(SX127X_REG_MODEM_CONFIG_2, sf | SX127X_TX_MODE_SINGLE, 7, 3);
    status = _mod->SPIsetRegValue(SX127X_REG_DETECT_OPTIMIZE, SX127X_DETECT_OPTIMIZE_SF_7_12, 2, 0);
    status = _mod->SPIsetRegValue(SX127X_REG_DETECTION_THRESHOLD, SX127X_DETECTION_THRESHOLD_SF_7_12);
  }
  
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set the sync word
  status = _mod->SPIsetRegValue(SX127X_REG_SYNC_WORD, syncWord);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set default preamble length
  status = _mod->SPIsetRegValue(SX127X_REG_PREAMBLE_MSB, SX127X_PREAMBLE_LENGTH_MSB);
  status = _mod->SPIsetRegValue(SX127X_REG_PREAMBLE_LSB, SX127X_PREAMBLE_LENGTH_LSB);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set mode to STANDBY
  status = setMode(SX127X_STANDBY);
  return(status);
}

void SX127x::generateNodeAdress() {
  for(uint8_t i = _addrEeprom; i < (_addrEeprom + 8); i++) {
    EEPROM.write(i, (uint8_t)random(0, 256));
  }
}

uint8_t SX127x::setMode(uint8_t mode) {
  _mod->SPIsetRegValue(SX127X_REG_OP_MODE, mode, 2, 0);
  return(ERR_NONE);
}

void SX127x::clearIRQFlags() {
  _mod->SPIwriteRegister(SX127X_REG_IRQ_FLAGS, 0b11111111);
}
