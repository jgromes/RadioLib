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
    ret = mod->SPIgetRegValue(address, msb, lsb);
    BOOST_TEST(ret == 0x3E);

    // invalid mask tests (swapped MSB and LSB, out of range bit masks)
    ret = mod->SPIgetRegValue(address, lsb, msb);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
    ret = mod->SPIgetRegValue(address, 10, lsb);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
    ret = mod->SPIgetRegValue(address, msb, 10);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
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
    ret = mod->SPIgetRegValue(address, msb, lsb);
    BOOST_TEST(ret == 0x3E);

    // invalid mask tests (swapped MSB and LSB, out of range bit masks)
    ret = mod->SPIgetRegValue(address, lsb, msb);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
    ret = mod->SPIgetRegValue(address, 10, lsb);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
    ret = mod->SPIgetRegValue(address, msb, 10);
    BOOST_TEST(ret == RADIOLIB_ERR_INVALID_BIT_RANGE);
  }

BOOST_AUTO_TEST_SUITE_END()
