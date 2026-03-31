#if !RADIOLIB_EXCLUDE_LORAWAN

#include "LoRaWANPacMan.h"
#include "LoRaWANPackageTS009.h"
#include <string.h>

// LoRaWANPackage implementation
LoRaWANPackage::LoRaWANPackage(uint8_t ts, LoRaWANNode* node, GetSecondsCb_t secondsCb)
  : packageIdentifier(ts), lenUp(0), lorawanNode(node), getSeconds_cb(secondsCb) {
  this->packageVersion = 0;
  memset(this->dataUp, 0, sizeof(this->dataUp));
}

size_t LoRaWANPackage::processData(const uint8_t* dataIn, size_t lenIn) {
  // Default implementation: assume the provided buffer is entirely consumed.
  // Derived classes should override and return actual bytes consumed.
  (void)dataIn;
  return(lenIn);
}

void LoRaWANPackage::getUplinkData(uint8_t* dataOut, size_t* lenOut) {
  if(this->lenUp > 0) {
    memcpy(dataOut, this->dataUp, this->lenUp);
    *lenOut = (uint8_t)this->lenUp;
    this->lenUp = 0;
  } else {
    *lenOut = 0;
  }
}

LoRaWANTaskInfo LoRaWANPackage::hasTask() {
  LoRaWANTaskInfo task;
  task.type = RADIOLIB_LORAWAN_TASK_NONE;
  task.time = 0;

  if(this->lenUp > 0) {
    task.type = RADIOLIB_LORAWAN_TASK_UPLINK;
    task.time = 0;
  }

  return(task);
}

void LoRaWANPackage::doAction() {
  // default: no action
}

// LoRaWANPackageManager implementation
LoRaWANPackageManager::LoRaWANPackageManager(LoRaWANNode* node, GetSecondsCb_t secondsCb) {
  // Initialize all packages to NULL and disabled
  for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_PACKAGES; i++) {
    this->packages[i] = NULL;
    this->packagePorts[i] = 0;
    this->enabledPackages[i] = false;
  }

  // Store node and callback reference
  this->lorawanNode = node;
  this->getSecondsCb = secondsCb;
}

int16_t LoRaWANPackageManager::enableTS009(PhysicalLayer* radio, DelaySecondsCb_t delayCb, UplinkIntervalCb_t intervalCb, RebootCb_t rebootCb) {
  // Check if node is not activated
  if(this->lorawanNode == NULL || this->lorawanNode->isActivated()) {
    return(RADIOLIB_ERR_NETWORK_NOT_JOINED);
  }

  // Create package if not already created
  if(this->packages[RADIOLIB_LORAWAN_PACKAGE_TS009] == NULL) {
    this->packages[RADIOLIB_LORAWAN_PACKAGE_TS009] = new LoRaWANPackageTS009(this->lorawanNode, this->getSecondsCb);
    this->packagePorts[RADIOLIB_LORAWAN_PACKAGE_TS009] = RADIOLIB_LORAWAN_FPORT_TS009;
  }

  // Set parameters on TS009 package
  LoRaWANPackageTS009* ts009 = static_cast<LoRaWANPackageTS009*>(this->packages[RADIOLIB_LORAWAN_PACKAGE_TS009]);
  ts009->setPhysicalLayer(radio);
  ts009->setDelaySecondsCallback(delayCb);
  ts009->setUplinkIntervalCallback(intervalCb);
  ts009->setRebootCallback(rebootCb);

  // Enable the package
  this->enabledPackages[RADIOLIB_LORAWAN_PACKAGE_TS009] = true;

  // Register the package's FPort so that downlinks on that FPort are not blocked
  this->lorawanNode->addAppPackage(RADIOLIB_LORAWAN_FPORT_TS009);

  return(RADIOLIB_ERR_NONE);
}

