#include "Module.h"

// the following is probably only needed on non-Arduino builds
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#if RADIOLIB_DEBUG
// needed for debug print
#include <stdarg.h>
#endif

#if defined(RADIOLIB_BUILD_ARDUINO)
#include "ArduinoHal.h"

Module::Module(uint32_t cs, uint32_t irq, uint32_t rst, uint32_t gpio) : csPin(cs), irqPin(irq), rstPin(rst), gpioPin(gpio) {
  this->hal = new ArduinoHal();
}

Module::Module(uint32_t cs, uint32_t irq, uint32_t rst, uint32_t gpio, SPIClass& spi, SPISettings spiSettings) : csPin(cs), irqPin(irq), rstPin(rst), gpioPin(gpio) {
  this->hal = new ArduinoHal(spi, spiSettings);
}
#endif

Module::Module(RadioLibHal *hal, uint32_t cs, uint32_t irq, uint32_t rst, uint32_t gpio) : csPin(cs), irqPin(irq), rstPin(rst), gpioPin(gpio) {
  this->hal = hal;
}

Module::Module(const Module& mod) {
  *this = mod;
}

Module& Module::operator=(const Module& mod) {
  memcpy((void*)&mod.spiConfig, &this->spiConfig, sizeof(SPIConfig_t));
  this->csPin = mod.csPin;
  this->irqPin = mod.irqPin;
  this->rstPin = mod.rstPin;
  this->gpioPin = mod.gpioPin;
  return(*this);
}

static volatile const char info[] = RADIOLIB_INFO;
void Module::init() {
  this->hal->init();
  this->hal->pinMode(csPin, this->hal->GpioModeOutput);
  this->hal->digitalWrite(csPin, this->hal->GpioLevelHigh);
  RADIOLIB_DEBUG_BASIC_PRINTLN(RADIOLIB_INFO);
}

void Module::term() {
  // stop hardware interfaces (if they were initialized by the library)
  this->hal->term();
}

int16_t Module::SPIgetRegValue(uint32_t reg, uint8_t msb, uint8_t lsb) {
  if((msb > 7) || (lsb > 7) || (lsb > msb)) {
    return(RADIOLIB_ERR_INVALID_BIT_RANGE);
  }

  uint8_t rawValue = SPIreadRegister(reg);
  uint8_t maskedValue = rawValue & ((0b11111111 << lsb) & (0b11111111 >> (7 - msb)));
  return(maskedValue);
}

int16_t Module::SPIsetRegValue(uint32_t reg, uint8_t value, uint8_t msb, uint8_t lsb, uint8_t checkInterval, uint8_t checkMask) {
  if((msb > 7) || (lsb > 7) || (lsb > msb)) {
    return(RADIOLIB_ERR_INVALID_BIT_RANGE);
  }

  uint8_t currentValue = SPIreadRegister(reg);
  uint8_t mask = ~((0b11111111 << (msb + 1)) | (0b11111111 >> (8 - lsb)));
  uint8_t newValue = (currentValue & ~mask) | (value & mask);
  SPIwriteRegister(reg, newValue);

  #if RADIOLIB_SPI_PARANOID
    // check register value each millisecond until check interval is reached
    // some registers need a bit of time to process the change (e.g. SX127X_REG_OP_MODE)
    RadioLibTime_t start = this->hal->micros();
    #if RADIOLIB_DEBUG_SPI
    uint8_t readValue = 0x00;
    #endif
    while(this->hal->micros() - start < (checkInterval * 1000)) {
      uint8_t val = SPIreadRegister(reg);
      if((val & checkMask) == (newValue & checkMask)) {
        // check passed, we can stop the loop
        return(RADIOLIB_ERR_NONE);
      }
      #if RADIOLIB_DEBUG_SPI
      readValue = val;
      #endif
    }

    // check failed, print debug info
    RADIOLIB_DEBUG_SPI_PRINTLN();
    RADIOLIB_DEBUG_SPI_PRINTLN("address:\t0x%X", reg);
    RADIOLIB_DEBUG_SPI_PRINTLN("bits:\t\t%d %d", msb, lsb);
    RADIOLIB_DEBUG_SPI_PRINTLN("value:\t\t0x%X", value);
    RADIOLIB_DEBUG_SPI_PRINTLN("current:\t0x%X", currentValue);
    RADIOLIB_DEBUG_SPI_PRINTLN("mask:\t\t0x%X", mask);
    RADIOLIB_DEBUG_SPI_PRINTLN("new:\t\t0x%X", newValue);
    RADIOLIB_DEBUG_SPI_PRINTLN("read:\t\t0x%X", readValue);

    return(RADIOLIB_ERR_SPI_WRITE_FAILED);
  #else
    return(RADIOLIB_ERR_NONE);
  #endif
}

