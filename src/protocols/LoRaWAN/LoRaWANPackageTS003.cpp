#if !RADIOLIB_EXCLUDE_LORAWAN

#include "LoRaWANPackageTS003.h"

LoRaWANPackageTS003::LoRaWANPackageTS003(LoRaWANPackageManager* pacMan, LoRaWANNode* node, GetSecondsCb_t secondsCb)
  : LoRaWANPackage(RADIOLIB_LORAWAN_PACKAGE_TS003, pacMan, node, secondsCb),
    setSeconds(NULL), tokenReq(0), transmissions(0), periodicity(0), nextAppReqTime(0) {
  this->packageVersion = 2;
}

void LoRaWANPackageTS003::setSecondsCb(SetSecondsCb_t cb) {
  this->setSeconds = cb;
}

LoRaWANTaskInfo LoRaWANPackageTS003::hasTask() {
  LoRaWANTaskInfo task;
  task.type = RADIOLIB_LORAWAN_TASK_NONE;
  task.time = 0;

  RadioLibTime_t nowSec = this->getSeconds();

  // check for any pending uplinks
  if(this->lenUp > 0) {
    task.type = RADIOLIB_LORAWAN_TASK_UPLINK;
    task.time = nowSec;
    return(task);
  }

  // check if we are doing AppTimeReq at all
  if(this->nextAppReqTime == 0) {
    return(task);
  }

  task.type = RADIOLIB_LORAWAN_TASK_UPLINK;
  task.time = this->nextAppReqTime;

  // if next time is in the future, schedule for that time
  if(this->nextAppReqTime > nowSec) {
    return(task);
  }

  // if next time is now or in the past, trigger AppTimeReq and schedule next one
  this->requestAppTime();

  // if there are forced retransmissions, ignore periodicity
  if(this->transmissions > 0) {
    this->transmissions -= 1;
    this->nextAppReqTime = nowSec + random(60);

  // otherwise schedule based on periodicity
  } else if(this->periodicity > 0) {
    this->nextAppReqTime += this->periodicity - 30 + random(60);
  }

  // if there are no forced retransmissions and no periodicity, do not schedule
  else {
    this->nextAppReqTime = 0;
  }
  
  return(task);
}

void LoRaWANPackageTS003::doAction() {
  return;
}

size_t LoRaWANPackageTS003::processData(const uint8_t* dataDown, size_t lenDown, LoRaWANEvent_t* event) {
  size_t procLen = 0;
  this->lenUp = 0;

  while (procLen < lenDown) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("CID = %02x, len = %d", dataDown[procLen], lenDown - procLen - 1);
  
    switch(dataDown[procLen]) {
      case(RADIOLIB_LORAWAN_TS003_PACKAGE_VERSION): {
        // no downlink payload
        procLen += 1;

        this->dataUp[this->lenUp + 0] = RADIOLIB_LORAWAN_TS003_PACKAGE_VERSION;
        this->dataUp[this->lenUp + 1] = this->packageIdentifier;
        this->dataUp[this->lenUp + 2] = this->packageVersion;
        this->lenUp += 3;
        
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("PackageIdentifier: %d, PackageVersion: %d", 
                                        this->dataUp[1], this->dataUp[2]);
  
      } break;
      case(RADIOLIB_LORAWAN_TS003_APP_TIME): {
        uint8_t tokenAns = dataDown[procLen + 5] & 0x0F;
        if(tokenAns != this->tokenReq) {
          procLen += 6;         // skip payload
          break;
        }
        this->tokenReq += 1;
        this->tokenReq %= 16;
        uint32_t uCorrection = 0;
        uCorrection |= (uint32_t)dataDown[procLen + 1];
        uCorrection |= (uint32_t)dataDown[procLen + 2] <<  8;
        uCorrection |= (uint32_t)dataDown[procLen + 3] << 16;
        uCorrection |= (uint32_t)dataDown[procLen + 4] << 24;
        procLen += 6;
        int32_t correction = (int32_t)uCorrection;

        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TimeCorrection: %d, TokenAns: %d", correction, tokenAns);

        // apply time correction via callback
        this->setSeconds(this->getSeconds() + correction);

        // clear pending AppTimeReq uplink transmissions
        this->transmissions = 0;

        // if correction is min/max value, trigger another AppTimeReq
        if(correction == 0x7FFFFFFF || correction == 0x80000000) {
          this->nextAppReqTime = this->getSeconds();

        // otherwise, schedule next AppTimeReq based on periodicity (if set)
        } else if(this->periodicity) {
          this->nextAppReqTime = this->getSeconds() + this->periodicity - 30 + random(60);
        } else {
          this->nextAppReqTime = 0;
        }
  
      } break;
      case(RADIOLIB_LORAWAN_TS003_APP_TIME_PERIODICITY): {
        uint32_t period = dataDown[procLen + 1];
        procLen += 2;

        this->dataUp[this->lenUp + 0] = RADIOLIB_LORAWAN_TS003_APP_TIME_PERIODICITY;
        this->dataUp[this->lenUp + 1] = 0;
        RadioLibTime_t now = this->getSeconds();
        this->dataUp[this->lenUp + 2] = (uint8_t)((now >> 0)  & 0xFF);
        this->dataUp[this->lenUp + 3] = (uint8_t)((now >> 8)  & 0xFF);
        this->dataUp[this->lenUp + 4] = (uint8_t)((now >> 16) & 0xFF);
        this->dataUp[this->lenUp + 5] = (uint8_t)((now >> 24) & 0xFF);
        this->lenUp += 6;

        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Period: %lu, Time: %lu", period, now);
        this->periodicity = 128 << period;
        this->nextAppReqTime = this->getSeconds() + this->periodicity - 30 + random(60);
        
      } break;
      case(RADIOLIB_LORAWAN_TS003_FORCE_DEVICE_RESYNC): {
        uint8_t trans = dataDown[procLen + 1] & 0x0F;
        procLen += 2;

        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Forced transmissions: %d", trans);
        if(trans > 0) {
          this->nextAppReqTime = this->getSeconds();
          trans--;
        }
        this->transmissions = trans;

      } break;
    }
  }

  return(procLen);
}

int16_t LoRaWANPackageTS003::requestAppTime(bool force) {
  this->dataUp[0] = RADIOLIB_LORAWAN_TS003_APP_TIME;
  
  RadioLibTime_t now = this->getSeconds();
  this->dataUp[1] = (uint8_t)((now >> 0)  & 0xFF);
  this->dataUp[2] = (uint8_t)((now >> 8)  & 0xFF);
  this->dataUp[3] = (uint8_t)((now >> 16) & 0xFF);
  this->dataUp[4] = (uint8_t)((now >> 24) & 0xFF);
  this->dataUp[5] = (uint8_t)force << 4 | (this->tokenReq & 0x0F);
  this->lenUp = 6;
  return(RADIOLIB_ERR_NONE);
}

#endif
