// TestTimeOnAirEquivalence.cpp
#include <boost/test/unit_test.hpp>
#include "TestHal.hpp"
#include "modules/SX126x/SX126x.h"
#include "modules/SX128x/SX128x.h"
#include "modules/LR11x0/LR11x0.h"
#include "Module.h"

// -----------------------------------------------------------------------------
// Testable wrappers
// -----------------------------------------------------------------------------

class SX126xTestable : public SX126x {
public:
    explicit SX126xTestable(Module* mod) : SX126x(mod) {}
    uint8_t currentPacketType = RADIOLIB_SX126X_PACKET_TYPE_LORA;
    uint8_t getPacketType() { return currentPacketType; }
};

class SX128xTestable : public SX128x {
public:
    explicit SX128xTestable(Module* mod) : SX128x(mod) {}
    uint8_t currentPacketType = RADIOLIB_SX128X_PACKET_TYPE_LORA;
    uint8_t getPacketType() { return currentPacketType; }
};

class LR11x0Testable : public LR11x0 {
public:
    explicit LR11x0Testable(Module* mod) : LR11x0(mod) {}
    uint8_t currentPacketType = RADIOLIB_LR11X0_PACKET_TYPE_LORA;
    int16_t getPacketType(uint8_t *type) { *type = currentPacketType; return RADIOLIB_ERR_NONE; }
};

// -----------------------------------------------------------------------------
// Fixture (shared for all radios)
// -----------------------------------------------------------------------------
struct TimeOnAirFixture {
    TestHal* hal = nullptr;
    EmulatedRadio* radioHardware = nullptr;
    Module* dummyModule = nullptr;

    TimeOnAirFixture() {
        hal = new TestHal();
        radioHardware = new EmulatedRadio();
        hal->connectRadio(radioHardware);
        dummyModule = new Module(
            hal,
            EMULATED_RADIO_NSS_PIN,
            EMULATED_RADIO_IRQ_PIN,
            EMULATED_RADIO_RST_PIN,
            EMULATED_RADIO_GPIO_PIN
        );
        dummyModule->init();
    }

    ~TimeOnAirFixture() {
        dummyModule->term();
        delete dummyModule;
        delete[] hal;
        delete[] radioHardware;
    }
};

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
template <typename RadioT>
static void compareTimeOnAir(RadioT* radio, size_t len) {
    auto oldVal = radio->getTimeOnAir_old(len);
    auto newVal = radio->getTimeOnAir(len);
    BOOST_TEST_MESSAGE("Len=" << len
                      << " old=" << oldVal
                      << " new=" << newVal);
    BOOST_CHECK_EQUAL(oldVal, newVal);
}

template <typename RadioT>
static void runEquivalenceTests(RadioT* radio,
                                const std::string& name,
                                std::initializer_list<size_t> lens) {
    BOOST_TEST_MESSAGE("--- Test getTimeOnAir " << name);
    for (auto len : lens) {
        compareTimeOnAir(radio, len);
    }
}

// -----------------------------------------------------------------------------
// Test Suite
// -----------------------------------------------------------------------------
BOOST_FIXTURE_TEST_SUITE(suite_TimeOnAirEquivalence, TimeOnAirFixture)

// --- SX126x ---
BOOST_FIXTURE_TEST_CASE(SX126x_LoRa_equivalence, TimeOnAirFixture) {
    SX126xTestable radio(dummyModule);
    radio.currentPacketType = RADIOLIB_SX126X_PACKET_TYPE_LORA;
    radio.setSpreadingFactor(7);
    radio.setBandwidth(125);
    radio.setCodingRate(5);
    radio.implicitHeader(false);
    radio.setPreambleLength(8);
    radio.setCRC(2);
    radio.forceLDRO(false);

    runEquivalenceTests(&radio, "SX126x LoRa", {1, 10, 50, 255});
}