void Module::SPIreadRegisterBurst(uint32_t reg, size_t numBytes, uint8_t* inBytes) {
  if(!this->spiConfig.stream) {
    SPItransfer(this->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ], reg, NULL, inBytes, numBytes);
  } else {
    uint8_t cmd[6];
    uint8_t* cmdPtr = cmd;
    for(int8_t i = (int8_t)this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8 - 1; i >= 0; i--) {
      *(cmdPtr++) = (this->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] >> 8*i) & 0xFF;
    }
    for(int8_t i = (int8_t)((this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR]/8) - 1); i >= 0; i--) {
      *(cmdPtr++) = (reg >> 8*i) & 0xFF;
    }
    SPItransferStream(cmd, this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8 + this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR]/8, false, NULL, inBytes, numBytes, true);
  }
}

uint8_t Module::SPIreadRegister(uint32_t reg) {
  uint8_t resp = 0;
  if(!spiConfig.stream) {
    SPItransfer(this->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ], reg, NULL, &resp, 1);
  } else {
    uint8_t cmd[6];
    uint8_t* cmdPtr = cmd;
    for(int8_t i = (int8_t)this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8 - 1; i >= 0; i--) {
      *(cmdPtr++) = (this->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] >> 8*i) & 0xFF;
    }
    for(int8_t i = (int8_t)((this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR]/8) - 1); i >= 0; i--) {
      *(cmdPtr++) = (reg >> 8*i) & 0xFF;
    }
    SPItransferStream(cmd, this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8 + this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR]/8, false, NULL, &resp, 1, true);
  }
  return(resp);
}

void Module::SPIwriteRegisterBurst(uint32_t reg, uint8_t* data, size_t numBytes) {
  if(!spiConfig.stream) {
    SPItransfer(spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE], reg, data, NULL, numBytes);
  } else {
    uint8_t cmd[6];
    uint8_t* cmdPtr = cmd;
    for(int8_t i = (int8_t)this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8 - 1; i >= 0; i--) {
      *(cmdPtr++) = (this->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] >> 8*i) & 0xFF;
    }
    for(int8_t i = (int8_t)((this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR]/8) - 1); i >= 0; i--) {
      *(cmdPtr++) = (reg >> 8*i) & 0xFF;
    }
    SPItransferStream(cmd, this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8 + this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR]/8, true, data, NULL, numBytes, true);
  }
}

void Module::SPIwriteRegister(uint32_t reg, uint8_t data) {
  if(!spiConfig.stream) {
    SPItransfer(spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE], reg, &data, NULL, 1);
  } else {
    uint8_t cmd[6];
    uint8_t* cmdPtr = cmd;
    for(int8_t i = (int8_t)this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8 - 1; i >= 0; i--) {
      *(cmdPtr++) = (this->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] >> 8*i) & 0xFF;
    }
    for(int8_t i = (int8_t)((this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR]/8) - 1); i >= 0; i--) {
      *(cmdPtr++) = (reg >> 8*i) & 0xFF;
    }
    SPItransferStream(cmd, this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8 + this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR]/8, true, &data, NULL, 1, true);
  }
}

