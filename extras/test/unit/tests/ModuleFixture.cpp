#include <boost/test/unit_test.hpp>

#include "ModuleFixture.hpp"

ModuleFixture::ModuleFixture() { 
  BOOST_TEST_MESSAGE("--- Module fixture setup ---"); 
  hal = new TestHal();
  radioHardware = new EmulatedRadio();
  hal->connectRadio(radioHardware);

  mod = new Module(hal, EMULATED_RADIO_NSS_PIN, EMULATED_RADIO_IRQ_PIN, EMULATED_RADIO_RST_PIN, EMULATED_RADIO_GPIO_PIN);
  mod->init();
}

ModuleFixture::~ModuleFixture() { 
  BOOST_TEST_MESSAGE("--- Module fixture teardown ---");
  mod->term();
  delete mod;
  delete hal;
}
