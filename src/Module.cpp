#include "Module.h"

Module::Module(int rx, int tx, HardwareSerial* useSer) {
  _cs = -1;
  _rx = rx;
  _tx = tx;
  _int0 = -1;
  _int1 = -1;

#ifdef RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  ModuleSerial = useSer;
#else
  ModuleSerial = new SoftwareSerial(_rx, _tx);
  (void)useSer;
#endif
}

Module::Module(int cs, int int0, int int1, SPIClass& spi, SPISettings spiSettings) {
  _cs = cs;
  _rx = -1;
  _tx = -1;
  _int0 = int0;
  _int1 = int1;
  _spi = &spi;
  _spiSettings = spiSettings;
}

Module::Module(int cs, int int0, int int1, int rx, int tx, SPIClass& spi, SPISettings spiSettings, HardwareSerial* useSer) {
  _cs = cs;
  _rx = rx;
  _tx = tx;
  _int0 = int0;
  _int1 = int1;
  _spi = &spi;
  _spiSettings = spiSettings;

#ifdef RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  ModuleSerial = useSer;
#else
  ModuleSerial = new SoftwareSerial(_rx, _tx);
  (void)useSer;
#endif
}

Module::Module(int cs, int int0, int int1, int int2, SPIClass& spi, SPISettings spiSettings) {
  _cs = cs;
  _rx = int2;
  _tx = -1;
  _int0 = int0;
  _int1 = int1;
  _spi = &spi;
  _spiSettings = spiSettings;
}

void Module::init(uint8_t interface, uint8_t gpio) {
  // select interface
  switch(interface) {
    case RADIOLIB_USE_SPI:
      setPin(_cs, OUTPUT);
      digitalWrite(_cs, HIGH);
      _spi->begin();
      break;
    case RADIOLIB_USE_UART:
#if defined(ESP32)
      ModuleSerial->begin(baudrate, SERIAL_8N1, _rx, _tx);
#else
      ModuleSerial->begin(baudrate);
#endif
      break;
    case RADIOLIB_USE_I2C:
      break;
  }

  // select GPIO
  switch(gpio) {
    case RADIOLIB_INT_NONE:
      break;
    case RADIOLIB_INT_0:
      setPin(_int0, INPUT);
      break;
    case RADIOLIB_INT_1:
      setPin(_int1, INPUT);
      break;
    case RADIOLIB_INT_BOTH:
      setPin(_int0, INPUT);
      setPin(_int1, INPUT);
      break;
  }
}

void Module::term() {
  // stop SPI
  _spi->end();
}

void Module::ATemptyBuffer() {
  while(ModuleSerial->available() > 0) {
    ModuleSerial->read();
  }
}

bool Module::ATsendCommand(const char* cmd) {
  ATemptyBuffer();
  ModuleSerial->print(cmd);
  ModuleSerial->print(AtLineFeed);
  return(ATgetResponse());
}

bool Module::ATsendData(uint8_t* data, uint32_t len) {
  ATemptyBuffer();
  for(uint32_t i = 0; i < len; i++) {
    ModuleSerial->write(data[i]);
  }

  ModuleSerial->print(AtLineFeed);
  return(ATgetResponse());
}

bool Module::ATgetResponse() {
  String data = "";
  uint32_t start = millis();
  while (millis() - start < _ATtimeout) {
    while(ModuleSerial->available() > 0) {
      char c = ModuleSerial->read();
      RADIOLIB_VERBOSE_PRINT(c);
      data += c;
    }

    if(data.indexOf("OK") != -1) {
      RADIOLIB_VERBOSE_PRINTLN();
      return(true);
    } else if (data.indexOf("ERROR") != -1) {
      RADIOLIB_VERBOSE_PRINTLN();
      return(false);
    }

  }
  RADIOLIB_VERBOSE_PRINTLN();
  return(false);
}

int16_t Module::SPIgetRegValue(uint8_t reg, uint8_t msb, uint8_t lsb) {
  if((msb > 7) || (lsb > 7) || (lsb > msb)) {
    return(ERR_INVALID_BIT_RANGE);
  }

  uint8_t rawValue = SPIreadRegister(reg);
  uint8_t maskedValue = rawValue & ((0b11111111 << lsb) & (0b11111111 >> (7 - msb)));
  return(maskedValue);
}

