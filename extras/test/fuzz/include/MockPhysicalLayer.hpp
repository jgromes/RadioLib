#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <RadioLib.h>

// minimalistic HAL for use in fuzzinh
class FuzzHal : public RadioLibHal {
  public:
    FuzzHal() : RadioLibHal(0, 1, 0, 1, 0, 1) {}

    void pinMode(uint32_t pin, uint32_t mode) override { (void)pin; (void)mode; }
    void digitalWrite(uint32_t pin, uint32_t value) override { (void)pin; (void)value; }
    uint32_t digitalRead(uint32_t pin) override { (void)pin; return(0); }
    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override { (void)interruptNum; (void)interruptCb; (void)mode; }
    void detachInterrupt(uint32_t interruptNum) override { (void)interruptNum; }
    void delay(RadioLibTime_t ms) override { (void)ms; }
    void delayMicroseconds(RadioLibTime_t us) override { (void)us; }
    RadioLibTime_t millis() override { return(0); }
    RadioLibTime_t micros() override { return(0); }
    long pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) override { (void)pin; (void)state; (void)timeout; return(0); }
    void spiBegin() override {}
    void spiBeginTransaction() override {}
    void spiTransfer(uint8_t* out, size_t len, uint8_t* in) override { (void)out; if(in) { memset(in, 0xFF, len); } }
    void spiEndTransaction() override {}
    void spiEnd() override {}
};

// minimalistic PHY for use in fuzzing
class FuzzPhysicalLayer : public PhysicalLayer {
  public:
    // mocked "packet" data that is returned upon readData call
    size_t currentPacketLength = 0;
    const uint8_t* currentPacketData = nullptr;
    Module* mod = nullptr;

    FuzzPhysicalLayer() {
      this->maxPacketLength = 256;
    }

    Module* getMod() override {
      return(this->mod);
    }

    size_t getPacketLength(bool update = true) override {
      (void)update;
      return(this->currentPacketLength);
    }

    // implementation reading packet "data" that has been previously provided by the fuzzer
    int16_t readData(uint8_t* data, size_t len) override {
      if(!data) { return(RADIOLIB_ERR_NULL_POINTER); }
      size_t toRead = len < this->currentPacketLength ? len : this->currentPacketLength;
      if(this->currentPacketData) {
        memcpy(data, this->currentPacketData, toRead);
      } else {
        memset(data, 0, toRead);
      }
      return(RADIOLIB_ERR_NONE);
    }
  };
