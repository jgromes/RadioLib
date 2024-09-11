// Semtech LR11x0 GNSS scanner, by Thorsten von Eicken, derived from WiFi scanner example
// The configuration is for a Seeed WIO Tracker 1110 dev board (red)
#include <RadioLib.h>

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
  printf("%s almanac status: '%s' (%d), SVs needing update: %d\n",
    gnss, lr1110_almanac_status[st], alm.status, alm.numUpdateNeeded);
  printf("    delay: %dms; subframes: %d; SVs: %d,%d; next subframe: %d\n",
    alm.timeUntilSubframe, alm.numSubframes, alm.nextSubframe4SvId, alm.nextSubframe5SvId, alm.nextSubframeStart);
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


// =====

bool startScan; // time to start next scan
volatile bool scanFlag = false; // scan completed flag

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
#if 0
    while (true) {
      Serial.println(F("\n[LR1110] ***** Starting GNSS time fetch ... "));
      radio.clearErrors();
      state = radio.gnssFetchTime(1, 1);
      if (state == RADIOLIB_ERR_NONE) {
        uint16_t errors = 0;
        state = radio.getErrors(&errors);
        Serial.print("Errors: "); Serial.println(errors, 16);
        break;
      } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        Serial.print("Busy is "); Serial.println(radio.mod->hal->digitalRead(radio.mod->gpioPin);
        uint16_t errors = 0;
        state = radio.getErrors(&errors);
        Serial.print("Errors: "); Serial.println(errors, 16);
        delay(1000);
        setup();
      }
    }


#else

  // check if the flag is set
  if(scanFlag) {
    // reset flag
    scanFlag = false;
    startScan = true;

    Serial.println(F("[LR1110] Reading GNSS scan result ... "));

    state = radio.getGnssScanResult(256);
    if(state != RADIOLIB_ERR_NONE) {
      Serial.print(F("failed, code ")); Serial.println(state);
      return;
    }

    LR11x0GnssAlmanacStatus_t almStatus;
    uint32_t start = millis();
    state = radio.getGnssAlmanacStatus(&almStatus);
    if(state != RADIOLIB_ERR_NONE) {
      Serial.print(F("getGnssAlmanacStatus failed, code ")); Serial.println(state);
      return;
    }
    printAlmStatus(almStatus);
    uint32_t dly = almStatus.gps.timeUntilSubframe;
    if (almStatus.gps.numUpdateNeeded > 0 && dly < 60000) {
      printf("\n[LR1110] Updating almanac in ~%dms\n", dly);
      dly -= millis()-start + 2300;
      if (dly < 60000) delay(dly);
#if 1
      uint8_t outcome;
      state = radio.updateGnssAlmanac(RADIOLIB_LR11X0_GNSS_CONSTELLATION_GPS, &outcome);
      if (state == RADIOLIB_ERR_NONE) {
        if (outcome > 10) outcome = 11;
        printf("Outcome: '%s' (%d)\n", lr1110_scan_mode[outcome], outcome);
      }
#else
      radio.setDioIrqParams(RADIOLIB_LR11X0_IRQ_GNSS_DONE|RADIOLIB_LR11X0_IRQ_GNSS_ABORT);
      state = radio.gnssAlmanacUpdateFromSat(RADIOLIB_LR11X0_GNSS_EFFORT_MID, 0x01);
      if(state != RADIOLIB_ERR_NONE) {
        printf("gnssAlmanacUpdateFromSat failed, code %d\n", state);
        return;
      }

      // wait for scan finished or timeout
      uint32_t timeout = almStatus.gps.numSubframes * 6000 + 4000; // 6 seconds per subframe
      start = millis();
      while (!digitalRead(radio.mod->getIrq())) {
        yield();
        if (millis() - start > timeout) {
          printf("Timeout waiting for almanac update\n");
          radio.standby(); // will fail but abort command
          return;
        }
      }
      // radio.showStatusErrors();
      radio.clearIrq(RADIOLIB_LR11X0_IRQ_ALL);
      printf("GPS almanac update done in %lu ms\n", millis() - start);

      uint8_t lsMode;
      state = radio.gnssReadLastScanModeLaunched(&lsMode);
      if (state == RADIOLIB_ERR_NONE) {
        if (lsMode > 10) lsMode = 11;
        printf("Outcome: '%s' (%d)\n", lr1110_scan_mode[lsMode], lsMode);
      }
#endif

      // see what the almanac status is now
      printf("Almanac update done\n");
      state = radio.getGnssAlmanacStatus(&almStatus);
      if(state != RADIOLIB_ERR_NONE) {
        Serial.print(F("getGnssAlmanacStatus failed, code ")); Serial.println(state);
        return;
      }
      printAlmStatus(almStatus);
    }

    delay(1000); // minor spacing out of scans
  }

  if (startScan) {
    startScan = false;    
    // start scanning again
    while (true) {
      Serial.println(F("\n[LR1110] Starting GNSS scan ... "));
      state = radio.gnssScan(&gnssCount);
      if (state == RADIOLIB_ERR_NONE) {
        scanFlag = 1;
        break;
      } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        configRadio();
      }
    }
  }
#endif
}