BOOST_FIXTURE_TEST_CASE(SX126x_GFSK_equivalence, TimeOnAirFixture) {
    SX126xTestable radio(dummyModule);
    radio.currentPacketType = RADIOLIB_SX126X_PACKET_TYPE_GFSK;
    radio.setPreambleLength(16);
    uint8_t syncWord[] = {0x2b, 0xd4};
    radio.setSyncBits(syncWord, 16);
    radio.setCRC(2);
    radio.setBitRate(100.0f); // in kbps

    runEquivalenceTests(&radio, "SX126x GFSK", {1, 16, 64, 200});
}

BOOST_FIXTURE_TEST_CASE(SX126x_LRFHSS_equivalence, TimeOnAirFixture) {
    SX126xTestable radio(dummyModule);
    radio.currentPacketType = RADIOLIB_SX126X_PACKET_TYPE_LR_FHSS;
    radio.setLrFhssConfig(RADIOLIB_LR11X0_LR_FHSS_BW_386_72, RADIOLIB_SX126X_LR_FHSS_CR_2_3, 2);

    runEquivalenceTests(&radio, "SX126x LR-FHSS", {1, 20, 100});
}

// --- SX128x ---
BOOST_FIXTURE_TEST_CASE(SX128x_LoRa_equivalence, TimeOnAirFixture) {
    SX128xTestable radio(dummyModule);
    radio.currentPacketType = RADIOLIB_SX128X_PACKET_TYPE_LORA;
    radio.setSpreadingFactor(8);
    radio.setBandwidth(406.25);
    radio.setCodingRate(7);
    radio.implicitHeader(false);
    radio.setPreambleLength(8);
    radio.setCRC(2);

    runEquivalenceTests(&radio, "SX128x LoRa", {1, 50, 200});
}

BOOST_FIXTURE_TEST_CASE(SX128x_GFSK_equivalence, TimeOnAirFixture) {
    SX128xTestable radio(dummyModule);
    radio.currentPacketType = RADIOLIB_SX128X_PACKET_TYPE_GFSK;
    radio.setPreambleLength(16);
    uint8_t syncWord[] = {0x2b, 0xd4};
    radio.setSyncWord(syncWord, 2);
    radio.setCRC(2);
    radio.setBitRate(250.0f);

    runEquivalenceTests(&radio, "SX128x GFSK", {1, 32, 64, 128});
}

// --- LR11x0 ---
BOOST_FIXTURE_TEST_CASE(LR11x0_LoRa_equivalence, TimeOnAirFixture) {
    LR11x0Testable radio(dummyModule);
    radio.currentPacketType = RADIOLIB_LR11X0_PACKET_TYPE_LORA;
    radio.setSpreadingFactor(10);
    radio.setBandwidth(250);
    radio.setCodingRate(5);
    radio.implicitHeader(false);
    radio.setPreambleLength(8);
    radio.setCRC(2);
    radio.forceLDRO(true);

    runEquivalenceTests(&radio, "LR11x0 LoRa", {1, 20, 100});
}

BOOST_FIXTURE_TEST_CASE(LR11x0_LRFHSS_equivalence, TimeOnAirFixture) {
    LR11x0Testable radio(dummyModule);
    radio.currentPacketType = RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS;
    radio.setLrFhssConfig(RADIOLIB_LR11X0_LR_FHSS_BW_136_72, RADIOLIB_LR11X0_LR_FHSS_CR_1_3, 1);

    runEquivalenceTests(&radio, "LR11x0 LR-FHSS", {1, 10, 50});
}

BOOST_FIXTURE_TEST_CASE(LR11x0_GFSK_equivalence, TimeOnAirFixture) {
    LR11x0Testable radio(dummyModule);
    radio.currentPacketType = RADIOLIB_LR11X0_PACKET_TYPE_GFSK;
    radio.setPreambleLength(16);
    uint8_t syncWord[] = {0x2b, 0xd4, 0x2b, 0xd4};
    radio.setSyncWord(syncWord, 4);
    radio.setCRC(2);
    radio.setBitRate(200.0f);

    runEquivalenceTests(&radio, "LR11x0 GFSK", {1, 32, 64, 200});
}

BOOST_AUTO_TEST_SUITE_END()
