#include "LR11x0.h"

#include "../../utils/CRC.h"
#include "../../utils/Cryptography.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR11X0

uint8_t LR11x0::roundRampTime(uint32_t rampTimeUs)
{
  uint8_t regVal;

  // Round up the ramp time to nearest discrete register value
  if(rampTimeUs <= 16) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_16U;
  } else if(rampTimeUs <= 32) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_32U;
  } else if(rampTimeUs <= 48) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_48U;
  } else if(rampTimeUs <= 64) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_64U;
  } else if(rampTimeUs <= 80) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_80U;
  } else if(rampTimeUs <= 96) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_96U;
  } else if(rampTimeUs <= 112) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_112U;
  } else if(rampTimeUs <= 128) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_128U;
  } else if(rampTimeUs <= 144) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_144U;
  } else if(rampTimeUs <= 160) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_160U;
  } else if(rampTimeUs <= 176) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_176U;
  } else if(rampTimeUs <= 192) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_192U;
  } else if(rampTimeUs <= 208) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_208U;
  } else if(rampTimeUs <= 240) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_240U;
  } else if(rampTimeUs <= 272) {
    regVal = RADIOLIB_LR11X0_PA_RAMP_272U;
  } else {  // 304
    regVal = RADIOLIB_LR11X0_PA_RAMP_304U;
  }

  return regVal;
}

int16_t LR11x0::writeRegMem32(uint32_t addr, const uint32_t* data, size_t len) {
  // check maximum size
  if(len > (RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN / sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->writeCommon(RADIOLIB_LR11X0_CMD_WRITE_REG_MEM, addr, data, len, false));
}

int16_t LR11x0::readRegMem32(uint32_t addr, uint32_t* data, size_t len) {
  // check maximum size
  if(len >= (RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN / sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // the request contains the address and length
  uint8_t reqBuff[5] = {
    (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF),
    (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
    (uint8_t)len,
  };

  // build buffers - later we need to ensure endians are correct,
  // so there is probably no way to do this without copying buffers and iterating
  #if RADIOLIB_STATIC_ONLY
  uint8_t rplBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
  uint8_t* rplBuff = new uint8_t[len * sizeof(uint32_t)];
  #endif

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_READ_REG_MEM, false, rplBuff, len * sizeof(uint32_t), reqBuff, sizeof(reqBuff));

  // convert endians
  if(data && (state == RADIOLIB_ERR_NONE)) {
    for(size_t i = 0; i < len; i++) {
      data[i] = ((uint32_t)rplBuff[2 + i * sizeof(uint32_t)] << 24) | ((uint32_t)rplBuff[3 + i * sizeof(uint32_t)] << 16) | ((uint32_t)rplBuff[4 + i * sizeof(uint32_t)] << 8) | (uint32_t)rplBuff[5 + i * sizeof(uint32_t)];
    }
  }

  #if !RADIOLIB_STATIC_ONLY
  delete[] rplBuff;
  #endif

  return(state);
}

int16_t LR11x0::writeBuffer8(const uint8_t* data, size_t len) {
  // check maximum size
  if(len > RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WRITE_BUFFER, true, const_cast<uint8_t*>(data), len));
}

int16_t LR11x0::readBuffer8(uint8_t* data, size_t len, size_t offset) {
  // check maximum size
  if(len > RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // build buffers
  size_t reqLen = 2 * sizeof(uint8_t) + len;
  #if RADIOLIB_STATIC_ONLY
  uint8_t reqBuff[sizeof(uint32_t) + RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
  uint8_t* reqBuff = new uint8_t[reqLen];
  #endif

  // set the offset and length
  reqBuff[0] = (uint8_t)offset;
  reqBuff[1] = (uint8_t)len;

  // send the request
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_READ_BUFFER, false, data, len, reqBuff, reqLen);
  #if !RADIOLIB_STATIC_ONLY
  delete[] reqBuff;
  #endif
  return(state);
}

int16_t LR11x0::clearRxBuffer(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CLEAR_RX_BUFFER, true, NULL, 0));
}

int16_t LR11x0::writeRegMemMask32(uint32_t addr, uint32_t mask, uint32_t data) {
  uint8_t buff[12] = {
    (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
    (uint8_t)((mask >> 24) & 0xFF), (uint8_t)((mask >> 16) & 0xFF), (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    (uint8_t)((data >> 24) & 0xFF), (uint8_t)((data >> 16) & 0xFF), (uint8_t)((data >> 8) & 0xFF), (uint8_t)(data & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WRITE_REG_MEM_MASK, true, buff, sizeof(buff)));
}

int16_t LR11x0::getStatus(uint8_t* stat1, uint8_t* stat2, uint32_t* irq) {
  uint8_t buff[6] = { 0 };

  // the status check command doesn't return status in the same place as other read commands
  // but only as the first byte (as with any other command), hence LR11x0::SPIcommand can't be used
  // it also seems to ignore the actual command, and just sending in bunch of NOPs will work
  int16_t state = this->mod->SPItransferStream(NULL, 0, false, NULL, buff, sizeof(buff), true);

  // pass the replies
  if(stat1) {
    *stat1 = buff[0];
  }
  if(stat2) {
    *stat2 = buff[1];
  }
  if(irq) {
    *irq = ((uint32_t)(buff[2]) << 24) | ((uint32_t)(buff[3]) << 16) | ((uint32_t)(buff[4]) << 8) | (uint32_t)buff[5];
  }

  return(state);
}

int16_t LR11x0::getVersion(uint8_t* hw, uint8_t* device, uint8_t* major, uint8_t* minor) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_VERSION, false, buff, sizeof(buff));

  // pass the replies
  if(hw) {
    *hw = buff[0];
  }
  if(device) {
    *device = buff[1];
  }
  if(major) {
    *major = buff[2];
  }
  if(minor) {
    *minor = buff[3];
  }

  return(state);
}

int16_t LR11x0::getErrors(uint16_t* err) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_ERRORS, false, buff, sizeof(buff));

  // pass the replies
  if(err) {
    *err = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1];
  }

  return(state);
}

