// Semtech LR11x0 GNSS scanner, by Thorsten von Eicken, derived from WiFi scanner example
// The configuration is for a Seeed WIO Tracker 1110 dev board (red)
#include <RadioLib.h>

// From https://github.com/jgromes/RadioLib/discussions/1101#discussioncomment-9576099
LR1110 radio = new Module(44, 2, 42, 43, SPI);

// set RF switch configuration for Wio Tracker with Wio WM1110 Module
// Wio WM1110 uses DIO5 and DIO6 for RF switching
// Wio Tracker 1110 dev board adds LNA for GNSS antenna controlled by PIN_GNSS_LNA
// NOTE: other boards may be different!
static const uint32_t rfswitch_dio_pins[] = { 
  RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6,
  PIN_GNSS_LNA, RADIOLIB_NC, RADIOLIB_NC
};

static const Module::RfSwitchMode_t rfswitch_table[] = {
  // mode                  DIO5  DIO6 GNSS
  { LR11x0::MODE_STBY,   { LOW,  LOW, LOW } },
  { LR11x0::MODE_RX,     { HIGH, LOW, LOW } },
  { LR11x0::MODE_TX,     { HIGH, HIGH,LOW } },
  { LR11x0::MODE_TX_HP,  { LOW,  HIGH,LOW } },
  { LR11x0::MODE_TX_HF,  { LOW,  LOW, LOW } },
  { LR11x0::MODE_GNSS,   { LOW,  LOW, HIGH } },
  { LR11x0::MODE_WIFI,   { LOW,  LOW, LOW } },
  END_OF_MODE_TABLE,
};

// =====

bool startScan;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // red LED
  digitalWrite(LED_BUILTIN, LED_STATE_ON);

  Serial.begin(115200);
  Serial.println("Hello!");
  uint32_t t0 = millis();
  while (!Serial.available() && millis()-t0 < 10000) { delay(10); } // blocks 'til USB is opened
  delay(1000); // get a nice long blink...
  digitalWrite(LED_BUILTIN, 1-LED_STATE_ON);
  Serial.println("\n\nWIO Tracker Demo");

  // set RF switch control configuration
  // this has to be done prior to calling begin()
  radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
  pinMode(PIN_GNSS_LNA, OUTPUT);
  digitalWrite(PIN_GNSS_LNA, 1);

  // initialize LR1110 with default settings
  Serial.print(F("[LR1110] Initializing ... "));
  SPI.setPins(47, 45, 46);
  SPI.begin();
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  state = radio.isGnssScanCapable();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Can do GNSS scan!");
  } else {
    Serial.print("CANNOT perform GNSS scan, code ");
    Serial.println(state);
    while (true) { delay(10); }
  }

  state = radio.configLfClock(0x5); // 32kHz Xtal, busy 'til xtal ready
  RADIOLIB_ASSERT(state);

  // set the function that will be called when scan completes
  radio.setIrqAction(setFlag);

  startScan = true;
}

// flag to indicate that a scan was completed
volatile bool scanFlag = false;
uint16_t gnssCount = 0;

// this function is called when a scan is completed
// IMPORTANT: this function MUST be 'void' type and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  scanFlag = true; // scan is complete, set the flag
}

void loop() {
  int state;

  // check if the flag is set
  if(scanFlag) {
    // reset flag
    scanFlag = false;
    startScan = true;

    Serial.println(F("[LR1110] Reading GNSS scan result ... "));

    state = radio.getGnssScanResult(256);
    if(state != RADIOLIB_ERR_NONE) {
      // some other error occurred
      Serial.print(F("failed, code "));
      Serial.println(state);
    }

    delay(1000);
  }

  if (startScan) {
    startScan = false;    
    // start scanning again
    Serial.println(F("\n[LR1110] ***** Starting GNSS scan ... "));
    while (true) {
      state = radio.gnssScan(&gnssCount);
      if (state == RADIOLIB_ERR_NONE) {
        break;
      } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        delay(1000);
      }
    }
  }
}
