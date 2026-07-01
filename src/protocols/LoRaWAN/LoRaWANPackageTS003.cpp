#if !RADIOLIB_EXCLUDE_LORAWAN

#include "LoRaWANPackageTS003.h"

LoRaWANPackageTS003::LoRaWANPackageTS003(LoRaWANPackageManager* pacMan, RadioLibHal* hal, LoRaWANNode* node, GetSecondsCb_t secondsCb)
  : LoRaWANPackage(RADIOLIB_LORAWAN_PACKAGE_TS003, pacMan, hal, node, secondsCb),
    setSeconds(NULL), tokenReq(0), transmissions(0), periodicity(0), nextAppReqTime(0), forceReq(false) {
  this->packageVersion = 2;
}

void LoRaWANPackageTS003::setSecondsCb(SetSecondsCb_t cb) {
  this->setSeconds = cb;
}

bool LoRaWANPackageTS003::handleTask(RadioLibTime_t* tNext, bool* uplinkDue) {
  *uplinkDue = false;

  RadioLibTime_t nowSec = this->getSeconds();

  // check if we are doing AppTimeReq at all
  if(this->nextAppReqTime == 0) {
    return(false);
  }

  // if next time is in the future, report its time
  if(this->nextAppReqTime > nowSec) {
    *tNext = this->nextAppReqTime;
    return(true);
  }

  // if next time is now or in the past, an AppTimeReq uplink is due now. The payload
  // (and schedule advance) is produced by buildUplink() at the moment of transmission
  // so the timestamp it carries is fresh, even if the send is delayed by dutycycle.
  *uplinkDue = true;
  *tNext = nowSec;
  return(true);
}

size_t LoRaWANPackageTS003::buildUplink(uint8_t* dataOut) {
  RadioLibTime_t nowSec = this->getSeconds();

  // build the AppTimeReq with the current time, captured here at send time
  size_t len = this->buildAppTimeReq(dataOut);

  // if there are forced retransmissions, ignore periodicity
  if(this->transmissions > 0) {
    this->transmissions -= 1;
    this->nextAppReqTime = nowSec + (rand() % 60);

  // otherwise schedule based on periodicity
  } else if(this->periodicity > 0) {
    this->nextAppReqTime += this->periodicity - 30 + (rand() % 60);
  }

  // if there are no forced retransmissions and no periodicity, do not schedule
  else {
    this->nextAppReqTime = 0;
  }

  return(len);
}

size_t LoRaWANPackageTS003::processData(const uint8_t* dataDown, size_t lenDown, uint8_t* dataOut, size_t* lenOut, LoRaWANEvent_t* event) {
  (void)event;

  *lenOut = 0;
  if(lenDown < 1) {
    return(0);
  }

  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("CID = %02x, len = %d", dataDown[0], lenDown - 1);

  switch(dataDown[0]) {
    case(RADIOLIB_LORAWAN_TS003_PACKAGE_VERSION): {
      // no downlink payload
      dataOut[0] = RADIOLIB_LORAWAN_TS003_PACKAGE_VERSION;
      dataOut[1] = this->packageIdentifier;
      dataOut[2] = this->packageVersion;
      *lenOut = 3;

      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("PackageIdentifier: %d, PackageVersion: %d",
                                      dataOut[1], dataOut[2]);
      return(1);

    } break;
    case(RADIOLIB_LORAWAN_TS003_APP_TIME): {
      uint8_t tokenAns = dataDown[5] & 0x0F;
      if(tokenAns != this->tokenReq) {
        return(6);            // skip payload
      }
      this->tokenReq += 1;
      this->tokenReq %= 16;
      uint32_t uCorrection = 0;
      uCorrection |= (uint32_t)dataDown[1];
      uCorrection |= (uint32_t)dataDown[2] <<  8;
      uCorrection |= (uint32_t)dataDown[3] << 16;
      uCorrection |= (uint32_t)dataDown[4] << 24;
      int32_t correction = (int32_t)uCorrection;

      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TimeCorrection: %d, TokenAns: %d", correction, tokenAns);

      // apply time correction via callback
      this->setSeconds(this->getSeconds() + correction);

      // clear pending AppTimeReq uplink transmissions
      this->transmissions = 0;

      // if correction is min/max value, trigger another AppTimeReq
      if(uCorrection == (uint32_t)0x7FFFFFFF || uCorrection == (uint32_t)0x80000000) {
        this->nextAppReqTime = this->getSeconds();

      // otherwise, schedule next AppTimeReq based on periodicity (if set)
      } else if(this->periodicity) {
        this->nextAppReqTime = this->getSeconds() + this->periodicity - 30 + (rand() % 60);
      } else {
        this->nextAppReqTime = 0;
      }
      return(6);

    } break;
    case(RADIOLIB_LORAWAN_TS003_APP_TIME_PERIODICITY): {
      uint32_t period = dataDown[1];

      dataOut[0] = RADIOLIB_LORAWAN_TS003_APP_TIME_PERIODICITY;
      dataOut[1] = 0;
      RadioLibTime_t now = this->getSeconds();
      dataOut[2] = (uint8_t)((now >> 0)  & 0xFF);
      dataOut[3] = (uint8_t)((now >> 8)  & 0xFF);
      dataOut[4] = (uint8_t)((now >> 16) & 0xFF);
      dataOut[5] = (uint8_t)((now >> 24) & 0xFF);
      *lenOut = 6;

      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Period: %lu, Time: %lu", period, now);
      this->periodicity = 128 << period;
      this->nextAppReqTime = this->getSeconds() + this->periodicity - 30 + (rand() % 60);
      return(2);

    } break;
    case(RADIOLIB_LORAWAN_TS003_FORCE_DEVICE_RESYNC): {
      uint8_t trans = dataDown[1] & 0x0F;

      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Forced transmissions: %d", trans);
      if(trans > 0) {
        this->nextAppReqTime = this->getSeconds();
        trans--;
      }
      this->transmissions = trans;
      return(2);

    } break;
    default: {
      // unknown command: length is not derivable, consume nothing
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TS003 unknown command 0x%02x, stopping", dataDown[0]);
      return(0);
    }
  }

  return(0);
}

int16_t LoRaWANPackageTS003::requestAppTime(bool force) {
  // schedule an AppTimeReq; handleTask() reports it due and buildUplink() builds it
  this->forceReq = force;
  this->nextAppReqTime = this->getSeconds();
  return(RADIOLIB_ERR_NONE);
}

size_t LoRaWANPackageTS003::buildAppTimeReq(uint8_t* dataOut) {
  dataOut[0] = RADIOLIB_LORAWAN_TS003_APP_TIME;

  RadioLibTime_t now = this->getSeconds();
  dataOut[1] = (uint8_t)((now >> 0)  & 0xFF);
  dataOut[2] = (uint8_t)((now >> 8)  & 0xFF);
  dataOut[3] = (uint8_t)((now >> 16) & 0xFF);
  dataOut[4] = (uint8_t)((now >> 24) & 0xFF);
  dataOut[5] = (uint8_t)this->forceReq << 4 | (this->tokenReq & 0x0F);
  this->forceReq = false;
  return(6);
}

#endif