void Module::SPItransfer(uint16_t cmd, uint32_t reg, uint8_t* dataOut, uint8_t* dataIn, size_t numBytes) {
  // prepare the buffers
  size_t buffLen = this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8 + this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR]/8 + numBytes;
  #if RADIOLIB_STATIC_ONLY
    uint8_t buffOut[RADIOLIB_STATIC_ARRAY_SIZE];
    uint8_t buffIn[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* buffOut = new uint8_t[buffLen];
    uint8_t* buffIn = new uint8_t[buffLen];
  #endif
  uint8_t* buffOutPtr = buffOut;

  // copy the command
  // TODO properly handle variable commands and addresses
  if(this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR] <= 8) {
    *(buffOutPtr++) = reg | cmd;
  } else {
    *(buffOutPtr++) = (reg >> 8) | cmd;
    *(buffOutPtr++) = reg & 0xFF;
  }

  // copy the data
  if(cmd == spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE]) {
    memcpy(buffOutPtr, dataOut, numBytes);
  } else {
    memset(buffOutPtr, this->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_NOP], numBytes);
  }

  // do the transfer
  this->hal->spiBeginTransaction();
  this->hal->digitalWrite(this->csPin, this->hal->GpioLevelLow);
  this->hal->spiTransfer(buffOut, buffLen, buffIn);
  this->hal->digitalWrite(this->csPin, this->hal->GpioLevelHigh);
  this->hal->spiEndTransaction();
  
  // copy the data
  if(cmd == spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ]) {
    memcpy(dataIn, &buffIn[this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR]/8], numBytes);
  }

  // print debug information
  #if RADIOLIB_DEBUG_SPI
    uint8_t* debugBuffPtr = NULL;
    if(cmd == spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE]) {
      RADIOLIB_DEBUG_SPI_PRINT("W\t%X\t", reg);
      debugBuffPtr = &buffOut[this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR]/8];
    } else if(cmd == spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ]) {
      RADIOLIB_DEBUG_SPI_PRINT("R\t%X\t", reg);
      debugBuffPtr = &buffIn[this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR]/8];
    }
    for(size_t n = 0; n < numBytes; n++) {
      RADIOLIB_DEBUG_SPI_PRINT_NOTAG("%X\t", debugBuffPtr[n]);
    }
    RADIOLIB_DEBUG_SPI_PRINTLN_NOTAG();
  #endif

  #if !RADIOLIB_STATIC_ONLY
    delete[] buffOut;
    delete[] buffIn;
  #endif
}

int16_t Module::SPIreadStream(uint16_t cmd, uint8_t* data, size_t numBytes, bool waitForGpio, bool verify) {
  uint8_t cmdBuf[2];
  uint8_t* cmdPtr = cmdBuf;
  for(int8_t i = (int8_t)this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8 - 1; i >= 0; i--) {
    *(cmdPtr++) = (cmd >> 8*i) & 0xFF;
  }
  return(this->SPIreadStream(cmdBuf, this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8, data, numBytes, waitForGpio, verify));
}

int16_t Module::SPIreadStream(uint8_t* cmd, uint8_t cmdLen, uint8_t* data, size_t numBytes, bool waitForGpio, bool verify) {
  // send the command
  int16_t state = this->SPItransferStream(cmd, cmdLen, false, NULL, data, numBytes, waitForGpio);
  RADIOLIB_ASSERT(state);

  #if !RADIOLIB_SPI_PARANOID
  (void)verify;
  return(RADIOLIB_ERR_NONE);
  #else

  // check the status
  if(verify && (this->spiConfig.checkStatusCb != nullptr)) {
    state = this->spiConfig.checkStatusCb(this);
  }

  return(state);
  #endif
}

