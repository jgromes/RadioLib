#if !RADIOLIB_EXCLUDE_LORAWAN

#include "LoRaWANPacMan.h"
#include "LoRaWANPackageTS003.h"
#include "LoRaWANPackageTS009.h"
#include <string.h>

// LoRaWANPackage implementation
LoRaWANPackage::LoRaWANPackage(uint8_t ts, LoRaWANPackageManager* pacMan, RadioLibHal* hal, LoRaWANNode* node, GetSecondsCb_t secondsCb)
  : packageIdentifier(ts), pacMan(pacMan), hal(hal), lorawanNode(node), getSeconds_cb(secondsCb) {
  this->packageVersion = 0;
}

size_t LoRaWANPackage::processData(const uint8_t* dataIn, size_t lenIn, uint8_t* dataOut, size_t* lenOut, LoRaWANEvent_t* event) {
  // Default implementation: consume nothing and produce no answer.
  // Derived classes override and return actual bytes consumed.
  (void)dataIn;
  (void)lenIn;
  (void)dataOut;
  (void)event;
  *lenOut = 0;
  return(0);
}

bool LoRaWANPackage::handleTask(RadioLibTime_t* tNext, bool* uplinkDue) {
  // base package has no scheduled work
  (void)tNext;
  *uplinkDue = false;
  return(false);
}

size_t LoRaWANPackage::buildUplink(uint8_t* dataOut) {
  // base package has no uplink
  (void)dataOut;
  return(0);
}

// #if defined(RADIOLIB_BUILD_ARDUINO)
// // LoRaWANPackageManager implementation
// LoRaWANPackageManager::LoRaWANPackageManager(LoRaWANNode* node, GetSecondsCb_t secondsCb) {
//   RadioLibHal* hal = new ArduinoHal();

//   return(LoRaWANPackageManager(hal, node, secondsCb));
// }
// #endif

// LoRaWANPackageManager implementation
LoRaWANPackageManager::LoRaWANPackageManager(RadioLibHal* hal, LoRaWANNode* node, GetSecondsCb_t secondsCb) {
  // initialize all packages to NULL and disabled
  for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_PACKAGES; i++) {
    this->packages[i] = NULL;
    this->packagePorts[i] = 0;
    this->enabledPackages[i] = false;
  }

  // store node and callback reference
  this->hal = hal;
  this->lorawanNode = node;
  this->getSecondsCb = secondsCb;

  // if the user does not provide their own AES-128, use the software one
  #if !RADIOLIB_CUSTOM_AES128
  static RadioLibSoftwareAES128 RadioLibAES128Instance;
  this->hal->aes128 = &RadioLibAES128Instance;
  #endif

  // Initialize the staging buffer, persistent TS007 buffer and transmission state
  this->ansBufferLen = 0;
  this->ansFPort = 0;
  this->ts007BufferLen = 0;
  this->commandToken = 0;
  this->managerPending = false;
  this->pendingBuildPackage = -1;
  this->txCursor = 0;
  this->txStop = 0;
  this->txForceFrag = false;
  this->txError = false;
}

