#include "Module.h"

// the following is probably only needed on non-Arduino builds
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#if defined(RADIOLIB_DEBUG)
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
  this->SPIreadCommand = mod.SPIreadCommand;
  this->SPIwriteCommand = mod.SPIwriteCommand;
  this->csPin = mod.csPin;
  this->irqPin = mod.irqPin;
  this->rstPin = mod.rstPin;
  this->gpioPin = mod.gpioPin;
  return(*this);
}

void Module::init() {
  this->hal->init();
  this->hal->pinMode(csPin, this->hal->GpioModeOutput);
  this->hal->digitalWrite(csPin, this->hal->GpioLevelHigh);
}

void Module::term() {
  // stop hardware interfaces (if they were initialized by the library)
  this->hal->term();
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
    uint32_t start = this->hal->micros();
    uint8_t readValue = 0x00;
    while(this->hal->micros() - start < (checkInterval * 1000)) {
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
    SPItransferStream(cmd, 3, false, NULL, inBytes, numBytes, true, RADIOLIB_MODULE_SPI_TIMEOUT);
  }
}

uint8_t Module::SPIreadRegister(uint16_t reg) {
  uint8_t resp = 0;
  if(!SPIstreamType) {
    SPItransfer(SPIreadCommand, reg, NULL, &resp, 1);
  } else {
    uint8_t cmd[] = { SPIreadCommand, (uint8_t)((reg >> 8) & 0xFF), (uint8_t)(reg & 0xFF) };
    SPItransferStream(cmd, 3, false, NULL, &resp, 1, true, RADIOLIB_MODULE_SPI_TIMEOUT);
  }
  return(resp);
}

void Module::SPIwriteRegisterBurst(uint16_t reg, uint8_t* data, size_t numBytes) {
  if(!SPIstreamType) {
    SPItransfer(SPIwriteCommand, reg, data, NULL, numBytes);
  } else {
    uint8_t cmd[] = { SPIwriteCommand, (uint8_t)((reg >> 8) & 0xFF), (uint8_t)(reg & 0xFF) };
    SPItransferStream(cmd, 3, true, data, NULL, numBytes, true, RADIOLIB_MODULE_SPI_TIMEOUT);
  }
}

void Module::SPIwriteRegister(uint16_t reg, uint8_t data) {
  if(!SPIstreamType) {
    SPItransfer(SPIwriteCommand, reg, &data, NULL, 1);
  } else {
    uint8_t cmd[] = { SPIwriteCommand, (uint8_t)((reg >> 8) & 0xFF), (uint8_t)(reg & 0xFF) };
    SPItransferStream(cmd, 3, true, &data, NULL, 1, true, RADIOLIB_MODULE_SPI_TIMEOUT);
  }
}

void Module::SPItransfer(uint8_t cmd, uint16_t reg, uint8_t* dataOut, uint8_t* dataIn, size_t numBytes) {
  // start SPI transaction
  this->hal->spiBeginTransaction();

  // pull CS low
  this->hal->digitalWrite(this->csPin, this->hal->GpioLevelLow);

  // send SPI register address with access command
  if(this->SPIaddrWidth <= 8) {
    this->hal->spiTransfer(reg | cmd);
  } else {
    this->hal->spiTransfer((reg >> 8) | cmd);
    this->hal->spiTransfer(reg & 0xFF);
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
        this->hal->spiTransfer(dataOut[n]);
        RADIOLIB_VERBOSE_PRINT("%X\t", dataOut[n]);
      }
    }
  } else if (cmd == SPIreadCommand) {
    if(dataIn != NULL) {
      for(size_t n = 0; n < numBytes; n++) {
        dataIn[n] = this->hal->spiTransfer(0x00);
        RADIOLIB_VERBOSE_PRINT("%X\t", dataIn[n]);
      }
    }
  }
  RADIOLIB_VERBOSE_PRINTLN();

  // release CS
  this->hal->digitalWrite(this->csPin, this->hal->GpioLevelHigh);

  // end SPI transaction
  this->hal->spiEndTransaction();
}

int16_t Module::SPIreadStream(uint8_t cmd, uint8_t* data, size_t numBytes, bool waitForGpio, bool verify) {
  return(this->SPIreadStream(&cmd, 1, data, numBytes, waitForGpio, verify));
}

int16_t Module::SPIreadStream(uint8_t* cmd, uint8_t cmdLen, uint8_t* data, size_t numBytes, bool waitForGpio, bool verify) {
  // send the command
  int16_t state = this->SPItransferStream(cmd, cmdLen, false, NULL, data, numBytes, waitForGpio, RADIOLIB_MODULE_SPI_TIMEOUT);
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
  int16_t state = this->SPItransferStream(cmd, cmdLen, true, data, NULL, numBytes, waitForGpio, RADIOLIB_MODULE_SPI_TIMEOUT);
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
  state = this->SPItransferStream(&cmd, 1, false, NULL, &spiStatus, 1, true, RADIOLIB_MODULE_SPI_TIMEOUT);
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
  uint32_t start = this->hal->millis();
  while(this->hal->digitalRead(this->gpioPin)) {
    this->hal->yield();
    if(this->hal->millis() - start >= timeout) {
      RADIOLIB_DEBUG_PRINTLN("Timed out waiting for GPIO pin, is it connected?");
      return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
    }
  }

  // pull NSS low
  this->hal->digitalWrite(this->csPin, this->hal->GpioLevelLow);

  // start transfer
  this->hal->spiBeginTransaction();

  // send command byte(s)
  for(uint8_t n = 0; n < cmdLen; n++) {
    this->hal->spiTransfer(cmd[n]);
  }

  // variable to save error during SPI transfer
  int16_t state = RADIOLIB_ERR_NONE;

  // send/receive all bytes
  if(write) {
    for(size_t n = 0; n < numBytes; n++) {
      // send byte
      uint8_t in = this->hal->spiTransfer(dataOut[n]);
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
    uint8_t in = this->hal->spiTransfer(this->SPInopCommand);
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
        dataIn[n] = this->hal->spiTransfer(this->SPInopCommand);
      }
    }
  }

  // stop transfer
  this->hal->spiEndTransaction();
  this->hal->digitalWrite(this->csPin, this->hal->GpioLevelHigh);

  // wait for GPIO to go high and then low
  if(waitForGpio) {
    this->hal->delayMicroseconds(1);
    uint32_t start = this->hal->millis();
    while(this->hal->digitalRead(this->gpioPin)) {
      this->hal->yield();
      if(this->hal->millis() - start >= timeout) {
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
  if((this->TimerSetupCb != nullptr) && (len != this->prevTimingLen)) {
    _prevTimingLen = len;
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

void Module::setRfSwitchPins(uint32_t rxEn, uint32_t txEn) {
  // This can be on the stack, setRfSwitchTable copies the contents
  const uint32_t pins[] = {
    rxEn, txEn, RADIOLIB_NC,
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

void Module::setRfSwitchTable(const uint32_t (&pins)[3], const RfSwitchMode_t table[]) {
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
