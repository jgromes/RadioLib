#ifndef TEST_HAL_HPP
#define TEST_HAL_HPP

#include <chrono>
#include <thread>
#include <fmt/format.h>

#include <RadioLib.h>

#include <boost/log/trivial.hpp>
#include <boost/format.hpp>

#if defined(TEST_HAL_LOG)
#define HAL_LOG(...) BOOST_TEST_MESSAGE(__VA_ARGS__)
#else
#define HAL_LOG(...) {}
#endif

#include "HardwareEmulation.hpp"

#define TEST_HAL_INPUT          (0)
#define TEST_HAL_OUTPUT         (1)
#define TEST_HAL_LOW            (0)
#define TEST_HAL_HIGH           (1)
#define TEST_HAL_RISING         (0)
#define TEST_HAL_FALLING        (1)

// number of emulated GPIO pins
#define TEST_HAL_NUM_GPIO_PINS  (32)

#define TEST_HAL_SPI_LOG_LENGTH (512)

class TestHal : public RadioLibHal {
  public:
    TestHal() : RadioLibHal(TEST_HAL_INPUT, TEST_HAL_OUTPUT, TEST_HAL_LOW, TEST_HAL_HIGH, TEST_HAL_RISING, TEST_HAL_FALLING) { }

    void init() override {
      HAL_LOG("TestHal::init()");

      // save program start timestamp
      start = std::chrono::high_resolution_clock::now();

      // init emulated GPIO
      for(int i = 0; i < TEST_HAL_NUM_GPIO_PINS; i++) {
        this->gpio[i].mode = 0;
        this->gpio[i].value = 0;
        this->gpio[i].event = false;
        this->gpio[i].func = PIN_UNASSIGNED;
      }

      // wipe history log
      this->spiLogWipe();
    }

    void term() override {
      HAL_LOG("TestHal::term()");
    }

    void pinMode(uint32_t pin, uint32_t mode) override {
      HAL_LOG("TestHal::pinMode(pin=" << pin << ", mode=" << mode << " [" << ((mode == TEST_HAL_INPUT) ? "INPUT" : "OUTPUT") << "])");
      
      // check the range
      BOOST_ASSERT_MSG(pin < TEST_HAL_NUM_GPIO_PINS, "Pin number out of range");

      // check known modes
      BOOST_ASSERT_MSG(((mode == TEST_HAL_INPUT) || (mode == TEST_HAL_OUTPUT)), "Invalid pin mode");

      // set mode
      this->gpio[pin].mode = mode;
    }

    void digitalWrite(uint32_t pin, uint32_t value) override {
      HAL_LOG("TestHal::digitalWrite(pin=" << pin << ", value=" << value << " [" << ((value == TEST_HAL_LOW) ? "LOW" : "HIGH") << "])");

      // check the range
      BOOST_ASSERT_MSG(pin < TEST_HAL_NUM_GPIO_PINS, "Pin number out of range");

      // check it is output
      BOOST_ASSERT_MSG(this->gpio[pin].mode == TEST_HAL_OUTPUT, "GPIO is not output!");

      // check known values
      BOOST_ASSERT_MSG(((value == TEST_HAL_LOW) || (value == TEST_HAL_HIGH)), "Invalid output value");

      // set value
      this->gpio[pin].value = value;
      this->gpio[pin].event = true;
      if(radio) {
        this->radio->HandleGPIO();
      }
      this->gpio[pin].event = false;
    }

    uint32_t digitalRead(uint32_t pin) override {
      HAL_LOG("TestHal::digitalRead(pin=" << pin << ")");

      // check the range
      BOOST_ASSERT_MSG(pin < TEST_HAL_NUM_GPIO_PINS, "Pin number out of range");

      // check it is input
      BOOST_ASSERT_MSG(this->gpio[pin].mode == TEST_HAL_INPUT, "GPIO is not input");

      // read the value
      uint32_t value = this->gpio[pin].value;
      HAL_LOG("TestHal::digitalRead(pin=" << pin << ")=" << value << " [" << ((value == TEST_HAL_LOW) ? "LOW" : "HIGH") << "]");
      return(value);
    }

    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override {
      HAL_LOG("TestHal::attachInterrupt(interruptNum=" << interruptNum << ", interruptCb=" << interruptCb << ", mode=" << mode << ")");

      // TODO implement
      (void)interruptNum;
      (void)interruptCb;
      (void)mode;
    }

    void detachInterrupt(uint32_t interruptNum) override {
      HAL_LOG("TestHal::detachInterrupt(interruptNum=" << interruptNum << ")");

      // TODO implement
      (void)interruptNum;
    }

