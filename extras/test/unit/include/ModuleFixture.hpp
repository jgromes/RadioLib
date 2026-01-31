#ifndef MODULE_FIXTURE_HPP
#define MODULE_FIXTURE_HPP

#include <RadioLib.h>

#include "TestHal.hpp"

// testing fixture
class ModuleFixture {
  public:
    TestHal* hal = nullptr;
    Module* mod = nullptr;
    EmulatedRadio* radioHardware = nullptr;

    ModuleFixture();
    ~ModuleFixture();
};

#endif