int16_t Module::SPIwriteStream(uint16_t cmd, uint8_t* data, size_t numBytes, bool waitForGpio, bool verify) {
  uint8_t cmdBuf[2];
  uint8_t* cmdPtr = cmdBuf;
  for(int8_t i = (int8_t)this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8 - 1; i >= 0; i--) {
    *(cmdPtr++) = (cmd >> 8*i) & 0xFF;
  }
  return(this->SPIwriteStream(cmdBuf, this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8, data, numBytes, waitForGpio, verify));
}

int16_t Module::SPIwriteStream(uint8_t* cmd, uint8_t cmdLen, uint8_t* data, size_t numBytes, bool waitForGpio, bool verify) {
  // send the command
  int16_t state = this->SPItransferStream(cmd, cmdLen, true, data, NULL, numBytes, waitForGpio);
  RADIOLIB_ASSERT(state);

  #if !RADIOLIB_SPI_PARANOID
  (void)verify;
  return(RADIOLIB_ERR_NONE);
  #else

  // check the status
  if(verify && (this->spiConfig.checkStatusCb != nullptr)) {
    state = this->spiConfig.checkStatusCb(this);
  }

  return(state);
  #endif
}

int16_t Module::SPIcheckStream() {
  int16_t state = RADIOLIB_ERR_NONE;

  #if RADIOLIB_SPI_PARANOID
  // get the status
  uint8_t spiStatus = 0;
  uint8_t cmdBuf[2];
  uint8_t* cmdPtr = cmdBuf;
  for(int8_t i = (int8_t)this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8 - 1; i >= 0; i--) {
    *(cmdPtr++) = ( this->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_STATUS] >> 8*i) & 0xFF;
  }
  state = this->SPItransferStream(cmdBuf, this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD]/8, false, NULL, &spiStatus, 1, true);
  RADIOLIB_ASSERT(state);

  // translate to RadioLib status code
  if(this->spiConfig.parseStatusCb != nullptr) {
    this->spiConfig.err = this->spiConfig.parseStatusCb(spiStatus);
  }
  #endif

  return(state);
}

