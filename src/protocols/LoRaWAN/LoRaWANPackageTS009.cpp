#if !RADIOLIB_EXCLUDE_LORAWAN

#include "LoRaWAN.h"
#include "LoRaWANPacMan.h"
#include "LoRaWANPackageTS009.h"
#include <string.h>

LoRaWANPackageTS009::LoRaWANPackageTS009(LoRaWANPackageManager* pacMan, RadioLibHal* hal, LoRaWANNode* node, GetSecondsCb_t secondsCb)
  : LoRaWANPackage(RADIOLIB_LORAWAN_PACKAGE_TS009, pacMan, hal, node, secondsCb),
    radio(NULL), delaySecondsCallback(NULL), uplinkIntervalCallback(NULL), confirmedCallback(NULL), rebootCallback(NULL) {
  this->packageVersion = 1;

  // retrieve whether the package is allowed to be enabled
  this->lorawanNode->getPersistencePackage(RADIOLIB_LORAWAN_PERSISTENCE_TS009, &this->enabled);
  if(!this->enabled) {
    return;
  }

  // default configuration for certification testing
  this->lorawanNode->setDatarate(5);
  this->lorawanNode->setADR(false);
  this->lorawanNode->setDutyCycle(true, 3600000);
}

void LoRaWANPackageTS009::setPhysicalLayer(PhysicalLayer* radio) {
  this->radio = radio;
}

void LoRaWANPackageTS009::setDelaySecondsCallback(DelaySecondsCb_t delayCb) {
  this->delaySecondsCallback = delayCb;
}

void LoRaWANPackageTS009::setUplinkIntervalCallback(UplinkIntervalCb_t intervalCb) {
  this->uplinkIntervalCallback = intervalCb;
}

void LoRaWANPackageTS009::setConfirmedCallback(ConfirmedCb_t confirmedCb) {
  this->confirmedCallback = confirmedCb;
}

void LoRaWANPackageTS009::setRebootCallback(RebootCb_t rebootCb) {
  this->rebootCallback = rebootCb;
}

