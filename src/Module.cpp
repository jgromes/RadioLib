#include "Module.h"

#if defined(RADIOLIB_BUILD_ARDUINO)

// we need this to emulate tone() on mbed Arduino boards
#if defined(RADIOLIB_MBED_TONE_OVERRIDE)
#include "mbed.h"
mbed::PwmOut *pwmPin = NULL;
#endif

Module::Module(RADIOLIB_PIN_TYPE cs, RADIOLIB_PIN_TYPE irq, RADIOLIB_PIN_TYPE rst, RADIOLIB_PIN_TYPE gpio):
  _cs(cs),
  _irq(irq),
  _rst(rst),
  _gpio(gpio)
{
  _spi = &RADIOLIB_DEFAULT_SPI;
  _initInterface = true;

  // this is Arduino build, pre-set callbacks
  setCb_pinMode(::pinMode);
  setCb_digitalRead(::digitalRead);
  setCb_digitalWrite(::digitalWrite);
  #if !defined(RADIOLIB_TONE_UNSUPPORTED)
  setCb_tone(::tone);
  setCb_noTone(::noTone);
  #endif
  setCb_attachInterrupt(::attachInterrupt);
  setCb_detachInterrupt(::detachInterrupt);
  #if !defined(RADIOLIB_YIELD_UNSUPPORTED)
  setCb_yield(::yield);
  #endif
  setCb_delay(::delay);
  setCb_delayMicroseconds(::delayMicroseconds);
  setCb_millis(::millis);
  setCb_micros(::micros);
  setCb_pulseIn(::pulseIn);
  setCb_SPIbegin(&Module::SPIbegin);
  setCb_SPIbeginTransaction(&Module::SPIbeginTransaction);
  setCb_SPItransfer(&Module::SPItransfer);
  setCb_SPIendTransaction(&Module::SPIendTransaction);
  setCb_SPIend(&Module::SPIend);
}

Module::Module(RADIOLIB_PIN_TYPE cs, RADIOLIB_PIN_TYPE irq, RADIOLIB_PIN_TYPE rst, RADIOLIB_PIN_TYPE gpio, SPIClass& spi, SPISettings spiSettings):
  _cs(cs),
  _irq(irq),
  _rst(rst),
  _gpio(gpio),
  _spiSettings(spiSettings)
{
  _spi = &spi;
  _initInterface = false;

  // this is Arduino build, pre-set callbacks
  setCb_pinMode(::pinMode);
  setCb_digitalRead(::digitalRead);
  setCb_digitalWrite(::digitalWrite);
  #if !defined(RADIOLIB_TONE_UNSUPPORTED)
  setCb_tone(::tone);
  setCb_noTone(::noTone);
  #endif
  setCb_attachInterrupt(::attachInterrupt);
  setCb_detachInterrupt(::detachInterrupt);
  #if !defined(RADIOLIB_YIELD_UNSUPPORTED)
  setCb_yield(::yield);
  #endif
  setCb_delay(::delay);
  setCb_delayMicroseconds(::delayMicroseconds);
  setCb_millis(::millis);
  setCb_micros(::micros);
  setCb_pulseIn(::pulseIn);
  setCb_SPIbegin(&Module::SPIbegin);
  setCb_SPIbeginTransaction(&Module::SPIbeginTransaction);
  setCb_SPItransfer(&Module::SPItransfer);
  setCb_SPIendTransaction(&Module::SPIendTransaction);
  setCb_SPIend(&Module::SPIend);
}
#else

Module::Module(RADIOLIB_PIN_TYPE cs, RADIOLIB_PIN_TYPE irq, RADIOLIB_PIN_TYPE rst, RADIOLIB_PIN_TYPE gpio):
  _cs(cs),
  _irq(irq),
  _rst(rst),
  _gpio(gpio)
{
  // not an Arduino build, it's up to the user to set all callbacks
}

#endif

Module::Module(const Module& mod) {
  *this = mod;
}

Module& Module::operator=(const Module& mod) {
  this->SPIreadCommand = mod.SPIreadCommand;
  this->SPIwriteCommand = mod.SPIwriteCommand;
  this->_cs = mod.getCs();
  this->_irq = mod.getIrq();
  this->_rst = mod.getRst();
  this->_gpio = mod.getGpio();

  return(*this);
}

