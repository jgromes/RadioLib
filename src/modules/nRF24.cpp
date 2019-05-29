#include "nRF24.h"

nRF24::nRF24(Module* mod) {
  _mod = mod;
}

int16_t nRF24::begin(int16_t freq, int16_t dataRate, uint8_t addrWidth) {
  // set module properties
  _mod->SPIreadCommand = NRF24_CMD_READ;
  _mod->SPIwriteCommand = NRF24_CMD_WRITE;
  _mod->init(USE_SPI, INT_BOTH);
  
  // override pin mode on INT0 (connected to nRF24 CE pin)
  pinMode(_mod->getInt0(), OUTPUT);
  
  // set frequency
  int16_t state = setFrequency(freq);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // set data rate
  state = setDataRate(dataRate);
  if(state != ERR_NONE) {
    return(state);
  }
  
  // set address width
  state = setAddressWidth(addrWidth);
  if(state != ERR_NONE) {
    return(state);
  }
  
  return(state);
}

int16_t nRF24::sleep() {
  return(_mod->SPIsetRegValue(NRF24_REG_CONFIG, NRF24_POWER_DOWN, 1, 1));
}

int16_t nRF24::standby() {
  return(_mod->SPIsetRegValue(NRF24_REG_CONFIG, NRF24_POWER_UP, 1, 1));
}

int16_t nRF24::transmit(String& str, uint8_t* addr) {
  return(nRF24::transmit(str.c_str(), addr));
}

int16_t nRF24::transmit(const char* str, uint8_t* addr) {
  return(nRF24::transmit((uint8_t*)str, strlen(str), addr));
}

int16_t nRF24::transmit(uint8_t* data, size_t len, uint8_t* addr) {
  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
     return(state);
  }
  
  // reverse address byte order (LSB must be written first)
  uint8_t* addrReversed = new uint8_t[_addrWidth];
  for(uint8_t i = 0; i < _addrWidth; i++) {
    addrReversed[i] = addr[_addrWidth - 1 - i];
  }
  
  // set transmit address
  _mod->SPIwriteRegisterBurst(NRF24_REG_TX_ADDR, addrReversed, _addrWidth);
  
  // check packet length
  if(len > 32) {
    return(ERR_PACKET_TOO_LONG);
  }
  
  // enable Tx_DataSent interrupt
  state = _mod->SPIsetRegValue(NRF24_REG_CONFIG, NRF24_MASK_RX_DR_IRQ_OFF | NRF24_MASK_TX_DS_IRQ_ON | NRF24_MASK_MAX_RT_IRQ_OFF, 6, 4);
  
  // fill Tx FIFO
  SPIwriteTxPayload(data, len);
  
  // enable primary Tx mode
  state |= _mod->SPIsetRegValue(NRF24_REG_CONFIG, NRF24_PTX, 0, 0);
  
  // CE high to start transmitting
  digitalWrite(_mod->getInt0(), HIGH);
  
  // wait until transmission is finished
  while(digitalRead(_mod->getInt1()));
  
  // CE low
  digitalWrite(_mod->getInt0(), LOW);
  
  // clear interrupt
  clearIRQ();
  
  return(state);
}

int16_t nRF24::setFrequency(int16_t freq) {
  // check allowed range
  if(!((freq >= 2400) && (freq <= 2525))) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
     return(state);
  }
  
  // set frequency
  uint8_t freqRaw = freq - 2400;
  state = _mod->SPIsetRegValue(NRF24_REG_RF_CH, freqRaw, 6, 0);
  return(state);
}

int16_t nRF24::setDataRate(int16_t dataRate) {
  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
     return(state);
  }
  
  // set data rate
  if(dataRate == 250) {
    state = _mod->SPIsetRegValue(NRF24_REG_RF_SETUP, NRF24_DR_250_KBPS, 5, 5);
    state |= _mod->SPIsetRegValue(NRF24_REG_RF_SETUP, NRF24_DR_250_KBPS, 3, 3);
  } else if(dataRate == 1000) {
    state = _mod->SPIsetRegValue(NRF24_REG_RF_SETUP, NRF24_DR_1_MBPS, 5, 5);
    state |= _mod->SPIsetRegValue(NRF24_REG_RF_SETUP, NRF24_DR_1_MBPS, 3, 3);
  } else if(dataRate == 2000) {
    state = _mod->SPIsetRegValue(NRF24_REG_RF_SETUP, NRF24_DR_2_MBPS, 5, 5);
    state |= _mod->SPIsetRegValue(NRF24_REG_RF_SETUP, NRF24_DR_2_MBPS, 3, 3);
  } else {
    return(ERR_INVALID_DATA_RATE);
  }
  
  return(state);
}

int16_t nRF24::setAddressWidth(uint8_t addrWidth) {
  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
     return(state);
  }
  
  // set address width
  switch(addrWidth) {
    case 3:
      state = _mod->SPIsetRegValue(NRF24_REG_SETUP_AW, NRF24_ADDRESS_3_BYTES, 1, 0);
      break;
    case 4:
      state = _mod->SPIsetRegValue(NRF24_REG_SETUP_AW, NRF24_ADDRESS_4_BYTES, 1, 0);
      break;
    case 5:
      state = _mod->SPIsetRegValue(NRF24_REG_SETUP_AW, NRF24_ADDRESS_5_BYTES, 1, 0);
      break;
    default:
      return(ERR_INVALID_ADDRESS_WIDTH);
  }
  
  // save address width
  _addrWidth = addrWidth;
  
  return(state);
}

