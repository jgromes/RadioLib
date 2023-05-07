// this is an autotest file for the SX126x
// runs on Raspberry Pi with Waveshare LoRaWAN hat

#include <RadioLib/RadioLib.h>
#include "PiHal.h"

PiHal* hal = new PiHal(1);
SX1261 radio = new Module(hal, 7, 17, 22, 4);

// the entry point for the program
int main(int argc, char** argv) {
  int state = RADIOLIB_ERR_UNKNOWN;

  state = radio.begin();
  printf("[SX1261] Test:begin() = %d\n", state);

  //state = radio.transmit("Hello World!");
  //printf("[SX1261] Test:transmit() = %d\n", state);

  hal->term();
  return(0);
}
