#if !RADIOLIB_EXCLUDE_LORAWAN

#include "LoRaWANPackageTS009.h"
#include "LoRaWAN.h"

LoRaWANPackageTS009::LoRaWANPackageTS009(LoRaWANNode* node, GetSecondsCb_t secondsCb)
  : LoRaWANPackage(RADIOLIB_LORAWAN_PACKAGE_TS009, node, secondsCb),
    radioModule(NULL) {
  this->packageVersion = 1;

  // default configuration for certification testing
  this->lorawanNode->setDatarate(5);
  this->lorawanNode->setADR(false);
  this->lorawanNode->setDutyCycle(true, 3600000);

  // LCTT has terrible timing
  this->lorawanNode->scanGuard = 150;
}

void LoRaWANPackageTS009::setPhysicalLayer(PhysicalLayer* radio) {
  this->radioModule = radio;
}

void LoRaWANPackageTS009::setRebootCallback(RebootCb_t rebootCb) {
  this->rebootCallback = rebootCb;
}

size_t LoRaWANPackageTS009::processData(const uint8_t* dataDown, size_t lenDown) {
  if(lenDown == 0 || dataDown == NULL) {
    return 0;
  }

  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TS009 CID = 0x%02x, len = %d", dataDown[0], lenDown - 1);

  this->lenUp = 0;

  switch(dataDown[0]) {
    case(RADIOLIB_LORAWAN_TS009_PACKAGE_VERSION): {
      this->lenUp = 3;
      this->dataUp[1] = RADIOLIB_LORAWAN_PACKAGE_TS009;
      this->dataUp[2] = 1;
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("PackageIdentifier: %d, PackageVersion: %d", 
                              this->dataUp[1], this->dataUp[2]);
    } break;

    case(RADIOLIB_LORAWAN_TS009_DUT_RESET): {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Device reset requested");
      if(this->rebootCallback) {
        this->rebootCallback();
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_DUT_JOIN): {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Reverting to Join state");
      if(this->lorawanNode) {
        this->lorawanNode->clearSession();
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_SWITCH_CLASS): {
      if(lenDown > 1) {
        uint8_t classType = dataDown[1];
        if(this->lorawanNode) {
          this->lorawanNode->setClass(classType);
          const char* className = (classType == 0) ? "A" : (classType == 1 ? "B" : "C");
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Switching to class: %s", className);
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
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TX Periodicity: %d seconds", newPeriod);
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_TX_FRAMES_CTRL): {
      if(lenDown > 1) {
        switch(dataDown[1]) {
          case 1:
            this->confirmed = false;
            RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TX Frames: Unconfirmed");
            break;
          case 2:
            this->confirmed = true;
            RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TX Frames: Confirmed");
            break;
          default:
            this->confirmed = this->confirmed;
            RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TX Frames: No change");
        }
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_ECHO_PAYLOAD): {
      if(lenDown > 1) {
        this->lenUp = lenDown;
        for(size_t i = 1; i < lenDown; i++) {
          this->dataUp[i] = dataDown[i] + 1;
        }
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Echoing %d bytes with increment", lenDown - 1);
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_RX_APP_CNT): {
      if(this->lorawanNode) {
        this->lenUp = 3;
        uint16_t aFcntDown16 = (uint16_t)this->lorawanNode->getAFCntDown();
        this->dataUp[1] = aFcntDown16 & 0xFF;
        this->dataUp[2] = aFcntDown16 >> 8;
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

    case(RADIOLIB_LORAWAN_TS009_PING_SLOT_INFO): {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("PingSlotInfo not implemented");
    } break;

    case(RADIOLIB_LORAWAN_TS009_TX_CW): {
      if(lenDown > 6 && this->radioModule) {
        uint16_t timeout = ((uint16_t)dataDown[2] << 8) | (uint16_t)dataDown[1];
        uint32_t freqRaw = ((uint32_t)dataDown[5] << 16) | ((uint32_t)dataDown[4] << 8) | 
                           ((uint32_t)dataDown[3]);
        float freq = (float)freqRaw / 10000.0;
        uint8_t txPower = dataDown[6];

        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TX CW: %7.3f MHz, %d dBm, %d s", freq, txPower, timeout);

        this->radioModule->setFrequency(freq);
        this->radioModule->setOutputPower(txPower);
        this->radioModule->transmitDirect();
        delay(timeout * 1000);
        this->radioModule->standby();
      }
    } break;

    case(RADIOLIB_LORAWAN_TS009_DUT_FPORT224_DISABLE): {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Package disable requested");
    } break;

    case(RADIOLIB_LORAWAN_TS009_DUT_VERSIONS): {
      this->lenUp = 13;
      this->dataUp[1] = RADIOLIB_VERSION_MAJOR;
      this->dataUp[2] = RADIOLIB_VERSION_MINOR;
      this->dataUp[3] = RADIOLIB_VERSION_PATCH;
      this->dataUp[4] = RADIOLIB_VERSION_EXTRA;

      this->dataUp[5] = 1;
#if (LORAWAN_VERSION == 1)
      this->dataUp[6] = 1;
      this->dataUp[7] = 0;
#else
      this->dataUp[6] = 0;
      this->dataUp[7] = 4;
#endif
      this->dataUp[8] = 0;

      this->dataUp[9] = 1;
      this->dataUp[10] = 0;
      this->dataUp[11] = 4;
      this->dataUp[12] = 0;

      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Device versions requested");
    } break;

    default: {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TS009 unknown command 0x%02x", dataDown[0]);
    } break;
  }

  // if data was set, copy the command ID
  if(this->lenUp > 0) {
    this->dataUp[0] = dataDown[0];
  }

  return lenDown;
}

#endif

