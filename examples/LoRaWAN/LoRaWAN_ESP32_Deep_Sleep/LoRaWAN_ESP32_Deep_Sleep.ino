/*
  RadioLib ESP32 Deep Sleep LoRaWAN example

  THIS EXAMPLE ONLY WORKS ON ESP32. It assumes the ESP32 stays powered, but puts
  it in a very low-power "deep sleep" mode between sends to the network.

  This example joins a LoRaWAN network and will send uplink packets. Before you
  start, you will have to register your device at
  https://www.thethingsnetwork.org/ After your device is registered, you can run
  this example. The device will join the network and start uploading data.

  LoRaWAN v1.1 requires storing parameters persistently. RadioLib does this by
  using the Arduino EEPROM storage. The ESP32 does not have EEPROM memory, so
  the Arduino code from Espressif has code that emulates EEPROM in its "NVS"
  flash key-value storage. This unfortunately means that every change of a
  single but re- writes the entire emulated "EEPROM" to flash again. 

  That would be great for the occasional configuration change, but we need to
  keep state after every single interaction with the LoRaWAN network. Flash
  memory can only be erased and written to a limited number of times. This is
  why on ESP32 RadioLib stores data using ESP32_RTC_EEPROM, which writes to the
  memory of the ESP32's onboard Real-Time Clock which is kept powered during the
  chip's "deep sleep", which uses almost no power.

  This example then backs up that RTC RAM memory to flash
    a) every time it is woken up with a wiped RTC RAM,
    b) whenever it receives a downlink packet, or 
    c) every so many times as set by BACKUP_EVERY.

  This backup is then automatically loaded when the RTC RAM is empty (after a
  reset or power failure) so that we hang on to enough state information to
  rejoin the network in a secure manner. 

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/

  For LoRaWAN details, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/LoRaWAN


  Last updated March 2024 for RadioLib 6.4.2
*/

// Create a backup copy of the RTC RAM to flash every so many times
#define BACKUP_EVERY 100

// include the library
#include <RadioLib.h>

// SX1262 has the following pin order:
// Module(NSS/CS, DIO1, RESET, BUSY)
SX1262 radio = new Module(8, 14, 12, 13);

// SX1278 has the following pin order:
// Module(NSS/CS, DIO0, RESET, DIO1)
// SX1278 radio = new Module(10, 2, 9, 3);

// create the node instance on the EU-868 band
// using the radio module and the encryption key
// make sure you are using the correct band
// based on your geographical location!
LoRaWANNode node(&radio, &EU868);

// for fixed bands with subband selection
// such as US915 and AU915, you must specify
// the subband that matches the Frequency Plan
// that you selected on your LoRaWAN console
// LoRaWANNode node(&radio, &US915, 2);

// Variables that are placed in RTC RAM survive deep sleep. This counter tells
// us how many times we've booted since the last reset or power loss.
RTC_DATA_ATTR int count = 1;

void setup() {
  Serial.begin(115200);
  
  // initialize radio (SX1262 / SX1278 / ... ) with default settings
  Serial.print("[Radio] Initializing ... ");
  int state = radio.begin();
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.printf("failed, code %i\n", state);
    while(true);
  }

  // JoinEUI - previous versions of LoRaWAN called this AppEUI
  // for development purposes you can use all zeros - see wiki for details
  uint64_t joinEUI = 0x0000000000000000;

  // DevEUI - The device's Extended Unique Identifier
  // TTN will generate one for you
  uint64_t devEUI =  0x----------------;

  // encryption keys used to secure the communication
  // TTN will generate them for you
  // see wiki for details on copying & pasting them
  uint8_t appKey[] = { 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--,   
                       0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x-- };
  uint8_t nwkKey[] = { 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--,   
                       0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x--, 0x-- };

  // Manages uplink intervals to the TTN Fair Use Policy
  node.setDutyCycle(true, 1250);

  bool gotDownlink = false;

  String strUp;
  String strDown;

  // Begin the join to the network
  Serial.print("[LoRaWAN] Attempting over-the-air activation ... ");
  if (count == 1) {
    Serial.print(" (forcing join) ");
    // We're forcing a new join message when we wake up with wiped RTC RAM to
    // make sure there is no outdated information sent and cmmunications are
    // restored promptly.
    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey, RADIOLIB_LORAWAN_DATA_RATE_UNUSED, true);
  } else {
    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  }
  if(state >= RADIOLIB_ERR_NONE) {
    Serial.println("success!");
    delay(2000);	// small delay between joining and uplink
  } else if (state == RADIOLIB_LORAWAN_MODE_OTAA){
    Serial.println("success! (resumed session)");
  } else {
    Serial.printf("failed, code %i\n", state);
    goto sleep;
  }

  // send uplink to port 10
  Serial.print("[LoRaWAN] Sending uplink packet ... ");
  strUp = "Hello! " + String(count);
  state = node.sendReceive(strUp, 10, strDown);
  if(state == RADIOLIB_ERR_NONE) {
    gotDownlink = true;
    Serial.println("received a downlink!");
    // print data of the packet (if there are any)
    Serial.print("[LoRaWAN] Data:\t\t");
    if(strDown.length() > 0) {
      Serial.println(strDown);
    } else {
      Serial.println("<MAC commands only>");
    }
    // print RSSI (Received Signal Strength Indicator)
    Serial.printf("[LoRaWAN] RSSI:\t%.2f dBm\n", radio.getRSSI());
    // print SNR (Signal-to-Noise Ratio)
    Serial.printf("[LoRaWAN] SNR:\t%.2f dB\n", radio.getSNR());
    // print frequency error
    Serial.printf("[LoRaWAN] Freq. err.:\t%.2f Hz\n", radio.getFrequencyError());
  
  } else if(state == RADIOLIB_ERR_RX_TIMEOUT) {
    Serial.println();

  } else {
    Serial.printf("failed, code %i\n", state);
  }

  // This saves to RTC RAM only, so it will survive deep sleep, but will be lost
  // on powercycle or reset.
  node.saveSession();

  // If we woke up with wiped RTC RAM, or received a message, we back it up to
  // flash.
  if (count == 1 || gotDownlink == true || count % BACKUP_EVERY == 0) {
    Serial.println("Saving RTC RAM to NVS flash.");
    EEPROM.toNVS();
  }
  
  count++;

  // Label to jump to from failed beginOTAA, so we try again later
  sleep:

  // Determine wait time before sending another packet
  uint32_t minimumDelay = 300000;                 // try to send once every 5 minutes
  uint32_t interval = node.timeUntilUplink();     // calculate minimum duty cycle delay (per FUP & law!)
  uint32_t delayMs = max(interval, minimumDelay); // cannot send faster than duty cycle allows
  Serial.printf("[LoRaWAN] Next uplink in %i s.\n", Serial.print(delayMs/1000));
 
  // Print message and give it time to get printed
  Serial.println("Entering deep sleep");
  delay(100);
  // Wakeup in microseconds from now
  esp_sleep_enable_timer_wakeup(delayMs * 1000);
  // Go to sleep
  esp_deep_sleep_start();
}

void loop() {
  // this code will never be reached
}