void Module::init() {
  this->pinMode(_cs, OUTPUT);
  this->digitalWrite(_cs, HIGH);
#if defined(RADIOLIB_BUILD_ARDUINO)
  if(_initInterface) {
    (this->*cb_SPIbegin)();
  }
#endif
}

void Module::term() {
  // stop hardware interfaces (if they were initialized by the library)
#if defined(RADIOLIB_BUILD_ARDUINO)
  if(!_initInterface) {
    return;
  }

  if(_spi != nullptr) {
    this->SPIend();
  }
#endif
}

int16_t Module::SPIgetRegValue(uint16_t reg, uint8_t msb, uint8_t lsb) {
  if((msb > 7) || (lsb > 7) || (lsb > msb)) {
    return(RADIOLIB_ERR_INVALID_BIT_RANGE);
  }

  uint8_t rawValue = SPIreadRegister(reg);
  uint8_t maskedValue = rawValue & ((0b11111111 << lsb) & (0b11111111 >> (7 - msb)));
  return(maskedValue);
}

int16_t Module::SPIsetRegValue(uint16_t reg, uint8_t value, uint8_t msb, uint8_t lsb, uint8_t checkInterval, uint8_t checkMask) {
  if((msb > 7) || (lsb > 7) || (lsb > msb)) {
    return(RADIOLIB_ERR_INVALID_BIT_RANGE);
  }

  uint8_t currentValue = SPIreadRegister(reg);
  uint8_t mask = ~((0b11111111 << (msb + 1)) | (0b11111111 >> (8 - lsb)));
  uint8_t newValue = (currentValue & ~mask) | (value & mask);
  SPIwriteRegister(reg, newValue);

  #if defined(RADIOLIB_SPI_PARANOID)
    // check register value each millisecond until check interval is reached
    // some registers need a bit of time to process the change (e.g. SX127X_REG_OP_MODE)
    uint32_t start = this->micros();
    uint8_t readValue = 0x00;
    while(this->micros() - start < (checkInterval * 1000)) {
      readValue = SPIreadRegister(reg);
      if((readValue & checkMask) == (newValue & checkMask)) {
        // check passed, we can stop the loop
        return(RADIOLIB_ERR_NONE);
      }
    }

    // check failed, print debug info
    RADIOLIB_DEBUG_PRINTLN();
    RADIOLIB_DEBUG_PRINTLN("address:\t0x%X", reg);
    RADIOLIB_DEBUG_PRINTLN("bits:\t\t%d %d", msb, lsb);
    RADIOLIB_DEBUG_PRINT("value:\t\t0x%X", value);
    RADIOLIB_DEBUG_PRINT("current:\t0x%X", currentValue);
    RADIOLIB_DEBUG_PRINT("mask:\t\t0x%X", mask);
    RADIOLIB_DEBUG_PRINT("new:\t\t0x%X", newValue);
    RADIOLIB_DEBUG_PRINTLN("read:\t\t0x%X", readValue);

    return(RADIOLIB_ERR_SPI_WRITE_FAILED);
  #else
    return(RADIOLIB_ERR_NONE);
  #endif
}

void Module::SPIreadRegisterBurst(uint16_t reg, size_t numBytes, uint8_t* inBytes) {
  if(!SPIstreamType) {
    SPItransfer(SPIreadCommand, reg, NULL, inBytes, numBytes);
  } else {
    uint8_t cmd[] = { SPIreadCommand, (uint8_t)((reg >> 8) & 0xFF), (uint8_t)(reg & 0xFF) };
    SPItransferStream(cmd, 3, false, NULL, inBytes, numBytes, true, 5000);
  }
}

uint8_t Module::SPIreadRegister(uint16_t reg) {
  uint8_t resp = 0;
  if(!SPIstreamType) {
    SPItransfer(SPIreadCommand, reg, NULL, &resp, 1);
  } else {
    uint8_t cmd[] = { SPIreadCommand, (uint8_t)((reg >> 8) & 0xFF), (uint8_t)(reg & 0xFF) };
    SPItransferStream(cmd, 3, false, NULL, &resp, 1, true, 5000);
  }
  return(resp);
}

