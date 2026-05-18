#if !defined(_RADIOLIB_ESP_HAL_H)
#define _RADIOLIB_ESP_HAL_H

#if defined(ESP_PLATFORM)

#include <RadioLib.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"

/*!
  \class EspHal
  \brief ESP-IDF hardware abstraction layer implementation for RadioLib.

  Uses public ESP-IDF driver APIs (esp_driver_gpio, esp_driver_spi) and is
  therefore portable across all ESP32 variants supported by ESP-IDF
  (ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6, ESP32-H2, ESP32-P4, ...).
*/
class EspHal : public RadioLibHal {
  public:
    /*!
      \brief Constructor.
      \param sck SPI clock pin.
      \param miso SPI MISO pin.
      \param mosi SPI MOSI pin.
      \param host SPI host to use. Defaults to SPI2_HOST.
      \param clockHz SPI clock frequency in Hz. Defaults to 2 MHz to match
                     the prior register-poking HAL implementation.
    */
    EspHal(int8_t sck, int8_t miso, int8_t mosi,
           spi_host_device_t host = SPI2_HOST,
           int clockHz = 2 * 1000 * 1000);

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

    void yield() override;
    void pullUpDown(uint32_t pin, bool enable, bool up) override;

  private:
    int8_t spiSCK;
    int8_t spiMISO;
    int8_t spiMOSI;
    spi_host_device_t spiHost;
    int spiClockHz;

    spi_device_handle_t spiDevice = nullptr;
    spi_transaction_t spiTrans = {};
};

#endif // ESP_PLATFORM

#endif