int16_t LR11x0::clearErrors(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CLEAR_ERRORS, true, NULL, 0));
}

int16_t LR11x0::calibrate(uint8_t params) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CALIBRATE, true, &params, 1));
}

int16_t LR11x0::setRegMode(uint8_t mode) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_REG_MODE, true, &mode, 1));
}

int16_t LR11x0::calibrateImageRejection(float freqMin, float freqMax) {
  uint8_t buff[2] = {
    (uint8_t)floor((freqMin - 1.0f) / 4.0f),
    (uint8_t)ceil((freqMax + 1.0f) / 4.0f)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CALIB_IMAGE, true, buff, sizeof(buff)));
}

int16_t LR11x0::setDioAsRfSwitch(uint8_t en, uint8_t stbyCfg, uint8_t rxCfg, uint8_t txCfg, uint8_t txHpCfg, uint8_t txHfCfg, uint8_t gnssCfg, uint8_t wifiCfg) {
  uint8_t buff[8] = { en, stbyCfg, rxCfg, txCfg, txHpCfg, txHfCfg, gnssCfg, wifiCfg };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_DIO_AS_RF_SWITCH, true, buff, sizeof(buff)));
}

int16_t LR11x0::setDioIrqParams(uint32_t irq1, uint32_t irq2) {
  uint8_t buff[8] = {
    (uint8_t)((irq1 >> 24) & 0xFF), (uint8_t)((irq1 >> 16) & 0xFF), (uint8_t)((irq1 >> 8) & 0xFF), (uint8_t)(irq1 & 0xFF),
    (uint8_t)((irq2 >> 24) & 0xFF), (uint8_t)((irq2 >> 16) & 0xFF), (uint8_t)((irq2 >> 8) & 0xFF), (uint8_t)(irq2 & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_DIO_IRQ_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setDioIrqParams(uint32_t irq) {
  return(setDioIrqParams(irq, this->gnss ? 0 : irq));
}

int16_t LR11x0::clearIrqState(uint32_t irq) {
  uint8_t buff[4] = {
    (uint8_t)((irq >> 24) & 0xFF), (uint8_t)((irq >> 16) & 0xFF), (uint8_t)((irq >> 8) & 0xFF), (uint8_t)(irq & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CLEAR_IRQ, true, buff, sizeof(buff)));
}

int16_t LR11x0::configLfClock(uint8_t setup) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_CONFIG_LF_CLOCK, true, &setup, 1));
}

int16_t LR11x0::setTcxoMode(uint8_t tune, uint32_t delay) {
  uint8_t buff[4] = {
    tune, (uint8_t)((delay >> 16) & 0xFF), (uint8_t)((delay >> 8) & 0xFF), (uint8_t)(delay & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_TCXO_MODE, true, buff, sizeof(buff)));
}

int16_t LR11x0::reboot(bool stay) {
  uint8_t buff[1] = { (uint8_t)(stay * 3) };
  return(this->mod->SPIwriteStream(RADIOLIB_LR11X0_CMD_REBOOT, buff, sizeof(buff), true, false));
}