void Module::SPIwriteRegisterBurst(uint16_t reg, uint8_t* data, size_t numBytes) {
  if(!SPIstreamType) {
    SPItransfer(SPIwriteCommand, reg, data, NULL, numBytes);
  } else {
    uint8_t cmd[] = { SPIwriteCommand, (uint8_t)((reg >> 8) & 0xFF), (uint8_t)(reg & 0xFF) };
    SPItransferStream(cmd, 3, true, data, NULL, numBytes, true, 5000);
  }
}

void Module::SPIwriteRegister(uint16_t reg, uint8_t data) {
  if(!SPIstreamType) {
    SPItransfer(SPIwriteCommand, reg, &data, NULL, 1);
  } else {
    uint8_t cmd[] = { SPIwriteCommand, (uint8_t)((reg >> 8) & 0xFF), (uint8_t)(reg & 0xFF) };
    SPItransferStream(cmd, 3, true, &data, NULL, 1, true, 5000);
  }
}

void Module::SPItransfer(uint8_t cmd, uint16_t reg, uint8_t* dataOut, uint8_t* dataIn, size_t numBytes) {
  // start SPI transaction
  this->beginTransaction();

  // pull CS low
  this->digitalWrite(_cs, LOW);

  // send SPI register address with access command
  if(this->SPIaddrWidth <= 8) {
    this->transfer(reg | cmd);
  } else {
    this->transfer((reg >> 8) | cmd);
    this->transfer(reg & 0xFF);
  }

  #if defined(RADIOLIB_VERBOSE)
    if(cmd == SPIwriteCommand) {
      RADIOLIB_VERBOSE_PRINT("W");
    } else if(cmd == SPIreadCommand) {
      RADIOLIB_VERBOSE_PRINT("R");
    }
    RADIOLIB_VERBOSE_PRINT("\t%X\t", reg);
  #endif

  // send data or get response
  if(cmd == SPIwriteCommand) {
    if(dataOut != NULL) {
      for(size_t n = 0; n < numBytes; n++) {
        this->transfer(dataOut[n]);
        RADIOLIB_VERBOSE_PRINT("%X\t", dataOut[n]);
      }
    }
  } else if (cmd == SPIreadCommand) {
    if(dataIn != NULL) {
      for(size_t n = 0; n < numBytes; n++) {
        dataIn[n] = this->transfer(0x00);
        RADIOLIB_VERBOSE_PRINT("%X\t", dataIn[n]);
      }
    }
  }
  RADIOLIB_VERBOSE_PRINTLN();

  // release CS
  this->digitalWrite(_cs, HIGH);

  // end SPI transaction
  this->endTransaction();
}

int16_t Module::SPIreadStream(uint8_t cmd, uint8_t* data, size_t numBytes, bool waitForGpio, bool verify) {
  return(this->SPIreadStream(&cmd, 1, data, numBytes, waitForGpio, verify));
}

int16_t Module::SPIreadStream(uint8_t* cmd, uint8_t cmdLen, uint8_t* data, size_t numBytes, bool waitForGpio, bool verify) {
  // send the command
  int16_t state = this->SPItransferStream(cmd, cmdLen, false, NULL, data, numBytes, waitForGpio, 5000);
  RADIOLIB_ASSERT(state);

  // check the status
  if(verify) {
    state = this->SPIcheckStream();
  }

  return(state);
}

int16_t Module::SPIwriteStream(uint8_t cmd, uint8_t* data, size_t numBytes, bool waitForGpio, bool verify) {
  return(this->SPIwriteStream(&cmd, 1, data, numBytes, waitForGpio, verify));
}

int16_t Module::SPIwriteStream(uint8_t* cmd, uint8_t cmdLen, uint8_t* data, size_t numBytes, bool waitForGpio, bool verify) {
  // send the command
  int16_t state = this->SPItransferStream(cmd, cmdLen, true, data, NULL, numBytes, waitForGpio, 5000);
  RADIOLIB_ASSERT(state);

  // check the status
  if(verify) {
    state = this->SPIcheckStream();
  }

  return(state);
}