int16_t nRF24::setReceivePipe(uint8_t pipeNum, uint8_t* addr) {
  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
     return(state);
  }
  
  // reverse byte order (LSB must be written first)
  uint8_t* addrReversed = new uint8_t[_addrWidth];
  for(uint8_t i = 0; i < _addrWidth; i++) {
    addrReversed[i] = addr[_addrWidth - 1 - i];
  }
  
  // write full pipe 0 - 1 address and enable the pipe
  switch(pipeNum) {
    case 0:
      _mod->SPIwriteRegisterBurst(NRF24_REG_RX_ADDR_P0, addrReversed, _addrWidth);
      state |= _mod->SPIsetRegValue(NRF24_REG_EN_RXADDR, NRF24_P0_ON, 0, 0);
    case 1:
      _mod->SPIwriteRegisterBurst(NRF24_REG_RX_ADDR_P1, addrReversed, _addrWidth);
      state |= _mod->SPIsetRegValue(NRF24_REG_EN_RXADDR, NRF24_P1_ON, 1, 1);
      break;
    default:
      return(ERR_INVALID_PIPE_NUMBER);
  }
  
  return(state);
}

int16_t nRF24::setReceivePipe(uint8_t pipeNum, uint8_t addrByte) {
  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
     return(state);
  }
  
  // write unique pipe 2 - 5 address and enable the pipe
  switch(pipeNum) {
    case 2:
      state = _mod->SPIsetRegValue(NRF24_REG_RX_ADDR_P2, addrByte);
      state |= _mod->SPIsetRegValue(NRF24_REG_EN_RXADDR, NRF24_P2_ON, 2, 2);
      break;
    case 3:
      state = _mod->SPIsetRegValue(NRF24_REG_RX_ADDR_P3, addrByte);
      state |= _mod->SPIsetRegValue(NRF24_REG_EN_RXADDR, NRF24_P3_ON, 3, 3);
      break;
    case 4:
      state = _mod->SPIsetRegValue(NRF24_REG_RX_ADDR_P4, addrByte);
      state |= _mod->SPIsetRegValue(NRF24_REG_EN_RXADDR, NRF24_P4_ON, 4, 4);
      break;
    case 5:
      state = _mod->SPIsetRegValue(NRF24_REG_RX_ADDR_P5, addrByte);
      state |= _mod->SPIsetRegValue(NRF24_REG_EN_RXADDR, NRF24_P5_ON, 5, 5);
      break;
    default:
      return(ERR_INVALID_PIPE_NUMBER);
  }
  
  return(state);
}

int16_t nRF24::disablePipe(uint8_t pipeNum) {
  // set mode to standby
  int16_t state = standby();
  if(state != ERR_NONE) {
     return(state);
  }
  
  switch(pipeNum) {
    case 0:
      state = _mod->SPIsetRegValue(NRF24_REG_EN_RXADDR, NRF24_P0_OFF, 0, 0);
      break;
    case 1:
      state = _mod->SPIsetRegValue(NRF24_REG_EN_RXADDR, NRF24_P1_OFF, 1, 1);
      break;
    case 2:
      state = _mod->SPIsetRegValue(NRF24_REG_EN_RXADDR, NRF24_P2_OFF, 2, 2);
      break;
    case 3:
      state = _mod->SPIsetRegValue(NRF24_REG_EN_RXADDR, NRF24_P3_OFF, 3, 3);
      break;
    case 4:
      state = _mod->SPIsetRegValue(NRF24_REG_EN_RXADDR, NRF24_P4_OFF, 4, 4);
      break;
    case 5:
      state = _mod->SPIsetRegValue(NRF24_REG_EN_RXADDR, NRF24_P5_OFF, 5, 5);
      break;
    default:
      return(ERR_INVALID_PIPE_NUMBER);
  }
  
  return(state);
}

void nRF24::SPIreadRxPayload(uint8_t numBytes, uint8_t* inBytes) {
  digitalWrite(_mod->getCs(), LOW);
  SPI.transfer(NRF24_CMD_READ_RX_PAYLOAD);
  for(uint8_t i = 0; i < numBytes; i++) {
    inBytes[i] = SPI.transfer(0x00);
  }
  digitalWrite(_mod->getCs(), HIGH);
}

void nRF24::SPIwriteTxPayload(uint8_t* data, uint8_t numBytes) {
  digitalWrite(_mod->getCs(), LOW);
  SPI.transfer(NRF24_CMD_WRITE_TX_PAYLOAD);
  for(uint8_t i = 0; i < numBytes; i++) {
    SPI.transfer(data[i]);
  }
  digitalWrite(_mod->getCs(), HIGH);
}

void nRF24::clearIRQ() {
  _mod->SPIsetRegValue(NRF24_REG_CONFIG, NRF24_MASK_RX_DR_IRQ_OFF | NRF24_MASK_TX_DS_IRQ_OFF | NRF24_MASK_MAX_RT_IRQ_OFF, 6, 4);
}
