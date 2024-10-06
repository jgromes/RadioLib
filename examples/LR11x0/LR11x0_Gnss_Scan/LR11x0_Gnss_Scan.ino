// Semtech LR11x0 GNSS scanner, by Thorsten von Eicken, derived from WiFi scanner example
// The configuration is for a Seeed WIO Tracker 1110 dev board (red)
#include <RadioLib.h>
#include "location.h"

// From https://github.com/jgromes/RadioLib/discussions/1101#discussioncomment-9576099
//                        cs, irq, rst busy
LR1110 radio = new Module(44,  2,   42, 43, SPI);

// set RF switch configuration for Wio Tracker with Wio WM1110 Module
// Wio WM1110 uses DIO5 and DIO6 for RF switching
// Wio Tracker 1110 dev board adds LNA for GNSS antenna controlled by PIN_GNSS_LNA
// See https://github.com/Seeed-Studio/LBM_WM1110/blob/main/src/internal/lbm_hal/ral_lr11xx_bsp.c#L121-L132
static const uint32_t rfswitch_dio_pins[] = { 
  RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6,
  RADIOLIB_LR11X0_DIO7, RADIOLIB_LR11X0_DIO8, RADIOLIB_NC
};

static const Module::RfSwitchMode_t rfswitch_table[] = {
  // mode                  DIO5  DIO6 DIO7 DIO8
  { LR11x0::MODE_STBY,   { LOW,  LOW, LOW, LOW } },
  { LR11x0::MODE_RX,     { HIGH, LOW, LOW, LOW } },
  { LR11x0::MODE_TX,     { HIGH, HIGH,LOW, LOW } },
  { LR11x0::MODE_TX_HP,  { LOW,  HIGH,LOW, LOW } },
  { LR11x0::MODE_TX_HF,  { LOW,  LOW, LOW, LOW } },
  { LR11x0::MODE_GNSS,   { LOW,  LOW, HIGH,LOW } },
  { LR11x0::MODE_WIFI,   { LOW,  LOW, LOW, HIGH } },
  END_OF_MODE_TABLE,
};

// https://github.com/Seeed-Studio/LBM_WM1110/blob/main/src/internal/lbm_hal/ral_lr11xx_bsp.c#L178
#define V_TCXO (1.8)

// =====

constexpr int lr1110_almanac_status_offset = 4;
static const char* lr1110_almanac_status[] = {
  "Internal time accuracy too low",
  "No time set",
  "Impossible to find next time",
  "No page id known",
  "No satellite to update",
  "At least 1 satellite must be updated",
  "Invalid",
};

void printAlmStatusPart(const char *gnss, LR11x0GnssAlmanacStatusPart_t &alm) {
  uint8_t st = alm.status + lr1110_almanac_status_offset;
  if (st > sizeof(lr1110_almanac_status)/4) st = sizeof(lr1110_almanac_status)/4-1;
  printf("%s almanac status: '%s' (%d)\n", gnss, lr1110_almanac_status[st], alm.status);
  printf("  SVs needing update: %d (SVs %d %d)\n",
    alm.numUpdateNeeded, alm.nextSubframe4SvId, alm.nextSubframe5SvId);
  printf("  Update in %.1fs, first subframe %d, %d subframes\n",
    float(alm.timeUntilSubframe)/1000, alm.nextSubframeStart, alm.numSubframes);
}

void printAlmStatus(LR11x0GnssAlmanacStatus_t &alm) {
  printAlmStatusPart("GPS", alm.gps);
  printAlmStatusPart("BDU", alm.beidou);
}

static const char* lr1110_scan_mode[] = {
  "Reserved",
  "Reserved",
  "Reserved",
  "Assisted mode (Time and Assisted Position known)",
  "Cold start mode (Time and Assisted Position unknown)",
  "Cold start mode (Time known and Assisted Position unknown)",
  "Fetch time or integrated 2D command launched",
  "Almanac update command launched without flash at the end",
  "Keep sync launched",
  "Almanac Update command launched with 1 constellation flashed at the end",
  "Almanac Update command launched with 2 constellation flashed at the end",
  "Invalid",
};

// ===== Power consumption information