int16_t Module::SPIcheckStream() {
  int16_t state = RADIOLIB_ERR_NONE;

  #if defined(RADIOLIB_SPI_PARANOID)
  // get the status
  uint8_t spiStatus = 0;
  uint8_t cmd = this->SPIstatusCommand;
  state = this->SPItransferStream(&cmd, 1, false, NULL, &spiStatus, 1, true, 5000);
  RADIOLIB_ASSERT(state);

  // translate to RadioLib status code
  if(this->SPIparseStatusCb != nullptr) {
    this->SPIstreamError = this->SPIparseStatusCb(spiStatus);
  }

  #endif

  return(state);
}

int16_t Module::SPItransferStream(uint8_t* cmd, uint8_t cmdLen, bool write, uint8_t* dataOut, uint8_t* dataIn, size_t numBytes, bool waitForGpio, uint32_t timeout) {
  #if defined(RADIOLIB_VERBOSE)
    uint8_t debugBuff[RADIOLIB_STATIC_ARRAY_SIZE];
  #endif

  // ensure GPIO is low
  uint32_t start = this->millis();
  while(this->digitalRead(this->getGpio())) {
    this->yield();
    if(this->millis() - start >= timeout) {
      this->digitalWrite(this->getCs(), HIGH);
      return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
    }
  }

  // pull NSS low
  this->digitalWrite(this->getCs(), LOW);

  // start transfer
  this->beginTransaction();

  // send command byte(s)
  for(uint8_t n = 0; n < cmdLen; n++) {
    this->transfer(cmd[n]);
  }

  // variable to save error during SPI transfer
  int16_t state = RADIOLIB_ERR_NONE;

  // send/receive all bytes
  if(write) {
    for(size_t n = 0; n < numBytes; n++) {
      // send byte
      uint8_t in = this->transfer(dataOut[n]);
      #if defined(RADIOLIB_VERBOSE)
        debugBuff[n] = in;
      #endif

      // check status
      if(this->SPIparseStatusCb != nullptr) {
        state = this->SPIparseStatusCb(in);
      }
    }

  } else {
    // skip the first byte for read-type commands (status-only)
    uint8_t in = this->transfer(this->SPInopCommand);
    #if defined(RADIOLIB_VERBOSE)
      debugBuff[0] = in;
    #endif

    // check status
    if(this->SPIparseStatusCb != nullptr) {
      state = this->SPIparseStatusCb(in);
    } else {
      state = RADIOLIB_ERR_NONE;
    }

    // read the data
    if(state == RADIOLIB_ERR_NONE) {
      for(size_t n = 0; n < numBytes; n++) {
        dataIn[n] = this->transfer(this->SPInopCommand);
      }
    }
  }

  // stop transfer
  this->endTransaction();
  this->digitalWrite(this->getCs(), HIGH);

  // wait for GPIO to go high and then low
  if(waitForGpio) {
    this->delayMicroseconds(1);
    uint32_t start = this->millis();
    while(this->digitalRead(this->getGpio())) {
      this->yield();
      if(this->millis() - start >= timeout) {
        state = RADIOLIB_ERR_SPI_CMD_TIMEOUT;
        break;
      }
    }
  }

  // print debug output
  #if defined(RADIOLIB_VERBOSE)
    // print command byte(s)
    RADIOLIB_VERBOSE_PRINT("CMD\t");
    for(uint8_t n = 0; n < cmdLen; n++) {
      RADIOLIB_VERBOSE_PRINT("%X\t", cmd[n]);
    }
    RADIOLIB_VERBOSE_PRINTLN();

    // print data bytes
    RADIOLIB_VERBOSE_PRINT("DAT");
    if(write) {
      RADIOLIB_VERBOSE_PRINT("W\t");
      for(size_t n = 0; n < numBytes; n++) {
        RADIOLIB_VERBOSE_PRINT("%X\t%X\t", dataOut[n], debugBuff[n]);
      }
      RADIOLIB_VERBOSE_PRINTLN();
    } else {
      RADIOLIB_VERBOSE_PRINT("R\t%X\t%X\t", this->SPInopCommand, debugBuff[0]);

      for(size_t n = 0; n < numBytes; n++) {
        RADIOLIB_VERBOSE_PRINT("%X\t%X\t", this->SPInopCommand, dataIn[n]);
      }
      RADIOLIB_VERBOSE_PRINTLN();
    }
    RADIOLIB_VERBOSE_PRINTLN();
  #endif

  return(state);
}