int16_t LoRaWANPackageManager::enableTS003(uint8_t fPort, SetSecondsCb_t setSecondsFunc) {
  // check if node is not activated
  if(this->lorawanNode == NULL || this->lorawanNode->isActivated()) {
    return(RADIOLIB_ERR_NETWORK_NOT_JOINED);
  }

  // verify callback is provided
  if(this->getSecondsCb == NULL || setSecondsFunc == NULL) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }

  // create package if not already created
  if(this->packages[RADIOLIB_LORAWAN_PACKAGE_TS003] == NULL) {
    this->packages[RADIOLIB_LORAWAN_PACKAGE_TS003] = new LoRaWANPackageTS003(this, this->lorawanNode, this->getSecondsCb);
    this->packagePorts[RADIOLIB_LORAWAN_PACKAGE_TS003] = fPort;
  }

  // set time handler callback on TS003 package
  LoRaWANPackageTS003* ts003 = static_cast<LoRaWANPackageTS003*>(this->packages[RADIOLIB_LORAWAN_PACKAGE_TS003]);
  ts003->setSecondsCb(setSecondsFunc);

  // enable the package
  this->enabledPackages[RADIOLIB_LORAWAN_PACKAGE_TS003] = true;

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANPackageManager::enableTS007() {
  // Check if node is not activated
  if(this->lorawanNode == NULL || this->lorawanNode->isActivated()) {
    return RADIOLIB_ERR_NETWORK_NOT_JOINED;
  }

  // TS007 is a manager-level package, not directly instantiated
  // Just enable it in the manager
  this->packagePorts[RADIOLIB_LORAWAN_PACKAGE_TS007] = RADIOLIB_LORAWAN_FPORT_TS007;
  this->enabledPackages[RADIOLIB_LORAWAN_PACKAGE_TS007] = true;

  // register FPort 225 so that multi-package downlinks are not blocked
  this->lorawanNode->addAppPackage(RADIOLIB_LORAWAN_FPORT_TS007);

  return RADIOLIB_ERR_NONE;
}

int16_t LoRaWANPackageManager::enableTS009(PhysicalLayer* radio, DelaySecondsCb_t delayCb, UplinkIntervalCb_t intervalCb, ConfirmedCb_t confirmedCb, RebootCb_t rebootCb) {
  // check if node is not activated
  if(this->lorawanNode == NULL || this->lorawanNode->isActivated()) {
    return(RADIOLIB_ERR_NETWORK_NOT_JOINED);
  }
  if(this->getSecondsCb == NULL) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }

  // create package if not already created
  if(this->packages[RADIOLIB_LORAWAN_PACKAGE_TS009] == NULL) {
    this->packages[RADIOLIB_LORAWAN_PACKAGE_TS009] = new LoRaWANPackageTS009(this, this->hal, this->lorawanNode, this->getSecondsCb);
    this->packagePorts[RADIOLIB_LORAWAN_PACKAGE_TS009] = RADIOLIB_LORAWAN_FPORT_TS009;
  }

  // set parameters on TS009 package
  LoRaWANPackageTS009* ts009 = static_cast<LoRaWANPackageTS009*>(this->packages[RADIOLIB_LORAWAN_PACKAGE_TS009]);
  ts009->setPhysicalLayer(radio);
  ts009->setDelaySecondsCallback(delayCb);
  ts009->setUplinkIntervalCallback(intervalCb);
  ts009->setConfirmedCallback(confirmedCb);
  ts009->setRebootCallback(rebootCb);

  // enable the package
  this->enabledPackages[RADIOLIB_LORAWAN_PACKAGE_TS009] = true;
  ts009->enabled = true;

  // register the package's FPort so that downlinks on that FPort are not blocked
  this->lorawanNode->addAppPackage(RADIOLIB_LORAWAN_FPORT_TS009);

  return(RADIOLIB_ERR_NONE);
}

void LoRaWANPackageManager::requestAppTime() {
  // if TS003 is enabled, trigger an immediate uplink with the AppTimeReq CID
  LoRaWANPackageTS003* ts003 = static_cast<LoRaWANPackageTS003*>(this->packages[RADIOLIB_LORAWAN_PACKAGE_TS003]);
  if(ts003 == NULL) {
    return;
  }
  ts003->requestAppTime();
}