int16_t LoRaWANPackageManager::processPackageDownlink(const uint8_t* dataIn, size_t lenIn, uint8_t fPort) {
  if(dataIn == NULL) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }

  // If the downlink arrived on a package-specific FPort (not the package
  // manager's TS007 FPort), forward the entire payload to that package.
  if(fPort != RADIOLIB_LORAWAN_FPORT_TS007) {
    // Find package registered on this FPort
    for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_PACKAGES; i++) {
      if(!this->enabledPackages[i]) continue;
      if(this->packagePorts[i] == fPort) {
        LoRaWANPackage* pkg = this->packages[i];
        if(pkg == NULL) {
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("No package registered for FPort %d, skipping", fPort);
          return(RADIOLIB_ERR_NONE);
        }
        RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Routing %d bytes on FPort %d to package %d", lenIn, fPort, i);
        // Forward entire payload to package; package returns bytes consumed
        size_t consumed = pkg->processData(dataIn, lenIn);
        if(consumed == 0) {
          RADIOLIB_DEBUG_PROTOCOL_PRINTLN("Package on FPort %d consumed 0 bytes", fPort);
        }
        // Package-specific answers remain in package buffer and will be
        // exposed via hasTask()/getUplinkData(). Nothing more to do here.
        return(RADIOLIB_ERR_NONE);
      }
    }
    // No package matched this FPort; nothing to do.
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("No package matched FPort %d, ignoring payload", fPort);
    return(RADIOLIB_ERR_NONE);
  }

  return(RADIOLIB_ERR_NONE);
}

LoRaWANTaskInfo LoRaWANPackageManager::hasTask() {
  LoRaWANTaskInfo chosen = {
    .type = RADIOLIB_LORAWAN_TASK_NONE,
    .time = 0xFFFFFFFF
  };

  for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_PACKAGES; i++) {
    if(!this->enabledPackages[i] || this->packages[i] == NULL) {
      continue;
    }

    LoRaWANTaskInfo task = this->packages[i]->hasTask();
    if(task.type == RADIOLIB_LORAWAN_TASK_NONE) {
      continue;
    }

    // if the task is an action with time 0, return immediately
    // do not take this shortcut for uplink tasks, as actions are instantaneous
    // and the uplink can be transmitted right after
    if(task.type == RADIOLIB_LORAWAN_TASK_ACTION && task.time == 0) {
      return(task);
    }

    // otherwise, track the earliest task across packages
    if(task.time < chosen.time) {
      chosen = task;
    }
  }

  return(chosen);
}

bool LoRaWANPackageManager::getUplinkData(uint8_t* dataOut, size_t* lenOut, uint8_t* fPort) {
  if(dataOut == NULL || lenOut == NULL || fPort == NULL) {
    return(false);
  }

  RadioLibTime_t now = this->getSecondsCb();
  
  // find first package that reports an UPLINK task
  for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_PACKAGES; i++) {
    if(!this->enabledPackages[i] || this->packages[i] == NULL) {
      continue;
    }

    LoRaWANTaskInfo task = this->packages[i]->hasTask();
    if(task.type == RADIOLIB_LORAWAN_TASK_UPLINK) {
      // only return uplink data if it's due (time <= now)
      if(task.time <= now) {
        this->packages[i]->getUplinkData(dataOut, lenOut);
        *fPort = this->packagePorts[i];
        return(*lenOut > 0);
      }
    }
  }

  *lenOut = 0;
  return(false);
}

bool LoRaWANPackageManager::doAction() {
  RadioLibTime_t now = this->getSecondsCb();

  for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_PACKAGES; i++) {
    if(!this->enabledPackages[i] || this->packages[i] == NULL) {
      continue;
    }

    LoRaWANTaskInfo task = this->packages[i]->hasTask();
    if(task.type == RADIOLIB_LORAWAN_TASK_ACTION) {
      // if action is due now, execute it and return
      if(task.time <= now) {
        this->packages[i]->doAction();
        return(true);
      }
    }
  }

  return(false);
}

bool LoRaWANPackageManager::getConfirmed() {
  LoRaWANPackageTS009* ts009 = static_cast<LoRaWANPackageTS009*>(this->packages[RADIOLIB_LORAWAN_PACKAGE_TS009]);
  if(ts009 == NULL) {
    return(false);
  }
  return(ts009->getConfirmed());
}

#endif
