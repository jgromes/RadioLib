#include "RadioLib.h"
#include "pigpio.h"

class PiHal : public Hal {
 public:
  PiHal(uint8_t spiChannel = 0, uint32_t spiSpeed = 2000000)
      : Hal(PI_INPUT, PI_OUTPUT, PI_LOW, PI_HIGH, RISING_EDGE, FALLING_EDGE),
        _spiChannel(spiChannel),
        _spiSpeed(spiSpeed) {}

  void init() override {
    gpioInitialise();
    spiBegin();
  }
  void term() override {
    spiEnd();
    gpioTerminate();
  }

  void pinMode(uint32_t pin, uint32_t mode) override {
    if (pin == RADIOLIB_NC) return;
    gpioSetMode(pin, mode);
  }

  void digitalWrite(uint32_t pin, uint32_t value) override {
    if (pin == RADIOLIB_NC) return;
    gpioWrite(pin, value);
  }

  uint32_t digitalRead(uint32_t pin) override {
    if (pin == RADIOLIB_NC) return 0;
    return gpioRead(pin);
  }

  void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void),
                       uint32_t mode) override {
    if (interruptNum == RADIOLIB_NC) return;
    gpioSetISRFunc(interruptNum, mode, 0, (gpioISRFunc_t)interruptCb);
  }

  void detachInterrupt(uint32_t interruptNum) override {
    if (interruptNum == RADIOLIB_NC) return;
    gpioSetISRFunc(interruptNum, NULL, NULL, nullptr);
  }

  void delay(unsigned long ms) override { gpioDelay(ms * 1000); }

  void delayMicroseconds(unsigned long us) override { gpioDelay(us); }

  unsigned long millis() override { return gpioTick() / 1000; }

  unsigned long micros() override { return gpioTick(); }

  long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override {
    if (pin == RADIOLIB_NC) return 0;
    gpioSetMode(pin, PI_INPUT);
    uint32_t start = gpioTick();
    uint32_t curtick = gpioTick();

    while (gpioRead(pin) == state)
      if ((gpioTick() - curtick) > timeout) return 0;
    while (gpioRead(pin) != state)
      if ((gpioTick() - curtick) > timeout) return 0;
    while (gpioRead(pin) == state)
      if ((gpioTick() - curtick) > timeout) return 0;

    return gpioTick() - start;
  }

  void spiBegin() {
    if (_spiHandle < 0) {
      _spiHandle = spiOpen(_spiChannel, _spiSpeed, 0);
    }
  }

  void spiBeginTransaction() {}

  uint8_t spiTransfer(uint8_t b) {
    char ret;
    spiXfer(_spiHandle, (char*)&b, &ret, 1);
    return ret;
  }

  void spiEndTransaction() {}

  void spiEnd() {
    if (_spiHandle >= 0) {
      spiClose(_spiHandle);
      _spiHandle = -1;
    }
  }

 private:
  const unsigned int _spiSpeed;
  const uint8_t _spiChannel;
  int _spiHandle = -1;
};

CC1101 radio = new Module(new PiHal(), 8, 24, RADIOLIB_NC, 25);

void onPacket() {
  uint8_t* byteArr = new uint8_t[128];
  int state = radio.readData(byteArr, sizeof(byteArr));

  if (state == RADIOLIB_ERR_NONE) {
    // packet was successfully received
    printf("success!\n");

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
  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    printf("timeout!\n");
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    printf("CRC error!\n");
  } else {
    printf("failed, code %d\n", state);
  }

  radio.startReceive();
}

int main(int argc, char** argv) {
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    printf("init failed, code %d", state);
    return 1;
  }

  radio.setGdo0Action(onPacket, RISING_EDGE);
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    printf("start receive failed, code %d", state);
    return 1;
  }
}