int16_t LR11x0::getVbat(float* vbat) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_VBAT, false, buff, sizeof(buff));

  // pass the replies
  if(vbat) {
    *vbat = (((float)buff[0] / 51.0f) - 1.0f) * 1.35f;
  }

  return(state);
}

int16_t LR11x0::getTemp(float* temp) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_TEMP, false, buff, sizeof(buff));

  // pass the replies
  if(temp) {
    uint16_t raw = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1];
    raw = raw & 0x07FF; // According LR1121 datasheet we need [0..10] bits
    *temp = 25.0f - (1000.0f / 1.7f) * (((float)raw / 2047.0f) * 1.35f - 0.7295f); // According LR1121 datasheet 1.35
  }

  return(state);
}

int16_t LR11x0::setFs(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_FS, true, NULL, 0));
}

int16_t LR11x0::getRandomNumber(uint32_t* rnd) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_RANDOM_NUMBER, false, buff, sizeof(buff));

  // pass the replies
  if(rnd) {
    *rnd = ((uint32_t)(buff[0]) << 24) | ((uint32_t)(buff[1]) << 16) | ((uint32_t)(buff[2]) << 8) | (uint32_t)buff[3];
  }

  return(state);
}

int16_t LR11x0::eraseInfoPage(void) {
  // only page 1 can be erased
  uint8_t buff[1] = { RADIOLIB_LR11X0_INFO_PAGE };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_ERASE_INFO_PAGE, true, buff, sizeof(buff)));
}

int16_t LR11x0::writeInfoPage(uint16_t addr, const uint32_t* data, size_t len) {
  // check maximum size
  if(len > (RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN / sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // build buffers - later we need to ensure endians are correct,
  // so there is probably no way to do this without copying buffers and iterating
  size_t buffLen = sizeof(uint8_t) + sizeof(uint16_t) + len * sizeof(uint32_t);
  #if RADIOLIB_STATIC_ONLY
  uint8_t dataBuff[sizeof(uint8_t) + sizeof(uint16_t) + RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
  uint8_t* dataBuff = new uint8_t[buffLen];
  #endif

  // set the address
  dataBuff[0] = RADIOLIB_LR11X0_INFO_PAGE;
  dataBuff[1] = (uint8_t)((addr >> 8) & 0xFF);
  dataBuff[2] = (uint8_t)(addr & 0xFF);

  // convert endians
  for(size_t i = 0; i < len; i++) {
    dataBuff[3 + i] = (uint8_t)((data[i] >> 24) & 0xFF);
    dataBuff[4 + i] = (uint8_t)((data[i] >> 16) & 0xFF);
    dataBuff[5 + i] = (uint8_t)((data[i] >> 8) & 0xFF);
    dataBuff[6 + i] = (uint8_t)(data[i] & 0xFF);
  }

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_WRITE_INFO_PAGE, true, dataBuff, buffLen);
  #if !RADIOLIB_STATIC_ONLY
  delete[] dataBuff;
  #endif
  return(state);
}

int16_t LR11x0::readInfoPage(uint16_t addr, uint32_t* data, size_t len) {
  // check maximum size
  if(len > (RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN / sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // the request contains the address and length
  uint8_t reqBuff[4] = {
    RADIOLIB_LR11X0_INFO_PAGE,
    (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
    (uint8_t)len,
  };

  // build buffers - later we need to ensure endians are correct,
  // so there is probably no way to do this without copying buffers and iterating
  #if RADIOLIB_STATIC_ONLY
  uint8_t rplBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
  uint8_t* rplBuff = new uint8_t[len * sizeof(uint32_t)];
  #endif

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_READ_INFO_PAGE, false, rplBuff, len * sizeof(uint32_t), reqBuff, sizeof(reqBuff));

  // convert endians
  if(data && (state == RADIOLIB_ERR_NONE)) {
    for(size_t i = 0; i < len; i++) {
      data[i] = ((uint32_t)rplBuff[2 + i * sizeof(uint32_t)] << 24) | ((uint32_t)rplBuff[3 + i * sizeof(uint32_t)] << 16) | ((uint32_t)rplBuff[4 + i * sizeof(uint32_t)] << 8) | (uint32_t)rplBuff[5 + i * sizeof(uint32_t)];
    }
  }

  #if !RADIOLIB_STATIC_ONLY
  delete[] rplBuff;
  #endif

  return(state);
}

int16_t LR11x0::getChipEui(uint8_t* eui) {
  RADIOLIB_ASSERT_PTR(eui);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_CHIP_EUI, false, eui, RADIOLIB_LR11X0_EUI_LEN));
}

int16_t LR11x0::getSemtechJoinEui(uint8_t* eui) {
  RADIOLIB_ASSERT_PTR(eui);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_SEMTECH_JOIN_EUI, false, eui, RADIOLIB_LR11X0_EUI_LEN));
}

