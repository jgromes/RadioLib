#include "SX127x.h"

SX127x::SX127x(Module* mod) {
  _mod = mod;
}

uint8_t SX127x::begin(uint8_t syncWord, int8_t power, uint16_t addrEeprom) {
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
    DEBUG_PRINTLN_STR("Found SX127x!");
  }
  
  // set LoRa sync word
  uint8_t state = SX127x::setSyncWord(syncWord);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // set output power
  state = SX127x::setOutputPower(power);
  if(state != ERR_NONE) {
    return(state);
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
  
  // set mode to standby
  setMode(SX127X_STANDBY);
  
  // set DIO mapping
  _mod->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_TX_DONE, 7, 6);
  
  // clear interrupt flags
  clearIRQFlags();
  
  // set packet length
  _mod->SPIsetRegValue(SX127X_REG_PAYLOAD_LENGTH, pack.length);
  
  // set FIFO pointers
  _mod->SPIsetRegValue(SX127X_REG_FIFO_TX_BASE_ADDR, SX127X_FIFO_TX_BASE_ADDR_MAX);
  _mod->SPIsetRegValue(SX127X_REG_FIFO_ADDR_PTR, SX127X_FIFO_TX_BASE_ADDR_MAX);
  
  // write packet to FIFO
  _mod->SPIwriteRegisterBurstStr(SX127X_REG_FIFO, (char*)pack.source, 8);
  _mod->SPIwriteRegisterBurstStr(SX127X_REG_FIFO, (char*)pack.destination, 8);
  _mod->SPIwriteRegisterBurstStr(SX127X_REG_FIFO, pack.data, pack.length - 16);
  
  // start transmission
  setMode(SX127X_TX);
  
  // wait for packet transmission or timeout
  uint32_t start = millis();
  while(!_mod->getInt0State()) {
    if(millis() - start > timeout) {
      clearIRQFlags();
      return(ERR_TX_TIMEOUT);
    }
  }
  
  // clear interrupt flags
  clearIRQFlags();
  
  return(ERR_NONE);
}

uint8_t SX127x::receive(Packet& pack) {
  // set mode to standby
  setMode(SX127X_STANDBY);
  
  // set DIO pin mapping
  _mod->SPIsetRegValue(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_RX_DONE | SX127X_DIO1_RX_TIMEOUT, 7, 4);
  
  // clear interrupt flags
  clearIRQFlags();
  
  // set FIFO pointers
  _mod->SPIsetRegValue(SX127X_REG_FIFO_RX_BASE_ADDR, SX127X_FIFO_RX_BASE_ADDR_MAX);
  _mod->SPIsetRegValue(SX127X_REG_FIFO_ADDR_PTR, SX127X_FIFO_RX_BASE_ADDR_MAX);
  
  // set mode to receive
  setMode(SX127X_RXSINGLE);
  
  // wait for packet reception or timeout
  uint32_t start = millis();
  while(!_mod->getInt0State()) {
    if(_mod->getInt1State()) {
      clearIRQFlags();
      return(ERR_RX_TIMEOUT);
    }
  }
  uint32_t elapsed = millis() - start;
  
  // check integrity CRC
  if(_mod->SPIgetRegValue(SX127X_REG_IRQ_FLAGS, 5, 5) == SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR) {
    return(ERR_CRC_MISMATCH);
  }
  
  // get packet length
  if(_sf != 6) {
    pack.length = _mod->SPIgetRegValue(SX127X_REG_RX_NB_BYTES);
  }
  
  // read packet addresses
  _mod->SPIreadRegisterBurstStr(SX127X_REG_FIFO, 8, (char*)pack.source);
  _mod->SPIreadRegisterBurstStr(SX127X_REG_FIFO, 8, (char*)pack.destination);
  
  // read packet data
  delete[] pack.data;
  pack.data = new char[pack.length - 15];
  _mod->SPIreadRegisterBurstStr(SX127X_REG_FIFO, pack.length - 16, pack.data);
  pack.data[pack.length - 16] = 0;
  
  // update data rate, RSSI and SNR
  dataRate = (pack.length*8.0)/((float)elapsed/1000.0);
  lastPacketRSSI = -157 + _mod->SPIgetRegValue(SX127X_REG_PKT_RSSI_VALUE);
  int8_t rawSNR = (int8_t)_mod->SPIgetRegValue(SX127X_REG_PKT_SNR_VALUE);
  lastPacketSNR = rawSNR / 4.0;
  
  // clear interrupt flags
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

uint8_t SX127x::setSyncWord(uint8_t syncWord) {
  setMode(SX127X_STANDBY);
  
  uint8_t state = _mod->SPIsetRegValue(SX127X_REG_SYNC_WORD, syncWord);
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
  
  uint8_t state = _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, power - 2, 3, 0);
  if(state == ERR_NONE) {
    _power = power;
  }
  
  return(state);
}

uint8_t SX127x::setFrequencyRaw(float newFreq) {
  // set mode to standby
  setMode(SX127X_STANDBY);
  
  // calculate register values
  uint32_t base = 1;
  uint32_t FRF = (newFreq * (base << 19)) / 32.0;
  
  // write registers
  uint8_t state = _mod->SPIsetRegValue(SX127X_REG_FRF_MSB, (FRF & 0xFF0000) >> 16);
  state |= _mod->SPIsetRegValue(SX127X_REG_FRF_MID, (FRF & 0x00FF00) >> 8);
  state |= _mod->SPIsetRegValue(SX127X_REG_FRF_LSB, FRF & 0x0000FF);
  
  return(state);
}

uint8_t SX127x::config() {
  // set mode to SLEEP
  uint8_t state = setMode(SX127X_SLEEP);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // set LoRa mode
  state = _mod->SPIsetRegValue(SX127X_REG_OP_MODE, SX127X_LORA, 7, 7);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // output power configuration
  state = _mod->SPIsetRegValue(SX127X_REG_PA_CONFIG, SX127X_PA_SELECT_BOOST | SX127X_OUTPUT_POWER);
  state |= _mod->SPIsetRegValue(SX127X_REG_OCP, SX127X_OCP_ON | SX127X_OCP_TRIM, 5, 0);
  state |= _mod->SPIsetRegValue(SX127X_REG_LNA, SX127X_LNA_GAIN_1 | SX127X_LNA_BOOST_ON);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // turn off frequency hopping
  state = _mod->SPIsetRegValue(SX127X_REG_HOP_PERIOD, SX127X_HOP_PERIOD_OFF);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // set default preamble length
  state = _mod->SPIsetRegValue(SX127X_REG_PREAMBLE_MSB, SX127X_PREAMBLE_LENGTH_MSB);
  state |= _mod->SPIsetRegValue(SX127X_REG_PREAMBLE_LSB, SX127X_PREAMBLE_LENGTH_LSB);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // set mode to STANDBY
  state = setMode(SX127X_STANDBY);
  return(state);
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
