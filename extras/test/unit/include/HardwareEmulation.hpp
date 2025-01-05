#ifndef HARDWARE_EMULATION_HPP
#define HARDWARE_EMULATION_HPP

#include <stdint.h>

// value that is returned by the emualted radio class when performing SPI transfer to it
#define EMULATED_RADIO_SPI_RETURN (0xFF)

// pin indexes
#define EMULATED_RADIO_NSS_PIN    (1)
#define EMULATED_RADIO_IRQ_PIN    (2)
#define EMULATED_RADIO_RST_PIN    (3)
#define EMULATED_RADIO_GPIO_PIN   (4)

enum PinFunction_t {
  PIN_UNASSIGNED = 0,
  PIN_CS,
  PIN_IRQ,
  PIN_RST,
  PIN_GPIO,
};

// structure for emulating GPIO pins
struct EmulatedPin_t {
  uint32_t mode;
  uint32_t value;
  bool event;
  PinFunction_t func; 
};

// structure for emulating SPI registers
struct EmulatedRegister_t {
  uint8_t value;
  uint8_t readOnlyBitFlags;
  bool bufferAccess;
};

// base class for emulated radio modules (SX126x etc.)
class EmulatedRadio {
  public:
    void connect(EmulatedPin_t* csPin, EmulatedPin_t* irqPin, EmulatedPin_t* rstPin, EmulatedPin_t* gpioPin) {
      this->cs = csPin;
      this->cs->func = PIN_CS;
      this->irq = irqPin;
      this->irq->func = PIN_IRQ;
      this->rst = rstPin;
      this->rst->func = PIN_RST;
      this->gpio = gpioPin;
      this->gpio->func = PIN_GPIO;
    }

    virtual uint8_t HandleSPI(uint8_t b) {
      (void)b;
      // handle the SPI input and generate output here
      return(EMULATED_RADIO_SPI_RETURN);
    }

    virtual void HandleGPIO() {
      // handle discrete GPIO signals here (e.g. reset state machine on NSS falling edge)
    }
  
  protected:
    // pointers to emulated GPIO pins
    // this is done via pointers so that the same GPIO entity is shared, like with a real hardware
    EmulatedPin_t* cs;
    EmulatedPin_t* irq;
    EmulatedPin_t* rst;
    EmulatedPin_t* gpio;
};

#endif