// Power consumed by the lr1110 in each phase as returned by GNSSReadCumulTiming
// the values here get multiplied by millisecs.
// The sample values come from the EVK board in DC-DC mode and are in mA·V, which
// multiplied by ms results in mA·ms or µAs, i.e. µJ (microJoules).
#define V (3.3) // 3.3V
static float lr1110_phase_power[12] = {
  3.15*V, // init
  11.9*V, 3.34*V, 10.7*V, 4.18*V, 1.21*V,  // GPS: capture, processing; adv scan: capture, processing, sleep
  13.5*V, 3.19*V, 12.6*V, 3.43*V, 1.21*V,  // Beidou: capture, processing; adv scan: capture, processing, sleep
  2.53*V,                                  // Demod: sleep(32Mhz)
};

// Print most of the info in the power consumption report, gleaned from
// https://github.com/Lora-net/SWL2001 lr11xx_gnss.c lr11xx_gnss_compute_power_consumption
// For more info, see the LR1110 User Manual, GnssReadCumulTiming command; also AN1200.70
void printPowerInfo(uint32_t *timing, uint8_t constDemod) {
  float power[11];
  for (int i=0; i<11; i++) {
    power[i] = float(timing[i]) * float(lr1110_phase_power[i]) / 32.768; // 32.768kHz
  }
  printf("Last scan timing, power, and %% of power:\n");
  float gpsTime = float(timing[1]+timing[2]+timing[3]+timing[4]+timing[5]) / 32768; // s
  float gpsPower = (power[1]+power[2]+power[3]+power[4]+power[5]) / 1000; // mJ
  float gpsCap = (power[1]+power[3])/10/gpsPower;
  float gpsCPU = (power[2]+power[4])/10/gpsPower;
  float gpsSleep = (power[5])/10/gpsPower;
  float gpsP1 = (power[1]+power[2])/10/gpsPower; // "phase 1"
  float gpsAdv = (power[3]+power[4]+power[5])/10/gpsPower; // "advanced scan" / "multi-scan"
  printf("  GPS: total %.1fs/%.1fmJ | P1:%.1f%% Adv:%.1f%% | capture:%.1f%% cpu:%.1f%% sleep:%.1f%%\n",
    gpsTime, gpsPower, gpsP1, gpsAdv, gpsCap, gpsCPU, gpsSleep);

  float bduTime = float(timing[6]+timing[7]+timing[8]+timing[9]+timing[10]) / 32768; // ms
  float bduPower = (power[6]+power[7]+power[8]+power[9]+power[10]) / 1000; // mJ
  float bduCap = (power[6]+power[8])/10/bduPower;
  float bduCPU = (power[7]+power[9])/10/bduPower;
  float bduSleep = (power[10])/10/bduPower;
  float bduP1 = (power[6]+power[7])/10/bduPower;
  float bduAdv = (power[8]+power[9]+power[10])/10/bduPower;
  printf("  Beidou: total %.1fs/%.1fmJ | P1:%.1f%% Adv:%.1f%% | capture:%.1f%% cpu:%.1f%% sleep:%.1f%%\n",
    bduTime, bduPower, bduP1, bduAdv, bduCap, bduCPU, bduSleep);

  uint8_t off = constDemod == 0 ? 1 : 6; // GPS vs. Beidou
  float demodTime = float(timing[11]+timing[12]+timing[13]+timing[14]) / 32768; // ms
  float demodCap = (timing[11] * lr1110_phase_power[off+2]) / 32768; // mJ
  float demodCPU = (timing[12] * lr1110_phase_power[off+3]) / 32768; // mJ
  float demodSleep = (timing[13] * lr1110_phase_power[off+4] + timing[14] * lr1110_phase_power[11]) / 32768; // mJ
  float demodPower = demodCap + demodCPU + demodSleep; // mJ
  printf("  %s demod: total %.1fs/%.1fmJ | capture:%.1f%% cpu:%.1f%% sleep:%.1f%%\n",
    constDemod == 0 ? "GPS" : "Beidou",
    demodTime, demodPower, demodCap*100/demodPower, demodCPU*100/demodPower, demodSleep*100/demodPower);

  float totalPower = power[0]/1000+gpsPower+bduPower+demodPower;
  printf("  TOTAL: %.1fs/%.1fmJ | GPS:%.1f%% Beidou:%.1f%% Demod:%.1f%% Init:%.1f%%\n",
    float(timing[29])/32768, totalPower,
    gpsPower*100/totalPower, bduPower*100/totalPower, demodPower*100/totalPower, power[0]/10/totalPower);
}