int16_t Module::SPIsetRegValue(uint8_t reg, uint8_t value, uint8_t msb, uint8_t lsb, uint8_t checkInterval) {
  if((msb > 7) || (lsb > 7) || (lsb > msb)) {
    return(ERR_INVALID_BIT_RANGE);
  }

  uint8_t currentValue = SPIreadRegister(reg);
  uint8_t mask = ~((0b11111111 << (msb + 1)) | (0b11111111 >> (8 - lsb)));
  uint8_t newValue = (currentValue & ~mask) | (value & mask);
  SPIwriteRegister(reg, newValue);

  // check register value each millisecond until check interval is reached
  // some registers need a bit of time to process the change (e.g. SX127X_REG_OP_MODE)
  uint32_t start = micros();
  uint8_t readValue = 0;
  while(micros() - start < (checkInterval * 1000)) {
    readValue = SPIreadRegister(reg);
    if(readValue == newValue) {
      // check passed, we can stop the loop
      return(ERR_NONE);
    }
  }

  // check failed, print debug info
  RADIOLIB_DEBUG_PRINTLN();
  RADIOLIB_DEBUG_PRINT(F("address:\t0x"));
  RADIOLIB_DEBUG_PRINTLN(reg, HEX);
  RADIOLIB_DEBUG_PRINT(F("bits:\t\t"));
  RADIOLIB_DEBUG_PRINT(msb);
  RADIOLIB_DEBUG_PRINT(' ');
  RADIOLIB_DEBUG_PRINTLN(lsb);
  RADIOLIB_DEBUG_PRINT(F("value:\t\t0b"));
  RADIOLIB_DEBUG_PRINTLN(value, BIN);
  RADIOLIB_DEBUG_PRINT(F("current:\t0b"));
  RADIOLIB_DEBUG_PRINTLN(currentValue, BIN);
  RADIOLIB_DEBUG_PRINT(F("mask:\t\t0b"));
  RADIOLIB_DEBUG_PRINTLN(mask, BIN);
  RADIOLIB_DEBUG_PRINT(F("new:\t\t0b"));
  RADIOLIB_DEBUG_PRINTLN(newValue, BIN);
  RADIOLIB_DEBUG_PRINT(F("read:\t\t0b"));
  RADIOLIB_DEBUG_PRINTLN(readValue, BIN);
  RADIOLIB_DEBUG_PRINTLN();

  return(ERR_SPI_WRITE_FAILED);
}

void Module::SPIreadRegisterBurst(uint8_t reg, uint8_t numBytes, uint8_t* inBytes) {
  SPItransfer(SPIreadCommand, reg, NULL, inBytes, numBytes);
}

uint8_t Module::SPIreadRegister(uint8_t reg) {
  uint8_t resp = 0;
  SPItransfer(SPIreadCommand, reg, NULL, &resp, 1);
  return(resp);
}

void Module::SPIwriteRegisterBurst(uint8_t reg, uint8_t* data, uint8_t numBytes) {
  SPItransfer(SPIwriteCommand, reg, data, NULL, numBytes);
}

void Module::SPIwriteRegister(uint8_t reg, uint8_t data) {
  SPItransfer(SPIwriteCommand, reg, &data, NULL, 1);
}

void Module::SPItransfer(uint8_t cmd, uint8_t reg, uint8_t* dataOut, uint8_t* dataIn, uint8_t numBytes) {
  // start SPI transaction
  _spi->beginTransaction(_spiSettings);

  // pull CS low
  digitalWrite(_cs, LOW);

  // send SPI register address with access command
  _spi->transfer(reg | cmd);
  RADIOLIB_VERBOSE_PRINT(reg | cmd, HEX);
  RADIOLIB_VERBOSE_PRINT('\t');
  RADIOLIB_VERBOSE_PRINT(reg | cmd, BIN);
  RADIOLIB_VERBOSE_PRINT('\t');

  // send data or get response
  if(cmd == SPIwriteCommand) {
    for(size_t n = 0; n < numBytes; n++) {
      _spi->transfer(dataOut[n]);
      RADIOLIB_VERBOSE_PRINT(dataOut[n], HEX);
      RADIOLIB_VERBOSE_PRINT('\t');
      RADIOLIB_VERBOSE_PRINT(dataOut[n], BIN);
      RADIOLIB_VERBOSE_PRINT('\t');
    }
  } else if (cmd == SPIreadCommand) {
    for(size_t n = 0; n < numBytes; n++) {
      dataIn[n] = _spi->transfer(0x00);
      RADIOLIB_VERBOSE_PRINT(dataIn[n], HEX);
      RADIOLIB_VERBOSE_PRINT('\t');
      RADIOLIB_VERBOSE_PRINT(dataIn[n], BIN);
      RADIOLIB_VERBOSE_PRINT('\t');
    }
  }
  RADIOLIB_VERBOSE_PRINTLN();

  // release CS
  digitalWrite(_cs, HIGH);

  // end SPI transaction
  _spi->endTransaction();
}

void Module::setPin(int16_t pin, uint8_t mode) {
  if(pin != -1) {
    pinMode(pin, mode);
  }
}
