#include <Arduino.h>
#include <RadioLib.h>

/*
  Certain packages require platform- or hardware-specific implementations.
  Currently, the package manager only implements TS009 (Certification Protocol), 
  which doesn't require any specific functions.
  However, the package manager is designed to support future packages without
  breaking changes. Hence, this file already exists even though its contents
  are limited.
*/

// Most packages need a true time reference in seconds (not time since boot).
// However, TS009 is an exception that does not require a real time reference,
// so we can just return millis()/1000.
RadioLibTime_t getSeconds() {
  return((RadioLibTime_t)millis() / 1000);
}

// This function is called by a package when it receives a reset command.
void performReboot() {
  Serial.printf("Rebooting now...\r\n");
  
  // ESP example:
  // ESP.restart();
}