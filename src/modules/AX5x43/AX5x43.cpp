#include "AX5x43.h"
#if !defined(RADIOLIB_EXCLUDE_AX5X43)

AX5x43::AX5x43(Module* module) : PhysicalLayer(RADIOLIB_AX5X43_FREQUENCY_STEP_SIZE, RADIOLIB_AX5X43_MAX_PACKET_LENGTH) {
  mod = module;
}

Module* AX5x43::getMod() {
  return(mod);
}

int16_t AX5x43::begin(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength) {
  // set module properties
  mod->SPIreadCommand |= RADIOLIB_AX5X43_SPI_LONG_ADDR_CMD;
  mod->SPIwriteCommand |= RADIOLIB_AX5X43_SPI_LONG_ADDR_CMD;
  mod->SPIaddrWidth = 16;
  mod->init();
  mod->pinMode(mod->getIrq(), INPUT);

  // try to find the CC1101 chip
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    int16_t version = getChipVersion();
    if(version == RADIOLIB_AX5X43_SILICONREV) {
      flagFound = true;
    } else {
      #if defined(RADIOLIB_DEBUG)
        RADIOLIB_DEBUG_PRINT(F("AX5x43 not found! ("));
        RADIOLIB_DEBUG_PRINT(i + 1);
        RADIOLIB_DEBUG_PRINT(F(" of 10 tries) RADIOLIB_AX5X43_REG_REVISION == "));

        char buffHex[7];
        sprintf(buffHex, "0x%04X", version);
        RADIOLIB_DEBUG_PRINT(buffHex);
        RADIOLIB_DEBUG_PRINT(F(", expected 0x0051"));
        RADIOLIB_DEBUG_PRINTLN();
      #endif
      mod->delay(10);
      i++;
    }
  }

  if(!flagFound) {
    RADIOLIB_DEBUG_PRINTLN(F("No AX5x43 found!"));
    mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  } else {
    RADIOLIB_DEBUG_PRINTLN(F("M\tAX5x43"));
  }

  // setting initial frequency to 0 will force setFrequency to perform VCO ranging
  freq = 0.0;

  // reset the module
  int16_t state = reset();
  RADIOLIB_ASSERT(state);

  // set mode to standby
  state = standby();
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = config();
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t AX5x43::reset() {
  // set the reset bit - only check the MSB, since mode will be set to power down
  int16_t state = mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_PWR_MODE, RADIOLIB_AX5X43_PWR_MODE_RESET_SET, 7, 7, 2, 0x80);
  RADIOLIB_ASSERT(state);

  // hold it there for a while
  mod->delayMicroseconds(100);

  // clear the reset bit
  state = mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_PWR_MODE, RADIOLIB_AX5X43_PWR_MODE_RESET_CLEAR, 7, 7);
  RADIOLIB_ASSERT(state);

  // set power mode to power down, as called for by the datasheet
  state = setMode(RADIOLIB_AX5X43_PWR_MODE_POWER_DOWN);

  // hold it there for a while
  mod->delayMicroseconds(100);

  return(state);
}

int16_t AX5x43::standby() {
  // set RF switch (if present)
  mod->setRfSwitchState(LOW, LOW);

  return(setMode(RADIOLIB_AX5X43_PWR_MODE_FIFO_ON));
}

int16_t AX5x43::setFrequency(float frq) {
  return(setFrequency(frq, false));
}