void Module::waitForMicroseconds(uint32_t start, uint32_t len) {
  #if defined(RADIOLIB_INTERRUPT_TIMING)
  (void)start;
  if((this->TimerSetupCb != nullptr) && (len != this->_prevTimingLen)) {
    _prevTimingLen = len;
    this->TimerSetupCb(len);
  }
  this->TimerFlag = false;
  while(!this->TimerFlag) {
    this->yield();
  }
  #else
   while(this->micros() - start < len) {
    this->yield();
  }
  #endif
}

void Module::pinMode(RADIOLIB_PIN_TYPE pin, RADIOLIB_PIN_MODE mode) {
  if((pin == RADIOLIB_NC) || (cb_pinMode == nullptr)) {
    return;
  }
  cb_pinMode(pin, mode);
}

void Module::digitalWrite(RADIOLIB_PIN_TYPE pin, RADIOLIB_PIN_STATUS value) {
  if((pin == RADIOLIB_NC) || (cb_digitalWrite == nullptr)) {
    return;
  }
  cb_digitalWrite(pin, value);
}

RADIOLIB_PIN_STATUS Module::digitalRead(RADIOLIB_PIN_TYPE pin) {
  if((pin == RADIOLIB_NC) || (cb_digitalRead == nullptr)) {
    return((RADIOLIB_PIN_STATUS)0);
  }
  return(cb_digitalRead(pin));
}

#if defined(ESP32)
// we need to cache the previous tone value for emulation on ESP32
int32_t prev = -1;
#endif

void Module::tone(RADIOLIB_PIN_TYPE pin, uint16_t value, uint32_t duration) {
  #if !defined(RADIOLIB_TONE_UNSUPPORTED)
  if((pin == RADIOLIB_NC) || (cb_tone == nullptr)) {
    return;
  }
  cb_tone(pin, value, duration);
  #else
  if(pin == RADIOLIB_NC) {
    return;
  }
    #if defined(ESP32)
      // ESP32 tone() emulation
      (void)duration;
      if(prev == -1) {
        ledcAttachPin(pin, RADIOLIB_TONE_ESP32_CHANNEL);
      }
      if(prev != value) {
        ledcWriteTone(RADIOLIB_TONE_ESP32_CHANNEL, value);
      }
      prev = value;
    #elif defined(RADIOLIB_MBED_TONE_OVERRIDE)
      // better tone for mbed OS boards
      (void)duration;
      if(!pwmPin) {
        pwmPin = new mbed::PwmOut(digitalPinToPinName(pin));
      }
      pwmPin->period(1.0 / value);
      pwmPin->write(0.5);
    #else
      (void)value;
      (void)duration;
    #endif
  #endif
}

void Module::noTone(RADIOLIB_PIN_TYPE pin) {
  #if !defined(RADIOLIB_TONE_UNSUPPORTED)
  if((pin == RADIOLIB_NC) || (cb_noTone == nullptr)) {
    return;
  }
  #if defined(ARDUINO_ARCH_STM32)
  cb_noTone(pin, false);
  #else
  cb_noTone(pin);
  #endif
  #else
  if(pin == RADIOLIB_NC) {
    return;
  }
    #if defined(ESP32)
      // ESP32 tone() emulation
      ledcDetachPin(pin);
      ledcWrite(RADIOLIB_TONE_ESP32_CHANNEL, 0);
      prev = -1;
    #elif defined(RADIOLIB_MBED_TONE_OVERRIDE)
      // better tone for mbed OS boards
      (void)pin;
      pwmPin->suspend();
    #endif
  #endif
}

void Module::attachInterrupt(RADIOLIB_PIN_TYPE interruptNum, void (*userFunc)(void), RADIOLIB_INTERRUPT_STATUS mode) {
  if((interruptNum == RADIOLIB_NC) || (cb_attachInterrupt == nullptr)) {
    return;
  }
  cb_attachInterrupt(interruptNum, userFunc, mode);
}