int16_t LR11x0::deriveRootKeysAndGetPin(uint8_t* pin) {
  RADIOLIB_ASSERT_PTR(pin);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_DERIVE_ROOT_KEYS_AND_GET_PIN, false, pin, RADIOLIB_LR11X0_PIN_LEN));
}

int16_t LR11x0::enableSpiCrc(bool en) {
  // TODO implement this
  (void)en;
  // LR11X0 CRC is gen 0xA6 (0x65 but reflected), init 0xFF, input and result reflected
  /*RadioLibCRCInstance.size = 8;
  RadioLibCRCInstance.poly = 0xA6;
  RadioLibCRCInstance.init = 0xFF;
  RadioLibCRCInstance.out = 0x00;
  RadioLibCRCInstance.refIn = true;
  RadioLibCRCInstance.refOut = true;*/
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t LR11x0::driveDiosInSleepMode(bool en) {
  uint8_t buff[1] = { (uint8_t)en };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_DRIVE_DIOS_IN_SLEEP_MODE, true, buff, sizeof(buff)));
}

int16_t LR11x0::resetStats(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_RESET_STATS, true, NULL, 0));
}

int16_t LR11x0::getStats(uint16_t* nbPktReceived, uint16_t* nbPktCrcError, uint16_t* data1, uint16_t* data2) {
  uint8_t buff[8] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_STATS, false, buff, sizeof(buff));

  // pass the replies
  if(nbPktReceived) {
    *nbPktReceived = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1];
  }
  if(nbPktCrcError) {
    *nbPktCrcError = ((uint16_t)(buff[2]) << 8) | (uint16_t)buff[3];
  }
  if(data1) {
    *data1 = ((uint16_t)(buff[4]) << 8) | (uint16_t)buff[5];
  }
  if(data2) {
    *data2 = ((uint16_t)(buff[6]) << 8) | (uint16_t)buff[7];
  }

  return(state);
}

int16_t LR11x0::getPacketType(uint8_t* type) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_PACKET_TYPE, false, buff, sizeof(buff));

  // pass the replies
  if(type) {
    *type = buff[0];
  }

  return(state);
}

int16_t LR11x0::getRxBufferStatus(uint8_t* len, uint8_t* startOffset) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_RX_BUFFER_STATUS, false, buff, sizeof(buff));

  // pass the replies
  if(len) {
    *len = buff[0];
  }
  if(startOffset) {
    *startOffset = buff[1];
  }

  return(state);
}

int16_t LR11x0::getPacketStatusLoRa(float* rssiPkt, float* snrPkt, float* signalRssiPkt) {
  uint8_t buff[3] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_PACKET_STATUS, false, buff, sizeof(buff));

  // pass the replies
  if(rssiPkt) {
    *rssiPkt = (float)buff[0] / -2.0f;
  }
  if(snrPkt) {
    *snrPkt = (float)((int8_t)buff[1]) / 4.0f;
  }
  if(signalRssiPkt) {
    *signalRssiPkt = buff[2];
  }

  return(state);
}

int16_t LR11x0::getPacketStatusGFSK(float* rssiSync, float* rssiAvg, uint8_t* rxLen, uint8_t* stat) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_PACKET_STATUS, false, buff, sizeof(buff));

  // pass the replies
  if(rssiSync) {
    *rssiSync = (float)buff[0] / -2.0f;
  }
  if(rssiAvg) {
    *rssiAvg = (float)buff[1] / -2.0f;
  }
  if(rxLen) {
    *rxLen = buff[2];
  }
  if(stat) {
    *stat = buff[3];
  }

  return(state);
}

int16_t LR11x0::getRssiInst(float* rssi) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_RSSI_INST, false, buff, sizeof(buff));

  // pass the replies
  if(rssi) {
    *rssi = (float)buff[0] / -2.0f;
  }

  return(state);
}

int16_t LR11x0::setGfskSyncWord(uint8_t* sync) {
  RADIOLIB_ASSERT_PTR(sync);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_GFSK_SYNC_WORD, true, sync, RADIOLIB_LR11X0_GFSK_SYNC_WORD_LEN));
}

int16_t LR11x0::setLoRaPublicNetwork(bool pub) {
  uint8_t buff[1] = { (uint8_t)pub };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_LORA_PUBLIC_NETWORK, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRx(uint32_t timeout) {
  uint8_t buff[3] = {
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RX, true, buff, sizeof(buff)));
}

