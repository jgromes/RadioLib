#if !defined(_RADIOLIB_LORAWAN_PACMAN_H) && !RADIOLIB_EXCLUDE_LORAWAN
#define _RADIOLIB_LORAWAN_PACMAN_H

#include "LoRaWAN.h"

// Number of known TSxxx packages
#define RADIOLIB_LORAWAN_NUM_PACKAGES           (8)

// Package ID numbering as per TS008 (TS011 ID is made up)
#define RADIOLIB_LORAWAN_PACKAGE_TS003                          (1)
#define RADIOLIB_LORAWAN_PACKAGE_TS004                          (3)
#define RADIOLIB_LORAWAN_PACKAGE_TS005                          (2)
#define RADIOLIB_LORAWAN_PACKAGE_TS006                          (4)
#define RADIOLIB_LORAWAN_PACKAGE_TS007                          (0)
#define RADIOLIB_LORAWAN_PACKAGE_TS009                          (5)
#define RADIOLIB_LORAWAN_PACKAGE_TS011                          (6)

// Package FPort (specified or recommended)
#define RADIOLIB_LORAWAN_FPORT_TS003                            (202)
#define RADIOLIB_LORAWAN_FPORT_TS004                            (201)
#define RADIOLIB_LORAWAN_FPORT_TS005                            (200)
#define RADIOLIB_LORAWAN_FPORT_TS006                            (203)
#define RADIOLIB_LORAWAN_FPORT_TS007                            (225)
#define RADIOLIB_LORAWAN_FPORT_TS009                            (224)
#define RADIOLIB_LORAWAN_FPORT_TS011                            (226)


// Forward-declare the manager so it can be friended by LoRaWANPackage
class LoRaWANPackageManager;

// Task types for PacMan queries
typedef enum {
  RADIOLIB_LORAWAN_TASK_NONE = 0,
  RADIOLIB_LORAWAN_TASK_ACTION,
  RADIOLIB_LORAWAN_TASK_UPLINK
} LoRaWANTaskType_t;

// Struct to hold next task query results
typedef struct {
  LoRaWANTaskType_t type;   // NONE / ACTION / UPLINK
  RadioLibTime_t time;      // Time of the task (0 = immediate / no deadline)
} LoRaWANTaskInfo;

class LoRaWANPackage {
  public:

    typedef RadioLibTime_t (*GetSecondsCb_t)();

    LoRaWANPackage(uint8_t ts, LoRaWANNode* node, GetSecondsCb_t secondsCb);

    // Process a sequence of package commands and return how many bytes were consumed
    virtual size_t processData(const uint8_t* dataIn, size_t lenIn);

    /*!
      \brief Query the next scheduled task for this package
      \returns `LoRaWANTaskInfo` containing task type and time
    */
    virtual LoRaWANTaskInfo hasTask();

    /*!
      \brief Perform an ACTION task (if any). Default no-op.
    */
    virtual void doAction();

    /*!
      \brief Get uplink data for UPLINK tasks. Returns internal buffer.
      \param dataOut Pointer to uplink buffer
      \param lenOut Pointer to uplink length
    */
    void getUplinkData(uint8_t* dataOut, size_t* lenOut);

    /*!
      \brief Get current time via the stored callback
      \returns Current time in seconds from the callback
    */
    RadioLibTime_t getSeconds() {
      if(this->getSeconds_cb != NULL) {
        return this->getSeconds_cb();
      }
      return 0;
    }
    
#if !RADIOLIB_GODMODE
  protected:
#endif
    
    uint8_t packageIdentifier;
    uint8_t packageVersion;
    uint8_t dataUp[255];
    size_t lenUp = 0;

    LoRaWANNode* lorawanNode;
    GetSecondsCb_t getSeconds_cb;

    // allow LoRaWANPackageManager to access protected members
    friend LoRaWANPackageManager;
};


class LoRaWANPackageManager {
  public:

    typedef RadioLibTime_t (*GetSecondsCb_t)();
    typedef void (*DelaySecondsCb_t)(uint32_t seconds);
    typedef void (*UplinkIntervalCb_t)(uint32_t intervalSeconds);
    typedef void (*RebootCb_t)();

    /*!
      \brief Create a package manager
      \param node Pointer to the LoRaWAN node
      \param getSeconds Pointer to getSeconds() function for time handling
    */
    
    LoRaWANPackageManager(LoRaWANNode* node, GetSecondsCb_t secondsCb);

    // Package enablement methods
    /*!
      \brief Enable TS009 Certification Protocol package
      \returns Status code
    */
    int16_t enableTS009(PhysicalLayer* radio, DelaySecondsCb_t delayCb, UplinkIntervalCb_t intervalCb, RebootCb_t rebootCb);

    // TS009
    bool getConfirmed();

    /*!
      \brief Process a multi-package downlink payload (TS007 section 3.1).
      \param dataIn Pointer to incoming payload
      \param lenIn Length of incoming payload
      \param ansOut Pointer to buffer to collect answers
      \param ansLen Pointer to store answer length
      \returns Status code
    */
    int16_t processPackageDownlink(const uint8_t* dataIn, size_t lenIn, uint8_t fPort = 0xFF);

    /*!
      \brief Query the next scheduled task across all packages and return the first
      \returns LoRaWANTaskInfo struct with time and action fields
    */
    LoRaWANTaskInfo hasTask();

    /*!
      \brief Fetch uplink data from the first package reporting an UPLINK task
      \param dataOut Pointer to uplink buffer
      \param lenOut Pointer to uplink length
      \param fPort Pointer to uplink FPort
      \returns true if data was fetched, false otherwise
    */
    bool getUplinkData(uint8_t* dataOut, size_t* lenOut, uint8_t* fPort);

    /*!
      \brief Execute the first applicable ACTION task (calls package->doAction()).
      \returns true if an action was performed, false otherwise
    */
    bool doAction();

    /*!
      \brief Process TS007 package management commands at the manager level
      \param dataDown Pointer to received downlink data
      \param lenDown Length of downlink data
      \param ansOut Pointer to buffer for answer
      \param ansLen Pointer to store answer length
      \returns Number of bytes consumed from dataDown
    */
    size_t processPackageManagerData(uint8_t* dataDown, size_t lenDown, uint8_t* ansOut, size_t* ansLen);

    bool isEnabledFPort(uint8_t fPort) {
      for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_PACKAGES; i++) {
        if(this->enabledPackages[i] && this->packagePorts[i] == fPort) {
          return true;
        }
      }
      return false;
    }
    
#if !RADIOLIB_GODMODE
  protected:
#endif
    
    LoRaWANNode* lorawanNode;
    LoRaWANPackage* packages[RADIOLIB_LORAWAN_NUM_PACKAGES];
    uint8_t packagePorts[RADIOLIB_LORAWAN_NUM_PACKAGES];
    bool enabledPackages[RADIOLIB_LORAWAN_NUM_PACKAGES];

    // Time handler functions
    GetSecondsCb_t getSecondsCb;
    
};

#endif