void Module::detachInterrupt(RADIOLIB_PIN_TYPE interruptNum) {
  if((interruptNum == RADIOLIB_NC) || (cb_detachInterrupt == nullptr)) {
    return;
  }
  cb_detachInterrupt(interruptNum);
}

void Module::yield() {
  if(cb_yield == nullptr) {
    return;
  }
  #if !defined(RADIOLIB_YIELD_UNSUPPORTED)
  cb_yield();
  #endif
}

void Module::delay(uint32_t ms) {
  if(cb_delay == nullptr) {
    return;
  }
  cb_delay(ms);
}

void Module::delayMicroseconds(uint32_t us) {
  if(cb_delayMicroseconds == nullptr) {
    return;
  }
  cb_delayMicroseconds(us);
}

uint32_t Module::millis() {
  if(cb_millis == nullptr) {
    return(0);
  }
  return(cb_millis());
}

uint32_t Module::micros() {
  if(cb_micros == nullptr) {
    return(0);
  }
  return(cb_micros());
}

uint32_t Module::pulseIn(RADIOLIB_PIN_TYPE pin, RADIOLIB_PIN_STATUS state, uint32_t timeout) {
  if(cb_pulseIn == nullptr) {
    return(0);
  }
  return(cb_pulseIn(pin, state, timeout));
}

void Module::begin() {
  if(cb_SPIbegin == nullptr) {
    return;
  }
  #if defined(RADIOLIB_BUILD_ARDUINO)
  (this->*cb_SPIbegin)();
  #else
  cb_SPIbegin();
  #endif
}

void Module::beginTransaction() {
  if(cb_SPIbeginTransaction == nullptr) {
    return;
  }
  #if defined(RADIOLIB_BUILD_ARDUINO)
  (this->*cb_SPIbeginTransaction)();
  #else
  cb_SPIbeginTransaction();
  #endif
}

uint8_t Module::transfer(uint8_t b) {
  if(cb_SPItransfer == nullptr) {
    return(0xFF);
  }
  #if defined(RADIOLIB_BUILD_ARDUINO)
  return((this->*cb_SPItransfer)(b));
  #else
  return(cb_SPItransfer(b));
  #endif
}

void Module::endTransaction() {
  if(cb_SPIendTransaction == nullptr) {
    return;
  }
  #if defined(RADIOLIB_BUILD_ARDUINO)
  (this->*cb_SPIendTransaction)();
  #else
  cb_SPIendTransaction();
  #endif
}

void Module::end() {
  if(cb_SPIend == nullptr) {
    return;
  }
  #if defined(RADIOLIB_BUILD_ARDUINO)
  (this->*cb_SPIend)();
  #else
  cb_SPIend();
  #endif
}

#if defined(RADIOLIB_BUILD_ARDUINO)
void Module::SPIbegin() {
  _spi->begin();
}

void Module::SPIbeginTransaction() {
  _spi->beginTransaction(_spiSettings);
}

uint8_t Module::SPItransfer(uint8_t b) {
  return(_spi->transfer(b));
}

void Module::SPIendTransaction() {
  _spi->endTransaction();
}

void Module::SPIend() {
  _spi->end();
}
#endif