int16_t LoRaWANPackageManager::processDownlink(const uint8_t* data, size_t len, LoRaWANEvent_t* eventDown) {
  if(data == NULL) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }

  // If the downlink arrived on a package-specific FPort (not the package
  // manager's TS007 FPort), dispatch its commands to the package on that FPort.
  if(eventDown->fPort != RADIOLIB_LORAWAN_FPORT_TS007) {
    // Find package registered on this FPort
    for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_PACKAGES; i++) {
      if(!this->enabledPackages[i]) {
        continue;
      }
      if(this->packagePorts[i] == eventDown->fPort) {
        LoRaWANPackage* pkg = this->packages[i];
        if(pkg == NULL) {
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("No package registered for FPort %d, skipping", eventDown->fPort);
          return(RADIOLIB_ERR_NONE);
        }
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Routing %d bytes on FPort %d to package %d", len, eventDown->fPort, i);

        // Drive the package command by command, collecting answers into the single
        // manager answer buffer. The package processes one command per call and
        // returns how many bytes it consumed.
        size_t pos = 0;
        size_t outLen = 0;
        while(pos < len && outLen < sizeof(this->ansBuffer)) {
          size_t aLen = 0;
          size_t consumed = pkg->processData(&data[pos], len - pos, &this->ansBuffer[outLen], &aLen, eventDown);
          if(consumed == 0) {
            break;
          }
          pos += consumed;
          outLen += aLen;
        }

        if(outLen > 0) {
          this->ansBufferLen = outLen;
          this->ansFPort = eventDown->fPort;
          this->txError = false;
          this->txForceFrag = false;
          this->txCursor = 0;
          this->txStop = outLen - 1;
          this->pendingBuildPackage = -1;
          this->managerPending = true;
        }
        return(RADIOLIB_ERR_NONE);
      }
    }

    // No package matched this FPort; nothing to do.
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("No package matched FPort %d, ignoring payload", eventDown->fPort);
    return(RADIOLIB_ERR_NONE);
  }

  // This is a multi-package access downlink on FPort 225 (TS007 section 3.1).
  // These messages SHALL NOT be sent via multicast; drop them silently if they are.
  if(eventDown->multicast) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Multi-package downlink received on multicast, dropping");
    return(RADIOLIB_ERR_NONE);
  }

  if(!this->enabledPackages[RADIOLIB_LORAWAN_PACKAGE_TS007]) {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Received FPort 225 downlink but TS007 is disabled");
    return(RADIOLIB_ERR_NONE);
  }

  if(len == 0) {
    return(RADIOLIB_ERR_NONE);
  }

  // A downlink that only contains a MultiPackBufferReq carries no Command Token and
  // must be the single command (TS007 section 4.4). Package 0 needs no PackageID
  // prefix, so such a downlink starts directly with CID 0x02.
  if(data[0] == RADIOLIB_LORAWAN_TS007_CID_MULTI_PACK_BUFFER) {
    if(len != 3) {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Received MultiPackBufferReq but other payload is present");
      return(RADIOLIB_ERR_NONE);
    }
    uint8_t startByte = data[1];
    uint8_t stopByte  = data[2];
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("MultiPackBufferReq start=%d stop=%d", startByte, stopByte);
    this->handleMultiPackBufferReq(startByte, stopByte);
    return(RADIOLIB_ERR_NONE);
  }

  // Normal command set: the last byte is the Command Token; the rest is the body.
  uint8_t token = data[len - 1];
  size_t bodyLen = len - 1;

  // Build the ANS buffer (answers only, no token) into the persistent TS007 buffer.
  // Dispatch one command at a time, switching packages on a PackageID field. Whenever
  // a request command is prefixed with a PackageID, that PackageID is also copied into
  // the answer (once per run of consecutive same-package commands).
  size_t outPos = 0;
  size_t pos = 0;
  uint8_t currentPkg = 0;       // first command belongs to package 0 unless prefixed
  int16_t pkgPrefixByte = -1;   // PackageID byte to copy into the answer, if any

  while(pos < bodyLen && outPos < sizeof(this->ts007Buffer)) {
    // A PackageID field (bit 7 set) selects the package for the following run
    if(data[pos] & RADIOLIB_LORAWAN_TS007_PACKAGE_ID_FLAG) {
      pkgPrefixByte = data[pos];
      currentPkg = data[pos] & ~RADIOLIB_LORAWAN_TS007_PACKAGE_ID_FLAG;
      pos += 1;
      if(pos >= bodyLen) {
        break;
      }
    }

    // Tentatively write the PackageID prefix; roll back if the command has no answer
    size_t runStart = outPos;
    bool wrotePrefix = false;
    if(pkgPrefixByte >= 0) {
      this->ts007Buffer[outPos++] = (uint8_t)pkgPrefixByte;
      wrotePrefix = true;
    }

    // Dispatch a single command to the current package
    size_t aLen = 0;
    size_t consumed = 0;
    if(currentPkg == RADIOLIB_LORAWAN_PACKAGE_TS007) {
      consumed = this->processPackageManagerData(&data[pos], bodyLen - pos, &this->ts007Buffer[outPos], &aLen);

    } else if(currentPkg < RADIOLIB_LORAWAN_NUM_PACKAGES &&
              this->enabledPackages[currentPkg] && this->packages[currentPkg] != NULL) {
      consumed = this->packages[currentPkg]->processData(&data[pos], bodyLen - pos, &this->ts007Buffer[outPos], &aLen, eventDown);

    } else {
      // Unknown or disabled package: we cannot know its command length, so stop.
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TS007 unknown/disabled package %d, stopping", currentPkg);
      outPos = runStart;
      break;
    }

    if(consumed == 0) {
      // Nothing consumed: avoid an infinite loop
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TS007 package %d consumed 0 bytes, stopping", currentPkg);
      outPos = runStart;
      break;
    }
    pos += consumed;

    if(aLen > 0) {
      outPos += aLen;
      pkgPrefixByte = -1;     // PackageID already emitted for this run
    } else if(wrotePrefix) {
      outPos = runStart;      // no answer: drop the dangling prefix, keep it for next cmd
    }
  }

  // The TS007 buffer is sized to the maximum (128 bytes); store the answers and token
  this->ts007BufferLen = outPos;
  this->ansFPort = RADIOLIB_LORAWAN_FPORT_TS007;
  this->commandToken = token;

  // Schedule a proactive uplink of the full buffer; getUplinkData() decides at send
  // time whether it fits in one frame or must be fragmented.
  this->txError = false;
  this->txForceFrag = false;
  this->txCursor = 0;
  this->txStop = (this->ts007BufferLen > 0) ? (this->ts007BufferLen - 1) : 0;
  this->pendingBuildPackage = -1;
  this->managerPending = true;

  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TS007 built ANS buffer of %d bytes, token %d",
                                  this->ts007BufferLen, this->commandToken);
  return(RADIOLIB_ERR_NONE);
}

