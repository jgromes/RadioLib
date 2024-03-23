
/*

This demonstrates how to save the join information in to permanent memory
so that if the power fails, batteries run out or are changed, the rejoin
is more efficient & happens sooner due to the way that LoRaWAN secures
the join process - see the wiki for more details.

This is typically useful for devices that need more power than a battery
driven sensor - something like a air quality monitor or GPS based device that
is likely to use up it's power source resulting in loss of the session.

The relevant code is flagged with a ##### comment

Saving the entire session is possible but not demonstrated here - it has
implications for flash wearing and complications with which parts of the 
session may have changed after an uplink. So it is assumed that the device
is going in to deep-sleep, as below, between normal uplinks.

*/

#if !defined(ESP32)
  #pragma error ("This is not the example your device is looking for - ESP32 only")
#endif

// ##### Load the ESP32 preferences facilites
#include <Preferences.h>
Preferences store;

// LoRaWAN config, credentials & pinmap
#include "config.h" 

#include <RadioLib.h>

// Utilities & vars to support ESP32 deep-sleep. The RTC_DATA_ATTR attribute
// puts these in to the RTC memory which is preserved during deep-sleep
RTC_DATA_ATTR uint16_t bootCount = 1;
RTC_DATA_ATTR uint16_t bootCountSinceUnsuccessfulJoin = 0;
RTC_DATA_ATTR uint8_t LWsession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];

// Abbreviated version from the Arduino-ESP32 package, see
// https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/deepsleep.html
// for the complete set of options
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println(F("Wake from sleep"));
  } else {
    Serial.print(F("Wake not caused by deep sleep: "));
    Serial.println(wakeup_reason);
  }

  Serial.print(F("Boot count: "));
  Serial.println(bootCount++);
}

// Put device in to lowest power deep-sleep mode
void gotoSleep(uint32_t seconds) {
  esp_sleep_enable_timer_wakeup(seconds * 1000UL * 1000UL); // Function uses uS
  Serial.println(F("Sleeping\n"));
  Serial.flush();

  esp_deep_sleep_start();

  // If this appears in the serial debug, we didn't go to sleep!
  // So take defensive action so we don't continually uplink
  Serial.println(F("\n\n### Sleep failed, delay of 5 minutes & then restart ###\n"));
  delay(5UL * 60UL * 1000UL);
  ESP.restart();
}



// Setup & execute all device functions ...
void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait for serial to be initalised
  delay(2000);  // Give time to switch to the serial monitor
  Serial.println(F("\nSetup"));
  print_wakeup_reason();

  int16_t state = 0;  // return value for calls to RadioLib

  // Setup the radio based on the pinmap (connections) in config.h
  Serial.println(F("Initalise the radio"));
  state = radio.begin();
  debug(state != RADIOLIB_ERR_NONE, F("Initalise radio failed"), state, true);

  Serial.println(F("Recalling LoRaWAN nonces & session"));
  // ##### Setup the flash storage
  store.begin("radiolib");
  // ##### If we have previously saved nonces, restore them
  if (store.isKey("nonces")) {
    uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];// Create somewhere to store nonces
    store.getBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);// Get them to the store
    state = node.setBufferNonces(buffer); // Send them to LoRaWAN
    debug(state != RADIOLIB_ERR_NONE, F("Restoring nonces buffer failed"), state, false);
  }

  // Recall session from RTC deep-sleep preserved variable
  state = node.setBufferSession(LWsession); // Send them to LoRaWAN stack
  // If we have booted at least once we should have a session to restore, so report any failure
  // Otherwise no point saying there's been a failure when it was bound to fail with an empty 
  // LWsession var. At this point, bootCount has already been incremented, hence the > 2
  debug((state != RADIOLIB_ERR_NONE) && (bootCount > 2), F("Restoring session buffer failed"), state, false);

  // Process the restored session or failing that, create a new one & 
  // return flag to indicate a fresh join is required
  Serial.println(F("Setup LoRaWAN session"));
  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey, false);
  // See comment above, no need to report a failure that is bound to occur on first boot
  debug((state != RADIOLIB_ERR_NONE) && (bootCount > 2), F("Restore session failed"), state, false);

  // Loop until successful join
  while (state != RADIOLIB_ERR_NONE) {
    Serial.println(F("Join ('login') to the LoRaWAN Network"));
    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey, true);

    if (state < RADIOLIB_ERR_NONE) {
      Serial.print(F("Join failed: "));
      Serial.println(state);

      // How long to wait before join attenpts. This is an interim solution pending 
      // implementation of TS001 LoRaWAN Specification section #7 - this doc applies to v1.0.4 & v1.1
      // It sleeps for longer & longer durations to give time for any gateway issues to resolve
      // or whatever is interfering with the device <-> gateway airwaves.
      uint32_t sleepForSeconds = min((bootCountSinceUnsuccessfulJoin++ + 1UL) * 60UL, 3UL * 60UL);
      Serial.print(F("Boots since unsuccessful join: "));
      Serial.println(bootCountSinceUnsuccessfulJoin);
      Serial.print(F("Retrying join in "));
      Serial.print(sleepForSeconds);
      Serial.println(F(" seconds"));

      gotoSleep(sleepForSeconds);

    } else {  // Join was successful
        Serial.println(F("Joined"));

        // ##### Save the join counters (nonces) to permanent store
        Serial.println(F("Saving nonces to flash"));
        uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE]; // Create somewhere to store nonces
        uint8_t *persist = node.getBufferNonces();  // Get pointer to nonces
        memcpy(buffer, persist, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);  // Copy in to buffer
        store.putBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE); // Send them to the store

        // We'll save the session after the uplink

        // Reset the failed join count
        bootCountSinceUnsuccessfulJoin = 0;

        delay(1000);  // Hold off off hitting the airwaves again too soon - an issue in the US

    } // if beginOTAA state
  } // while join

  // ##### Close the store
  store.end();  
  

  // ----- And now for the main event -----
  Serial.println(F("Sending uplink"));

  // Read some inputs
  uint8_t Digital2 = digitalRead(2);
  uint16_t Analog1 = analogRead(A1);

  // Build payload byte array
  uint8_t uplinkPayload[3];
  uplinkPayload[0] = Digital2;
  uplinkPayload[1] = highByte(Analog1);   // See notes for high/lowByte functions
  uplinkPayload[2] = lowByte(Analog1);
  
  // Perform an uplink
  state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload));    
  debug((state != RADIOLIB_ERR_RX_TIMEOUT) && (state != RADIOLIB_ERR_NONE), F("Error in sendReceive"), state, false);

  Serial.print(F("FcntUp: "));
  Serial.println(node.getFcntUp());

  // Now save session to RTC memory
  uint8_t *persist = node.getBufferSession();
  memcpy(LWsession, persist, RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
  
  // Wait until next uplink - observing legal & TTN FUP constraints
  gotoSleep(uplinkIntervalSeconds);

}


// The ESP32 wakes from deep-sleep and starts from the very beginning
// which is a very good place to start, as any singing nun knows.
// It then goes back to sleep, so loop() is never called and which is
// why it is empty.

void loop() {}

