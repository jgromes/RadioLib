// boost test header
#include <boost/test/unit_test.hpp>

// mock HAL
#include "ModuleFixture.hpp"

static int16_t srcParseStatusCb(uint8_t in) { (void)in; return(RADIOLIB_ERR_UNKNOWN); }
static int16_t dstParseStatusCb(uint8_t in) { (void)in; return(RADIOLIB_ERR_UNKNOWN); }
static int16_t srcCheckStatusCb(Module* mod) { (void)mod; return(RADIOLIB_ERR_UNKNOWN); }
static int16_t dstCheckStatusCb(Module* mod) { (void)mod; return(RADIOLIB_ERR_UNKNOWN); }

BOOST_FIXTURE_TEST_SUITE(suite_Module, ModuleFixture)

  BOOST_FIXTURE_TEST_CASE(Module_CopyConstructor, ModuleFixture)
  {
    BOOST_TEST_MESSAGE("--- Test Module::CopyConstructor ---");

    // intentionally not using the Module fixture, since we will be modifying members arbitrarily
    Module src(hal, EMULATED_RADIO_NSS_PIN, EMULATED_RADIO_IRQ_PIN, EMULATED_RADIO_RST_PIN, EMULATED_RADIO_GPIO_PIN);
    Module dst(hal, -1*EMULATED_RADIO_NSS_PIN, -1*EMULATED_RADIO_IRQ_PIN, -1*EMULATED_RADIO_RST_PIN, -1*EMULATED_RADIO_GPIO_PIN);

    // set the source
    src.spiConfig.stream = true;
    src.spiConfig.err = 0xAA;
    for(unsigned int i = 0; i < sizeof(src.spiConfig.cmds)/sizeof(src.spiConfig.cmds[0]); i++) { src.spiConfig.cmds[i] = (2*i) + 1; }
    for(unsigned int i = 0; i < sizeof(src.spiConfig.widths)/sizeof(src.spiConfig.widths[0]); i++) { src.spiConfig.widths[i] = Module::BITS_8; }
    for(unsigned int i = 0; i < sizeof(src.rfSwitchPins)/sizeof(src.rfSwitchPins[0]); i++) { src.rfSwitchPins[i] = (2*i) + 1; }
    src.spiConfig.statusPos = 0x55;
    src.spiConfig.parseStatusCb = srcParseStatusCb;
    src.spiConfig.checkStatusCb = srcCheckStatusCb;
    src.spiConfig.timeout = 3000;
    const Module::RfSwitchMode_t srcRfMode[] = { { Module::MODE_IDLE, { 0, 0 } } };
    src.rfSwitchTable = srcRfMode;
    
    // set up the "modules" so that they are unequal
    dst.spiConfig.stream = !src.spiConfig.stream;
    dst.spiConfig.err = ~src.spiConfig.err;
    for(unsigned int i = 0; i < sizeof(dst.spiConfig.cmds)/sizeof(dst.spiConfig.cmds[0]); i++) { dst.spiConfig.cmds[i] = -1*src.spiConfig.cmds[i]; }
    for(unsigned int i = 0; i < sizeof(dst.spiConfig.widths)/sizeof(dst.spiConfig.widths[0]); i++) { dst.spiConfig.widths[i] = Module::BITS_24; }
    for(unsigned int i = 0; i < sizeof(dst.rfSwitchPins)/sizeof(dst.rfSwitchPins[0]); i++) { dst.rfSwitchPins[i] = -1*src.rfSwitchPins[i]; }
    dst.spiConfig.statusPos = ~src.spiConfig.statusPos;
    dst.spiConfig.parseStatusCb = dstParseStatusCb;
    dst.spiConfig.checkStatusCb = dstCheckStatusCb;
    dst.spiConfig.timeout = -1*src.spiConfig.timeout;
    const Module::RfSwitchMode_t dstRfMode[] = { { Module::MODE_RX, { 1, 1 } } };
    dst.rfSwitchTable = dstRfMode;

    // check unequality before copy
    BOOST_TEST(src.csPin != dst.csPin);
    BOOST_TEST(src.irqPin != dst.irqPin);
    BOOST_TEST(src.rstPin != dst.rstPin);
    BOOST_TEST(src.gpioPin != dst.gpioPin);
    BOOST_TEST(src.spiConfig.stream != dst.spiConfig.stream);
    BOOST_TEST(src.spiConfig.err != dst.spiConfig.err);
    for(unsigned int i = 0; i < sizeof(src.spiConfig.cmds)/sizeof(src.spiConfig.cmds[0]); i++) { BOOST_TEST(dst.spiConfig.cmds[i] != src.spiConfig.cmds[i]); }
    for(unsigned int i = 0; i < sizeof(src.spiConfig.widths)/sizeof(src.spiConfig.widths[0]); i++) { BOOST_TEST(dst.spiConfig.widths[i] != src.spiConfig.widths[i]); }
    BOOST_TEST(src.spiConfig.statusPos != dst.spiConfig.statusPos);
    BOOST_TEST(src.spiConfig.parseStatusCb != dst.spiConfig.parseStatusCb);
    BOOST_TEST(src.spiConfig.checkStatusCb != dst.spiConfig.checkStatusCb);
    BOOST_TEST(src.spiConfig.timeout != dst.spiConfig.timeout);
    for(unsigned int i = 0; i < sizeof(src.rfSwitchPins)/sizeof(src.rfSwitchPins[0]); i++) { BOOST_TEST(dst.rfSwitchPins[i] != src.rfSwitchPins[i]); }
    BOOST_TEST(src.rfSwitchTable != dst.rfSwitchTable);

    // make the copy
    dst = src;

    // check that source remained unchanged
    BOOST_TEST(src.spiConfig.parseStatusCb == srcParseStatusCb);

    // check equality after copy
    BOOST_TEST(src.csPin == dst.csPin);
    BOOST_TEST(src.irqPin == dst.irqPin);
    BOOST_TEST(src.rstPin == dst.rstPin);
    BOOST_TEST(src.gpioPin == dst.gpioPin);
    BOOST_TEST(src.spiConfig.stream == dst.spiConfig.stream);
    BOOST_TEST(src.spiConfig.err == dst.spiConfig.err);
    for(unsigned int i = 0; i < sizeof(src.spiConfig.cmds)/sizeof(src.spiConfig.cmds[0]); i++) { BOOST_TEST(dst.spiConfig.cmds[i] == src.spiConfig.cmds[i]); }
    for(unsigned int i = 0; i < sizeof(src.spiConfig.widths)/sizeof(src.spiConfig.widths[0]); i++) { BOOST_TEST(dst.spiConfig.widths[i] == src.spiConfig.widths[i]); }
    BOOST_TEST(src.spiConfig.statusPos == dst.spiConfig.statusPos);
    BOOST_TEST(src.spiConfig.parseStatusCb == dst.spiConfig.parseStatusCb);
    BOOST_TEST(src.spiConfig.checkStatusCb == dst.spiConfig.checkStatusCb);
    BOOST_TEST(src.spiConfig.timeout == dst.spiConfig.timeout);
    for(unsigned int i = 0; i < sizeof(src.rfSwitchPins)/sizeof(src.rfSwitchPins[0]); i++) { BOOST_TEST(dst.rfSwitchPins[i] == src.rfSwitchPins[i]); }
    BOOST_TEST(src.rfSwitchTable == dst.rfSwitchTable);
  }

  BOOST_FIXTURE_TEST_CASE(Module_SPIgetRegValue_reg, ModuleFixture)
  {
    BOOST_TEST_MESSAGE("--- Test Module::SPIgetRegValue register access ---");
    int16_t ret;

    // basic register read with default config
    const uint8_t address = 0x12;
    const uint8_t spiTxn[] = { address, 0x00 };
    ret = mod->SPIgetRegValue(address);

    // check return code, value and history log
    BOOST_TEST(ret >= RADIOLIB_ERR_NONE);
    BOOST_TEST(ret == EMULATED_RADIO_SPI_RETURN);
    BOOST_TEST(hal->spiLogMemcmp(spiTxn, sizeof(spiTxn)) == 0);

    // register read masking test
    const uint8_t msb = 5;
    const uint8_t lsb = 1;
    const uint8_t maskedValue = 0x3E;
    ret = mod->SPIgetRegValue(address, msb, lsb);
    BOOST_TEST(ret == maskedValue);

    // invalid mask tests (swapped MSB and LSB, out of range bit masks)
    ret = mod->SPIgetRegValue(address, lsb, msb);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
    ret = mod->SPIgetRegValue(address, 10, lsb);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
    ret = mod->SPIgetRegValue(address, msb, 10);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
  }

  BOOST_FIXTURE_TEST_CASE(Module_SPIsetRegValue_reg, ModuleFixture)
  {
    BOOST_TEST_MESSAGE("--- Test Module::SPIsetRegValue register access ---");
    int16_t ret;

    // basic register write with default config
    const uint8_t address = 0x12;
    const uint8_t value = 0xAB;
    const uint8_t spiTxn[] = { address, 0x00, 0x80 | address, value };
    ret = mod->SPIsetRegValue(address, value);

    // check return code and history log
    // this will return write error because the bare emulated radio has no internal logic
    BOOST_TEST(ret == RADIOLIB_ERR_SPI_WRITE_FAILED);
    BOOST_TEST(hal->spiLogMemcmp(spiTxn, sizeof(spiTxn)) == 0);

    // register write masking test
    const uint8_t msb = 5;
    const uint8_t lsb = 1;
    const uint8_t maskedValue = 0xEB;
    const uint8_t spiTxn2[] = { address, 0x00, 0x80 | address, maskedValue };
    ret = mod->SPIsetRegValue(address, value, msb, lsb);
    BOOST_TEST(ret == RADIOLIB_ERR_SPI_WRITE_FAILED);
    BOOST_TEST(hal->spiLogMemcmp(spiTxn2, sizeof(spiTxn2)) == 0);

    // invalid mask tests (swapped MSB and LSB, out of range bit masks)
    ret = mod->SPIsetRegValue(address, value, lsb, msb);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
    ret = mod->SPIsetRegValue(address, value, 10, lsb);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
    ret = mod->SPIsetRegValue(address, value, msb, 10);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);

    // check interval test
    const uint8_t interval = 200;
    const unsigned long start = hal->micros();
    ret = mod->SPIsetRegValue(address, value, 7, 0, interval);
    const unsigned long stop = hal->micros();
    BOOST_TEST(ret == RADIOLIB_ERR_SPI_WRITE_FAILED);
    BOOST_TEST(hal->spiLogMemcmp(spiTxn, sizeof(spiTxn)) == 0);
    const unsigned long elapsed = stop - start;
    BOOST_TEST(elapsed >= (unsigned long)interval*1000UL);

    // disabled check mask test
    ret = mod->SPIsetRegValue(address, value, 7, 0, 2, 0);
    BOOST_TEST(ret == RADIOLIB_ERR_NONE);
    BOOST_TEST(hal->spiLogMemcmp(spiTxn, sizeof(spiTxn)) == 0);

    // forced write test
    ret = mod->SPIsetRegValue(address, value, 7, 0, 2, 0xFF, true);
    BOOST_TEST(ret == RADIOLIB_ERR_SPI_WRITE_FAILED);
    BOOST_TEST(hal->spiLogMemcmp(spiTxn, sizeof(spiTxn)) == 0);
  }

  BOOST_FIXTURE_TEST_CASE(Module_SPIgetRegValue_stream, ModuleFixture)
  {
    BOOST_TEST_MESSAGE("--- Test Module::SPIgetRegValue stream access ---");
    int16_t ret;

    // change settings to stream type
    mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR] = Module::BITS_16;
    mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_8;
    mod->spiConfig.statusPos = 1;
    mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] = RADIOLIB_SX126X_CMD_READ_REGISTER;
    mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] = RADIOLIB_SX126X_CMD_WRITE_REGISTER;
    mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_NOP] = RADIOLIB_SX126X_CMD_NOP;
    mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_STATUS] = RADIOLIB_SX126X_CMD_GET_STATUS;
    mod->spiConfig.stream = true;

    // basic register read
    const uint8_t address = 0x12;
    const uint8_t spiTxn[] = { RADIOLIB_SX126X_CMD_READ_REGISTER, 0x00, address, 0x00, 0x00 };
    ret = mod->SPIgetRegValue(address);

    // check return code, value and history log
    BOOST_TEST(ret >= RADIOLIB_ERR_NONE);
    BOOST_TEST(ret == EMULATED_RADIO_SPI_RETURN);
    BOOST_TEST(hal->spiLogMemcmp(spiTxn, sizeof(spiTxn)) == 0);

    // register read masking test
    const uint8_t msb = 5;
    const uint8_t lsb = 1;
    const uint8_t maskedValue = 0x3E;
    ret = mod->SPIgetRegValue(address, msb, lsb);
    BOOST_TEST(ret == maskedValue);

    // invalid mask tests (swapped MSB and LSB, out of range bit masks)
    ret = mod->SPIgetRegValue(address, lsb, msb);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
    ret = mod->SPIgetRegValue(address, 10, lsb);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
    ret = mod->SPIgetRegValue(address, msb, 10);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
  }

  BOOST_FIXTURE_TEST_CASE(Module_SPIsetRegValue_stream, ModuleFixture)
  {
    BOOST_TEST_MESSAGE("--- Test Module::SPIsetRegValue stream access ---");
    int16_t ret;

    // change settings to stream type
    mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR] = Module::BITS_16;
    mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_8;
    mod->spiConfig.statusPos = 1;
    mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] = RADIOLIB_SX126X_CMD_READ_REGISTER;
    mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] = RADIOLIB_SX126X_CMD_WRITE_REGISTER;
    mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_NOP] = RADIOLIB_SX126X_CMD_NOP;
    mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_STATUS] = RADIOLIB_SX126X_CMD_GET_STATUS;
    mod->spiConfig.stream = true;

    // basic register write with default config
    const uint8_t address = 0x12;
    const uint8_t value = 0xAB;
    const uint8_t spiTxn[] = { 
      RADIOLIB_SX126X_CMD_READ_REGISTER,  0x00, address, 0x00, 0x00,
      RADIOLIB_SX126X_CMD_WRITE_REGISTER, 0x00, address, value,
    };
    ret = mod->SPIsetRegValue(address, value);

    // check return code and history log
    // this will return write error because the bare emulated radio has no internal logic
    BOOST_TEST(ret == RADIOLIB_ERR_SPI_WRITE_FAILED);
    BOOST_TEST(hal->spiLogMemcmp(spiTxn, sizeof(spiTxn)) == 0);

    // register write masking test
    const uint8_t msb = 5;
    const uint8_t lsb = 1;
    const uint8_t maskedValue = 0xEB;
    const uint8_t spiTxn2[] = { 
      RADIOLIB_SX126X_CMD_READ_REGISTER,  0x00, address, 0x00, 0x00,
      RADIOLIB_SX126X_CMD_WRITE_REGISTER, 0x00, address, maskedValue,
    };
    ret = mod->SPIsetRegValue(address, value, msb, lsb);
    BOOST_TEST(ret == RADIOLIB_ERR_SPI_WRITE_FAILED);
    BOOST_TEST(hal->spiLogMemcmp(spiTxn2, sizeof(spiTxn2)) == 0);

    // invalid mask tests (swapped MSB and LSB, out of range bit masks)
    ret = mod->SPIsetRegValue(address, value, lsb, msb);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
    ret = mod->SPIsetRegValue(address, value, 10, lsb);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
    ret = mod->SPIsetRegValue(address, value, msb, 10);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);

    // check interval test
    const uint8_t interval = 200;
    const unsigned long start = hal->micros();
    ret = mod->SPIsetRegValue(address, value, 7, 0, interval);
    const unsigned long stop = hal->micros();
    BOOST_TEST(ret == RADIOLIB_ERR_SPI_WRITE_FAILED);
    BOOST_TEST(hal->spiLogMemcmp(spiTxn, sizeof(spiTxn)) == 0);
    const unsigned long elapsed = stop - start;
    BOOST_TEST(elapsed >= (unsigned long)interval*1000UL);

    // disabled check mask test
    ret = mod->SPIsetRegValue(address, value, 7, 0, 2, 0);
    BOOST_TEST(ret == RADIOLIB_ERR_NONE);
    BOOST_TEST(hal->spiLogMemcmp(spiTxn, sizeof(spiTxn)) == 0);

    // forced write test
    ret = mod->SPIsetRegValue(address, value, 7, 0, 2, 0xFF, true);
    BOOST_TEST(ret == RADIOLIB_ERR_SPI_WRITE_FAILED);
    BOOST_TEST(hal->spiLogMemcmp(spiTxn, sizeof(spiTxn)) == 0);
  }

BOOST_AUTO_TEST_SUITE_END()