void LoRaWANPackageManager::handleMultiPackBufferReq(uint8_t startByte, uint8_t stopByte) {
  // Invalid request: no stored buffer, StartByte out of range, or StopByte < StartByte.
  // Respond with a MultiPackBufferFrag carrying Error=0xFF (TS007 section 4.4).
  this->ansFPort = RADIOLIB_LORAWAN_FPORT_TS007;
  this->pendingBuildPackage = -1;
  if(this->ts007BufferLen == 0 || startByte > this->ts007BufferLen - 1 || stopByte < startByte) {
    this->txError = true;
    this->managerPending = true;
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TS007 MultiPackBufferReq invalid, queueing error");
    return;
  }

  // Valid request: retransmit ts007Buffer[startByte .. min(stopByte, len-1)] as fragments.
  this->txError = false;
  this->txForceFrag = true;
  this->txCursor = startByte;
  this->txStop = (stopByte > this->ts007BufferLen - 1) ? (this->ts007BufferLen - 1) : stopByte - 1; // TODO LCTT bug!
  this->managerPending = true;
}

bool LoRaWANPackageManager::handleTask(RadioLibTime_t* tNext) {
  // a staged uplink (TS007 response/fragment, dedicated-FPort answer, or scheduled
  // package uplink) is due now
  RadioLibTime_t tNow = this->getSecondsCb();

  if(this->managerPending) {
    *tNext = tNow + this->lorawanNode->timeUntilUplink(true);
    return(true);
  }

  bool found = false;
  RadioLibTime_t earliest = 0xFFFFFFFF;

  // drive each package's scheduled task, letting it write any due uplink into the
  // single manager answer buffer
  for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_PACKAGES; i++) {
    if(!this->enabledPackages[i] || this->packages[i] == NULL) {
      continue;
    }

    RadioLibTime_t t = 0;
    bool uplinkDue = false;
    if(!this->packages[i]->handleTask(&t, &uplinkDue)) {
      continue;
    }

    // an uplink is to be sent? apply dutycycle constraints
    if(uplinkDue) {
      t += this->lorawanNode->timeUntilUplink(true);
    }

    // an uplink is to be sent now? stage it and return. The payload bytes are not
    // built here; they are produced by the package's buildUplink() at send time
    // (getUplinkData) so time-sensitive data stays fresh despite any dutycycle delay.
    if(t <= tNow && uplinkDue) {
      this->ansFPort = this->packagePorts[i];
      this->txError = false;
      this->txForceFrag = false;
      this->txCursor = 0;
      this->pendingBuildPackage = i;
      this->managerPending = true;
      *tNext = tNow;
      return(true);
    }

    // otherwise track the earliest task across packages
    found = true;
    if(t < earliest) {
      earliest = t;
    }
  }

  if(found) {
    *tNext = earliest;
  } else {
    *tNext = UINT32_MAX;
  }
  return(found);
}

