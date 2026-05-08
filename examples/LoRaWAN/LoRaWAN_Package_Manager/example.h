/*
  Certain packages require platform- or hardware-specific implementations.
  Currently, the package manager only implements TS009 (Certification Protocol).
  However, the package manager is designed to support future packages without
  breaking changes. 
  
  This file provides example implementations of the callbacks that the 
  Package Manager uses to interact with the device and/or application.

  Note: the functions used MUST be configured for your device to function
  correctly with the enabled packages.
*/

#include <Arduino.h>
#include <RadioLib.h>
#include "config.h"

// Most packages need a true time reference in seconds (not time since boot).
// This is the current time in seconds (GPS time since epoch - not Unix!).
// Commented below is a reference implementation for the ESP32.
RadioLibTime_t getSeconds() {
  return(0);

  // return((RadioLibTime_t)time(NULL));
}

// Configure the current true time in seconds (GPS time since epoch - not Unix!).
// Commented below is a reference implementation for the ESP32.
inline void setSeconds(RadioLibTime_t seconds) {
  (void)seconds;

  // struct timeval tv;
  // tv.tv_sec = seconds;
  // tv.tv_usec = 0;
  // settimeofday(&tv, NULL);
}

// This function is called by a package when it receives a reset command.
void performReboot() {
  Serial.println(F("Rebooting now..."));
  
  // ESP example:
  // ESP.restart();
}

// This function is called by a package when it needs to delay for a duration.
void delaySeconds(RadioLibTime_t seconds) {
  Serial.print(F("Delay requested for "));
  Serial.print(seconds);
  Serial.println(F(" seconds"));
  
  // Application specific implementation to handle delays.
  delay(seconds * 1000);
}

// This function is called by a package when it receives a TX periodicity change command.
void setUplinkInterval(RadioLibTime_t intervalSeconds) {
  Serial.print(F("Setting uplink interval to "));
  Serial.print(intervalSeconds);
  Serial.println(F(" seconds"));
  
  // Application specific implementation to set the uplink interval.
  uplinkIntervalSeconds = intervalSeconds;
}
