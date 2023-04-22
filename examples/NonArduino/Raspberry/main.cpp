/*
   RadioLib Non-Arduino Raspberry Pi Example
   
   This example shows how to use RadioLib without Arduino.
   In this case, a CC1101 module is connected to Raspberry Pi
   using the pigpio library.

   Can be used as a starting point to port RadioLib to any platform!
   See this API reference page for details on the RadioLib hardware abstraction
   https://jgromes.github.io/RadioLib/class_hal.html

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include "RadioLib.h"

// include the library for Raspberry GPIO pins
#include "pigpio.h"

// create a new Raspberry Pi hardware abstraction layer
// using the pigpio library
// the HAL must inherit from the base RadioLibHal class
// and implement all of its virtual methods
class PiHal : public RadioLibHal {
  public:
    // default constructor - initializes the base HAL and any needed private members
    PiHal(uint8_t spiChannel = 0, uint32_t spiSpeed = 2000000)
      : RadioLibHal(PI_INPUT, PI_OUTPUT, PI_LOW, PI_HIGH, RISING_EDGE, FALLING_EDGE), 
      _spiChannel(spiChannel),
      _spiSpeed(spiSpeed) {
      
    }

    void init() override {
      // first initialise pigpio library
      gpioInitialise();

      // now the SPI
      spiBegin();
    }

    void term() override {
      // stop the SPI
      spiEnd();

      // and now the pigpio library
      gpioTerminate();
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
      if (_spiHandle >= 0) {
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

// now we can create the radio module
// the first argument is a new isntance of the HAL class defined above
// the others are pin numbers
CC1101 radio = new Module(new PiHal(), 8, 24, RADIOLIB_NC, 25);

// forward declaration of ISR function
void onPacket();

// the entry point for the program
int main(int argc, char** argv) {
  // initialize just like with Arduino
  printf("[CC1101] Initializing ... ");
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    printf("failed, code %d", state );
    return(1);
  }

  // set the function that will be called
  // when new packet is received
  // RISING_EDGE is from the pigpio library
  radio.setGdo0Action(onPacket, RISING_EDGE);

  // start listening for packets
  printf(F("[CC1101] Starting to listen ... "));
  state = radio.startReceive();
  if(state != RADIOLIB_ERR_NONE) {
    printf("failed, code %d", state);
    return(1);
  }
}

void onPacket() {
  // packet received, read the data
  uint8_t byteArr[128];
  int state = radio.readData(byteArr, sizeof(byteArr));

  if (state == RADIOLIB_ERR_NONE) {
    // packet was successfully received
    printf("[CC1101] Received packet!");

    // print the data of the packet
    printf("[CC1101] Data:\t\t");
    for (int b = 0; b < sizeof(byteArr); b++){
      printf("%X", byteArr[b]);
    }
    printf("\n");

    // print RSSI (Received Signal Strength Indicator)
    // of the last received packet
    printf("[CC1101] RSSI:\t\t%d dBm\n", radio.getRSSI());

    // print LQI (Link Quality Indicator)
    // of the last received packet, lower is better
    printf("[CC1101] LQI:\t\t%d\n", radio.getLQI());
  
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    printf("[CC1101] CRC error!\n");
  
  } else {
    // some other error occurred
    printf("[CC1101] Failed, code %d\n", state);
  }

  // put module back to listen mode
  radio.startReceive();
}
