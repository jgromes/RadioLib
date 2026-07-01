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

// TS007 Multi-Package Access package (PackageIdentifier 0) commands and constants
#define RADIOLIB_LORAWAN_TS007_CID_PKG_VERSION                  (0x00)
#define RADIOLIB_LORAWAN_TS007_CID_DEV_PACKAGE                  (0x01)
#define RADIOLIB_LORAWAN_TS007_CID_MULTI_PACK_BUFFER            (0x02)
#define RADIOLIB_LORAWAN_TS007_PACKAGE_VERSION_NUM              (1)
#define RADIOLIB_LORAWAN_TS007_ERROR                            (0xFF)

// A PackageID field has bit 7 set; CommandIDs are always < 128
#define RADIOLIB_LORAWAN_TS007_PACKAGE_ID_FLAG                  (0x80)

// Maximum size of the ANS buffer (TS007 section 3.1)
#define RADIOLIB_LORAWAN_TS007_ANS_BUFFER_SIZE                  (128)

// Forward-declare the manager so it can be friended by LoRaWANPackage
class LoRaWANPackageManager;

/*!
  \class LoRaWANPackage
  \brief Common interface for all application packages.
*/
class LoRaWANPackage {
  public:

    typedef RadioLibTime_t (*GetSecondsCb_t)();

    // copy constructor is removed to prevent users from creating copies of this class
    LoRaWANPackage(const LoRaWANPackage& obj) = delete;

    /*!
      \brief Process a single package command, writing any answer into the buffer
      provided by the package manager.
      \param dataIn Pointer to incoming command data
      \param lenIn Length of incoming data
      \param dataOut Pointer to the manager's answer buffer (write answer here)
      \param lenOut Pointer that receives the number of answer bytes written
      \param event Downlink event details
      \returns Number of bytes consumed
    */
    virtual size_t processData(const uint8_t* dataIn, size_t lenIn, uint8_t* dataOut, size_t* lenOut, LoRaWANEvent_t* event);

    /*!
      \brief Execute any scheduled task that is due now and report when this package
      next needs servicing. When an uplink is due now it reports so via uplinkDue; 
      the bytes are produced later by buildUplink() at the actual moment of transmission.
      \param tNext Pointer that receives the time of the next task.
      \param uplinkDue Pointer set to true if an uplink is ready to be sent now.
      \returns true if the package has a pending or scheduled task, false otherwise
    */
    virtual bool handleTask(RadioLibTime_t* tNext, bool* uplinkDue);

    /*!
      \brief Build the uplink that handleTask() reported as due, at the moment of
      transmission, and advance the package's transmit schedule.
      \param dataOut Pointer to the manager's answer buffer (write uplink here)
      \returns Number of uplink bytes written (0 if nothing to send)
    */
    virtual size_t buildUplink(uint8_t* dataOut);

    /*!
      \brief Get current time via the stored callback
      \returns Current time in seconds from the callback
    */
    RadioLibTime_t getSeconds() {
      if(this->getSeconds_cb != NULL) {
        return(this->getSeconds_cb());
      }
      return(0);
    }

#if !RADIOLIB_GODMODE
  protected:
#endif

    /*!
      \brief Default constructor.
      \param pacMan Pointer to the package manager for persistence handling
      \param hal Pointer to RadioLib HAL
      \param node Pointer to the LoRaWAN node
      \param secondsCb Pointer to getSeconds() function for time handling
    */
    LoRaWANPackage(uint8_t ts, LoRaWANPackageManager* pacMan, RadioLibHal* hal, LoRaWANNode* node, GetSecondsCb_t secondsCb);

    uint8_t packageIdentifier;
    uint8_t packageVersion;

    LoRaWANPackageManager* pacMan;
    RadioLibHal* hal;
    LoRaWANNode* lorawanNode;
    GetSecondsCb_t getSeconds_cb;

    // allow LoRaWANPackageManager to access the protected members
    friend LoRaWANPackageManager;
};

/*!
  \class LoRaWANPackageManager
  \brief Interface to manage multiple LoRaWAN application packages.
*/
class LoRaWANPackageManager {
  public:

    typedef RadioLibTime_t (*GetSecondsCb_t)();
    typedef void (*SetSecondsCb_t)(RadioLibTime_t seconds);
    typedef void (*DelaySecondsCb_t)(RadioLibTime_t seconds);
    typedef void (*UplinkIntervalCb_t)(RadioLibTime_t intervalSeconds);
    typedef void (*ConfirmedCb_t)(bool confirmed);
    typedef void (*RebootCb_t)();

    /*!
      \brief Create a package manager
      \param hal Pointer to RadioLib HAL
      \param node Pointer to the LoRaWAN node
      \param getSeconds Pointer to getSeconds() function for time handling
    */    
    LoRaWANPackageManager(RadioLibHal* hal, LoRaWANNode* node, GetSecondsCb_t secondsCb);