size_t LoRaWANPackageTS009::processData(const uint8_t* dataDown, size_t lenDown, uint8_t* dataOut, size_t* lenOut, LoRaWANEvent_t* event) {
  (void)event;
  *lenOut = 0;
  if(!this->enabled || lenDown == 0 || dataDown == NULL) {
    return(0);
  }

  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("CID = 0x%02x, len = %d", dataDown[0], lenDown - 1);

  size_t len = 0;

  switch(dataDown[0]) {
    case(RADIOLIB_LORAWAN_TS009_PACKAGE_VERSION): {
      len = 3;
      dataOut[1] = RADIOLIB_LORAWAN_PACKAGE_TS009;
      dataOut[2] = 1;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("PackageIdentifier: %d, PackageVersion: %d", 
                                      dataOut[1], dataOut[2]);
    } break;

    case(RADIOLIB_LORAWAN_TS009_DUT_RESET): {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Device reset requested");
      this->reboot();
    } break;

    case(RADIOLIB_LORAWAN_TS009_DUT_JOIN): {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Reverting to Join state");
      if(this->lorawanNode) {
        this->lorawanNode->clearSession();
        this->lorawanNode->setDatarate(5);
        this->lorawanNode->setADR(false);
        this->lorawanNode->setDutyCycle(true, 3600000);
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_SWITCH_CLASS): {
      if(lenDown > 1) {
        uint8_t classType = dataDown[1];
        if(this->lorawanNode) {
          this->lorawanNode->setClass(classType);
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Switching to Class %c", "ABC"[classType]);
        }
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_ADR_BIT_CHANGE): {
      if(lenDown > 1) {
        if(this->lorawanNode) {
          this->lorawanNode->setADR((bool)dataDown[1]);
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ADR: %d", dataDown[1]);
        }
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_REGIONAL_DUTY_CYCLE): {
      if(lenDown > 1) {
        if(this->lorawanNode) {
          if(dataDown[1]) {
            this->lorawanNode->setDutyCycle(true, 36000);
          } else {
            this->lorawanNode->setDutyCycle(true, 3600000);
          }
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Duty cycle: %d", dataDown[1]);
        }
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_TX_PERIODICITY_CHANGE): {
      if(lenDown > 1) {
        uint32_t defaultIntervalSecs = 30;
        uint32_t intervals[11] = {defaultIntervalSecs, 5, 10, 20, 30, 40, 50, 60, 120, 240, 480};
        uint32_t newPeriod = (dataDown[1] < 11) ? intervals[dataDown[1]] : defaultIntervalSecs;
        if(this->uplinkIntervalCallback) {
          this->uplinkIntervalCallback(newPeriod);
        }
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TX Periodicity: %d seconds", newPeriod);
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_TX_FRAMES_CTRL): {
      if(lenDown > 1) {
        switch(dataDown[1]) {
          case 1:
            if(this->confirmedCallback) {
              this->confirmedCallback(false);
            }
            RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TX Frames: Unconfirmed");
            break;
          case 2:
            if(this->confirmedCallback) {
              this->confirmedCallback(true);
            }
            RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TX Frames: Confirmed");
            break;
          default:
            RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TX Frames: No change");
        }
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_ECHO_PAYLOAD): {
      if(lenDown > 1) {
        for(size_t i = 1; i < lenDown; i++) {
          dataOut[i] = dataDown[i] + 1;
        }
        // clip to maximum payload size
        len = RADIOLIB_MIN(lenDown, this->lorawanNode->getMaxPayloadLen());
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Echoing %d bytes with increment", len - 1);
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_RX_APP_CNT): {
      if(this->lorawanNode) {
        len = 3;
        uint16_t aFcntDown16 = (uint16_t)this->lorawanNode->getRxFCnt();
        for(uint8_t i = 0; i < RADIOLIB_LORAWAN_MAX_NUM_MC_GROUPS; i++) {
          aFcntDown16 += (uint16_t)this->lorawanNode->getRxFCntMulticast(i);
        }
        dataOut[1] = aFcntDown16 & 0xFF;
        dataOut[2] = aFcntDown16 >> 8;
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("aFCntDown16: %d", aFcntDown16);
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_RX_APP_CNT_RESET): {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Resetting Application Frame counter (not implemented)");
    } break;

    case(RADIOLIB_LORAWAN_TS009_LINK_CHECK): {
      if(this->lorawanNode) {
        this->lorawanNode->sendMacCommandReq(RADIOLIB_LORAWAN_MAC_LINK_CHECK);
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("LinkCheck MAC command requested");
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_DEVICE_TIME): {
      if(this->lorawanNode) {
        this->lorawanNode->sendMacCommandReq(RADIOLIB_LORAWAN_MAC_DEVICE_TIME);
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("DeviceTime MAC command requested");
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_FRAG_SESSION_CNT): {
      if(this->lorawanNode) {
        uint8_t nonceBuf[8];
        this->lorawanNode->getPersistencePackage(RADIOLIB_LORAWAN_PERSISTENCE_TS004, nonceBuf);
        uint8_t fragIndex = dataDown[1] & 0x03;
        len = 4;
        dataOut[1] = fragIndex;
        dataOut[2] = nonceBuf[fragIndex * 2];
        dataOut[3] = nonceBuf[fragIndex * 2 + 1];
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("FragSession index: %d, Cnt: %d", fragIndex, 
                                        (nonceBuf[fragIndex * 2 + 1] << 8) | nonceBuf[fragIndex * 2]);
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_TX_CW): {
      if(lenDown > 6 && this->radio) {
        uint16_t timeout = ((uint16_t)dataDown[2] << 8) | (uint16_t)dataDown[1];
        uint32_t freqRaw = ((uint32_t)dataDown[5] << 16) | ((uint32_t)dataDown[4] << 8) | 
                           ((uint32_t)dataDown[3]);
        float freq = (float)freqRaw / 10000.0f;
        uint8_t txPower = dataDown[6];

        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TX CW: %7.3f MHz, %d dBm, %d s", freq, txPower, timeout);

        this->radio->setFrequency(freq);
        this->radio->setOutputPower(txPower);
        this->radio->transmitDirect();
        this->delaySecondsCallback(timeout);
        this->radio->standby();
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_DUT_FPORT224_DISABLE): {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Disabling package");
      this->enabled = false;
      const uint8_t* flag = &this->enabled;
      this->lorawanNode->setPersistencePackage(RADIOLIB_LORAWAN_PERSISTENCE_TS009, flag);
      
      // give flash saving some time before rebooting
      this->hal->delay(100);
      this->reboot();

    } break;

    case(RADIOLIB_LORAWAN_TS009_DUT_VERSIONS): {
      len = 13;
      dataOut[1] = RADIOLIB_VERSION_MAJOR;
      dataOut[2] = RADIOLIB_VERSION_MINOR;
      dataOut[3] = RADIOLIB_VERSION_PATCH;
      dataOut[4] = RADIOLIB_VERSION_EXTRA;

      dataOut[5] = 1;
#if (LORAWAN_VERSION == 1)
      dataOut[6] = 1;
      dataOut[7] = 0;
#else
      dataOut[6] = 0;
      dataOut[7] = 4;
#endif
      dataOut[8] = 0;

      dataOut[9] = 1;
      dataOut[10] = 0;
      dataOut[11] = 4;
      dataOut[12] = 0;

      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Device versions requested");
    } break;

    default: {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TS009 unknown command 0x%02x", dataDown[0]);
    } break;
  }

  // if data was set, copy the command ID
  if(len > 0) {
    dataOut[0] = dataDown[0];
  }

  *lenOut = len;
  return(lenDown);
}

#endif