int16_t Module::SPItransferStream(const uint8_t* cmd, uint8_t cmdLen, bool write, uint8_t* dataOut, uint8_t* dataIn, size_t numBytes, bool waitForGpio) {
  // prepare the output buffer
  size_t buffLen = cmdLen + numBytes;
  if(!write) {
    buffLen += (this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] / 8);
  }
  #if RADIOLIB_STATIC_ONLY
    uint8_t buffOut[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* buffOut = new uint8_t[buffLen];
  #endif
  uint8_t* buffOutPtr = buffOut;

  // copy the command
  for(uint8_t n = 0; n < cmdLen; n++) {
    *(buffOutPtr++) = cmd[n];
  }

  // copy the data
  if(write) {
    memcpy(buffOutPtr, dataOut, numBytes);
  } else {
    memset(buffOutPtr, this->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_NOP], numBytes + (this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] / 8));
  }

  // ensure GPIO is low
  if(waitForGpio) {
    if(this->gpioPin == RADIOLIB_NC) {
      this->hal->delay(50);
    } else {
      RadioLibTime_t start = this->hal->millis();
      while(this->hal->digitalRead(this->gpioPin)) {
        this->hal->yield();
        if(this->hal->millis() - start >= this->spiConfig.timeout) {
          RADIOLIB_DEBUG_BASIC_PRINTLN("GPIO pre-transfer timeout, is it connected?");
          #if !RADIOLIB_STATIC_ONLY
            delete[] buffOut;
          #endif
          return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
        }
      }
    }
  }

  // prepare the input buffer
  #if RADIOLIB_STATIC_ONLY
    uint8_t buffIn[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* buffIn = new uint8_t[buffLen];
  #endif

  // do the transfer
  this->hal->spiBeginTransaction();
  this->hal->digitalWrite(this->csPin, this->hal->GpioLevelLow);
  this->hal->spiTransfer(buffOut, buffLen, buffIn);
  this->hal->digitalWrite(this->csPin, this->hal->GpioLevelHigh);
  this->hal->spiEndTransaction();

  // wait for GPIO to go high and then low
  if(waitForGpio) {
    if(this->gpioPin == RADIOLIB_NC) {
      this->hal->delay(1);
    } else {
      this->hal->delayMicroseconds(1);
      RadioLibTime_t start = this->hal->millis();
      while(this->hal->digitalRead(this->gpioPin)) {
        this->hal->yield();
        if(this->hal->millis() - start >= this->spiConfig.timeout) {
          RADIOLIB_DEBUG_BASIC_PRINTLN("GPIO post-transfer timeout, is it connected?");
          #if !RADIOLIB_STATIC_ONLY
            delete[] buffOut;
            delete[] buffIn;
          #endif
          return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
        }
      }
    }
  }

  // parse status
  int16_t state = RADIOLIB_ERR_NONE;
  if((this->spiConfig.parseStatusCb != nullptr) && (numBytes > 0)) {
    state = this->spiConfig.parseStatusCb(buffIn[this->spiConfig.statusPos]);
  }
  
  // copy the data
  if(!write) {
    // skip the status bytes if present
    memcpy(dataIn, &buffIn[cmdLen + (this->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] / 8)], numBytes);
  }

  // print debug information
  #if RADIOLIB_DEBUG_SPI
    // print command byte(s)
    RADIOLIB_DEBUG_SPI_PRINT("CMD");
    if(write) {
      RADIOLIB_DEBUG_SPI_PRINT_NOTAG("W\t");
    } else {
      RADIOLIB_DEBUG_SPI_PRINT_NOTAG("R\t");
    }
    size_t n = 0;
    for(; n < cmdLen; n++) {
      RADIOLIB_DEBUG_SPI_PRINT_NOTAG("%X\t", cmd[n]);
    }
    RADIOLIB_DEBUG_SPI_PRINTLN_NOTAG();

    // print data bytes
    RADIOLIB_DEBUG_SPI_PRINT("SI\t");
    for(n = 0; n < cmdLen; n++) {
      RADIOLIB_DEBUG_SPI_PRINT_NOTAG("\t");
    }
    for(; n < buffLen; n++) {
      RADIOLIB_DEBUG_SPI_PRINT_NOTAG("%X\t", buffOut[n]);
    }
    RADIOLIB_DEBUG_SPI_PRINTLN_NOTAG();
    RADIOLIB_DEBUG_SPI_PRINT("SO\t");
    for(n = 0; n < buffLen; n++) {
      RADIOLIB_DEBUG_SPI_PRINT_NOTAG("%X\t", buffIn[n]);
    }
    RADIOLIB_DEBUG_SPI_PRINTLN_NOTAG();
  #endif

  #if !RADIOLIB_STATIC_ONLY
    delete[] buffOut;
    delete[] buffIn;
  #endif

  return(state);
}

void Module::waitForMicroseconds(RadioLibTime_t start, RadioLibTime_t len) {
  #if RADIOLIB_INTERRUPT_TIMING
  (void)start;
  if((this->TimerSetupCb != nullptr) && (len != this->prevTimingLen)) {
    prevTimingLen = len;
    this->TimerSetupCb(len);
  }
  this->TimerFlag = false;
  while(!this->TimerFlag) {
    this->hal->yield();
  }
  #else
   while(this->hal->micros() - start < len) {
    this->hal->yield();
  }
  #endif
}

uint32_t Module::reflect(uint32_t in, uint8_t bits) {
  uint32_t res = 0;
  for(uint8_t i = 0; i < bits; i++) {
    res |= (((in & ((uint32_t)1 << i)) >> i) << (bits - i - 1));
  }
  return(res);
}

