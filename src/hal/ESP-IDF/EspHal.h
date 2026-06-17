// EspHal.h
// ESP-IDF HAL for RadioLib using spi_master driver
// This avoids conflicts with other SPI devices on the same bus

#ifndef RADIOLIB_ESP_IDF_HAL
#define RADIOLIB_ESP_IDF_HAL

#if defined(ESP_PLATFORM)
#include "RadioLib.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_rom_sys.h" // For esp_rom_delay_us
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <cinttypes> // For PRIu32

// Arduino-style macros for RadioLib compatibility
#ifndef LOW
#define LOW (0x0)
#endif
#ifndef HIGH
#define HIGH (0x1)
#endif
#define INPUT (GPIO_MODE_INPUT)
#define OUTPUT (GPIO_MODE_OUTPUT)
#define RISING (GPIO_INTR_POSEDGE)
#define FALLING (GPIO_INTR_NEGEDGE)

// These are left undefined by the generic build; define them for the ESP-IDF HAL
#if !defined(RADIOLIB_ESP32)
#define RADIOLIB_ESP32
#endif
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
         spi_host_device_t host = SPI2_HOST, uint32_t clockHz = 2000000)
      : RadioLibHal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING), spiSCK(sck),
        spiMISO(miso), spiMOSI(mosi), spiHost(host), spiClockHz(clockHz),
        spiDevice(nullptr), busInitialized(false), deviceAdded(false),
        halInitialized(false), tonePrevFreq(-1) {}

  virtual ~EspHal() { term(); }

  void init() override {
    if (!halInitialized) {
      spiBegin();
      // Install the application-wide GPIO ISR service once.
      // Loosely accept duplicate installations (ESP_ERR_INVALID_STATE)
      esp_err_t ret = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
      if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE("EspHal", "Failed to install GPIO ISR service: %s",
                 esp_err_to_name(ret));
      }
      halInitialized = true;
    }
  }

  void term() override {
    if (halInitialized) {
      spiEnd();
      halInitialized = false;
    }
  }

  // GPIO operations
  void pinMode(uint32_t pin, uint32_t mode) override {
    if (pin == RADIOLIB_NC) {
      return;
    }

    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = (mode == OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&conf);
  }

  void digitalWrite(uint32_t pin, uint32_t value) override {
    if (pin == RADIOLIB_NC) {
      return;
    }
    gpio_set_level((gpio_num_t)pin, value);
  }

  uint32_t digitalRead(uint32_t pin) override {
    if (pin == RADIOLIB_NC) {
      return 0;
    }
    return gpio_get_level((gpio_num_t)pin);
  }

  void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void),
                       uint32_t mode) override {
    if (interruptNum == RADIOLIB_NC) {
      return;
    }

    gpio_set_intr_type((gpio_num_t)interruptNum, (gpio_int_type_t)mode);
    gpio_isr_handler_add((gpio_num_t)interruptNum, (gpio_isr_t)interruptCb,
                         nullptr);
  }

  void detachInterrupt(uint32_t interruptNum) override {
    if (interruptNum == RADIOLIB_NC) {
      return;
    }
    gpio_isr_handler_remove((gpio_num_t)interruptNum);
  }

  // Timing functions
  void delay(unsigned long ms) override { vTaskDelay(pdMS_TO_TICKS(ms)); }

  void delayMicroseconds(unsigned long us) override { esp_rom_delay_us(us); }

  unsigned long millis() override {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
  }

  unsigned long micros() override {
    return (unsigned long)esp_timer_get_time();
  }

  long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override {
    if (pin == RADIOLIB_NC) {
      return 0;
    }

    unsigned long startMicros = micros();
    unsigned long currentMicros;

    // Wait for pulse to start
    while (this->digitalRead(pin) != state) {
      currentMicros = micros();
      if (currentMicros - startMicros >= timeout) {
        return 0;
      }
    }

    // Measure pulse duration
    unsigned long pulseStart = micros();
    while (this->digitalRead(pin) == state) {
      currentMicros = micros();
      if (currentMicros - pulseStart >= timeout) {
        return 0;
      }
    }

    return micros() - pulseStart;
  }

  // GPIO pull-up/pull-down configuration
  void pullUpDown(uint32_t pin, bool enable, bool up) override {
    if (pin == RADIOLIB_NC) {
      return;
    }

    if (enable) {
      gpio_set_pull_mode((gpio_num_t)pin,
                         up ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY);
    } else {
      gpio_set_pull_mode((gpio_num_t)pin, GPIO_FLOATING);
    }
  }

  // Tone generation using the LEDC peripheral (lazily initialised on first use)
  void tone(uint32_t pin, unsigned int frequency,
            RadioLibTime_t duration = 0) override {
    if (pin == RADIOLIB_NC) {
      return;
    }
    (void)duration;

    // Configure the LEDC timer + channel only once
    if (tonePrevFreq == -1) {
      ledc_timer_config_t timer_config = {};
      timer_config.speed_mode = LEDC_LOW_SPEED_MODE;
      timer_config.duty_resolution = LEDC_TIMER_10_BIT;
      timer_config.timer_num = LEDC_TIMER_0;
      timer_config.freq_hz = (frequency > 0) ? frequency : 1000;
      timer_config.clk_cfg = LEDC_AUTO_CLK;
      ledc_timer_config(&timer_config);

      ledc_channel_config_t channel_config = {};
      channel_config.gpio_num = (int)pin;
      channel_config.speed_mode = LEDC_LOW_SPEED_MODE;
      channel_config.channel = (ledc_channel_t)RADIOLIB_TONE_ESP32_CHANNEL;
      channel_config.timer_sel = LEDC_TIMER_0;
      channel_config.duty = 512; // 50% duty at 10-bit resolution
      channel_config.hpoint = 0;
      ledc_channel_config(&channel_config);
    }

    // Update the frequency only when it actually changes
    if ((int32_t)frequency != tonePrevFreq) {
      ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, frequency);
      ledc_set_duty(LEDC_LOW_SPEED_MODE,
                    (ledc_channel_t)RADIOLIB_TONE_ESP32_CHANNEL, 512);
      ledc_update_duty(LEDC_LOW_SPEED_MODE,
                       (ledc_channel_t)RADIOLIB_TONE_ESP32_CHANNEL);
      tonePrevFreq = (int32_t)frequency;
    }
  }

  void noTone(uint32_t pin) override {
    if (pin == RADIOLIB_NC) {
      return;
    }
    if (tonePrevFreq == -1) {
      return; // never started
    }
    ledc_stop(LEDC_LOW_SPEED_MODE,
              (ledc_channel_t)RADIOLIB_TONE_ESP32_CHANNEL, 0);
    tonePrevFreq = -1;
  }

  // SPI operations using spi_master driver
  void spiBegin() {
    ESP_LOGD("EspHal", "Initializing SPI on host %d (SCK=%d, MISO=%d, MOSI=%d)",
             spiHost, spiSCK, spiMISO, spiMOSI);

    // Try to initialize the bus - it may already be initialized by another
    // component (shared bus). While RadioLib does not
    // allocate DMA-capable buffers and the transfers here are small,
    // SPI_DMA_CH_AUTO is set because with SPI_DMA_DISABLED the SPI transaction
    // is limited to 64 bytes (and our radio MTU can be much bigger).
    spi_bus_config_t bus_config = {};
    bus_config.mosi_io_num = spiMOSI;
    bus_config.miso_io_num = spiMISO;
    bus_config.sclk_io_num = spiSCK;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;
    bus_config.data4_io_num = -1;
    bus_config.data5_io_num = -1;
    bus_config.data6_io_num = -1;
    bus_config.data7_io_num = -1;
    bus_config.max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE;
    bus_config.flags = 0;
    bus_config.isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO;
    bus_config.intr_flags = 0;

    esp_err_t ret = spi_bus_initialize(spiHost, &bus_config, SPI_DMA_CH_AUTO);
    if (ret == ESP_OK) {
      busInitialized = true;
      ESP_LOGD("EspHal", "SPI bus initialized successfully");
    } else if (ret == ESP_ERR_INVALID_STATE) {
      // Bus already initialized - this is OK for shared bus
      busInitialized = false; // We didn't init it, so we won't free it
      ESP_LOGD("EspHal", "SPI bus already initialized (shared bus)");
    } else {
      ESP_LOGE("EspHal", "Failed to initialize SPI bus: %s",
               esp_err_to_name(ret));
      return;
    }

    // Add the device. CS is left to RadioLib (software-driven via
    // digitalWrite on the Module's csPin), so spics_io_num is -1.
    spi_device_interface_config_t dev_config = {};
    dev_config.command_bits = 0;
    dev_config.address_bits = 0;
    dev_config.dummy_bits = 0;
    dev_config.mode = 0; // SPI mode 0 (CPOL=0, CPHA=0)
    dev_config.clock_source = SPI_CLK_SRC_DEFAULT;
    dev_config.duty_cycle_pos = 128; // 50% duty cycle
    dev_config.cs_ena_pretrans = 0;
    dev_config.cs_ena_posttrans = 0;
    dev_config.clock_speed_hz = (int)spiClockHz;
    dev_config.input_delay_ns = 0;
    dev_config.spics_io_num = -1; // RadioLib drives CS in software
    dev_config.flags = 0;
    dev_config.queue_size = 1;
    dev_config.pre_cb = nullptr;
    dev_config.post_cb = nullptr;

    ret = spi_bus_add_device(spiHost, &dev_config, &spiDevice);
    if (ret != ESP_OK) {
      ESP_LOGE("EspHal", "Failed to add SPI device: %s", esp_err_to_name(ret));
      return;
    }
    deviceAdded = true;
    ESP_LOGD("EspHal", "SPI device added (clock=%" PRIu32 " Hz, software CS)",
             spiClockHz);
  }

  void spiBeginTransaction() {
    // Acquire the SPI bus for this device
    if (spiDevice != nullptr) {
      spi_device_acquire_bus(spiDevice, portMAX_DELAY);
    }
  }

  void spiTransfer(uint8_t *out, size_t len, uint8_t *in) {
    if (spiDevice == nullptr) {
      ESP_LOGE("EspHal", "SPI device not initialized!");
      return;
    }

    if (len == 0) {
      return;
    }

    spi_transaction_t trans = {};
    trans.length = len * 8; // length in bits
    trans.tx_buffer = out;
    trans.rx_buffer = in;

    esp_err_t ret = spi_device_polling_transmit(spiDevice, &trans);
    if (ret != ESP_OK) {
      ESP_LOGE("EspHal", "SPI transfer failed: %s", esp_err_to_name(ret));
    }
  }

  void spiEndTransaction() {
    // Release the SPI bus
    if (spiDevice != nullptr) {
      spi_device_release_bus(spiDevice);
    }
  }

  void spiEnd() {
    // Remove device from bus
    if (deviceAdded && spiDevice != nullptr) {
      spi_bus_remove_device(spiDevice);
      spiDevice = nullptr;
      deviceAdded = false;
      ESP_LOGD("EspHal", "SPI device removed");
    }

    // Free the bus only if we initialized it
    if (busInitialized) {
      spi_bus_free(spiHost);
      busInitialized = false;
      ESP_LOGD("EspHal", "SPI bus freed");
    }
  }

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
#endif // RADIOLIB_ESP_IDF_HAL
