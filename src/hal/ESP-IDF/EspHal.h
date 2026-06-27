// EspHal.h
// ESP-IDF HAL for RadioLib using spi_master driver
// This avoids conflicts with other SPI devices on the same bus

#if !defined(_RADIOLIB_ESP_IDF_HAL_H)
#define _RADIOLIB_ESP_IDF_HAL_H

#if defined(ESP_PLATFORM)
#include "RadioLib.h"
#include "driver/spi_master.h"

// RADIOLIB_ESP32 is now set by BuildOpt.h for ESP_PLATFORM builds.
// RADIOLIB_TONE_ESP32_CHANNEL stays here because LEDC_CHANNEL_0 is only
// visible via driver/ledc.h, which BuildOpt.h does not pull in.
#if !defined(RADIOLIB_TONE_ESP32_CHANNEL)
#define RADIOLIB_TONE_ESP32_CHANNEL (LEDC_CHANNEL_0)
#endif

/**
 * @brief ESP-IDF HAL for RadioLib using spi_master driver
 * This implementation properly handles SPI transactions and works with shared
 * SPI buses
 */
class EspHal : public RadioLibHal {
public:
  /**
   * @brief Constructor
   * @param sck SPI clock pin
   * @param miso SPI MISO pin
   * @param mosi SPI MOSI pin
   * @param host SPI host to use (SPI2_HOST or SPI3_HOST)
   * @param clockHz SPI clock frequency in Hz (default 2 MHz to match the
   *                NonArduino example; raise per-module, e.g.
   *                SPI_MASTER_FREQ_8M for SX12xx if supported by the module)
   */
  EspHal(int8_t sck, int8_t miso, int8_t mosi,
         spi_host_device_t host = SPI2_HOST, uint32_t clockHz = 2000000);

  virtual ~EspHal();

  // implementations of pure virtual RadioLibHal methods
  void pinMode(uint32_t pin, uint32_t mode) override;
  void digitalWrite(uint32_t pin, uint32_t value) override;
  uint32_t digitalRead(uint32_t pin) override;
  void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void),
                       uint32_t mode) override;
  void detachInterrupt(uint32_t interruptNum) override;
  void delay(unsigned long ms) override;
  void delayMicroseconds(unsigned long us) override;
  unsigned long millis() override;
  unsigned long micros() override;
  long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override;
  void spiBegin();
  void spiBeginTransaction();
  void spiTransfer(uint8_t *out, size_t len, uint8_t *in);
  void spiEndTransaction();
  void spiEnd();

  // implementations of virtual RadioLibHal methods
  void init() override;
  void term() override;
  void tone(uint32_t pin, unsigned int frequency,
            RadioLibTime_t duration = 0) override;
  void noTone(uint32_t pin) override;
  void yield() override;
  void pullUpDown(uint32_t pin, bool enable, bool up) override;

private:
  int8_t spiSCK;
  int8_t spiMISO;
  int8_t spiMOSI;
  spi_host_device_t spiHost;
  uint32_t spiClockHz;
  spi_device_handle_t spiDevice;
  bool busInitialized;
  bool deviceAdded;
  bool halInitialized;
  int32_t tonePrevFreq;
};
#endif
#endif // _RADIOLIB_ESP_IDF_HAL_H
