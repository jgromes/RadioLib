#include <boost/test/unit_test.hpp>
#include "modules/SX126x/SX126x.h"
#include "modules/SX127x/SX127x.h"
#include "modules/SX128x/SX128x.h"
#include "modules/LR11x0/LR11x0.h"

// --- Config structure ---
struct RadioConfig {
    std::string name; // Radio name
    ModemType_t modem;
    DataRate_t dr;
    PacketConfig_t pc;
    std::vector<size_t> payload_len;
    std::vector<RadioLibTime_t> expected_toa; // Expected time on air in microseconds from Semtech calculators
};

// --- Test configurations with golden values ---
std::vector<RadioConfig> allConfigs = {
    { "SX126x", RADIOLIB_MODEM_LORA,  {.lora={7,125,5}}, {.lora={8,false,true,true}}, {1,10,50,255}, {30976,46336,128256,548096} }, // 30.97, 46.33, 128.25, 548.09
    { "SX126x", RADIOLIB_MODEM_LORA,  {.lora={11,250,8}}, {.lora={16,true,false,false}}, {5,15,100,200}, {296960,362496,1411072,2590720} }, // 296.96, 362.49, 1410, 2590
    { "SX126x", RADIOLIB_MODEM_FSK,   {.fsk={100,10}},   {.fsk={16,16,2}}, {1,16,64,200}, {560,1760,5600,16480} },
    { "SX126x", RADIOLIB_MODEM_LRFHSS,{.lrFhss={RADIOLIB_LR11X0_LR_FHSS_BW_386_72,RADIOLIB_SX126X_LR_FHSS_CR_2_3,false}}, {.lrFhss={2}}, {1,20,100}, {3784697,4259832,6324212} },

    { "SX127x", RADIOLIB_MODEM_LORA, {.lora={6,125,6}}, {.lora={8,false,true,false}}, {7,23,98,156}, {23000,39000,115000,174000} }, // 20.61, 39.04, 115.84, 174.21 
    { "SX127x", RADIOLIB_MODEM_LORA, {.lora={8,250,8}}, {.lora={32,true,true,false}}, {10,20,80,160}, {70000,87000,210000,373000} }, // 69.89, 86.27, 209.15, 372.99
    { "SX127x", RADIOLIB_MODEM_FSK,  {.fsk={100,5}},    {.fsk={16,16,3}}, {1,16,32,61}, {640,1840,3120,5440} },

    { "SX128x", RADIOLIB_MODEM_LORA, {.lora={5,400,5}}, {.lora={8,false,true,false}}, {1,50,200}, {2580,10179,34180} }, // 2.54, 10.02, 33.65
    { "SX128x", RADIOLIB_MODEM_LORA, {.lora={12,800,7}}, {.lora={16,false,true,true}}, {10,100,250}, {216319,861440,1936640} }, // 212.99, 848.19, 1910
    { "SX128x", RADIOLIB_MODEM_FSK,  {.fsk={250,100}},  {.fsk={16,16,2}}, {1,32,64,128}, {224,1216,2240,4288} },

    { "LR11x0", RADIOLIB_MODEM_LORA, {.lora={10,250,5}}, {.lora={8,false,true,true}}, {1,20,100}, {103424,205824,615424} }, // 103.42, 205.82, 615.42
    { "LR11x0", RADIOLIB_MODEM_LORA, {.lora={11,500,6}}, {.lora={32,true,false,false}}, {10,25,200}, {205824,279552,1065984} }, // 205.82, 279.55, 1070
    { "LR11x0", RADIOLIB_MODEM_FSK,  {.fsk={200,50}},   {.fsk={16,32,2}}, {1,32,64,200}, {360,1600,2880,8320} },
    { "LR11x0", RADIOLIB_MODEM_LRFHSS,{.lrFhss={RADIOLIB_LR11X0_LR_FHSS_BW_136_72,RADIOLIB_LR11X0_LR_FHSS_CR_1_3,true}}, {.lrFhss={1}}, {1,10,50}, {1949692,2392059,4456440} },    
};

BOOST_AUTO_TEST_SUITE(suite_TimeOnAir)

BOOST_AUTO_TEST_CASE(TimeOnAir_AllRadios) {
    for (const auto& cfg : allConfigs) {
        BOOST_TEST_MESSAGE("--- Test calculateTimeOnAir " << cfg.name << ", modem=" << cfg.modem << " ---");

        for (size_t i = 0; i < cfg.payload_len.size(); i++) {
            auto len = cfg.payload_len[i];
            RadioLibTime_t toa = 0;

            if (cfg.name == "SX126x") {
                SX126x dummy(nullptr);
                toa = dummy.calculateTimeOnAir(cfg.modem, cfg.dr, cfg.pc, len);
            } else if (cfg.name == "SX127x") {
                SX127x dummy(nullptr);
                toa = dummy.calculateTimeOnAir(cfg.modem, cfg.dr, cfg.pc, len);
            } else if (cfg.name == "SX128x") {
                SX128x dummy(nullptr);
                toa = dummy.calculateTimeOnAir(cfg.modem, cfg.dr, cfg.pc, len);
            } else if (cfg.name == "LR11x0") {
                LR11x0 dummy(nullptr);
                toa = dummy.calculateTimeOnAir(cfg.modem, cfg.dr, cfg.pc, len);
            }

            BOOST_CHECK_EQUAL(toa, cfg.expected_toa[i]);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
