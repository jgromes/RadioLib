/*
   RadioLib Non-Arduino Raspberry Pi Example

   This example shows how to use RadioLib without Arduino.
   In this case, a Raspberry Pi with WaveShare SX1302 LoRaWAN Hat
   using the pigpio library.

   Can be used as a starting point to port RadioLib to any platform!
   See this API reference page for details on the RadioLib hardware abstraction
   https://jgromes.github.io/RadioLib/class_hal.html

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// include the library for Raspberry GPIO pins
#include "pigpio.h"

// create a new Raspberry Pi hardware abstraction layer
// using the pigpio library
// the HAL must inherit from the base RadioLibHal class
// and implement all of its virtual methods
class PiHal : public RadioLibHal {
  public:
    // default constructor - initializes the base HAL and any needed private members
    PiHal(uint8_t spiChannel, uint32_t spiSpeed = 2000000)
      : RadioLibHal(PI_INPUT, PI_OUTPUT, PI_LOW, PI_HIGH, RISING_EDGE, FALLING_EDGE), 
      _spiChannel(spiChannel),
      _spiSpeed(spiSpeed) {
    }

    void init() override {
      // first initialise pigpio library
      gpioInitialise();

      // now the SPI
      spiBegin();

      // Waveshare LoRaWAN Hat also needs pin 18 to be pulled high to enable the radio
      gpioSetMode(18, PI_OUTPUT);
      gpioWrite(18, PI_HIGH);
    }

    void term() override {
      // stop the SPI
      spiEnd();

      // and now the pigpio library
      gpioTerminate();

      // finally, pull the enable pin low
      gpioSetMode(18, PI_OUTPUT);
      gpioWrite(18, PI_LOW);
    }

    // GPIO-related methods (pinMode, digitalWrite etc.) should check
    // RADIOLIB_NC as an alias for non-connected pins
    void pinMode(uint32_t pin, uint32_t mode) override {
      if(pin == RADIOLIB_NC) {
        return;
      }

      gpioSetMode(pin, mode);
    }

    void digitalWrite(uint32_t pin, uint32_t value) override {
      if(pin == RADIOLIB_NC) {
        return;
      }

      gpioWrite(pin, value);
    }

    uint32_t digitalRead(uint32_t pin) override {
      if(pin == RADIOLIB_NC) {
        return(0);
      }

      return(gpioRead(pin));
    }

    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override {
      if(interruptNum == RADIOLIB_NC) {
        return;
      }

      gpioSetISRFunc(interruptNum, mode, 0, (gpioISRFunc_t)interruptCb);
    }

    void detachInterrupt(uint32_t interruptNum) override {
      if(interruptNum == RADIOLIB_NC) {
        return;
      }

      gpioSetISRFunc(interruptNum, NULL, NULL, nullptr);
    }

    void delay(unsigned long ms) override {
      gpioDelay(ms * 1000);
    }

    void delayMicroseconds(unsigned long us) override {
      gpioDelay(us);
    }

    unsigned long millis() override {
      return(gpioTick() / 1000);
    }

    unsigned long micros() override {
      return(gpioTick());
    }

    long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override {
      if(pin == RADIOLIB_NC) {
        return(0);
      }

      gpioSetMode(pin, PI_INPUT);
      uint32_t start = gpioTick();
      uint32_t curtick = gpioTick();

      while(gpioRead(pin) == state) {
        if((gpioTick() - curtick) > timeout) {
          return(0);
        }
      }

      return(gpioTick() - start);
    }

   void spiBegin() {
      if(_spiHandle < 0) {
        _spiHandle = spiOpen(_spiChannel, _spiSpeed, 0);
      }
    }

    void spiBeginTransaction() {}

    uint8_t spiTransfer(uint8_t b) {
      char ret;
      spiXfer(_spiHandle, (char*)&b, &ret, 1);
      return(ret);
    }

    void spiEndTransaction() {}

    void spiEnd() {
      if(_spiHandle >= 0) {
        spiClose(_spiHandle);
        _spiHandle = -1;
      }
    }

  private:
    // the HAL can contain any additional private members
    const unsigned int _spiSpeed;
    const uint8_t _spiChannel;
    int _spiHandle = -1;
};

// create a new instance of the HAL class
// use SPI channel 1, because on Waveshare LoRaWAN Hat,
// the SX1261 CS is connected to CE1
PiHal* hal = new PiHal(1);

// now we can create the radio module
// the first argument is a new instance of the HAL class defined above
// the others are pin numbers
// pinout corresponds to the Waveshare LoRaWAN Hat
// NSS pin:   7
// DIO1 pin:  17
// NRST pin:  22
// BUSY pin:  4
SX1261 radio = new Module(hal, 7, 17, 22, 4);

// the entry point for the program
int main(int argc, char** argv) {
  // initialize just like with Arduino
  printf("[SX1261] Initializing ... ");
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    printf("failed, code %d\n", state);
    return(1);
  }
  printf("success!\n");

  // loop forever
  for(;;) {
    // send a packet
    printf("[SX1261] Transmitting packet ... ");
    state = radio.transmit("Hello World!");
    if(state == RADIOLIB_ERR_NONE) {
      // the packet was successfully transmitted
      printf("success!");

      // wait for a second before transmitting again
      hal->delay(1000);
    
    } else {
      printf("failed, code %d\n", state);

    }
  
  }

  return(0);
}
