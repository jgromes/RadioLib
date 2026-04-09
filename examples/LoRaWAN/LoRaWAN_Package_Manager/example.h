/*
  Certain packages require platform- or hardware-specific implementations.
  Currently, the package manager only implements TS009 (Certification Protocol).
  However, the package manager is designed to support future packages without
  breaking changes. 
  
  This file provides example implementations of the callbacks that the 
  Package Manager uses to interact with the device and/or application.
*/

#include <Arduino.h>
#include <RadioLib.h>
#include "config.h"

// Most packages need a true time reference in seconds (not time since boot).
// However, TS009 is an exception that does not require a real time reference,
// so we can just return millis()/1000.
RadioLibTime_t getSeconds() {
  return((RadioLibTime_t)millis() / 1000);
}

// This function is called by a package when it receives a reset command.
void performReboot() {
  Serial.println(F("Rebooting now..."));
  
  // ESP example:
  // ESP.restart();
}

// This function is called by a package when it needs to delay for a duration.
void delaySeconds(uint32_t seconds) {
  Serial.print(F("Delay requested for "));
  Serial.print(seconds);
  Serial.println(F(" seconds"));
  
  // Application specific implementation to handle delays.
  delay(seconds * 1000);
}

// This function is called by a package when it receives a TX periodicity change command.
void setUplinkInterval(uint32_t intervalSeconds) {
  Serial.print(F("Setting uplink interval to "));
  Serial.print(intervalSeconds);
  Serial.println(F(" seconds"));
  
  // Application specific implementation to set the uplink interval.
  uplinkIntervalSeconds = intervalSeconds;
}
