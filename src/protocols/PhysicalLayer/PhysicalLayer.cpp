#include "PhysicalLayer.h"

#include <string.h>

PhysicalLayer::PhysicalLayer() {
  this->freqStep = 1;
  this->maxPacketLength = 1;
  #if !RADIOLIB_EXCLUDE_DIRECT_RECEIVE
  this->bufferBitPos = 0;
  this->bufferWritePos = 0;
  #endif
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t PhysicalLayer::transmit(__FlashStringHelper* fstr, uint8_t addr) {
  // read flash string length
  size_t len = 0;
  PGM_P p = reinterpret_cast<PGM_P>(fstr);
  while(true) {
    char c = RADIOLIB_NONVOLATILE_READ_BYTE(p++);
    len++;
    if(c == '\0') {
      break;
    }
  }

  // dynamically allocate memory
  #if RADIOLIB_STATIC_ONLY
    char str[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    char* str = new char[len];
  #endif

  // copy string from flash
  p = reinterpret_cast<PGM_P>(fstr);
  for(size_t i = 0; i < len; i++) {
    str[i] = RADIOLIB_NONVOLATILE_READ_BYTE(p + i);
  }

  // transmit string
  int16_t state = transmit(str, addr);
  #if !RADIOLIB_STATIC_ONLY
    delete[] str;
  #endif
  return(state);
}

int16_t PhysicalLayer::transmit(String& str, uint8_t addr) {
  return(transmit(str.c_str(), addr));
}
#endif

int16_t PhysicalLayer::transmit(const char* str, uint8_t addr) {
  return(transmit(reinterpret_cast<uint8_t*>(const_cast<char*>(str)), strlen(str), addr));
}

int16_t PhysicalLayer::transmit(const uint8_t* data, size_t len, uint8_t addr) {
  (void)data;
  (void)len;
  (void)addr;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t PhysicalLayer::receive(String& str, size_t len, RadioLibTime_t timeout) {
  int16_t state = RADIOLIB_ERR_NONE;

  // user can override the length of data to read
  size_t length = len;

  // build a temporary buffer
  #if RADIOLIB_STATIC_ONLY
    uint8_t data[RADIOLIB_STATIC_ARRAY_SIZE + 1];
  #else
    uint8_t* data = NULL;
    if(length == 0) {
      data = new uint8_t[this->maxPacketLength + 1];
    } else {
      data = new uint8_t[length + 1];
    }
    RADIOLIB_ASSERT_PTR(data);
  #endif

  // attempt packet reception
  state = receive(data, length, timeout);

  // any of the following leads to at least some data being available
  // let's leave the decision of whether to keep it or not up to the user
  if((state == RADIOLIB_ERR_NONE) || (state == RADIOLIB_ERR_CRC_MISMATCH) || (state == RADIOLIB_ERR_LORA_HEADER_DAMAGED)) {
    // read the number of actually received bytes (for unknown packets)
    if(len == 0) {
      length = getPacketLength(false);
    }

    // add null terminator
    data[length] = 0;

    // initialize Arduino String class
    str = String(reinterpret_cast<char*>(data));
  }

  // deallocate temporary buffer
  #if !RADIOLIB_STATIC_ONLY
    delete[] data;
  #endif

  return(state);
}
#endif

int16_t PhysicalLayer::receive(uint8_t* data, size_t len, RadioLibTime_t timeout) {
  (void)data;
  (void)len;
  (void)timeout;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::sleep() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::standby() {
  return(standby(RADIOLIB_STANDBY_DEFAULT));
}

int16_t PhysicalLayer::standby(uint8_t mode) {
  (void)mode;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::startReceive() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::startReceive(uint32_t timeout, RadioLibIrqFlags_t irqFlags, RadioLibIrqFlags_t irqMask, size_t len) {
  RadioModeConfig_t cfg = {
    .receive = {
      .timeout = timeout,
      .syncSymbols = 0,
      .irqFlags = irqFlags,
      .irqMask = irqMask,
      .len = len,
    }
  };

  int16_t state = this->stageMode(RADIOLIB_RADIO_MODE_RX, &cfg);
  RADIOLIB_ASSERT(state);
  return(this->launchMode());
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t PhysicalLayer::startTransmit(String& str, uint8_t addr) {
  return(startTransmit(str.c_str(), addr));
}
#endif

int16_t PhysicalLayer::startTransmit(const char* str, uint8_t addr) {
  return(startTransmit(reinterpret_cast<uint8_t*>(const_cast<char*>(str)), strlen(str), addr));
}

int16_t PhysicalLayer::startTransmit(const uint8_t* data, size_t len, uint8_t addr) {
  RadioModeConfig_t cfg = {
    .transmit = {
      .data = data,
      .len = len,
      .addr = addr,
    }
  };

  int16_t state = this->stageMode(RADIOLIB_RADIO_MODE_TX, &cfg);
  RADIOLIB_ASSERT(state);
  return(this->launchMode());
}

int16_t PhysicalLayer::finishTransmit() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::finishReceive() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t PhysicalLayer::readData(String& str, size_t len) {
  int16_t state = RADIOLIB_ERR_NONE;

  // read the number of actually received bytes
  size_t length = getPacketLength();

  if((len < length) && (len != 0)) {
    // user requested less bytes than were received, this is allowed (but frowned upon)
    // requests for more data than were received will only return the number of actually received bytes (unlike PhysicalLayer::receive())
    length = len;
  }

  // build a temporary buffer
  #if RADIOLIB_STATIC_ONLY
    uint8_t data[RADIOLIB_STATIC_ARRAY_SIZE + 1];
  #else
    uint8_t* data = new uint8_t[length + 1];
    RADIOLIB_ASSERT_PTR(data);
  #endif

  // read the received data
  state = readData(data, length);

  // any of the following leads to at least some data being available
  // let's leave the decision of whether to keep it or not up to the user
  if((state == RADIOLIB_ERR_NONE) || (state == RADIOLIB_ERR_CRC_MISMATCH) || (state == RADIOLIB_ERR_LORA_HEADER_DAMAGED)) {
    // add null terminator
    data[length] = 0;

    // initialize Arduino String class
    str = String(reinterpret_cast<char*>(data));
  }

  // deallocate temporary buffer
  #if !RADIOLIB_STATIC_ONLY
    delete[] data;
  #endif

  return(state);
}
#endif

int16_t PhysicalLayer::readData(uint8_t* data, size_t len) {
  (void)data;
  (void)len;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::transmitDirect(uint32_t frf) {
  (void)frf;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::receiveDirect() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setFrequency(uint32_t freq) {
  (void)freq;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setBitRate(uint32_t br) {
  (void)br;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setFrequencyDeviation(uint32_t freqDev) {
  (void)freqDev;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setSpreadingFactor(uint8_t sf) {
  (void)sf;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setBandwidth(uint32_t bw) {
  (void)bw;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setCodingRate(uint8_t cr, bool longInterleave) {
  (void)cr;
  (void)longInterleave;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setLrFhssConfig(uint8_t bw, uint8_t cr, bool narrowGrid, uint8_t hdrCount, uint16_t hopSeed) {
  (void)bw;
  (void)cr;
  (void)narrowGrid;
  (void)hdrCount;
  (void)hopSeed;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setDataShaping(uint8_t sh) {
  (void)sh;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setEncoding(uint8_t encoding) {
  (void)encoding;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::invertIQ(bool enable) {
  (void)enable;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setOutputPower(int8_t power) {
  (void)power;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setSyncWord(uint8_t* sync, size_t len) {
  (void)sync;
  (void)len;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setPreambleLength(size_t len, size_t detLen) {
  (void)len;
  (void)detLen;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setDataRate(DataRate_t dr, ModemType_t modem) {
  // get the current modem
  ModemType_t currentModem;
  int16_t state = this->getModem(&currentModem);
  RADIOLIB_ASSERT(state);

  // switch over if the requested modem is different
  if(modem != RADIOLIB_MODEM_NONE && modem != currentModem) {
    state = this->standby();
    RADIOLIB_ASSERT(state);
    state = this->setModem(modem);
    RADIOLIB_ASSERT(state);
  }
  
  if(modem == RADIOLIB_MODEM_NONE) {
    modem = currentModem;
  }

  // select interpretation based on modem
  if(modem == RADIOLIB_MODEM_FSK) {
    // set the bit rate
    state = this->setBitRate(dr.fsk.bitRate);
    RADIOLIB_ASSERT(state);

    // set the frequency deviation
    state = this->setFrequencyDeviation(dr.fsk.freqDev);

  } else if(modem == RADIOLIB_MODEM_LORA) {
    // set the spreading factor
    state = this->setSpreadingFactor(dr.lora.spreadingFactor);
    RADIOLIB_ASSERT(state);

    // set the bandwidth
    state = this->setBandwidth(dr.lora.bandwidth);
    RADIOLIB_ASSERT(state);

    // set the coding rate
    state = this->setCodingRate(dr.lora.codingRate);
  
  } else if(modem == RADIOLIB_MODEM_LRFHSS) {
    // set the basic config
    state = this->setLrFhssConfig(dr.lrFhss.bw, dr.lrFhss.cr, dr.lrFhss.narrowGrid);
  }

  return(state);
}

int16_t PhysicalLayer::checkDataRate(DataRate_t dr, ModemType_t modem) {
  (void)dr;
  (void)modem;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

size_t PhysicalLayer::getPacketLength(bool update) {
  (void)update;
  // technically this should be returning some unsigned value, not a negative status code
  // however, 0 could be misinterpreted in this case
  // the value of RADIOLIB_ERR_UNSUPPORTED (-25) when converted to 32-bit unsigned int is 4294967271
  // that is conmfortably out of range for any possible packet length
  return((size_t)RADIOLIB_ERR_UNSUPPORTED);
}

float PhysicalLayer::getRSSI() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

float PhysicalLayer::getSNR() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

float PhysicalLayer::getNumSymbols(DataRate_t dr, PacketConfig_t pc, size_t len) {
  uint8_t sfCoeff1_x4 = 17; // (4.25 * 4)
  uint8_t sfCoeff2 = 8;
  if(dr.lora.spreadingFactor == 5 || dr.lora.spreadingFactor == 6) {
    sfCoeff1_x4 = 25; // 6.25 * 4
    sfCoeff2 = 0;
  }
  uint8_t sfDivisor = 4*dr.lora.spreadingFactor;
  if(pc.lora.ldrOptimize) {
    sfDivisor = 4*(dr.lora.spreadingFactor - 2);
  }
  const int8_t bitsPerCrc = 16;
  const int8_t N_symbol_header = pc.lora.implicitHeader ? 0 : 20;

  // numerator of equation in section 6.1.4 of SX1268 datasheet v1.1 (might not actually be bitcount, but it has len * 8)
  int16_t bitCount = (int16_t) 8 * len + pc.lora.crcEnabled * bitsPerCrc - 4 * dr.lora.spreadingFactor  + sfCoeff2 + N_symbol_header;
  if(bitCount < 0) {
    bitCount = 0;
  }

  // add (sfDivisor) - 1 to the numerator to give integer CEIL(...)
  uint16_t nPreCodedSymbols = (bitCount + (sfDivisor - 1)) / (sfDivisor);

  // preamble can be 65k, therefore nSymbol_x4 needs to be 32 bit
  uint32_t nSymbol_x4 = (pc.lora.preambleLength + 8) * 4 + sfCoeff1_x4 + nPreCodedSymbols * dr.lora.codingRate * 4;
  return((float)nSymbol_x4 / 4.0f);
}

RadioLibTime_t PhysicalLayer::calculateTimeOnAir(ModemType_t modem, DataRate_t dr, PacketConfig_t pc, size_t len) {
  // check active modem
  if(modem == ModemType_t::RADIOLIB_MODEM_LORA) {
    // everything is in microseconds to allow integer arithmetic
    // some constants have .25, these are multiplied by 4, and have _x4 postfix to indicate that fact
    uint32_t symbolLength_us = (RADIOLIB_UNIT_MEGA(1) << dr.lora.spreadingFactor) / dr.lora.bandwidth;
    size_t nSymbol_x4 = PhysicalLayer::getNumSymbols(dr, pc, len) * 4;
    return((symbolLength_us * nSymbol_x4) / 4);

  } else if(modem == ModemType_t::RADIOLIB_MODEM_FSK) {
    size_t num_bits = ((uint32_t)pc.fsk.crcLength * 8UL) + (uint32_t)pc.fsk.syncWordLength + (uint32_t)pc.fsk.preambleLength + ((uint32_t)len * 8UL);
    return((num_bits * RADIOLIB_UNIT_MEGA(1)) / dr.fsk.bitRate);

  } else if(modem == ModemType_t::RADIOLIB_MODEM_LRFHSS) {
    // calculate the number of bits based on coding rate
    uint16_t N_bits;
    switch(dr.lrFhss.cr) {
      case RADIOLIB_LR_FHSS_CR_5_6:
        N_bits = ((len * 6) + 4) / 5; // this is from the official LR11xx driver, but why the extra +4?
        break;
      case RADIOLIB_LR_FHSS_CR_2_3:
        N_bits = (len * 3) / 2;
        break;
      case RADIOLIB_LR_FHSS_CR_1_2:
        N_bits = len * 2;
        break;
      case RADIOLIB_LR_FHSS_CR_1_3:
        N_bits = len * 3;
        break;
      default:
        return(RADIOLIB_ERR_INVALID_CODING_RATE);
    }

    // calculate number of bits when accounting for unaligned last block
    uint16_t N_payBits = (N_bits / RADIOLIB_LR_FHSS_FRAG_BITS) * RADIOLIB_LR_FHSS_BLOCK_BITS;
    uint16_t N_lastBlockBits = N_bits % RADIOLIB_LR_FHSS_FRAG_BITS;
    if(N_lastBlockBits) {
      N_payBits += N_lastBlockBits + 2;
    }

    // add header bits
    uint16_t N_totalBits = (RADIOLIB_LR_FHSS_HEADER_BITS * pc.lrFhss.hdrCount) + N_payBits;
    return(((uint32_t)N_totalBits * 8 * 1000000UL) * (1.0f / RADIOLIB_LR_FHSS_BIT_RATE));
  
  } else {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  return(0);
}

RadioLibTime_t PhysicalLayer::getTimeOnAir(size_t len) {
  (void)len;
  return(0);
}

RadioLibTime_t PhysicalLayer::calculateRxTimeout(RadioLibTime_t timeoutUs) {
  (void)timeoutUs;
  return(0); 
}

uint32_t PhysicalLayer::getIrqMapped(RadioLibIrqFlags_t irq) {
  // iterate over all set bits and build the module-specific flags
  uint32_t irqRaw = 0;
  for(uint8_t i = 0; i < 8*(sizeof(RadioLibIrqFlags_t)); i++) {
    if((irq & (uint32_t)(1UL << i)) && (this->irqMap[i] != RADIOLIB_IRQ_NOT_SUPPORTED)) {
      irqRaw |= this->irqMap[i];
    }
  }

  return(irqRaw);
}

int16_t PhysicalLayer::checkIrq(RadioLibIrqType_t irq) {
  if((irq > RADIOLIB_IRQ_TIMEOUT) || (this->irqMap[irq] == RADIOLIB_IRQ_NOT_SUPPORTED)) {
    return(RADIOLIB_ERR_UNSUPPORTED);
  }
  
  return((getIrqFlags() & this->irqMap[irq]) != 0);
}

int16_t PhysicalLayer::setIrq(RadioLibIrqFlags_t irq) {
  return(setIrqFlags(getIrqMapped(irq)));
}

int16_t PhysicalLayer::clearIrq(RadioLibIrqFlags_t irq) {
  return(clearIrqFlags(getIrqMapped(irq)));
}

uint32_t PhysicalLayer::getIrqFlags() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setIrqFlags(uint32_t irq) {
  (void)irq;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::clearIrqFlags(uint32_t irq) {
  (void)irq;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::startChannelScan() {
  return(RADIOLIB_ERR_UNSUPPORTED); 
}

int16_t PhysicalLayer::startChannelScan(const ChannelScanConfig_t &config) {
  (void)config;
  return(RADIOLIB_ERR_UNSUPPORTED); 
}

int16_t PhysicalLayer::getChannelScanResult() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::scanChannel() {
  return(RADIOLIB_ERR_UNSUPPORTED); 
}

int16_t PhysicalLayer::scanChannel(const ChannelScanConfig_t &config) {
  (void)config;
  return(RADIOLIB_ERR_UNSUPPORTED); 
}

int32_t PhysicalLayer::random(int32_t max) {
  if(max == 0) {
    return(0);
  }

  // get random bytes from the radio
  uint8_t randBuff[4];
  for(uint8_t i = 0; i < 4; i++) {
    randBuff[i] = randomByte();
  }

  // create 32-bit TRNG number
  int32_t randNum = ((int32_t)randBuff[0] << 24) | ((int32_t)randBuff[1] << 16) | ((int32_t)randBuff[2] << 8) | ((int32_t)randBuff[3]);
  if(randNum < 0) {
    randNum *= -1;
  }
  return(randNum % max);
}

int32_t PhysicalLayer::random(int32_t min, int32_t max) {
  if(min >= max) {
    return(min);
  }

  return(PhysicalLayer::random(max - min) + min);
}

uint8_t PhysicalLayer::randomByte() {
  return(0);
}

int16_t PhysicalLayer::startDirect() {
  // disable encodings
  int16_t state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  // disable shaping
  state = setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);

  // set frequency deviation to the lowest possible value
  state = setFrequencyDeviation(0);
  return(state);
}

#if !RADIOLIB_EXCLUDE_DIRECT_RECEIVE
int16_t PhysicalLayer::available() {
  return(this->bufferWritePos);
}

void PhysicalLayer::dropSync() {
  if(this->directSyncWordLen > 0) {
    this->gotSync = false;
    this->syncBuffer = 0;
  }
}

uint8_t PhysicalLayer::read(bool drop) {
  if(drop) {
    dropSync();
  }
  this->bufferWritePos--;
  return(this->buffer[this->bufferReadPos++]);
}

int16_t PhysicalLayer::setDirectSyncWord(uint32_t syncWord, uint8_t len) {
  if(len > 32) {
    return(RADIOLIB_ERR_INVALID_SYNC_WORD);
  }
  this->directSyncWordMask = 0xFFFFFFFF >> (32 - len);
  this->directSyncWordLen = len;
  this->directSyncWord = syncWord;

  // override sync word matching when length is set to 0
  if(this->directSyncWordLen == 0) {
    this->gotSync = true;
  }

  return(RADIOLIB_ERR_NONE);
}

void PhysicalLayer::updateDirectBuffer(uint8_t bit) {
  // check sync word
  if(!this->gotSync) {
    this->syncBuffer <<= 1;
    this->syncBuffer |= bit;

    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("S\t%lu", (long unsigned int)this->syncBuffer);

    if((this->syncBuffer & this->directSyncWordMask) == this->directSyncWord) {
      this->gotSync = true;
      this->bufferWritePos = 0;
      this->bufferReadPos = 0;
      this->bufferBitPos = 0;
    }

  } else {
    // save the bit
    if(bit) {
      this->buffer[this->bufferWritePos] |= 0x01 << this->bufferBitPos;
    } else {
      this->buffer[this->bufferWritePos] &= ~(0x01 << this->bufferBitPos);
    }
    this->bufferBitPos++;

    // check complete byte
    if(this->bufferBitPos == 8) {
      this->buffer[this->bufferWritePos] = rlb_reflect(this->buffer[this->bufferWritePos], 8);
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("R\t%X", this->buffer[this->bufferWritePos]);

      this->bufferWritePos++;
      this->bufferBitPos = 0;
    }
  }
}

void PhysicalLayer::setDirectAction(void (*func)(void)) {
  (void)func;
}

void PhysicalLayer::readBit(uint32_t pin) {
  (void)pin;
}

#endif

void PhysicalLayer::setIrqAction(void (*func)(void), bool rising) {
  Module* mod = getMod();
  mod->hal->attachInterrupt(mod->hal->pinToInterrupt(mod->getIrq()), func, 
    rising ? mod->hal->GpioInterruptRising : mod->hal->GpioInterruptFalling);
}

void PhysicalLayer::clearIrqAction() {
  Module* mod = getMod();
  mod->hal->detachInterrupt(mod->hal->pinToInterrupt(mod->getIrq()));
}

int16_t PhysicalLayer::setModem(ModemType_t modem) {
  (void)modem;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::getModem(ModemType_t* modem) {
  (void)modem;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::getMode(RadioModeType_t* mode) {
  (void)mode;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::stageMode(RadioModeType_t mode, RadioModeConfig_t* cfg) {
  (void)mode;
  (void)cfg;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::launchMode() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::writeTxBuffer(const uint8_t* data, size_t len) {
  (void)data;
  (void)len;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::clearTxBuffer() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

#if RADIOLIB_INTERRUPT_TIMING
void PhysicalLayer::setInterruptSetup(void (*func)(uint32_t)) {
  Module* mod = getMod();
  mod->TimerSetupCb = func;
}

void PhysicalLayer::setTimerFlag() {
  Module* mod = getMod();
  mod->TimerFlag = true;
}
#endif

int16_t PhysicalLayer::calculateRxDutyCycle(size_t txPreLen, size_t rxPreLen, uint16_t minSymbols, DataRate_t* dr, uint32_t* wakePeriod, uint32_t* sleepPeriod) {
  RADIOLIB_ASSERT_PTR(dr);
  RADIOLIB_ASSERT_PTR(wakePeriod);
  RADIOLIB_ASSERT_PTR(sleepPeriod);
  
  uint16_t senderPreambleLength = txPreLen;
  if(senderPreambleLength == 0) {
    senderPreambleLength = rxPreLen;
  } else if(senderPreambleLength > rxPreLen) {
    // the unit must be configured to expect a preamble length at least as long as the sender is using
    return(RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);
  }
  if(minSymbols == 0) {
    if(dr->lora.spreadingFactor <= 6) {
      minSymbols = 12;
    } else {
      minSymbols = 8;
    }
  }

  // if we're not to sleep at all, just use the standard startReceive
  if(2 * minSymbols > senderPreambleLength) {
    *sleepPeriod = 0;
    RADIOLIB_DEBUG_BASIC_PRINTLN("Auto sleep period too short");
    return(RADIOLIB_ERR_NONE);
  }

  // worst case is that the sender starts transmitting when we're just less than minSymbols from going back to sleep.
  // in this case, we don't catch minSymbols before going to sleep,
  // so we must be awake for at least that long before the sender stops transmitting.
  uint16_t sleepSymbols = senderPreambleLength - 2 * minSymbols;

  uint32_t symbolLength = ((uint32_t)(10 * 1000) << dr->lora.spreadingFactor) / (10 * dr->lora.bandwidth);
  *sleepPeriod = symbolLength * sleepSymbols;
  RADIOLIB_DEBUG_BASIC_PRINTLN("Auto sleep period: %lu", (long unsigned int)*sleepPeriod);

  // when the unit detects a preamble, it starts a timer that will timeout if it doesn't receive a header in time.
  // the duration is sleepPeriod + 2 * wakePeriod.
  // The sleepPeriod doesn't take into account shutdown and startup time for the unit (~1ms)
  // We need to ensure that the timeout is longer than senderPreambleLength.
  // So we must satisfy: wakePeriod > (preamblePeriod - (sleepPeriod - 1000)) / 2. (A)
  // we also need to ensure the unit is awake to see at least minSymbols. (B)
  *wakePeriod = RADIOLIB_MAX(
    (symbolLength * (senderPreambleLength + 1) - (*sleepPeriod - 1000)) / 2, // (A)
    symbolLength * (minSymbols + 1)); //(B)
  RADIOLIB_DEBUG_BASIC_PRINTLN("Auto wake period: %lu", (long unsigned int)*wakePeriod);

  return(RADIOLIB_ERR_NONE);
}

int16_t PhysicalLayer::setOutputPowerOffset(int8_t offset) {
  memset(this->paOffsetLut, offset, sizeof(this->paOffsetLut));
  return(RADIOLIB_ERR_NONE);
}

int16_t PhysicalLayer::setOutputPowerOffset(const int8_t* lut, size_t steps) {
  RADIOLIB_ASSERT_PTR(lut);
  if(steps > sizeof(this->paOffsetLut)) {
    return(RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  }
  memcpy(this->paOffsetLut, lut, steps);
  return(RADIOLIB_ERR_NONE);
}

int16_t PhysicalLayer::applyOutputPowerOffset(int8_t base, const int8_t* pwrIn, int8_t* pwrOut) {
  RADIOLIB_ASSERT_PTR(pwrIn);
  RADIOLIB_ASSERT_PTR(pwrOut);
  
  // update the minimum and maximum supported power
  this->powerMin = base + this->paOffsetLut[0];
  this->powerMax = base + this->paSteps - 1 + this->paOffsetLut[this->paSteps - 1];

  // check the range
  RADIOLIB_CHECK_RANGE(*pwrIn, this->powerMin, this->powerMax, RADIOLIB_ERR_INVALID_OUTPUT_POWER);

  // base is the minimum value that the radio can output on its own, pwrIn is the request from user
  // find first entry in the LUT that achieves the target output
  uint8_t idx = 0;
  bool found = false;
  for(; idx < this->paSteps; idx++) {
    if(base + idx + this->paOffsetLut[idx] >= *pwrIn) {
      found = true;
      break;
    }
  }
  if(!found) { 
    RADIOLIB_DEBUG_BASIC_PRINTLN("Found no setting for PA offset to reach %d dBm!", *pwrIn);
    return(RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  }

  // output is the base of the table and the index we found 
  *pwrOut = base + idx;
  return(RADIOLIB_ERR_NONE);
}

int16_t PhysicalLayer::findRxBw(uint32_t rxBw, const uint32_t* rxBwAvg, const uint8_t* lut, size_t lutSize, uint32_t rxBwMax, uint8_t* val) {
  // iterate through the table and find whether the user-provided value
  // is lower than the pre-computed average of the adjacent bandwidth values
  // if it is, we consider that to be a match even though the actual value is not precise
  for(size_t i = 0; i < (lutSize - 1); i++) {
    if(rxBw < rxBwAvg[i]) {
      *val = lut[i];
      return(RADIOLIB_ERR_NONE);
    }
  }

  // if nothing matched up to here, match with the last value
  if(rxBw <= rxBwMax) {
    *val = lut[lutSize - 1];
    return(RADIOLIB_ERR_NONE);
  }

  return(RADIOLIB_ERR_INVALID_RX_BANDWIDTH);
}