    // Package enable methods
    /*!
      \brief Enable TS003 Application Time Synchronization package
      \param fPort The FPort to which this package will listen
      \param setSecondsFunc Pointer to setSeconds() function
      \returns \ref status_codes
    */
    int16_t enableTS003(uint8_t fPort, SetSecondsCb_t setSecondsFunc);

    /*!
      \brief Enable TS007 Multi-package Access Control package
      \returns \ref status_codes
    */
    int16_t enableTS007();

    /*!
      \brief Enable TS009 Certification Protocol package.
      \param radio Pointer to the PhysicalLayer radio instance for RF testing.
      \param delayCb Callback to delay for a number of seconds during RF testing.
      \param intervalCb Callback to set the uplink interval for the device.
      \param confirmedCb Callback to set whether subsequent uplinks should be confirmed.
      \param rebootCb Callback to perform a device reboot.
      \returns \ref status_codes
    */
    int16_t enableTS009(PhysicalLayer* radio, DelaySecondsCb_t delayCb, UplinkIntervalCb_t intervalCb, ConfirmedCb_t confirmedCb, RebootCb_t rebootCb);

    // TS003
    /*!
      \brief Send an application time request.
      \returns \ref status_codes
    */
    void requestAppTime();

    /*!
      \brief Process a multi-package downlink payload (TS007 section 3.1).
      \param data Pointer to incoming payload
      \param len Length of incoming payload
      \param eventDown Pointer to LoRaWANEvent_t downlink event struct
      \returns \ref status_codes
    */
    int16_t processDownlink(const uint8_t* data, size_t len, LoRaWANEvent_t* eventDown);

    /*!
      \brief Execute any due tasks across all packages and report when the manager
      next needs servicing.
      \param tNext Pointer that receives the relative time (in seconds) until the
      next task. Returns 0 if uplink data is ready to be sent now. It includes
      dutycycle constraints if applicable.
      \returns true if any package has a pending or scheduled task, false otherwise
    */
    bool handleTask(RadioLibTime_t* tNext);

    /*!
      \brief Fetch uplink data from the first package reporting an UPLINK task
      \param dataOut Pointer to uplink buffer
      \param lenOut Pointer to uplink length
      \param fPort Pointer to uplink FPort
      \returns true if data was fetched, false otherwise
    */
    bool getUplinkData(uint8_t* dataOut, size_t* lenOut, uint8_t* fPort);

    bool isEnabledFPort(uint8_t fPort) {
      for(uint8_t i = 0; i < RADIOLIB_LORAWAN_NUM_PACKAGES; i++) {
        if(this->enabledPackages[i] && this->packagePorts[i] == fPort) {
          return(true);
        }
      }
      return(false);
    }
    
#if !RADIOLIB_GODMODE
  protected:
#endif

    // Set up transmission of a MultiPackBufferReq answer (TS007 section 4.4).
    // Uses the persistent ANS buffer; sets the transmit cursor or the error flag.
    void handleMultiPackBufferReq(uint8_t startByte, uint8_t stopByte);

    // Process the TS007 multi-package access package's own commands
    // (PackageVersionReq, DevPackageReq).
    size_t processPackageManagerData(const uint8_t* dataDown, size_t lenDown, uint8_t* ansOut, size_t* ansLen);

    RadioLibHal* hal;
    LoRaWANNode* lorawanNode;
    LoRaWANPackage* packages[RADIOLIB_LORAWAN_NUM_PACKAGES];
    uint8_t packagePorts[RADIOLIB_LORAWAN_NUM_PACKAGES];
    bool enabledPackages[RADIOLIB_LORAWAN_NUM_PACKAGES];

    // Time handler functions
    GetSecondsCb_t getSecondsCb;

    // Staging buffer for a plain (non multi-package) uplink: a dedicated-FPort answer
    // or a scheduled package uplink. Packages write here (via the dataOut buffer they
    // are handed); it is overwritten on each newly staged uplink.
    uint8_t ansBuffer[255];
    size_t ansBufferLen;
    uint8_t ansFPort;       // FPort on which the staged uplink will be sent (225 = TS007)

    // Persistent TS007 ANS buffer (answers only, no Command Token). Kept in memory for
    // on-demand retransmission via MultiPackBufferReq until a new multi-package command
    // set is received. Separate from ansBuffer.
    uint8_t ts007Buffer[RADIOLIB_LORAWAN_TS007_ANS_BUFFER_SIZE];
    size_t ts007BufferLen;
    uint8_t commandToken;   // TS007 Command Token to append to FPort 225 uplinks

    // Transmission state for a pending uplink (TS007 fragments are sent over several
    // calls; a plain package uplink is sent in one)
    bool managerPending;    // a staged uplink is due now
    int8_t pendingBuildPackage; // package index that builds its uplink at send time, or -1 = prebuilt buffer
    size_t txCursor;        // next BaseByte to transmit (TS007 fragmentation)
    size_t txStop;          // last TS007 ANS buffer index to transmit (inclusive)
    bool txForceFrag;       // always wrap in MultiPackBufferFrag (Req response)
    bool txError;           // next uplink is the [0x02][0xFF] error frame

};

#endif