// EspHal.cpp
// Definitions for the ESP-IDF HAL.

#include "EspHal.h"

// Only build the native ESP-IDF HAL for pure ESP-IDF targets. Under Arduino
// (including Arduino-ESP32, which also defines ESP_PLATFORM) RadioLib uses
// ArduinoHal instead, and this file's IDF spi_master driver code may not even
// compile against the older ESP-IDF bundled with the Arduino core. Mirrors the
// RADIOLIB_BUILD_ARDUINO guard in ArduinoHal.cpp.
#if defined(ESP_PLATFORM) && !defined(ARDUINO)

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_rom_sys.h" // For esp_rom_delay_us
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <cinttypes> // For PRIu32

// EspHal::delay() uses vTaskDelay(pdMS_TO_TICKS(ms)) for the bulk of the
// wait, so its resolution is the FreeRTOS tick. At the IDF default of
// CONFIG_FREERTOS_HZ=100 the tick is 10 ms, which makes timing-sensitive
// radio operations like LoRaWAN RX1/RX2 windows unreliable. Refuse to
// build unless the user opts out explicitly via RADIOLIB_RELAX_RTOS_TICK.
#if defined(CONFIG_FREERTOS_HZ) && (CONFIG_FREERTOS_HZ < 1000)
  #if !defined(RADIOLIB_RELAX_RTOS_TICK) || (RADIOLIB_RELAX_RTOS_TICK != 1)
    #error "RadioLib EspHal: CONFIG_FREERTOS_HZ is below 1000; set it to 1000 in sdkconfig for accurate delay() resolution, or define RADIOLIB_RELAX_RTOS_TICK=1 to bypass this check."
  #endif
#endif

EspHal::EspHal(int8_t sck, int8_t miso, int8_t mosi,
               spi_host_device_t host, uint32_t clockHz)
    : RadioLibHal(GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, 0, 1,
                  GPIO_INTR_POSEDGE, GPIO_INTR_NEGEDGE),
      spiSCK(sck), spiMISO(miso), spiMOSI(mosi), spiHost(host),
      spiClockHz(clockHz), spiDevice(nullptr), busInitialized(false),
      deviceAdded(false), halInitialized(false), tonePrevFreq(-1) {}

EspHal::~EspHal() { EspHal::term(); }

// implementations of pure virtual RadioLibHal methods

void EspHal::pinMode(uint32_t pin, uint32_t mode) {
  if (pin == RADIOLIB_NC) {
    return;
  }

  gpio_config_t conf = {
      .pin_bit_mask = (1ULL << pin),
      .mode = (mode == GPIO_MODE_OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&conf);
}

void EspHal::digitalWrite(uint32_t pin, uint32_t value) {
  if (pin == RADIOLIB_NC) {
    return;
  }
  gpio_set_level((gpio_num_t)pin, value);
}

uint32_t EspHal::digitalRead(uint32_t pin) {
  if (pin == RADIOLIB_NC) {
    return 0;
  }
  return gpio_get_level((gpio_num_t)pin);
}

void EspHal::attachInterrupt(uint32_t interruptNum,
                             void (*interruptCb)(void), uint32_t mode) {
  if (interruptNum == RADIOLIB_NC) {
    return;
  }

  gpio_set_intr_type((gpio_num_t)interruptNum, (gpio_int_type_t)mode);
  gpio_isr_handler_add((gpio_num_t)interruptNum, (gpio_isr_t)interruptCb,
                       nullptr);
}

void EspHal::detachInterrupt(uint32_t interruptNum) {
  if (interruptNum == RADIOLIB_NC) {
    return;
  }
  gpio_isr_handler_remove((gpio_num_t)interruptNum);
}

void EspHal::delay(unsigned long ms) {
  if (ms == 0) {
    return;
  }

  // Hybrid sleep + busy-wait so the call is accurate even when
  // CONFIG_FREERTOS_HZ < 1000. Sleep all-but-one tick under the
  // scheduler, then busy-wait the remainder with microsecond precision.
  // Worst-case busy-wait is one tick period (10 ms at the IDF default).
  const uint64_t target_us = (uint64_t)esp_timer_get_time() + (uint64_t)ms * 1000ULL;

  const TickType_t ticks = pdMS_TO_TICKS(ms);
  if (ticks > 1) {
    vTaskDelay(ticks - 1);
  }

  int64_t remaining_us = (int64_t)target_us - esp_timer_get_time();
  if (remaining_us > 0) {
    esp_rom_delay_us((uint32_t)remaining_us);
  }
}

void EspHal::delayMicroseconds(unsigned long us) { esp_rom_delay_us(us); }

unsigned long EspHal::millis() {
  return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

unsigned long EspHal::micros() {
  return (unsigned long)esp_timer_get_time();
}

long EspHal::pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) {
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

void EspHal::spiBegin() {
  ESP_LOGD("EspHal",
           "Initializing SPI on host %d (SCK=%d, MISO=%d, MOSI=%d)", spiHost,
           spiSCK, spiMISO, spiMOSI);

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

void EspHal::spiBeginTransaction() {
  // Acquire the SPI bus for this device
  if (spiDevice != nullptr) {
    spi_device_acquire_bus(spiDevice, portMAX_DELAY);
  }
}

void EspHal::spiTransfer(uint8_t *out, size_t len, uint8_t *in) {
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

void EspHal::spiEndTransaction() {
  // Release the SPI bus
  if (spiDevice != nullptr) {
    spi_device_release_bus(spiDevice);
  }
}

void EspHal::spiEnd() {
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

// implementations of virtual RadioLibHal methods

void EspHal::init() {
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

void EspHal::term() {
  if (halInitialized) {
    spiEnd();
    halInitialized = false;
  }
}

void EspHal::tone(uint32_t pin, unsigned int frequency,
                  RadioLibTime_t duration) {
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

void EspHal::noTone(uint32_t pin) {
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

void EspHal::yield() { taskYIELD(); }

void EspHal::pullUpDown(uint32_t pin, bool enable, bool up) {
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

#endif // ESP_PLATFORM && !ARDUINO
