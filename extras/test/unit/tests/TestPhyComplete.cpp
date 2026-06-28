#include <boost/test/unit_test.hpp>

#include "ModuleFixture.hpp"

#include "modules/SX126x/SX1261.h"
#include "modules/SX126x/SX1262.h"
#include "modules/SX126x/SX1268.h"
#include "modules/SX127x/SX1272.h"
#include "modules/SX127x/SX1273.h"
#include "modules/SX127x/SX1276.h"
#include "modules/SX127x/SX1277.h"
#include "modules/SX127x/SX1278.h"
#include "modules/SX127x/SX1279.h"
#include "modules/SX128x/SX1280.h"
#include "modules/SX128x/SX1281.h"
#include "modules/SX128x/SX1282.h"
#include "modules/LR11x0/LR1110.h"
#include "modules/LR11x0/LR1120.h"
#include "modules/LR11x0/LR1121.h"
#include "modules/LR2021/LR2021.h"

BOOST_FIXTURE_TEST_SUITE(suite_PhyComplete, ModuleFixture)

BOOST_FIXTURE_TEST_CASE(PhyComplete_AllRadios, ModuleFixture) {
  struct RadioPhy {
    std::string name; // Radio name
    PhysicalLayer* phy;
  };

  hal->spiLogEnabled = false;

  std::vector<RadioPhy> allPhys = {
    { "SX1261", new SX1261(mod) },
    { "SX1262", new SX1262(mod) },
    { "SX1268", new SX1268(mod) },
    { "SX1272", new SX1272(mod) },
    { "SX1273", new SX1273(mod) },
    { "SX1276", new SX1276(mod) },
    { "SX1277", new SX1277(mod) },
    { "SX1278", new SX1278(mod) },
    { "SX1279", new SX1272(mod) },
    { "SX1280", new SX1280(mod) },
    { "SX1281", new SX1281(mod) },
    { "SX1282", new SX1282(mod) },
    { "LR1110", new LR1110(mod) },
    { "LR1120", new LR1120(mod) },
    { "LR1121", new LR1121(mod) },
    { "LR2021", new LR2021(mod) },
  };

  int state;
  uint8_t testBuff[256] = { 0 };
  for(const auto& radio : allPhys) {
    BOOST_TEST_MESSAGE("--- Test phyComplete " << radio.name << " ---");

    state = radio.phy->transmit(testBuff, 1, 0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->sleep();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->standby();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->standby(0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->startReceive();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->startReceive(0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->receive(testBuff, 1);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->startTransmit(testBuff, 1, 0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->finishTransmit();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->finishReceive();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->readData(testBuff, 1);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->transmitDirect();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->receiveDirect();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->setFrequency(0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->setBitRate(0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->setFrequencyDeviation(0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->setDataShaping(0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->setEncoding(0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->invertIQ(false);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->setOutputPower(0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->checkOutputPower(0, nullptr);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->setSyncWord(testBuff, 1);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->setPreambleLength(0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    DataRate_t dr = { .lora = { .spreadingFactor = 0, .bandwidth = 0, .codingRate = 0 } };
    state = radio.phy->setDataRate(dr);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->checkDataRate(dr);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->getPacketLength();
    BOOST_TEST(state != 0);

    state = radio.phy->getRSSI();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->getSNR();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    PacketConfig_t pc = { .lora = { .preambleLength = 0, .implicitHeader = false, 
      .crcEnabled = false, .ldrOptimize = false } };
    state = radio.phy->calculateTimeOnAir(RADIOLIB_MODEM_NONE, dr, pc, 0);
    BOOST_TEST(state != 0);
    
    state = radio.phy->getTimeOnAir(100);
    BOOST_TEST(state != 0);
    
    state = radio.phy->calculateRxTimeout(100000);
    BOOST_TEST(state != 0);

    state = radio.phy->getIrqFlags();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->setIrqFlags(0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->clearIrqFlags(0);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->startChannelScan();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    const ChannelScanConfig_t csc = { .rssi = { .limit = 0 } };
    state = radio.phy->startChannelScan(csc);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->getChannelScanResult();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->scanChannel();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->scanChannel(csc);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->randomByte();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);

    state = radio.phy->setModem(RADIOLIB_MODEM_NONE);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);
    
    ModemType_t modem;
    state = radio.phy->getModem(&modem);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);
    
    RadioModeConfig_t cfg = { .receive = { .timeout = 0, .irqFlags = RADIOLIB_IRQ_RX_DEFAULT_FLAGS, 
      .irqMask = RADIOLIB_IRQ_RX_DEFAULT_MASK, .len = 0 }};
    state = radio.phy->stageMode(RADIOLIB_RADIO_MODE_RX, &cfg);
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);
    
    radio.phy->stagedMode = RADIOLIB_RADIO_MODE_RX;
    state = radio.phy->launchMode();
    BOOST_TEST(state != RADIOLIB_ERR_UNSUPPORTED);
  }
  
}

BOOST_AUTO_TEST_SUITE_END()
