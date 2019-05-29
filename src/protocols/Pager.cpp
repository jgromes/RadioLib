#include "Pager.h"

PagerClient::PagerClient(PhysicalLayer* phy) {
  _phy = phy;
}

int16_t PagerClient::begin(float base, uint16_t speed) {
  // calculate duration of 1 bit in us
  _bitDuration = (uint32_t)1000000/speed;

  // calculate 24-bit frequency
  _base = (base * (uint32_t(1) << _phy->getDivExponent())) / _phy->getCrystalFreq();

  // calculate module carrier frequency resolution
  uint16_t step = round((_phy->getCrystalFreq() * 1000000) / (uint32_t(1) << _phy->getDivExponent()));

  // calculate raw frequency shift
  _shift = FREQ_SHIFT_HZ/step;

  // set module frequency deviation to 0
  int16_t state = _phy->setFrequencyDeviation(0);

  return(state);
}

int16_t PagerClient::transmit(String& str, uint32_t addr, uint8_t encoding) {
  return(PagerClient::transmit(str.c_str(), addr, encoding));
}

int16_t PagerClient::transmit(const char* str, uint32_t addr, uint8_t encoding) {
  return(PagerClient::transmit((uint8_t*)str, strlen(str), addr, encoding));
}

int16_t PagerClient::transmit(uint8_t* data, size_t len, uint32_t addr, uint8_t encoding) {
  // get symbol bit length based on encoding
  uint8_t symbolLength = 0;
  if(encoding == BCD) {
    symbolLength = 4;

    // replace ASCII characters with BCD representations
    for(size_t i = 0; i < len; i++) {
      switch(data[i]) {
        case '*':
          data[i] = 0x0A;
          break;
        case 'U':
          data[i] = 0x0B;
          break;
        case ' ':
          data[i] = 0x0C;
          break;
        case '-':
          data[i] = 0x0D;
          break;
        case ')':
          data[i] = 0x0E;
          break;
        case '(':
          data[i] = 0x0F;
          break;
        default:
          data[i] -= '0';
          break;
      }
    }

  } else if(encoding == ASCII) {
    symbolLength = 7;
  } else {
    return(ERR_UNKNOWN);
  }

  // get target position in batch (3 LSB from address determine frame position in batch)
  uint8_t framePos = addr & 0b0000000000000111;

  // get address that will be written into address frame
  uint16_t frameAddr = (addr & 0b1111111111111000) >> 3;

  // calculate the number of 20-bit data blocks
  size_t numDataBlocks = (len * symbolLength) / MESSAGE_BITS_LENGTH;
  if((len * symbolLength) % MESSAGE_BITS_LENGTH > 0) {
    numDataBlocks += 1;
  }

  // calculate number of batches
  size_t numBatches = (1 + framePos + numDataBlocks) / 16 + 1;
  if((1 + numDataBlocks) % 16 == 0) {
    numBatches -= 1;
  }

  // calculate message length in 32-bit code words
  size_t msgLen = PREAMBLE_LENGTH + (1 + 16) * numBatches;

  // build the message
  uint32_t* msg = new uint32_t[msgLen];
  // TODO: BCD padding?
  memset(msg, 0x00, msgLen);

  // set preamble
  for(size_t i = 0; i < PREAMBLE_LENGTH; i++) {
    msg[i] = PREAMBLE_CODE_WORD;
  }

  // set frame synchronization code words
  for(size_t i = 0; i < numBatches; i++) {
    msg[PREAMBLE_LENGTH + i*(1 + 16)] = FRAME_SYNC_CODE_WORD;
  }

  // set unused code words to idle
  for(size_t i = 0; i < framePos; i++) {
    msg[PREAMBLE_LENGTH + 1 + i] = IDLE_CODE_WORD;
  }

  // write address code word
  msg[PREAMBLE_LENGTH + 1 + framePos] = addParity(encodeBCH((addr << 1) | ADDRESS_CODE_WORD));

  // split the data into 20-bit blocks
  size_t bitPos = MESSAGE_BITS_LENGTH;
  size_t blockPos = PREAMBLE_LENGTH + 1;
  for(size_t i = 0; i < len; i++) {
    // check if the next data symbol fits into the remaining space in current 20-bit block
    if(bitPos >= symbolLength) {
      // insert the whole data symbol into current block
      msg[blockPos] |= (uint32_t)data[i] << (bitPos - symbolLength);
      bitPos -= symbolLength;
    } else {
      // split the symbol between two blocks
      uint8_t msbPart = data[i];
      size_t lsbLen =  symbolLength - bitPos;
      msg[blockPos] |= msbPart >> lsbLen;
      blockPos++;
      bitPos = MESSAGE_BITS_LENGTH;
      uint8_t lsbPart = data[i] & ((1 << lsbLen) - 1);
      msg[blockPos] |= (uint32_t)lsbPart << (bitPos - lsbLen);
      bitPos -= lsbLen;
    }
  }

  // write message code words

  // transmit the message
  PagerClient::write(msg, msgLen);

  delete[] msg;

  // turn transmitter off
  _phy->standby();

  return(ERR_NONE);
}

void PagerClient::write(uint32_t* data, size_t len) {
  // write code words from buffer
  for(size_t i = 0; i < len; i++) {
    PagerClient::write(data[i]);
  }
}

void PagerClient::write(uint32_t codeWord) {
  // write single code word
  for(uint8_t i = 0; i <= 31; i++) {
    uint32_t mask = (uint32_t)0x01 << i;
    if(codeWord & mask) {
      // send 1
      uint32_t start = micros();
      _phy->transmitDirect(_base + _shift);
      while(micros() - start < _bitDuration);
    } else {
      // send 0
      uint32_t start = micros();
      _phy->transmitDirect(_base - _shift);
      while(micros() - start < _bitDuration);
    }
  }
}

uint32_t PagerClient::encodeBCH(uint32_t data) {
  return(data);
}


uint32_t PagerClient::addParity(uint32_t msg) {
  return(msg);
}