#if RADIOLIB_DEBUG
void Module::hexdump(const char* level, uint8_t* data, size_t len, uint32_t offset, uint8_t width, bool be) {
  size_t rem_len = len;
  for(size_t i = 0; i < len; i+=16) {
    char str[120];
    sprintf(str, "%08" PRIx32 ": ", (uint32_t)i+offset);
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
          sprintf(&str[10 + (j+m)*3], "%02x ", data[i+j+k+m]);
          m++;
        }
      } else {
        sprintf(&str[10 + (j)*3], "%02x ", data[i+j]);
      }
    }
    for(size_t j = line_len; j < 16; j++) {
      sprintf(&str[10 + j*3], "   ");
    }
    //str[56] = '|';
    str[58] = ' ';

    // at this point we need to start escaping "%" characters
    char* strPtr = &str[59];
    for(size_t j = 0; j < line_len; j++) {
      char c = data[i+j];
      if((c < ' ') || (c > '~')) {
        c = '.';
      } else if(c == '%') {
        *strPtr++ = '%';
      }
      sprintf(strPtr++, "%c", c);
      
    }
    for(size_t j = line_len; j < 16; j++) {
      sprintf(strPtr++, "   ");
    }
    if(level) {
      RADIOLIB_DEBUG_PRINT(level);
    }
    RADIOLIB_DEBUG_PRINT(str);
    RADIOLIB_DEBUG_PRINTLN();
    rem_len -= 16;
  }
}

void Module::regdump(const char* level, uint16_t start, size_t len) {
  #if RADIOLIB_STATIC_ONLY
    uint8_t buff[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    uint8_t* buff = new uint8_t[len];
  #endif
  SPIreadRegisterBurst(start, len, buff);
  hexdump(level, buff, len, start);
  #if !RADIOLIB_STATIC_ONLY
    delete[] buff;
  #endif
}
#endif

#if RADIOLIB_DEBUG && defined(RADIOLIB_BUILD_ARDUINO)
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
  len = RADIOLIB_DEBUG_PORT.write(reinterpret_cast<const uint8_t*>(buffer), len);
  if (buffer != temp) {
    delete[] buffer;
  }
  return len;
}
#endif

void Module::setRfSwitchPins(uint32_t rxEn, uint32_t txEn) {
  // This can be on the stack, setRfSwitchTable copies the contents
  const uint32_t pins[] = {
    rxEn, txEn, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC,
  };
  
  // This must be static, since setRfSwitchTable stores a reference.
  static const RfSwitchMode_t table[] = {
    { MODE_IDLE,  {this->hal->GpioLevelLow,  this->hal->GpioLevelLow} },
    { MODE_RX,    {this->hal->GpioLevelHigh, this->hal->GpioLevelLow} },
    { MODE_TX,    {this->hal->GpioLevelLow,  this->hal->GpioLevelHigh} },
    END_OF_MODE_TABLE,
  };
  setRfSwitchTable(pins, table);
}

void Module::setRfSwitchTable(const uint32_t (&pins)[RFSWITCH_MAX_PINS], const RfSwitchMode_t table[]) {
  memcpy(this->rfSwitchPins, pins, sizeof(this->rfSwitchPins));
  this->rfSwitchTable = table;
  for(size_t i = 0; i < RFSWITCH_MAX_PINS; i++)
    this->hal->pinMode(pins[i], this->hal->GpioModeOutput);
}

const Module::RfSwitchMode_t *Module::findRfSwitchMode(uint8_t mode) const {
  const RfSwitchMode_t *row = this->rfSwitchTable;
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
  const uint32_t *value = &row->values[0];
  for(size_t i = 0; i < RFSWITCH_MAX_PINS; i++) {
    uint32_t pin = this->rfSwitchPins[i];
    if (pin != RADIOLIB_NC)
      this->hal->digitalWrite(pin, *value);
    ++value;
  }
}
