#if !defined(_RADIOLIB_ESP_HAL_H)
#define _RADIOLIB_ESP_HAL_H

#if defined(ESP_PLATFORM)

// include RadioLib
#include <RadioLib.h>

// include all the dependencies
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32/rom/gpio.h"
#include "soc/rtc.h"
#include "soc/dport_reg.h"
#include "soc/spi_reg.h"
#include "soc/spi_struct.h"
#include "driver/gpio.h"
#include "hal/gpio_hal.h"
#include "esp_timer.h"
#include "esp_log.h"

// this example only works on ESP32 and is unlikely to work on ESP32S2/S3 etc.
// if you need high portability, you should probably use Arduino anyway ...
#if CONFIG_IDF_TARGET_ESP32 == 0
  #error This HAL only supports ESP32 targets. Support for ESP32S2/S3 etc. can be added by adjusting this file to user needs.
#endif

// define Arduino-style macros
#define LOW                         (0x0)
#define HIGH                        (0x1)
#define INPUT                       (0x01)
#define OUTPUT                      (0x03)
#define RISING                      (0x01)
#define FALLING                     (0x02)
#define NOP()                       asm volatile ("nop")

#define MATRIX_DETACH_OUT_SIG       (0x100)
#define MATRIX_DETACH_IN_LOW_PIN    (0x30)

/*!
  \class EspHal
  \brief ESP-IDF hardware abstraction layer implementation for RadioLib.
  This implementation drives SPI by poking the ESP32 peripheral registers
  directly. ESP32 only.
*/
class EspHal : public RadioLibHal {
  public:
    EspHal(int8_t sck, int8_t miso, int8_t mosi);

    void init() override;
    void term() override;

    void pinMode(uint32_t pin, uint32_t mode) override;
    void digitalWrite(uint32_t pin, uint32_t value) override;
    uint32_t digitalRead(uint32_t pin) override;

    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override;
    void detachInterrupt(uint32_t interruptNum) override;

    void delay(unsigned long ms) override;
    void delayMicroseconds(unsigned long us) override;
    unsigned long millis() override;
    unsigned long micros() override;
    long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override;

    void spiBegin() override;
    void spiBeginTransaction() override;
    void spiTransfer(uint8_t* out, size_t len, uint8_t* in) override;
    void spiEndTransaction() override;
    void spiEnd() override;

    uint8_t spiTransferByte(uint8_t b);

  private:
    int8_t spiSCK;
    int8_t spiMISO;
    int8_t spiMOSI;
    spi_dev_t* spi = (volatile spi_dev_t *)(DR_REG_SPI2_BASE);
};

#endif // ESP_PLATFORM

#endif