bool LoRaWANPackageManager::getUplinkData(uint8_t* dataOut, size_t* lenOut, uint8_t* fPort) {
  if(dataOut == NULL || lenOut == NULL || fPort == NULL || this->getSecondsCb == NULL) {
    return(false);
  }

  if(!this->managerPending) {
    *lenOut = 0;
    return(false);
  }

  *fPort = this->ansFPort;

  // A plain (non multi-package) uplink is sent as-is on its package's FPort
  if(this->ansFPort != RADIOLIB_LORAWAN_FPORT_TS007) {
    // A scheduled package uplink is built now, at the moment of transmission, so
    // time-sensitive data (e.g. the TS003 timestamp) is captured fresh.
    if(this->pendingBuildPackage >= 0) {
      *lenOut = this->packages[this->pendingBuildPackage]->buildUplink(dataOut);
      this->pendingBuildPackage = -1;
      this->managerPending = false;
      return(*lenOut > 0);
    }

    // Otherwise it is a prebuilt dedicated-FPort answer staged by processData
    memcpy(dataOut, this->ansBuffer, this->ansBufferLen);
    *lenOut = this->ansBufferLen;
    this->managerPending = false;
    return(true);
  }

  // TS007 multi-package uplink on FPort 225 (whole buffer, fragment, or error)

  // Invalid MultiPackBufferReq: respond with MultiPackBufferFrag carrying Error=0xFF
  if(this->txError) {
    dataOut[0] = RADIOLIB_LORAWAN_TS007_CID_MULTI_PACK_BUFFER;
    dataOut[1] = RADIOLIB_LORAWAN_TS007_ERROR;
    *lenOut = 2;
    this->txError = false;
    this->managerPending = false;
    return(true);
  }

  // Maximum applicative payload for the current data rate
  size_t maxLen = this->lorawanNode->getMaxPayloadLen();

  // Whole buffer: if it fits with the appended token, transmit it unmodified
  if(!this->txForceFrag && this->txCursor == 0 && (this->ts007BufferLen + 1) <= maxLen) {
    memcpy(dataOut, this->ts007Buffer, this->ts007BufferLen);
    dataOut[this->ts007BufferLen] = this->commandToken;
    *lenOut = this->ts007BufferLen + 1;
    this->managerPending = false;
    return(true);
  }

  // Otherwise fragment using MultiPackBufferFrag: [0x02][BaseByte][bytes][token]
  // Each fragment carries up to (maxLen - 3) ANS bytes (CID + BaseByte + token).
  size_t avail = (maxLen > 3) ? (maxLen - 3) : 1;
  size_t remaining = (this->txStop >= this->txCursor) ? (this->txStop - this->txCursor + 1) : 0;
  size_t chunk = (remaining < avail) ? remaining : avail;

  dataOut[0] = RADIOLIB_LORAWAN_TS007_CID_MULTI_PACK_BUFFER;
  dataOut[1] = (uint8_t)this->txCursor;
  memcpy(&dataOut[2], &this->ts007Buffer[this->txCursor], chunk);
  dataOut[2 + chunk] = this->commandToken;
  *lenOut = chunk + 3;

  this->txCursor += chunk;
  if(this->txCursor > this->txStop) {
    // all requested bytes sent; keep the ANS buffer for further retransmissions
    this->managerPending = false;
    this->txForceFrag = false;
  }
  return(true);
}

