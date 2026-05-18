#include "EspHal.h"

#if defined(ESP_PLATFORM)

#include <string.h>

// all of the following is needed to calculate SPI clock divider
#define ClkRegToFreq(reg)           (apb_freq / (((reg)->clkdiv_pre + 1) * ((reg)->clkcnt_n + 1)))

typedef union {
  uint32_t value;
  struct {
    uint32_t clkcnt_l:       6;
    uint32_t clkcnt_h:       6;
    uint32_t clkcnt_n:       6;
    uint32_t clkdiv_pre:    13;
    uint32_t clk_equ_sysclk: 1;
  };
} spiClk_t;

static uint32_t getApbFrequency() {
  rtc_cpu_freq_config_t conf;
  rtc_clk_cpu_freq_get_config(&conf);

  if(conf.freq_mhz >= 80) {
    return(80 * MHZ);
  }

  return((conf.source_freq_mhz * MHZ) / conf.div);
}

static uint32_t spiFrequencyToClockDiv(uint32_t freq) {
  uint32_t apb_freq = getApbFrequency();
  if(freq >= apb_freq) {
    return SPI_CLK_EQU_SYSCLK;
  }

  const spiClk_t minFreqReg = { 0x7FFFF000 };
  uint32_t minFreq = ClkRegToFreq((spiClk_t*) &minFreqReg);
  if(freq < minFreq) {
    return minFreqReg.value;
  }

  uint8_t calN = 1;
  spiClk_t bestReg = { 0 };
  int32_t bestFreq = 0;
  while(calN <= 0x3F) {
    spiClk_t reg = { 0 };
    int32_t calFreq;
    int32_t calPre;
    int8_t calPreVari = -2;

    reg.clkcnt_n = calN;

    while(calPreVari++ <= 1) {
      calPre = (((apb_freq / (reg.clkcnt_n + 1)) / freq) - 1) + calPreVari;
      if(calPre > 0x1FFF) {
        reg.clkdiv_pre = 0x1FFF;
      } else if(calPre <= 0) {
        reg.clkdiv_pre = 0;
      } else {
        reg.clkdiv_pre = calPre;
      }
      reg.clkcnt_l = ((reg.clkcnt_n + 1) / 2);
      calFreq = ClkRegToFreq(&reg);
      if(calFreq == (int32_t) freq) {
        memcpy(&bestReg, &reg, sizeof(bestReg));
        break;
      } else if(calFreq < (int32_t) freq) {
        if(RADIOLIB_ABS(freq - calFreq) < RADIOLIB_ABS(freq - bestFreq)) {
          bestFreq = calFreq;
          memcpy(&bestReg, &reg, sizeof(bestReg));
        }
      }
    }
    if(calFreq == (int32_t) freq) {
      break;
    }
    calN++;
  }
  return(bestReg.value);
}

EspHal::EspHal(int8_t sck, int8_t miso, int8_t mosi)
  : RadioLibHal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING),
    spiSCK(sck), spiMISO(miso), spiMOSI(mosi) {
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

  gpio_hal_context_t gpiohal;
  gpiohal.dev = GPIO_LL_GET_HW(GPIO_PORT_0);

  gpio_config_t conf = {
    .pin_bit_mask = (1ULL<<pin),
    .mode = (gpio_mode_t)mode,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = (gpio_int_type_t)gpiohal.dev->pin[pin].int_type,
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

  gpio_install_isr_service((int)ESP_INTR_FLAG_IRAM);
  gpio_set_intr_type((gpio_num_t)interruptNum, (gpio_int_type_t)(mode & 0x7));

  // this uses function typecasting, which is not defined when the functions have different signatures
  // untested and might not work
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
  vTaskDelay(ms / portTICK_PERIOD_MS);
}

void EspHal::delayMicroseconds(unsigned long us) {
  uint64_t m = (uint64_t)esp_timer_get_time();
  if(us) {
    uint64_t e = (m + us);
    if(m > e) { // overflow
      while((uint64_t)esp_timer_get_time() > e) {
        NOP();
      }
    }
    while((uint64_t)esp_timer_get_time() < e) {
      NOP();
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

  this->pinMode(pin, INPUT);
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
  // enable peripheral
  DPORT_SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, DPORT_SPI2_CLK_EN);
  DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, DPORT_SPI2_RST);

  // reset the control struct
  this->spi->slave.trans_done = 0;
  this->spi->slave.val = 0;
  this->spi->pin.val = 0;
  this->spi->user.val = 0;
  this->spi->user1.val = 0;
  this->spi->ctrl.val = 0;
  this->spi->ctrl1.val = 0;
  this->spi->ctrl2.val = 0;
  this->spi->clock.val = 0;
  this->spi->user.usr_mosi = 1;
  this->spi->user.usr_miso = 1;
  this->spi->user.doutdin = 1;
  for(uint8_t i = 0; i < 16; i++) {
    this->spi->data_buf[i] = 0x00000000;
  }

  // set SPI mode 0
  this->spi->pin.ck_idle_edge = 0;
  this->spi->user.ck_out_edge = 0;

  // set bit order to MSB first
  this->spi->ctrl.wr_bit_order = 0;
  this->spi->ctrl.rd_bit_order = 0;

  // set the clock
  this->spi->clock.val = spiFrequencyToClockDiv(2000000);

  // initialize pins
  this->pinMode(this->spiSCK, OUTPUT);
  this->pinMode(this->spiMISO, INPUT);
  this->pinMode(this->spiMOSI, OUTPUT);
  gpio_matrix_out(this->spiSCK, HSPICLK_OUT_IDX, false, false);
  gpio_matrix_in(this->spiMISO, HSPIQ_OUT_IDX, false);
  gpio_matrix_out(this->spiMOSI, HSPID_IN_IDX, false, false);
}

void EspHal::spiBeginTransaction() {
  // not needed - in ESP32 Arduino core, this function
  // repeats clock div, mode and bit order configuration
}

uint8_t EspHal::spiTransferByte(uint8_t b) {
  this->spi->mosi_dlen.usr_mosi_dbitlen = 7;
  this->spi->miso_dlen.usr_miso_dbitlen = 7;
  this->spi->data_buf[0] = b;
  this->spi->cmd.usr = 1;
  while(this->spi->cmd.usr);
  return(this->spi->data_buf[0] & 0xFF);
}

void EspHal::spiTransfer(uint8_t* out, size_t len, uint8_t* in) {
  for(size_t i = 0; i < len; i++) {
    in[i] = this->spiTransferByte(out[i]);
  }
}

void EspHal::spiEndTransaction() {
  // nothing needs to be done here
}

void EspHal::spiEnd() {
  // detach pins
  gpio_matrix_out(this->spiSCK, MATRIX_DETACH_OUT_SIG, false, false);
  gpio_matrix_in(this->spiMISO, MATRIX_DETACH_IN_LOW_PIN, false);
  gpio_matrix_out(this->spiMOSI, MATRIX_DETACH_OUT_SIG, false, false);
}

#endif // ESP_PLATFORM