int16_t AX5x43::setFrequency(float frq, bool forceRanging) {
  // check valid range
  RADIOLIB_CHECK_RANGE(frq, 27.0, 1050.0, RADIOLIB_ERR_INVALID_FREQUENCY);

  // calculate raw value
  uint32_t freqRaw = (frq * (uint32_t(1) << RADIOLIB_AX5X43_DIV_EXPONENT)) / RADIOLIB_AX5X43_CRYSTAL_FREQ;

  // force the LSB as per datasheet recommendation
  // at most, this will introduce 1.55 Hz error
  freqRaw |= 0x01;

  // set the registers (only frequency A is used right now)
  int16_t state = mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_FREQ_A_3, (uint8_t)(freqRaw >> 24));
  state |= mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_FREQ_A_2, (uint8_t)(freqRaw >> 16));
  state |= mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_FREQ_A_1, (uint8_t)(freqRaw >> 8));
  state |= mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_FREQ_A_0, (uint8_t)freqRaw);
  RADIOLIB_ASSERT(state);

  // check if we need to perform VCO autoranging
  if(forceRanging || (fabs(freq - frq) > 2.5f)) {
    // do ranging now
    state = setMode(RADIOLIB_AX5X43_PWR_MODE_STANDBY);
    RADIOLIB_ASSERT(state);

    // set default range
    state = mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_PLL_RANGING_A, RADIOLIB_AX5X43_PLL_VCO_RNG_DEFAULT, 3, 0);
    RADIOLIB_ASSERT(state);

    // set the start bit
    state = mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_PLL_RANGING_A, RADIOLIB_AX5X43_PLL_RNG_START, 4, 4);
    RADIOLIB_ASSERT(state);

    // wait for finish or error
    uint32_t start = mod->millis();
    state = RADIOLIB_ERR_RANGING_TIMEOUT;
    while(mod->millis() - start < 5000) {
      if(mod->SPIgetRegValue(RADIOLIB_AX5X43_REG_PLL_RANGING_A, 5, 5) == RADIOLIB_AX5X43_PLL_RNG_ERR) {
        state = RADIOLIB_ERR_RANGING_FAILED;
        break;
      } else if(!mod->SPIgetRegValue(RADIOLIB_AX5X43_PLL_RNG_START, 5, 5)) {
        state = RADIOLIB_ERR_NONE;
        break;
      }
    }
    RADIOLIB_ASSERT(state);

    // all done!
    state = standby();
    RADIOLIB_ASSERT(state);
  }

  // update cached frequency
  freq = frq;
  return(state);
}

int16_t AX5x43::setBitRate(float br) {
  // check valid range
  RADIOLIB_CHECK_RANGE(br, 0.1, 125.0, RADIOLIB_ERR_INVALID_BIT_RATE);

  // calculate raw value
  uint32_t brRaw = (br * (uint32_t(1) << RADIOLIB_AX5X43_DIV_EXPONENT)) / RADIOLIB_AX5X43_CRYSTAL_FREQ;

  // set the registers
  int16_t state = mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_TX_RATE_2, (uint8_t)(brRaw >> 16));
  state |= mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_TX_RATE_1, (uint8_t)(brRaw >> 8));
  state |= mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_TX_RATE_0, (uint8_t)brRaw);
  RADIOLIB_ASSERT(state);

  // update cached value
  bitRate = br;
  return(state);
}

int16_t AX5x43::setFrequencyDeviation(float freqDev) {
  // check valid range
  RADIOLIB_CHECK_RANGE(freqDev, 0, 125.0, RADIOLIB_ERR_INVALID_BIT_RATE);

  // calculate raw value
  uint32_t freqDevRaw = (freqDev * (uint32_t(1) << RADIOLIB_AX5X43_DIV_EXPONENT)) / RADIOLIB_AX5X43_CRYSTAL_FREQ;

  // set the registers
  int16_t state = mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_FSK_DEV_2, (uint8_t)(freqDevRaw >> 16));
  state |= mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_FSK_DEV_1, (uint8_t)(freqDevRaw >> 8));
  state |= mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_FSK_DEV_0, (uint8_t)freqDevRaw);
  return(state);
}

int16_t AX5x43::setPreambleLength(uint16_t preambleLength) {
  // check valid range
  RADIOLIB_CHECK_RANGE(preambleLength, 0, 32, RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);

  // just update a cached variable - on AX5x43, preamble is actually written into FIFO
  preambleLen = preambleLength;
  return(RADIOLIB_ERR_NONE);
}

