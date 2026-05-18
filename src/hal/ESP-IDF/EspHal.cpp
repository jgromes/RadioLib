#include "EspHal.h"

#if defined(ESP_PLATFORM)

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"

static const char* TAG = "EspHal";

#define ESP_HAL_NOP() asm volatile ("nop")

EspHal::EspHal(int8_t sck, int8_t miso, int8_t mosi, spi_host_device_t host, int clockHz)
  : RadioLibHal(GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, 0, 1,
                GPIO_INTR_POSEDGE, GPIO_INTR_NEGEDGE),
    spiSCK(sck), spiMISO(miso), spiMOSI(mosi),
    spiHost(host), spiClockHz(clockHz) {
}

void EspHal::init() {
  spiBegin();
}

void EspHal::term() {
  spiEnd();
}

void EspHal::pinMode(uint32_t pin, uint32_t mode) {
  if(pin == RADIOLIB_NC) {
    return;
  }

  gpio_config_t conf = {
    .pin_bit_mask = (1ULL << pin),
    .mode = (gpio_mode_t)mode,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&conf);
}

void EspHal::digitalWrite(uint32_t pin, uint32_t value) {
  if(pin == RADIOLIB_NC) {
    return;
  }

  gpio_set_level((gpio_num_t)pin, value);
}

uint32_t EspHal::digitalRead(uint32_t pin) {
  if(pin == RADIOLIB_NC) {
    return(0);
  }

  return(gpio_get_level((gpio_num_t)pin));
}

void EspHal::attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) {
  if(interruptNum == RADIOLIB_NC) {
    return;
  }

  // ESP_ERR_INVALID_STATE just means the service is already installed
  esp_err_t err = gpio_install_isr_service((int)ESP_INTR_FLAG_IRAM);
  if(err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGW(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(err));
    return;
  }

  gpio_set_intr_type((gpio_num_t)interruptNum, (gpio_int_type_t)mode);
  gpio_isr_handler_add((gpio_num_t)interruptNum, (void (*)(void*))interruptCb, NULL);
}

void EspHal::detachInterrupt(uint32_t interruptNum) {
  if(interruptNum == RADIOLIB_NC) {
    return;
  }

  gpio_isr_handler_remove((gpio_num_t)interruptNum);
  gpio_wakeup_disable((gpio_num_t)interruptNum);
  gpio_set_intr_type((gpio_num_t)interruptNum, GPIO_INTR_DISABLE);
}

void EspHal::delay(unsigned long ms) {
  vTaskDelay(pdMS_TO_TICKS(ms));
}

void EspHal::delayMicroseconds(unsigned long us) {
  uint64_t m = (uint64_t)esp_timer_get_time();
  if(us) {
    uint64_t e = (m + us);
    if(m > e) { // overflow
      while((uint64_t)esp_timer_get_time() > e) {
        ESP_HAL_NOP();
      }
    }
    while((uint64_t)esp_timer_get_time() < e) {
      ESP_HAL_NOP();
    }
  }
}

unsigned long EspHal::millis() {
  return((unsigned long)(esp_timer_get_time() / 1000ULL));
}

unsigned long EspHal::micros() {
  return((unsigned long)(esp_timer_get_time()));
}

long EspHal::pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) {
  if(pin == RADIOLIB_NC) {
    return(0);
  }

  this->pinMode(pin, GPIO_MODE_INPUT);
  uint32_t start = this->micros();
  uint32_t curtick = this->micros();

  while(this->digitalRead(pin) == state) {
    if((this->micros() - curtick) > timeout) {
      return(0);
    }
  }

  return(this->micros() - start);
}

void EspHal::spiBegin() {
  spi_bus_config_t busCfg = {};
  busCfg.miso_io_num = spiMISO;
  busCfg.mosi_io_num = spiMOSI;
  busCfg.sclk_io_num = spiSCK;
  busCfg.quadwp_io_num = -1;
  busCfg.quadhd_io_num = -1;
  busCfg.max_transfer_sz = 128;

  esp_err_t err = spi_bus_initialize(spiHost, &busCfg, SPI_DMA_CH_AUTO);
  if(err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
    return;
  }

  spi_device_interface_config_t devCfg = {};
  devCfg.clock_speed_hz = spiClockHz;
  devCfg.mode = 0;
  devCfg.spics_io_num = -1; // CS handled by RadioLib via digitalWrite
  devCfg.queue_size = 1;

  err = spi_bus_add_device(spiHost, &devCfg, &spiDevice);
  if(err != ESP_OK) {
    ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(err));
    spiDevice = nullptr;
  }
}

void EspHal::spiBeginTransaction() {
  memset(&spiTrans, 0, sizeof(spiTrans));
}

void EspHal::spiTransfer(uint8_t* out, size_t len, uint8_t* in) {
  if(spiDevice == nullptr) {
    return;
  }

  spiTrans.length = len * 8;
  spiTrans.tx_buffer = out;
  spiTrans.rx_buffer = in;

  esp_err_t err = spi_device_transmit(spiDevice, &spiTrans);
  if(err != ESP_OK) {
    ESP_LOGE(TAG, "spi_device_transmit failed: %s", esp_err_to_name(err));
  }
}

void EspHal::spiEndTransaction() {
  // nothing to do - per-transaction state lives in spiTrans
}

void EspHal::spiEnd() {
  if(spiDevice != nullptr) {
    spi_bus_remove_device(spiDevice);
    spiDevice = nullptr;
  }
  spi_bus_free(spiHost);
}

void EspHal::yield() {
  taskYIELD();
}

void EspHal::pullUpDown(uint32_t pin, bool enable, bool up) {
  if(pin == RADIOLIB_NC) {
    return;
  }

  gpio_pull_mode_t mode;
  if(!enable) {
    mode = GPIO_FLOATING;
  } else {
    mode = up ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;
  }
  gpio_set_pull_mode((gpio_num_t)pin, mode);
}

#endif // ESP_PLATFORM