uint8_t Module::flipBits(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

uint16_t Module::flipBits16(uint16_t i) {
  i = (i & 0xFF00) >> 8 | (i & 0x00FF) << 8;
  i = (i & 0xF0F0) >> 4 | (i & 0x0F0F) << 4;
  i = (i & 0xCCCC) >> 2 | (i & 0x3333) << 2;
  i = (i & 0xAAAA) >> 1 | (i & 0x5555) << 1;
  return i;
}

void Module::hexdump(uint8_t* data, size_t len, uint32_t offset, uint8_t width, bool be) {
  size_t rem_len = len;
  for(size_t i = 0; i < len; i+=16) {
    char str[80];
    sprintf(str, "%07" PRIx32 "  ", i+offset);
    size_t line_len = 16;
    if(rem_len < line_len) {
      line_len = rem_len;
    }
    for(size_t j = 0; j < line_len; j+=width) {
      if(width > 1) {
        int m = 0;
        int step = width/2;
        if(be) {
          step *= -1;
        }
        for(int32_t k = width - 1; k >= -width + 1; k+=step) {
          sprintf(&str[8 + (j+m)*3], "%02x ", data[i+j+k+m]);
          m++;
        }
      } else {
        sprintf(&str[8 + (j)*3], "%02x ", data[i+j]);
      }
    }
    for(size_t j = line_len; j < 16; j++) {
      sprintf(&str[8 + j*3], "   ");
    }
    str[56] = '|';
    str[57] = ' ';
    for(size_t j = 0; j < line_len; j++) {
      char c = data[i+j];
      if((c < ' ') || (c > '~')) {
        c = '.';
      }
      sprintf(&str[58 + j], "%c", c);
    }
    for(size_t j = line_len; j < 16; j++) {
      sprintf(&str[58 + j], "   ");
    }
    RADIOLIB_DEBUG_PRINT(str);
    RADIOLIB_DEBUG_PRINTLN();
    rem_len -= 16;
  }
}

void Module::regdump(uint16_t start, size_t len) {
  #if defined(RADIOLIB_STATIC_ONLY)
    uint8_t buff[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* buff = new uint8_t[len];
  #endif
  SPIreadRegisterBurst(start, len, buff);
  hexdump(buff, len, start);
  #if !defined(RADIOLIB_STATIC_ONLY)
    delete[] buff;
  #endif
}

#if defined(RADIOLIB_DEBUG) and defined(RADIOLIB_BUILD_ARDUINO)
// https://github.com/esp8266/Arduino/blob/65579d29081cb8501e4d7f786747bf12e7b37da2/cores/esp8266/Print.cpp#L50
size_t Module::serialPrintf(const char* format, ...) {
  va_list arg;
  va_start(arg, format);
  char temp[64];
  char* buffer = temp;
  size_t len = vsnprintf(temp, sizeof(temp), format, arg);
  va_end(arg);
  if (len > sizeof(temp) - 1) {
    buffer = new char[len + 1];
    if (!buffer) {
      return 0;
    }
    va_start(arg, format);
    vsnprintf(buffer, len + 1, format, arg);
    va_end(arg);
  }
  len = RADIOLIB_DEBUG_PORT.write((const uint8_t*)buffer, len);
  if (buffer != temp) {
    delete[] buffer;
  }
  return len;
}
#endif

void Module::setRfSwitchPins(RADIOLIB_PIN_TYPE rxEn, RADIOLIB_PIN_TYPE txEn) {
  // This can be on the stack, setRfSwitchTable copies the contents
  const RADIOLIB_PIN_TYPE pins[] = {
    rxEn, txEn, RADIOLIB_NC,
  };
  // This must be static, since setRfSwitchTable stores a reference.
  static constexpr RfSwitchMode_t table[] = {
    {MODE_IDLE,  {LOW,  LOW}},
    {MODE_RX,    {HIGH, LOW}},
    {MODE_TX,    {LOW,  HIGH}},
    END_OF_MODE_TABLE,
  };
  setRfSwitchTable(pins, table);
}

void Module::setRfSwitchTable(const RADIOLIB_PIN_TYPE (&pins)[3], const RfSwitchMode_t table[]) {
  memcpy(_rfSwitchPins, pins, sizeof(_rfSwitchPins));
  _rfSwitchTable = table;
  for(size_t i = 0; i < RFSWITCH_MAX_PINS; i++)
    this->pinMode(pins[i], OUTPUT);
}

const Module::RfSwitchMode_t *Module::findRfSwitchMode(uint8_t mode) const {
  const RfSwitchMode_t *row = _rfSwitchTable;
  while (row && row->mode != MODE_END_OF_TABLE) {
    if (row->mode == mode)
      return row;
    ++row;
  }
  return nullptr;
}

void Module::setRfSwitchState(uint8_t mode) {
  const RfSwitchMode_t *row = findRfSwitchMode(mode);
  if(!row) {
    // RF switch control is disabled or does not have this mode
    return;
  }

  // set pins
  const RADIOLIB_PIN_STATUS *value = &row->values[0];
  for(size_t i = 0; i < RFSWITCH_MAX_PINS; i++) {
    RADIOLIB_PIN_TYPE pin = _rfSwitchPins[i];
    if (pin != RADIOLIB_NC)
      this->digitalWrite(pin, *value);
    ++value;
  }
}