int16_t LR11x0::setTx(uint32_t timeout) {
  uint8_t buff[3] = {
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_TX, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRfFrequency(uint32_t rfFreq) {
  uint8_t buff[4] = {
    (uint8_t)((rfFreq >> 24) & 0xFF), (uint8_t)((rfFreq >> 16) & 0xFF),
    (uint8_t)((rfFreq >> 8) & 0xFF), (uint8_t)(rfFreq & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RF_FREQUENCY, true, buff, sizeof(buff)));
}

int16_t LR11x0::autoTxRx(uint32_t delay, uint8_t intMode, uint32_t timeout) {
  uint8_t buff[7] = {
    (uint8_t)((delay >> 16) & 0xFF), (uint8_t)((delay >> 8) & 0xFF), (uint8_t)(delay & 0xFF), intMode,
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_AUTO_TX_RX, true, buff, sizeof(buff)));
}

int16_t LR11x0::setCadParams(uint8_t symNum, uint8_t detPeak, uint8_t detMin, uint8_t cadExitMode, uint32_t timeout) {
  uint8_t buff[7] = {
    symNum, detPeak, detMin, cadExitMode,
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_CAD_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setPacketType(uint8_t type) {
  uint8_t buff[1] = { type };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_PACKET_TYPE, true, buff, sizeof(buff)));
}

int16_t LR11x0::setModulationParamsLoRa(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro) {
  // calculate symbol length and enable low data rate optimization, if auto-configuration is enabled
  if(this->ldroAuto) {
    float symbolLength = (float)(uint32_t(1) << this->spreadingFactor) / (float)this->bandwidthKhz;
    if(symbolLength >= 16.0f) {
      this->ldrOptimize = RADIOLIB_LR11X0_LORA_LDRO_ENABLED;
    } else {
      this->ldrOptimize = RADIOLIB_LR11X0_LORA_LDRO_DISABLED;
    }
  } else {
    this->ldrOptimize = ldro;
  }

  uint8_t buff[4] = { sf, bw, cr, this->ldrOptimize };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_MODULATION_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setModulationParamsGFSK(uint32_t br, uint8_t sh, uint8_t rxBw, uint32_t freqDev) {
  uint8_t buff[10] = {
    (uint8_t)((br >> 24) & 0xFF), (uint8_t)((br >> 16) & 0xFF),
    (uint8_t)((br >> 8) & 0xFF), (uint8_t)(br & 0xFF), sh, rxBw,
    (uint8_t)((freqDev >> 24) & 0xFF), (uint8_t)((freqDev >> 16) & 0xFF),
    (uint8_t)((freqDev >> 8) & 0xFF), (uint8_t)(freqDev & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_MODULATION_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setModulationParamsLrFhss(uint32_t br, uint8_t sh) {
  uint8_t buff[5] = {
    (uint8_t)((br >> 24) & 0xFF), (uint8_t)((br >> 16) & 0xFF),
    (uint8_t)((br >> 8) & 0xFF), (uint8_t)(br & 0xFF), sh
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_MODULATION_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setModulationParamsSigfox(uint32_t br, uint8_t sh) {
  // same as for LR-FHSS
  return(this->setModulationParamsLrFhss(br, sh));
}

int16_t LR11x0::setPacketParamsLoRa(uint16_t preambleLen, uint8_t hdrType, uint8_t payloadLen, uint8_t crcType, uint8_t invertIQ) {
  uint8_t buff[6] = {
    (uint8_t)((preambleLen >> 8) & 0xFF), (uint8_t)(preambleLen & 0xFF),
    hdrType, payloadLen, crcType, invertIQ
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_PACKET_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setPacketParamsGFSK(uint16_t preambleLen, uint8_t preambleDetectorLen, uint8_t syncWordLen, uint8_t addrCmp, uint8_t packType, uint8_t payloadLen, uint8_t crcType, uint8_t whiten) {
  uint8_t buff[9] = {
    (uint8_t)((preambleLen >> 8) & 0xFF), (uint8_t)(preambleLen & 0xFF),
    preambleDetectorLen, syncWordLen, addrCmp, packType, payloadLen, crcType, whiten
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_PACKET_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setPacketParamsSigfox(uint8_t payloadLen, uint16_t rampUpDelay, uint16_t rampDownDelay, uint16_t bitNum) {
  uint8_t buff[7] = {
    payloadLen, (uint8_t)((rampUpDelay >> 8) & 0xFF), (uint8_t)(rampUpDelay & 0xFF),
    (uint8_t)((rampDownDelay >> 8) & 0xFF), (uint8_t)(rampDownDelay & 0xFF),
    (uint8_t)((bitNum >> 8) & 0xFF), (uint8_t)(bitNum & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_PACKET_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setTxParams(int8_t pwr, uint8_t ramp) {
  uint8_t buff[2] = { (uint8_t)pwr, ramp };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_TX_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setPacketAdrs(uint8_t node, uint8_t broadcast) {
  uint8_t buff[2] = { node, broadcast };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_PACKET_ADRS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRxTxFallbackMode(uint8_t mode) {
  uint8_t buff[1] = { mode };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RX_TX_FALLBACK_MODE, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRxDutyCycle(uint32_t rxPeriod, uint32_t sleepPeriod, uint8_t mode) {
  uint8_t buff[7] = {
    (uint8_t)((rxPeriod >> 16) & 0xFF), (uint8_t)((rxPeriod >> 8) & 0xFF), (uint8_t)(rxPeriod & 0xFF),
    (uint8_t)((sleepPeriod >> 16) & 0xFF), (uint8_t)((sleepPeriod >> 8) & 0xFF), (uint8_t)(sleepPeriod & 0xFF),
    mode
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RX_DUTY_CYCLE, true, buff, sizeof(buff)));
}

int16_t LR11x0::setPaConfig(uint8_t paSel, uint8_t regPaSupply, uint8_t paDutyCycle, uint8_t paHpSel) {
  uint8_t buff[4] = { paSel, regPaSupply, paDutyCycle, paHpSel };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_PA_CONFIG, true, buff, sizeof(buff)));
}

int16_t LR11x0::stopTimeoutOnPreamble(bool stop) {
  uint8_t buff[1] = { (uint8_t)stop };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_STOP_TIMEOUT_ON_PREAMBLE, true, buff, sizeof(buff)));
}

int16_t LR11x0::setCad(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_CAD, true, NULL, 0));
}

int16_t LR11x0::setTxCw(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_TX_CW, true, NULL, 0));
}

int16_t LR11x0::setTxInfinitePreamble(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_TX_INFINITE_PREAMBLE, true, NULL, 0));
}

int16_t LR11x0::setLoRaSynchTimeout(uint8_t symbolNum) {
  uint8_t buff[1] = { symbolNum };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_LORA_SYNCH_TIMEOUT, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRangingAddr(uint32_t addr, uint8_t checkLen) {
  uint8_t buff[5] = {
    (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF),
    (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF), checkLen
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RANGING_ADDR, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRangingReqAddr(uint32_t addr) {
  uint8_t buff[4] = {
    (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF),
    (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RANGING_REQ_ADDR, true, buff, sizeof(buff)));
}

int16_t LR11x0::getRangingResult(uint8_t type, float* res) {
  uint8_t reqBuff[1] = { type };
  uint8_t rplBuff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_RANGING_RESULT, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);

  if(res) {
    if(type == RADIOLIB_LR11X0_RANGING_RESULT_DISTANCE) {
      uint32_t raw = ((uint32_t)(rplBuff[0]) << 24) | ((uint32_t)(rplBuff[1]) << 16) | ((uint32_t)(rplBuff[2]) << 8) | (uint32_t)rplBuff[3];
      *res = ((float)(raw * 3e8)) / ((float)(4096 * this->bandwidthKhz * 1000));
    } else {
      *res = (float)rplBuff[3] / 2.0f;
    }
  }

  return(state);
}

int16_t LR11x0::setRangingTxRxDelay(uint32_t delay) {
  uint8_t buff[4] = {
    (uint8_t)((delay >> 24) & 0xFF), (uint8_t)((delay >> 16) & 0xFF),
    (uint8_t)((delay >> 8) & 0xFF), (uint8_t)(delay & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RANGING_TX_RX_DELAY, true, buff, sizeof(buff)));
}

int16_t LR11x0::setGfskCrcParams(uint32_t init, uint32_t poly) {
  uint8_t buff[8] = {
    (uint8_t)((init >> 24) & 0xFF), (uint8_t)((init >> 16) & 0xFF),
    (uint8_t)((init >> 8) & 0xFF), (uint8_t)(init & 0xFF),
    (uint8_t)((poly >> 24) & 0xFF), (uint8_t)((poly >> 16) & 0xFF),
    (uint8_t)((poly >> 8) & 0xFF), (uint8_t)(poly & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_GFSK_CRC_PARAMS, true, buff, sizeof(buff)));

}

int16_t LR11x0::setGfskWhitParams(uint16_t seed) {
  uint8_t buff[2] = {
    (uint8_t)((seed >> 8) & 0xFF), (uint8_t)(seed & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_GFSK_WHIT_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRangingParameter(uint8_t symbolNum) {
  // the first byte is reserved
  uint8_t buff[2] = { 0x00, symbolNum };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RANGING_PARAMETER, true, buff, sizeof(buff)));
}

int16_t LR11x0::setRssiCalibration(const int8_t* tune, int16_t gainOffset) {
  uint8_t buff[11] = {
    (uint8_t)((tune[0] & 0x0F) | (uint8_t)(tune[1] & 0x0F) << 4),
    (uint8_t)((tune[2] & 0x0F) | (uint8_t)(tune[3] & 0x0F) << 4),
    (uint8_t)((tune[4] & 0x0F) | (uint8_t)(tune[5] & 0x0F) << 4),
    (uint8_t)((tune[6] & 0x0F) | (uint8_t)(tune[7] & 0x0F) << 4),
    (uint8_t)((tune[8] & 0x0F) | (uint8_t)(tune[9] & 0x0F) << 4),
    (uint8_t)((tune[10] & 0x0F) | (uint8_t)(tune[11] & 0x0F) << 4),
    (uint8_t)((tune[12] & 0x0F) | (uint8_t)(tune[13] & 0x0F) << 4),
    (uint8_t)((tune[14] & 0x0F) | (uint8_t)(tune[15] & 0x0F) << 4),
    (uint8_t)((tune[16] & 0x0F) | (uint8_t)(tune[17] & 0x0F) << 4),
    (uint8_t)(((uint16_t)gainOffset >> 8) & 0xFF), (uint8_t)(gainOffset & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RSSI_CALIBRATION, true, buff, sizeof(buff)));
}

int16_t LR11x0::setLoRaSyncWord(uint8_t sync) {
  uint8_t buff[1] = { sync };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_LORA_SYNC_WORD, true, buff, sizeof(buff)));
}

int16_t LR11x0::lrFhssBuildFrame(uint8_t hdrCount, uint8_t cr, uint8_t grid, bool hop, uint8_t bw, uint16_t hopSeq, int8_t devOffset, const uint8_t* payload, size_t len) {
  // check maximum size
  const uint8_t maxLen[4][4] = {
    { 189, 178, 167, 155, },
    { 151, 142, 133, 123, },
    { 112, 105,  99,  92, },
    {  74,  69,  65,  60, },
  };
  if((cr > RADIOLIB_LR11X0_LR_FHSS_CR_1_3) || ((hdrCount - 1) > (int)sizeof(maxLen[0])) || (len > maxLen[cr][hdrCount - 1])) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // build buffers
  size_t buffLen = 9 + len;
  #if RADIOLIB_STATIC_ONLY
  uint8_t dataBuff[9 + 190];
  #else
  uint8_t* dataBuff = new uint8_t[buffLen];
  #endif

  // set properties of the packet
  dataBuff[0] = hdrCount;
  dataBuff[1] = cr;
  dataBuff[2] = RADIOLIB_LR11X0_LR_FHSS_MOD_TYPE_GMSK;
  dataBuff[3] = grid;
  dataBuff[4] = (uint8_t)hop;
  dataBuff[5] = bw;
  dataBuff[6] = (uint8_t)((hopSeq >> 8) & 0x01);
  dataBuff[7] = (uint8_t)(hopSeq & 0xFF);
  dataBuff[8] = devOffset;
  memcpy(&dataBuff[9], payload, len);

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_LR_FHSS_BUILD_FRAME, true, dataBuff, buffLen);
  #if !RADIOLIB_STATIC_ONLY
  delete[] dataBuff;
  #endif
  return(state);
}

int16_t LR11x0::lrFhssSetSyncWord(uint32_t sync) {
  uint8_t buff[4] = {
    (uint8_t)((sync >> 24) & 0xFF), (uint8_t)((sync >> 16) & 0xFF),
    (uint8_t)((sync >> 8) & 0xFF), (uint8_t)(sync & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_LR_FHSS_SET_SYNC_WORD, true, buff, sizeof(buff)));
}

int16_t LR11x0::configBleBeacon(uint8_t chan, const uint8_t* payload, size_t len) {
  return(this->bleBeaconCommon(RADIOLIB_LR11X0_CMD_CONFIG_BLE_BEACON, chan, payload, len));
}

int16_t LR11x0::getLoRaRxHeaderInfos(uint8_t* info) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GET_LORA_RX_HEADER_INFOS, false, buff, sizeof(buff));

  // pass the replies
  if(info) {
    *info = buff[0];
  }

  return(state);
}

int16_t LR11x0::bleBeaconSend(uint8_t chan, const uint8_t* payload, size_t len) {
  return(this->bleBeaconCommon(RADIOLIB_LR11X0_CMD_BLE_BEACON_SEND, chan, payload, len));
}

int16_t LR11x0::bleBeaconCommon(uint16_t cmd, uint8_t chan, const uint8_t* payload, size_t len) {
  // check maximum size
  // TODO what is the actual maximum?
  if(len > RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // build buffers
  #if RADIOLIB_STATIC_ONLY
  uint8_t dataBuff[sizeof(uint8_t) + RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
  uint8_t* dataBuff = new uint8_t[sizeof(uint8_t) + len];
  #endif

  // set the channel
  dataBuff[0] = chan;
  memcpy(&dataBuff[1], payload, len);

  int16_t state = this->SPIcommand(cmd, true, dataBuff, sizeof(uint8_t) + len);
  #if !RADIOLIB_STATIC_ONLY
  delete[] dataBuff;
  #endif
  return(state);
}

int16_t LR11x0::bootEraseFlash(void) {
  // erasing flash takes about 2.5 seconds, temporarily tset SPI timeout to 3 seconds
  RadioLibTime_t timeout = this->mod->spiConfig.timeout;
  this->mod->spiConfig.timeout = 3000;
  int16_t state = this->mod->SPIwriteStream(RADIOLIB_LR11X0_CMD_BOOT_ERASE_FLASH, NULL, 0, false, false);
  this->mod->spiConfig.timeout = timeout;
  return(state);
}

int16_t LR11x0::bootWriteFlashEncrypted(uint32_t offset, const uint32_t* data, size_t len, bool nonvolatile) {
  // check maximum size
  if(len > (RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN / sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->writeCommon(RADIOLIB_LR11X0_CMD_BOOT_WRITE_FLASH_ENCRYPTED, offset, data, len, nonvolatile));
}

int16_t LR11x0::bootReboot(bool stay) {
  uint8_t buff[1] = { (uint8_t)stay };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_BOOT_REBOOT, true, buff, sizeof(buff)));
}

int16_t LR11x0::bootGetPin(uint8_t* pin) {
  RADIOLIB_ASSERT_PTR(pin);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_BOOT_GET_PIN, false, pin, RADIOLIB_LR11X0_PIN_LEN));
}

int16_t LR11x0::bootGetChipEui(uint8_t* eui) {
  RADIOLIB_ASSERT_PTR(eui);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_BOOT_GET_CHIP_EUI, false, eui, RADIOLIB_LR11X0_EUI_LEN));
}

int16_t LR11x0::bootGetJoinEui(uint8_t* eui) {
  RADIOLIB_ASSERT_PTR(eui);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_BOOT_GET_JOIN_EUI, false, eui, RADIOLIB_LR11X0_EUI_LEN));
}

