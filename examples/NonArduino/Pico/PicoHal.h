#ifndef PICO_HAL_H
#define PICO_HAL_H

// include RadioLib
#include <RadioLib.h>

// include the necessary Pico libraries
#include <pico/stdlib.h>
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/multicore.h"

uint32_t toneLoopPin;
unsigned int toneLoopFrequency;
unsigned long toneLoopDuration;

// pre-calculated pulse-widths for 1200 and 2200Hz
// we do this to save calculation time (see https://github.com/khoih-prog/RP2040_PWM/issues/6)
#define SLEEP_1200 416.666
#define SLEEP_2200 227.272

// === NOTE ===
// The tone(...) implementation uses the second core on the RPi Pico. This is to diminish as much 
// jitter in the output tones as possible.

void toneLoop(){
  gpio_set_dir(toneLoopPin, GPIO_OUT);

  uint32_t sleep_dur;
  if (toneLoopFrequency == 1200) {
    sleep_dur = SLEEP_1200;
  } else if (toneLoopFrequency == 2200) {
    sleep_dur = SLEEP_2200;
  } else {
    sleep_dur = 500000 / toneLoopFrequency;
  }


  // tone bitbang
  while(1){
    gpio_put(toneLoopPin, 1);
    sleep_us(sleep_dur);
    gpio_put(toneLoopPin, 0);
    sleep_us(sleep_dur);
    tight_loop_contents();
  }
}

// create a new Raspberry Pi Pico hardware abstraction 
// layer using the Pico SDK
// the HAL must inherit from the base RadioLibHal class
// and implement all of its virtual methods
class PicoHal : public RadioLibHal {
public:
  PicoHal(spi_inst_t *spiChannel, uint32_t misoPin, uint32_t mosiPin, uint32_t sckPin, uint32_t spiSpeed = 500 * 1000)
    : RadioLibHal(GPIO_IN, GPIO_OUT, 0, 1, GPIO_IRQ_EDGE_RISE, GPIO_IRQ_EDGE_FALL),
    _spiChannel(spiChannel),
    _spiSpeed(spiSpeed),
    _misoPin(misoPin),
    _mosiPin(mosiPin),
    _sckPin(sckPin){}

  void init() override {
    stdio_init_all();
    spiBegin();
  }

  void term() override {
    spiEnd();
  }

  // GPIO-related methods (pinMode, digitalWrite etc.) should check
  // RADIOLIB_NC as an alias for non-connected pins
  void pinMode(uint32_t pin, uint32_t mode) override {
    if (pin == RADIOLIB_NC) {
      return;
    }

    gpio_init(pin);
    gpio_set_dir(pin, mode);
  }

  void digitalWrite(uint32_t pin, uint32_t value) override {
    if (pin == RADIOLIB_NC) {
      return;
    }

    gpio_put(pin, (bool)value);
  }

  uint32_t digitalRead(uint32_t pin) override {
    if (pin == RADIOLIB_NC) {
      return 0;
    }

    return gpio_get(pin);
  }

  void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override {
    if (interruptNum == RADIOLIB_NC) {
      return;
    }

    gpio_set_irq_enabled_with_callback(interruptNum, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, (gpio_irq_callback_t)interruptCb);
  }

  void detachInterrupt(uint32_t interruptNum) override {
    if (interruptNum == RADIOLIB_NC) {
      return;
    }

    gpio_set_irq_enabled_with_callback(interruptNum, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false, NULL);
  }

  void delay(unsigned long ms) override {
    sleep_ms(ms);
  }

  void delayMicroseconds(unsigned long us) override {
    sleep_us(us);
  }

  unsigned long millis() override {
    return to_ms_since_boot(get_absolute_time());
  }

  unsigned long micros() override {
    return to_us_since_boot(get_absolute_time());
  }

  long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override {
    if (pin == RADIOLIB_NC) {
      return 0;
    }

    this->pinMode(pin, GPIO_IN);
    uint32_t start = this->micros();
    uint32_t curtick = this->micros();

    while (this->digitalRead(pin) == state) {
      if ((this->micros() - curtick) > timeout) {
        return 0;
      }
    }

    return (this->micros() - start);
  }

  void tone(uint32_t pin, unsigned int frequency, unsigned long duration = 0) override {
    // tones on the Pico are generated using bitbanging. This process is offloaded to the Pico's second core
    multicore_reset_core1();
    toneLoopPin = pin;
    toneLoopFrequency = frequency;
    toneLoopDuration = duration;
    multicore_launch_core1(toneLoop);
  }

  void noTone(uint32_t pin) override {
    multicore_reset_core1();
  }

  void spiBegin() {
    spi_init(_spiChannel, _spiSpeed);
    spi_set_format(_spiChannel, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(_sckPin, GPIO_FUNC_SPI);
    gpio_set_function(_mosiPin, GPIO_FUNC_SPI);
    gpio_set_function(_misoPin, GPIO_FUNC_SPI);
  }

  void spiBeginTransaction() {}

  void spiTransfer(uint8_t *out, size_t len, uint8_t *in) {
    spi_write_read_blocking(_spiChannel, out, in, len);
  }

  void yield() override {
  	tight_loop_contents();
  }

  void spiEndTransaction() {}

  void spiEnd() {
    spi_deinit(_spiChannel);
  }

private:
  // the HAL can contain any additional private members
  spi_inst_t *_spiChannel;
  uint32_t _spiSpeed;
  uint32_t _misoPin;
  uint32_t _mosiPin;
  uint32_t _sckPin;
};

#endif
