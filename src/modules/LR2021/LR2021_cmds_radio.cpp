#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::setRfFrequency(uint32_t rfFreq) {
  return(this->setU32(RADIOLIB_LR2021_CMD_SET_RF_FREQUENCY, rfFreq));
}

int16_t LR2021::setRxPath(uint8_t rxPath, uint8_t rxBoost) {
  uint8_t buff[] = { (uint8_t)(rxPath & 0x01), (uint8_t)(rxBoost & 0x07) };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RX_PATH, true, buff, sizeof(buff));
  if (state == RADIOLIB_ERR_NONE) {
    state = this->setDCDCworkaround();
  }
  return(state);
}

int16_t LR2021::getRssiInst(float* rssi) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_RSSI_INST, false, buff, sizeof(buff));
  if(rssi) {
    uint16_t raw = ((uint16_t)(buff[0]) << 1) | (uint16_t)((buff[1] >> 7) & 0x01);
    *rssi = (float)raw * -0.5f;
  }
  return(state);
}

int16_t LR2021::setRssiCalibration(uint8_t rxPath, const uint16_t gain[RADIOLIB_LR2021_GAIN_TABLE_LENGTH], const uint8_t noiseFloor[RADIOLIB_LR2021_GAIN_TABLE_LENGTH]) {
  uint8_t buff[1 + 3*RADIOLIB_LR2021_GAIN_TABLE_LENGTH] = { 0 };
  buff[0] = rxPath;
  for(uint8_t i = 0; i < RADIOLIB_LR2021_GAIN_TABLE_LENGTH; i++) {
    buff[1 + 3*i] = (uint8_t)((gain[i] & 0x300) >> 8);
    buff[2 + 3*i] = (uint8_t)(gain[i] & 0xFF);
    buff[3 + 3*i] = noiseFloor[i];
  }
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RSSI_CALIBRATION, true, buff, sizeof(buff)));
}

int16_t LR2021::setTimestampSource(uint8_t index, uint8_t source) {
  uint8_t buff[] = { (uint8_t)(((index & 0x03) << 4) | (source & 0x0F)) };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_TIMESTAMP_SOURCE, true, buff, sizeof(buff)));
}

int16_t LR2021::getTimestampValue(uint8_t index, uint32_t* timestamp) {
  uint8_t reqBuff[] = { (uint8_t)(index & 0x03) };
  uint8_t rplBuff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_TIMESTAMP_VALUE, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  if(timestamp) { *timestamp = ((uint32_t)(rplBuff[0]) << 24) | ((uint32_t)(rplBuff[1]) << 16) | ((uint32_t)(rplBuff[2]) << 8) | (uint32_t)rplBuff[3]; }
  return(state);
}

int16_t LR2021::setCca(uint32_t duration, uint8_t gain) {
  uint8_t buff[] = {
    (uint8_t)((duration >> 16) & 0xFF), (uint8_t)((duration >> 8) & 0xFF), (uint8_t)(duration & 0xFF), gain,
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_CCA, true, buff, sizeof(buff)));
}

int16_t LR2021::getCcaResult(float* rssiMin, float* rssiMax, float* rssiAvg) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_CCA_RESULT, false, buff, sizeof(buff));
  uint16_t raw;
  if(rssiMin) {
    raw = ((uint16_t)(buff[0]) << 1) | (uint16_t)((buff[3] >> 2) & 0x01);
    *rssiMin = (float)raw * -0.5f;
  }
  if(rssiMax) {
    raw = ((uint16_t)(buff[1]) << 1) | (uint16_t)((buff[3] >> 1) & 0x01);
    *rssiMax = (float)raw * -0.5f;
  }
  if(rssiAvg) {
    raw = ((uint16_t)(buff[2]) << 1) | (uint16_t)((buff[3] >> 0) & 0x01);
    *rssiAvg = (float)raw * -0.5f;
  }
  return(state);
}

int16_t LR2021::setCadParams(uint32_t cadTimeout, uint8_t threshold, uint8_t exitMode, uint32_t trxTimeout) {
  uint8_t buff[] = {
    (uint8_t)((cadTimeout >> 16) & 0xFF), (uint8_t)((cadTimeout >> 8) & 0xFF), (uint8_t)(cadTimeout & 0xFF),
    threshold, (uint8_t)(exitMode & 0x03),
    (uint8_t)((trxTimeout >> 16) & 0xFF), (uint8_t)((trxTimeout >> 8) & 0xFF), (uint8_t)(trxTimeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_CAD_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setCad(void) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_CAD, true, NULL, 0));
}

int16_t LR2021::selPa(uint8_t pa) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SEL_PA, true, &pa, sizeof(pa)));
}

int16_t LR2021::setPaConfig(uint8_t pa, uint8_t paLfMode, uint8_t paLfDutyCycle, uint8_t paLfSlices, uint8_t paHfDutyCycle) {
  uint8_t buff[] = {
    (uint8_t)((pa << 7) | (paLfMode & 0x03)), 
    (uint8_t)(((paLfDutyCycle & 0x0F) << 4) | (paLfSlices & 0x0F)),
    (uint8_t)((paHfDutyCycle & 0x1F)),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_PA_CONFIG, true, buff, sizeof(buff)));
}

int16_t LR2021::setTxParams(int8_t txPower, uint8_t rampTime) {
  uint8_t buff[] = { (uint8_t)txPower, rampTime };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_TX_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setPacketType(uint8_t packetType) {
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_SET_PACKET_TYPE, true, &packetType, sizeof(packetType));
  if (state == RADIOLIB_ERR_NONE) {
    state = this->resetDCDCworkaround();
  }
  return(state);
}

int16_t LR2021::getPacketType(uint8_t* packetType) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_GET_PACKET_TYPE, false, packetType, sizeof(uint8_t)));
}

// undocumented commands below, credit to https://github.com/beala/lr2021-re

int16_t LR2021::setInternalParam(const uint8_t* params, size_t len) {
  // get/set of internal parameters at sub-indices 0x0B-0x15 of the struct at RAM 0x800438,
  // the exact parameter layout has not been decoded
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_INTERNAL_PARAM, true, const_cast<uint8_t*>(params), len));
}

int16_t LR2021::setClockIrqConfig(uint8_t cfg) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_CLOCK_IRQ_CONFIG, true, &cfg, sizeof(cfg)));
}

int16_t LR2021::setAgcConfig(const uint8_t* params, size_t len) {
  // AGC configuration, ends up in 0xF30C28[28:24], [22:21] and [20],
  // the exact parameter layout has not been decoded
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_AGC_CONFIG, true, const_cast<uint8_t*>(params), len));
}

int16_t LR2021::setPacketEngineReg(uint16_t cmd, const uint8_t* buff, size_t len) {
  // lower-level version of the documented FSK whitening/CRC/syncword/address commands,
  // cmd is one of RADIOLIB_LR2021_CMD_SET_PACKET_ENGINE_REG00-13
  return(this->SPIcommand(cmd, true, const_cast<uint8_t*>(buff), len));
}

int16_t LR2021::getPacketEngineStatus(uint8_t* data) {
  // undocumented getter, contents not decoded
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_GET_PACKET_ENGINE_STATUS, false, data, 6));
}

int16_t LR2021::getPacketEngineStats(uint8_t* data) {
  // undocumented getter, contents not decoded
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_GET_PACKET_ENGINE_STATS, false, data, 18));
}

#endif
