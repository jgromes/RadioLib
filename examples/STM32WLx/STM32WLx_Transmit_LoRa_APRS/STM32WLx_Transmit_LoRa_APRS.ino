// this example uses the embedded LoRa radio to transmit on 433.775 mhz with APRS encoding. 
// it shows a normal aprs.sendPosition, as well as a aprs.sendMicE command. You should choose one. 
// this has been tested to work on both a Nucleo_WL55JC2 (400 mhz version) and the Ebyte E77 module dev kit (400 mhz version)
// include the library
#include <Arduino.h>
#include <RadioLib.h>

int count = 0;

// no need to configure pins, signals are routed to the radio internally
STM32WLx radio = new STM32WLx_Module();

APRSClient aprs(&radio);
#ifdef ARDUINO_LORA_E5_DEV_BOARD
// set RF switch configuration for EBytes E77 dev board
// PB3 is an LED - activates while transmitting
// NOTE: other boards may be different!
//       Some boards may not have either LP or HP.
//       For those, do not set the LP/HP entry in the table.
static const uint32_t rfswitch_pins[] =
                         {PA6,  PA7,  PB3, RADIOLIB_NC, RADIOLIB_NC};
static const Module::RfSwitchMode_t rfswitch_table[] = {
  {STM32WLx::MODE_IDLE,  {LOW,  LOW,  LOW}},
  {STM32WLx::MODE_RX,    {LOW, HIGH, LOW}},
  {STM32WLx::MODE_TX_LP, {HIGH, LOW, HIGH}},
  {STM32WLx::MODE_TX_HP, {HIGH, LOW, HIGH}},
  END_OF_MODE_TABLE,
};

#else
// set RF switch configuration nucleo_wl55jc boards
// NOTE: other boards may be different!
//       Some boards may not have either LP or HP.
//       For those, do not set the LP/HP entry in the table.
static const uint32_t rfswitch_pins[] =
                         {PC3,  PC5,  PC5, RADIOLIB_NC, RADIOLIB_NC};
static const Module::RfSwitchMode_t rfswitch_table[] = {
  {STM32WLx::MODE_IDLE,  {LOW,  LOW,  LOW}},
  {STM32WLx::MODE_RX,    {HIGH, HIGH, LOW}},
  {STM32WLx::MODE_TX_LP, {HIGH, HIGH, HIGH}},
  {STM32WLx::MODE_TX_HP, {HIGH, LOW, HIGH}},
  END_OF_MODE_TABLE,
};
#endif

void setup() {
  Serial.begin(9600);
  radio.setRfSwitchTable(rfswitch_pins, rfswitch_table);
  // set RF switch control configuration
  // this has to be done prior to calling begin()
  //radio.setRfSwitchTable(rfswitch_pins, rfswitch_table);

  // initialize STM32WL with default settings, except frequency
  Serial.print(F("[STM32WL] Initializing ... "));
  //int state = radio.begin(433.775);
  // frequency:                   433.775 MHz
  // bandwidth:                   125 kHz
  // spreading factor:            12
  // coding rate:                 4/5
  int state = radio.begin(433.775, 125, 12, 5);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // initialize APRS client
  Serial.print(F("[APRS] Initializing ... "));
  // symbol:                      '>' (car)
  // callsign                     "NOCALL"  // your call sign
  // SSID                         1
  char source[] = "NOCALL"; // insert your amateur radio callsign here
  state = aprs.begin('>', source, 1);

  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set appropriate TCXO voltage for Nucleo WL55JC1, WL55JC2, or E77 boards
  state = radio.setTCXO(1.7);
  Serial.print(F("Set TCXO voltage ... "));
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set output power to 22 dBm (accepted range is -17 - 22 dBm)
  if (radio.setOutputPower(22) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    Serial.println(F("Selected output power is invalid for this module!"));
    while (true) { delay(10); }
  }
}

void loop() {
  count = count + 1;
  Serial.print(F("[APRS] Sending position ... "));
  
  // send a location with message, no timestamp
  // destSSID is set to 1, as APRS over LoRa uses WIDE1-1 path by default
  char destination[] = "APZATV"; // APZxxx means experimental. More info can be read on page 13-14 of http://www.aprs.org/doc/APRS101.PDF 
  // see https://om1amj.sk/index.php/conversion-of-gps-coordinates-to-aprs-format    
  char latitude[] = "3659.86N";  // actual lat/lon values will need to be converted to aprs lat/lon 
  char longitude[] = "12135.26W";
  char message[] = "this is sendPosition";
  char timestamp[] = "093045z";
  //int state = aprs.sendPosition(destination, 1, latitude, longitude, message, timestamp);
  int state = aprs.sendPosition(destination, 1, latitude, longitude, message);
  delay(5000);

  char status[] = "this is LoRa Mic-E";
  Serial.print(F("[Mic-E] Sending position ... "));
  // send Mic-E encoded messages ( uses actual lat/lon values and don't need to be converted)
  //int state = aprs.sendMicE(37.0027, -121.5826, 270, 0, RADIOLIB_APRS_MIC_E_TYPE_EN_ROUTE);  // simplest form, no status
  state |= state = aprs.sendMicE(36.9976, -121.5877, 270, 0, RADIOLIB_APRS_MIC_E_TYPE_EN_ROUTE, NULL, 0, NULL, status);
  delay(5000);

  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // wait for 10 minutes before transmitting again
  delay(600000);
}
