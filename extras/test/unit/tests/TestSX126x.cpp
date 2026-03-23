#include <boost/test/unit_test.hpp>

#include "ModuleFixture.hpp"

#include "modules/SX126x/SX126x.h"

class EmulatedSX126x : public EmulatedRadio {
  public:
    bool failRxBufferStatus = false;
    bool failPacketType = false;
    uint8_t packetType = RADIOLIB_SX126X_PACKET_TYPE_GFSK;
    uint8_t rxBufferStatus[2] = {0x11, 0x22};

    void HandleGPIO() override {
      if(this->cs && this->cs->event && (this->cs->value == 0)) {
        this->txnPos = 0;
        this->currentCmd = 0x00;
      }
    }

    uint8_t HandleSPI(uint8_t b) override {
      if(this->txnPos == 0) {
        this->currentCmd = b;
        this->txnPos++;
        return(0x00);
      }

      uint8_t out = 0x32;
      if(this->txnPos == 1) {
        if((this->currentCmd == RADIOLIB_SX126X_CMD_GET_RX_BUFFER_STATUS) && this->failRxBufferStatus) {
          out = RADIOLIB_SX126X_STATUS_CMD_TIMEOUT;
        } else if((this->currentCmd == RADIOLIB_SX126X_CMD_GET_PACKET_TYPE) && this->failPacketType) {
          out = RADIOLIB_SX126X_STATUS_CMD_TIMEOUT;
        }
      } else {
        switch(this->currentCmd) {
          case RADIOLIB_SX126X_CMD_GET_PACKET_TYPE:
            out = this->packetType;
            break;
          case RADIOLIB_SX126X_CMD_GET_RX_BUFFER_STATUS:
            out = this->rxBufferStatus[this->txnPos - 2];
            break;
          case RADIOLIB_SX126X_CMD_GET_IRQ_STATUS:
            out = 0x00;
            break;
          default:
            out = EMULATED_RADIO_SPI_RETURN;
            break;
        }
      }

      this->txnPos++;
      return(out);
    }

  private:
    uint8_t currentCmd = 0x00;
    size_t txnPos = 0;
};

class SX126xFixture {
  public:
    TestHal* hal = nullptr;
    Module* mod = nullptr;
    EmulatedSX126x hw;
    SX126x* radio = nullptr;

    SX126xFixture() {
      hal = new TestHal();
      hal->connectRadio(&hw);
      mod = new Module(hal, EMULATED_RADIO_NSS_PIN, EMULATED_RADIO_IRQ_PIN, EMULATED_RADIO_RST_PIN, EMULATED_RADIO_GPIO_PIN);
      mod->init();

      radio = new SX126x(mod);
      mod->hal->pinMode(mod->getIrq(), mod->hal->GpioModeInput);
      mod->hal->pinMode(mod->getGpio(), mod->hal->GpioModeInput);
      mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR] = Module::BITS_16;
      mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_8;
      mod->spiConfig.statusPos = 1;
      mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] = RADIOLIB_SX126X_CMD_READ_REGISTER;
      mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] = RADIOLIB_SX126X_CMD_WRITE_REGISTER;
      mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_NOP] = RADIOLIB_SX126X_CMD_NOP;
      mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_STATUS] = RADIOLIB_SX126X_CMD_GET_STATUS;
      mod->spiConfig.stream = true;
      mod->spiConfig.parseStatusCb = SX126x::SPIparseStatus;
      mod->spiConfig.timeout = 5;
    }

    ~SX126xFixture() {
      delete radio;
      mod->term();
      delete mod;
      delete hal;
    }
};

BOOST_FIXTURE_TEST_SUITE(suite_SX126x, SX126xFixture)

  BOOST_AUTO_TEST_CASE(SX126x_tryGetPacketLength_reports_command_timeout) {
    hw.packetType = RADIOLIB_SX126X_PACKET_TYPE_GFSK;
    hw.failRxBufferStatus = true;

    size_t length = 99;
    uint8_t offset = 77;
    int16_t state = radio->tryGetPacketLength(&length, true, &offset);

    BOOST_TEST(state == RADIOLIB_ERR_SPI_CMD_TIMEOUT);
    BOOST_TEST(length == (size_t)99);
    BOOST_TEST(offset == 77);
  }

  BOOST_AUTO_TEST_CASE(SX126x_getPacketLength_compatibility_wrapper_does_not_return_zero_on_command_timeout) {
    hw.packetType = RADIOLIB_SX126X_PACKET_TYPE_GFSK;
    hw.failRxBufferStatus = true;

    size_t length = radio->getPacketLength();

    BOOST_TEST(length == (size_t)RADIOLIB_SX126X_MAX_PACKET_LENGTH);
  }

  BOOST_AUTO_TEST_CASE(SX126x_readData_fails_closed_when_rx_buffer_status_read_fails) {
    hw.packetType = RADIOLIB_SX126X_PACKET_TYPE_GFSK;
    hw.failRxBufferStatus = true;

    uint8_t data[8] = {0};
    int16_t state = radio->readData(data, sizeof(data));

    BOOST_TEST(state == RADIOLIB_ERR_SPI_CMD_TIMEOUT);
  }

BOOST_AUTO_TEST_SUITE_END()