int16_t AX5x43::transmit(uint8_t* data, size_t len, uint8_t addr) {
  // calculate timeout (5ms + 500 % of expected time-on-air)
  uint32_t timeout = 5000000 + (uint32_t)((((float)(len * 8)) / (bitRate * 1000.0)) * 5000000.0);

  // start transmission
  int16_t state = startTransmit(data, len, addr);
  RADIOLIB_ASSERT(state);

  uint32_t start = mod->micros();
  while(!mod->digitalRead(mod->getIrq())) {
    mod->yield();

    if(mod->micros() - start > timeout) {
      finishTransmit();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }

  return(finishTransmit());
}

int16_t AX5x43::receive(uint8_t* data, size_t len) {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t AX5x43::startTransmit(uint8_t* data, size_t len, uint8_t addr) {
  // set mode to full TX first
  int16_t state = setMode(RADIOLIB_AX5X43_PWR_MODE_FULL_TX);
  RADIOLIB_ASSERT(state);

  // write the preamble
  uint8_t pre[32 + 1];
  pre[0] = RADIOLIB_AX5X43_FIFO_TX_DATA_UNENC;
  memset(&pre[1], 0xAA, preambleLen);
  writeFifoChunk(RADIOLIB_AX5X43_FIFO_CHUNK_HDR_DATA, pre, preambleLen + 1);

  // write the data

  // wait until crystal is running
  uint32_t start = mod->millis();
  state = RADIOLIB_ERR_TX_TIMEOUT;
  while(mod->millis() - start < 5000) {
    if(mod->SPIgetRegValue(RADIOLIB_AX5X43_REG_XTAL_STATUS, 0, 0) == RADIOLIB_AX5X43_XTAL_RUNNING) {
      state = RADIOLIB_ERR_NONE;
      break;
    }
  }
  if(state != RADIOLIB_ERR_NONE) {
    finishTransmit();
    return(state);
  }

  // commit FIFO - this tarts the actual transmission
  mod->SPIwriteRegister(RADIOLIB_AX5X43_REG_FIFO_STAT, RADIOLIB_AX5X43_FIFO_CMD_COMMIT);
  return(RADIOLIB_ERR_NONE);
}

int16_t AX5x43::finishTransmit() {
  // set mode to standby to disable transmitter/RF switch
  return(standby());
}

int16_t AX5x43::readData(uint8_t* data, size_t len) {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t AX5x43::transmitDirect(uint32_t frf) {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t AX5x43::receiveDirect() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t AX5x43::setDataShaping(uint8_t sh) {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t AX5x43::setEncoding(uint8_t encoding) {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

size_t AX5x43::getPacketLength(bool update) {
  return(0);
}

uint8_t AX5x43::randomByte() {
  return(0);
}

void AX5x43::setDirectAction(void (*func)(void)) {

}

void AX5x43::readBit(RADIOLIB_PIN_TYPE pin) {

}

int16_t AX5x43::getChipVersion() {
  return(mod->SPIgetRegValue(RADIOLIB_AX5X43_REG_REVISION));
}

int16_t AX5x43::config() {
  // set the "performance tuning" magic registers
  int16_t state = mod->SPIsetRegValue(0xF00, 0x0F);
  state |= mod->SPIsetRegValue(0xF0D, 0x03);
  state |= mod->SPIsetRegValue(0xF1C, 0x07);
  state |= mod->SPIsetRegValue(0xF1C, 0x07);
  state |= mod->SPIsetRegValue(0xF44, 0x24);
  RADIOLIB_ASSERT(state);

  /// \todo change this based on TCXO presence
  state = mod->SPIsetRegValue(0xF10, 0x03);
  state |= mod->SPIsetRegValue(0xF11, 0x07);
  RADIOLIB_ASSERT(state);

  /// \todo change this based on PLL RF divide-by-2
  state = mod->SPIsetRegValue(0xF34, 0x08);
  RADIOLIB_ASSERT(state);

  if(RADIOLIB_AX5X43_CRYSTAL_FREQ < 24.8f) {
    state = mod->SPIsetRegValue(0xF35, 0x10);
  } else {
    state = mod->SPIsetRegValue(0xF35, 0x11);
  }
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t AX5x43::setMode(uint8_t mode) {
  return(mod->SPIsetRegValue(RADIOLIB_AX5X43_REG_PWR_MODE, mode, 3, 0));
}

void AX5x43::writeFifoChunk(uint8_t hdr, uint8_t* data, size_t len) {
  // write the header
  mod->SPIwriteRegister(RADIOLIB_AX5X43_REG_FIFO_DATA, hdr);
  if((data == NULL) || (len == 0)) {
    return;
  }

  // optionally, write the data
  // if it is one of the variable length chunks, write the length byte
  if((hdr == RADIOLIB_AX5X43_FIFO_CHUNK_HDR_DATA) || (hdr == RADIOLIB_AX5X43_FIFO_CHUNK_HDR_TXPWR)) {
    mod->SPIwriteRegister(RADIOLIB_AX5X43_REG_FIFO_DATA, len);
  }

  // now write the data
  for(size_t i = 0; i < len; i++) {
    mod->SPIwriteRegister(RADIOLIB_AX5X43_REG_FIFO_DATA, data[i]);
  }
}

#endif