// ===== Geographic distance

float haversine(float lat1, float lon1, float lat2, float lon2) { // radians -> meters
  float avgLat = sin((lat1-lat2)/2);
  float avgLon = sin((lon1-lon2)/2);
  float a = avgLat*avgLat + cos(lat1)*cos(lat2)*avgLon*avgLon;
  return 12742000 * atan2(sqrt(a), sqrt(1-a));
}

// ===== Initialization

bool startScan; // time to start next scan
volatile bool scanFlag = false; // scan completed flag
uint32_t scanStartAt = 0; // millis() when starting scan

void configRadio() {
  Serial.println(F("\n[LR1110] Initializing ... "));
  int state = radio.beginGNSS(V_TCXO, RADIOLIB_LR11X0_GNSS_CONSTELLATION_GPS);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("beginGNSS failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
  // setRfSwitchTable pushes the info into the chip, so it has to happen after beginGNSS which resets the chip
  radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);

  state = radio.isGnssScanCapable();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Can do GNSS scan!");
  } else {
    Serial.print("CANNOT perform GNSS scan, code ");
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set the function that will be called when scan completes
  // radio.setIrqAction(setFlag);

  startScan = true;
  scanFlag = false;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // red LED
  digitalWrite(LED_BUILTIN, LED_STATE_ON);

  Serial.begin(115200);
  Serial.println("Hello!");
  uint32_t t0 = millis();
  while (!Serial.available() && millis()-t0 < 10000) { delay(10); } // blocks 'til USB is opened
  delay(5000); // get a nice long blink...
  digitalWrite(LED_BUILTIN, 1-LED_STATE_ON);
  Serial.println("\n\nWIO Tracker Demo");

  // set RF switch control configuration
  // this has to be done prior to calling begin()
  // radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
  pinMode(PIN_GNSS_LNA, OUTPUT);
  digitalWrite(PIN_GNSS_LNA, 1);

  // initialize LR1110 with default settings
  SPI.setPins(47, 45, 46);
  SPI.begin();

  configRadio();
}

// ===== Loop

// this function is called when a scan is completed
// IMPORTANT: this function MUST be 'void' type and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  scanFlag = true; // scan is complete, set the flag
  Serial.println("IRQ!");
}