int16_t LR11x0::writeCommon(uint16_t cmd, uint32_t addrOffset, const uint32_t* data, size_t len, bool nonvolatile) {
  // build buffers - later we need to ensure endians are correct,
  // so there is probably no way to do this without copying buffers and iterating
  size_t buffLen = sizeof(uint32_t) + len * sizeof(uint32_t);
  #if RADIOLIB_STATIC_ONLY
  uint8_t dataBuff[sizeof(uint32_t) + RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
  uint8_t* dataBuff = new uint8_t[buffLen];
  #endif

  // set the address or offset
  dataBuff[0] = (uint8_t)((addrOffset >> 24) & 0xFF);
  dataBuff[1] = (uint8_t)((addrOffset >> 16) & 0xFF);
  dataBuff[2] = (uint8_t)((addrOffset >> 8) & 0xFF);
  dataBuff[3] = (uint8_t)(addrOffset & 0xFF);

  // convert endians
  for(size_t i = 0; i < len; i++) {
    uint32_t bin = 0;
    if(nonvolatile) {
      uint32_t* ptr = const_cast<uint32_t*>(data) + i;
      bin = RADIOLIB_NONVOLATILE_READ_DWORD(ptr);
    } else {
      bin = data[i];
    }
    dataBuff[4 + i * sizeof(uint32_t)] = (uint8_t)((bin >> 24) & 0xFF);
    dataBuff[5 + i * sizeof(uint32_t)] = (uint8_t)((bin >> 16) & 0xFF);
    dataBuff[6 + i * sizeof(uint32_t)] = (uint8_t)((bin >> 8) & 0xFF);
    dataBuff[7 + i * sizeof(uint32_t)] = (uint8_t)(bin & 0xFF);
  }

  int16_t state = this->mod->SPIwriteStream(cmd, dataBuff, buffLen, true, false);
  #if !RADIOLIB_STATIC_ONLY
  delete[] dataBuff;
  #endif
  return(state);
}

#endif