size_t LoRaWANPackageManager::processPackageManagerData(const uint8_t* dataDown, size_t lenDown, uint8_t* ansOut, size_t* ansLen) {
  // Process a single TS007 Multi-Package Access package (PackageIdentifier 0) command:
  //   0x00 PackageVersionReq -> PackageVersionAns
  //   0x01 DevPackageReq     -> DevPackageAns
  // 0x02 (MultiPackBufferReq) is handled separately as it must be the single command
  // in a downlink, so it is treated as an unknown command here. The manager drives the
  // command loop, so this processes only one command and returns the bytes consumed.

  if(dataDown == NULL || ansOut == NULL || ansLen == NULL) {
    return(0);
  }

  *ansLen = 0;
  if(lenDown < 1) {
    return(0);
  }

  size_t ansPos = 0;

  switch(dataDown[0]) {
    case(RADIOLIB_LORAWAN_TS007_CID_PKG_VERSION): {
      // PackageVersionReq has no payload
      ansOut[ansPos++] = RADIOLIB_LORAWAN_TS007_CID_PKG_VERSION;
      ansOut[ansPos++] = RADIOLIB_LORAWAN_PACKAGE_TS007;              // PackageIdentifier (0)
      ansOut[ansPos++] = RADIOLIB_LORAWAN_TS007_PACKAGE_VERSION_NUM;  // PackageVersion (1)
      *ansLen = ansPos;

      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TS007 PackageVersionAns: id=%d ver=%d",
                                      RADIOLIB_LORAWAN_PACKAGE_TS007, RADIOLIB_LORAWAN_TS007_PACKAGE_VERSION_NUM);
      return(1);

    } break;

    case(RADIOLIB_LORAWAN_TS007_CID_DEV_PACKAGE): {
      // DevPackageReq has no payload

      // DevPackageAns: [NbPackages][id, ver, port]* (TS007 section 4.2).
      // NbPackages (low nibble) counts every package, including multi-package (TS007).
      ansOut[ansPos++] = RADIOLIB_LORAWAN_TS007_CID_DEV_PACKAGE;

      size_t nbPos = ansPos++;   // reserve the Nb Packages byte, fill in after counting
      uint8_t total = 0;

      // TS007 (multi-package access) itself is always present
      ansOut[ansPos++] = RADIOLIB_LORAWAN_PACKAGE_TS007;
      ansOut[ansPos++] = RADIOLIB_LORAWAN_TS007_PACKAGE_VERSION_NUM;
      ansOut[ansPos++] = RADIOLIB_LORAWAN_FPORT_TS007;
      total++;

      // followed by each enabled package in identifier order
      for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_PACKAGES; i++) {
        if(i == RADIOLIB_LORAWAN_PACKAGE_TS007) {
          continue;
        }
        if(this->enabledPackages[i] && this->packages[i] != NULL) {
          ansOut[ansPos++] = i;
          ansOut[ansPos++] = this->packages[i]->packageVersion;
          ansOut[ansPos++] = this->packagePorts[i];
          total++;
        }
      }

      ansOut[nbPos] = (total & 0x0F);
      *ansLen = ansPos;

      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TS007 DevPackageAns: %d packages", total);
      return(1);

    } break;

    default: {
      // Unknown command (or MultiPackBufferReq in an invalid position): stop here.
      // The length of an unknown command is not derivable, so we cannot continue.
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("TS007 unknown command 0x%02x, stopping", dataDown[0]);
      return(0);
    }
  }

  return(0);
}

#endif
