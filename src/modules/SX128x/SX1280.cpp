#include "SX1280.h"
#if !defined(RADIOLIB_EXCLUDE_SX128X)

SX1280::SX1280(Module* mod) : SX1281(mod) {

}

int16_t SX1280::range(bool master, uint32_t addr) {
  // start ranging
  int16_t state = startRanging(master, addr);
  RADIOLIB_ASSERT(state);

  // wait until ranging is finished
  uint32_t start = Module::millis();
  while(!Module::digitalRead(_mod->getIrq())) {
    Module::yield();
    if(Module::millis() - start > 10000) {
      clearIrqStatus();
      standby();
      return(ERR_RANGING_TIMEOUT);
    }
  }

  // clear interrupt flags
  state = clearIrqStatus();
  RADIOLIB_ASSERT(state);

  // set mode to standby
  state = standby();

  return(state);
}

int16_t SX1280::startRanging(bool master, uint32_t addr) {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == SX128X_PACKET_TYPE_LORA) || (modem == SX128X_PACKET_TYPE_RANGING))) {
    return(ERR_WRONG_MODEM);
  }

  // ensure modem is set to ranging
  int16_t state = ERR_NONE;
  if(modem == SX128X_PACKET_TYPE_LORA) {
    state = setPacketType(SX128X_PACKET_TYPE_RANGING);
    RADIOLIB_ASSERT(state);
  }

  // set modulation parameters
  state = setModulationParams(_sf, _bw, _cr);
  RADIOLIB_ASSERT(state);

  // set packet parameters
  state = setPacketParamsLoRa(_preambleLengthLoRa, _headerType, _payloadLen, _crcLoRa);
  RADIOLIB_ASSERT(state);

  // check all address bits
  uint8_t regValue;
  state = readRegister(SX128X_REG_SLAVE_RANGING_ADDRESS_WIDTH, &regValue, 1);
  RADIOLIB_ASSERT(state);
  regValue &= 0b00111111;
  regValue |= 0b11000000;
  state = writeRegister(SX128X_REG_SLAVE_RANGING_ADDRESS_WIDTH, &regValue, 1);
  RADIOLIB_ASSERT(state);

  // set remaining parameter values
  uint32_t addrReg = SX128X_REG_SLAVE_RANGING_ADDRESS_BYTE_3;
  uint32_t irqMask = SX128X_IRQ_RANGING_SLAVE_RESP_DONE | SX128X_IRQ_RANGING_SLAVE_REQ_DISCARD;
  uint32_t irqDio1 = SX128X_IRQ_RANGING_SLAVE_RESP_DONE;
  if(master) {
    addrReg = SX128X_REG_MASTER_RANGING_ADDRESS_BYTE_3;
    irqMask = SX128X_IRQ_RANGING_MASTER_RES_VALID | SX128X_IRQ_RANGING_MASTER_TIMEOUT;
    irqDio1 = SX128X_IRQ_RANGING_MASTER_RES_VALID;
  }

  // set ranging address
  uint8_t addrBuff[] = { (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF) };
  state = writeRegister(addrReg, addrBuff, 4);
  RADIOLIB_ASSERT(state);

  // set DIO mapping
  state = setDioIrqParams(irqMask, irqDio1);
  RADIOLIB_ASSERT(state);

  // set role and start ranging
  if(master) {
    state = setRangingRole(SX128X_RANGING_ROLE_MASTER);
    RADIOLIB_ASSERT(state);

    state = setTx(SX128X_TX_TIMEOUT_NONE);
    RADIOLIB_ASSERT(state);

  } else {
    state = setRangingRole(SX128X_RANGING_ROLE_SLAVE);
    RADIOLIB_ASSERT(state);

    state = setRx(SX128X_RX_TIMEOUT_INF);
    RADIOLIB_ASSERT(state);

  }

  return(state);
}

float SX1280::getRangingResult() {
  // read the register values
  uint8_t data[4];
  int16_t state = readRegister(SX128X_REG_RANGING_RESULT_MSB, data + 1, 3);
  RADIOLIB_ASSERT(state);

  // calculate the real result
  uint32_t raw = 0;
  memcpy(&raw, data, sizeof(uint32_t));
  return((float)raw * (150.0/(4.096 * _bwKhz)));
}

#endif