    void delay(unsigned long ms) override {
      HAL_LOG("TestHal::delay(ms=" << ms << ")");
      const auto start = std::chrono::high_resolution_clock::now();

      // sleep_for is sufficient for ms-precision sleep
      std::this_thread::sleep_for(std::chrono::duration<unsigned long, std::milli>(ms));

      // measure and print
      const auto end = std::chrono::high_resolution_clock::now();
      const std::chrono::duration<double, std::milli> elapsed = end - start;
      HAL_LOG("TestHal::delay(ms=" << ms << ")=" << elapsed.count() << "ms");
    }

    void delayMicroseconds(unsigned long us) override {
      HAL_LOG("TestHal::delayMicroseconds(us=" << us << ")");
      const auto start = std::chrono::high_resolution_clock::now();

      // busy wait is needed for microseconds precision
      const auto len = std::chrono::microseconds(us);
      while(std::chrono::high_resolution_clock::now() - start < len);

      // measure and print
      const auto end = std::chrono::high_resolution_clock::now();
      const std::chrono::duration<double, std::micro> elapsed = end - start;
      HAL_LOG("TestHal::delayMicroseconds(us=" << us << ")=" << elapsed.count() << "us");
    }

    void yield() override {
      HAL_LOG("TestHal::yield()");
    }

    unsigned long millis() override {
      HAL_LOG("TestHal::millis()");
      std::chrono::time_point now = std::chrono::high_resolution_clock::now();
      auto res = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->start);
      HAL_LOG("TestHal::millis()=" << res.count());
      return(res.count());
    }

    unsigned long micros() override {
      HAL_LOG("TestHal::micros()");
      std::chrono::time_point now = std::chrono::high_resolution_clock::now();
      auto res = std::chrono::duration_cast<std::chrono::microseconds>(now - this->start);
      HAL_LOG("TestHal::micros()=" << res.count());
      return(res.count());
    }

    long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override {
      HAL_LOG("TestHal::pulseIn(pin=" << pin << ", state=" << state << ", timeout=" << timeout << ")");

      // TODO implement
      (void)pin;
      (void)state;
      (void)timeout;
      return(0);
    }

    void spiBegin() {
      HAL_LOG("TestHal::spiBegin()");
    }

    void spiBeginTransaction() {
      HAL_LOG("TestHal::spiBeginTransaction()");
    }

    void spiTransfer(uint8_t* out, size_t len, uint8_t* in) {
      HAL_LOG("TestHal::spiTransfer(len=" << len << ")");
      
      for(size_t i = 0; i < len; i++) {
        // append to log
        (*this->spiLogPtr++) = out[i];

        // process the SPI byte
        in[i] = this->radio->HandleSPI(out[i]);

        // artificial delay to emulate SPI running at a finite speed
        // this is added because timeouts are based on time duration,
        // so we need to make sure some time actually elapses
        this->delayMicroseconds(100);

        // output debug
        HAL_LOG(fmt::format("out={:#02x}, in={:#02x}", out[i], in[i]));
      }
    }

    void spiEndTransaction() {
      HAL_LOG("TestHal::spiEndTransaction()");
    }

    void spiEnd() {
      HAL_LOG("TestHal::spiEnd()");
    }

    void tone(uint32_t pin, unsigned int frequency, unsigned long duration = 0) {
      HAL_LOG("TestHal::tone(pin=" << pin << ", frequency=" << frequency << ", duration=" << duration << ")");

      // TODO implement
      (void)pin;
      (void)frequency;
      (void)duration;
    }

    void noTone(uint32_t pin) {
      HAL_LOG("TestHal::noTone(pin=" << pin << ")");

      // TODO implement
      (void)pin;
    }

    // method to compare buffer to the internal SPI log, for verifying SPI transactions
    int spiLogMemcmp(const void* in, size_t n) {
      int ret = memcmp(this->spiLog, in, n);
      this->spiLogWipe();
      return(ret);
    }

    void spiLogWipe() {
      memset(this->spiLog, 0x00, TEST_HAL_SPI_LOG_LENGTH);
      this->spiLogPtr = this->spiLog;
    }

    // method that "connects" the emualted radio hardware to this HAL
    void connectRadio(EmulatedRadio* r) {
      this->radio = r;
      this->radio->connect(&this->gpio[EMULATED_RADIO_NSS_PIN],
                           &this->gpio[EMULATED_RADIO_IRQ_PIN],
                           &this->gpio[EMULATED_RADIO_RST_PIN],
                           &this->gpio[EMULATED_RADIO_GPIO_PIN]);
    }

  private:
    // array of emulated GPIO pins
    EmulatedPin_t gpio[TEST_HAL_NUM_GPIO_PINS];

    // start time point
    std::chrono::time_point<std::chrono::high_resolution_clock> start;

    // emulated radio hardware
    EmulatedRadio* radio;

    // SPI history log
    uint8_t spiLog[TEST_HAL_SPI_LOG_LENGTH];
    uint8_t* spiLogPtr;
};

#endif