void loop() {
  int state;
  uint16_t gnssCount = 0;

  // check if the scan-complete flag is set
  if(scanFlag) {
    // reset flag
    scanFlag = false;
    startScan = true;

    printf("[LR1110] Reading GNSS scan result ...\n");

    // Fetch the scan result (doesn't do much yet)
    state = radio.getGnssScanResult(256);
    if(state != RADIOLIB_ERR_NONE) {
      printf("failed, code %d\n", state);
      return;
    }

    // see whether we have the time
    uint8_t timeErr;
    uint32_t gpsTime, nbUs, timeAcc;
    uint32_t gotTimeAt = millis();
    state = radio.gnssReadTime(&timeErr, &gpsTime, &nbUs, &timeAcc);
    if (state == RADIOLIB_ERR_NONE) {
      if (timeErr == 0) {
        uint32_t unixTime = gpsTime + 315964800 - 18; // 18 leap seconds since 1980
        printf("GPS time=%d %dus accuracy=%dus unix=%d\n", gpsTime, nbUs, timeAcc, unixTime);
      } else {
        printf("GPS time not available (%s)\n",
          timeErr == 1 ? "No 32kHz clock" : timeErr == 2 ? "WN or ToW missing" : "unknown");
      }
    }

    uint16_t rsize=0;
    state = radio.gnssGetResultSize(&rsize);
    if (state == RADIOLIB_ERR_NONE && rsize > 0) {
      uint8_t *buf = (uint8_t *)calloc(rsize, 1);
      state = radio.gnssReadResults(buf, rsize); // handles buf==nullptr
      if (state == RADIOLIB_ERR_NONE) {
        switch (buf[0]) {
        case 1:
          if (timeErr == 0 && timeAcc < 2000000) {
            // we got reasonably accurate time info, include in printf
            // adjust to start of scan with 1s fudget, LoRaCloud is not explicit when to measure
            // and this seems to get very close to the time it ends up reporting
            uint32_t adjTime = gpsTime - (gotTimeAt-scanStartAt)/1000 + 1;
            printf("NAV @%d: ", adjTime);
          } else {
            printf("NAV: ");
          }
          for (int i=1; i<rsize; i++) printf("%02x", buf[i]);
          printf("\n");
          break;
        case 0:
          printf("Scan result status: %d\n", buf[1]);
          break;
        case 2:
          printf("Almanac update message (%d bytes)\n", rsize-1);
          break;
        default:
          printf("Unknown result message (dest=%d, size=%d)\n", buf[0], rsize);
          break;
        }
      }
    }

    // ===== Print various information about the scan

    // print position
    {
      uint8_t error=9;
      uint8_t numSV=0;
      float oneLat=0, oneLon=0, filtLat=0, filtLon=0;
      uint16_t oneAcc=0, filtAcc=0;
      uint16_t oneXtal=0, filtXtal=0;
      state = radio.gnssReadDopplerSolverRes(&error, &numSV,
        &oneLat, &oneLon, &oneAcc, &oneXtal, &filtLat, &filtLon, &filtAcc, &filtXtal);
      if (state != RADIOLIB_ERR_NONE) {
        printf("Reading position failed, code %d", state);
      } else if (error > 0) {
        printf("Position demodulation error %d\n", error);
      } else {
        printf("Demodulated position (using %d SVs):\n", numSV);
        #define R(x) (x*3.141593/180)
        float oneErr = haversine(R(REAL_LAT), R(REAL_LON), R(oneLat), R(oneLon));
        printf("  One-shot: %f %f | accuracy:%d[?] xtal:%dppb | real err: %.0fm\n", oneLat, oneLon, oneAcc, oneXtal, oneErr);
        float filtErr = haversine(R(REAL_LAT), R(REAL_LON), R(filtLat), R(filtLon));
        printf("  Filtered: %f %f | accuracy:%d[?] xtal:%dppb | real err: %.0fm\n", filtLat, filtLon, filtAcc, filtXtal, filtErr);
        #undef R
      }
    }

    // print info about the timing and power consumption
    uint32_t timing[31] = { 0 };
    uint8_t constDemod = 0;
    state = radio.gnssReadCumulTiming(timing, &constDemod);
    if (state == RADIOLIB_ERR_NONE) {
      printPowerInfo(timing, constDemod);
    }

    // ===== Print and check the almanac status and perform an almanac update if appropriate

    LR11x0GnssAlmanacStatus_t almStatus;
    uint32_t start = millis();
    state = radio.getGnssAlmanacStatus(&almStatus);
    if(state != RADIOLIB_ERR_NONE) {
      printf("getGnssAlmanacStatus failed, code %d\n", state);
      return;
    }
    printAlmStatus(almStatus);
    uint32_t dly = almStatus.gps.timeUntilSubframe;
    if (almStatus.gps.numUpdateNeeded > 0 && dly < 60000) { // needed and possible soon
      printf("\n[LR1110] Updating almanac in ~%dms\n", dly);
      dly -= millis()-start + 2300;
      if (dly < 60000) delay(dly); // >60k means less than 2.3s left (or we missed it)

      uint8_t outcome;
      state = radio.updateGnssAlmanac(RADIOLIB_LR11X0_GNSS_CONSTELLATION_GPS, &outcome);
      if (state == RADIOLIB_ERR_NONE) {
        if (outcome > 10) outcome = 11;
        printf("Outcome: '%s' (%d)\n", lr1110_scan_mode[outcome], outcome);
      }

      // see what the almanac status is now
      printf("Almanac update done\n");
      state = radio.getGnssAlmanacStatus(&almStatus);
      if (state == RADIOLIB_ERR_NONE) {
        printAlmStatus(almStatus);
      }

      // print info about the timing and power consumption
      uint32_t timing[31] = { 0 };
      uint8_t constDemod = 0;
      state = radio.gnssReadCumulTiming(timing, &constDemod);
      if (state == RADIOLIB_ERR_NONE) {
        printPowerInfo(timing, constDemod);
      }

    }

    delay(1000); // minor spacing out of scans
  }

  if (startScan) {
    startScan = false;    
    // start scanning again
    while (true) {
      Serial.println(F("\n[LR1110] Starting GNSS scan ... "));
      scanStartAt = millis();
      state = radio.gnssScan(&gnssCount);
      if (state == RADIOLIB_ERR_NONE) {
        scanFlag = 1;
        break;
      } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        configRadio(); // reset radio
      }
    }
  }
}
