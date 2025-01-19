// boost test header
#include <boost/test/unit_test.hpp>

// mock HAL
#include "TestHal.hpp"

// testing fixture
struct ModuleFixture {
  TestHal* hal = nullptr;
  Module* mod = nullptr;
  EmulatedRadio* radioHardware = nullptr;

  ModuleFixture()  { 
    BOOST_TEST_MESSAGE("--- Module fixture setup ---"); 
    hal = new TestHal();
    radioHardware = new EmulatedRadio();
    hal->connectRadio(radioHardware);

    mod = new Module(hal, EMULATED_RADIO_NSS_PIN, EMULATED_RADIO_IRQ_PIN, EMULATED_RADIO_RST_PIN, EMULATED_RADIO_GPIO_PIN);
    mod->init();
  }

  ~ModuleFixture() { 
    BOOST_TEST_MESSAGE("--- Module fixture teardown ---");
    mod->term();
    delete[] mod;
    delete[] hal;
  }
};

BOOST_FIXTURE_TEST_SUITE(suite_Module, ModuleFixture)

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
