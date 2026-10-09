#if !defined(_RADIOLIB_LORAWAN_PACKAGE_TS003_H) && !RADIOLIB_EXCLUDE_LORAWAN
#define _RADIOLIB_LORAWAN_PACKAGE_TS003_H

#include "LoRaWANPacMan.h"

// The major benefit of using this package compared to using the DeviceTimeReq MAC
// command is that, most of the time, the Application Server is not required to answer the
// AppTimeReq command from the end-device. A downlink is only required when the end
// device clock is outside the accuracy requirement that is targeted by the application. In
// contrast, every DeviceTimeReq MAC command sent by the end-device requires an answer
// from the Network Server.

#define RADIOLIB_LORAWAN_TS003_PACKAGE_VERSION        (0x00)
#define RADIOLIB_LORAWAN_TS003_APP_TIME               (0x01)
#define RADIOLIB_LORAWAN_TS003_APP_TIME_PERIODICITY   (0x02)
#define RADIOLIB_LORAWAN_TS003_FORCE_DEVICE_RESYNC    (0x03)

/*!
  \class LoRaWANPackageTS003
  \brief LoRaWAN Application package for TS003 Application Time.
*/
class LoRaWANPackageTS003 : public LoRaWANPackage {
  public:

    // copy constructor is removed to prevent users from creating copies of this class
    LoRaWANPackageTS003(const LoRaWANPackageTS003& obj) = delete;

    /*!
      \brief Process a single TS003 command, writing any answer into the provided buffer
      \param dataDown Pointer to received command data
      \param lenDown Length of command data
      \param dataOut Pointer to the manager's answer buffer
      \param lenOut Pointer that receives the number of answer bytes written
      \param event Pointer to downlink event data
      \returns Number of bytes consumed
    */
    size_t processData(const uint8_t* dataDown, size_t lenDown, uint8_t* dataOut, size_t* lenOut, LoRaWANEvent_t* event) override;

    /*!
      \brief Schedule an application time request to be sent at the next opportunity
      \param force Require a response, even if the clock is already in sync
      \returns \ref status_codes
    */
    int16_t requestAppTime(bool force = false);

    /*!
      \brief Set the setSeconds callback function
      \param setSeconds Callback function to set device time
    */
    typedef void (*SetSecondsCb_t)(RadioLibTime_t);
    void setSecondsCb(SetSecondsCb_t setSecondsCb);

    /*!
      \brief Report when the next AppTimeReq is due and whether one is due now.
      Does not build the payload (see buildUplink).
      \param tNext Pointer that receives the time of the next task
      \param uplinkDue Pointer set to true if an AppTimeReq uplink is due now
      \returns true if a task is pending or scheduled, false otherwise
    */
    bool handleTask(RadioLibTime_t* tNext, bool* uplinkDue) override;

    /*!
      \brief Build the due AppTimeReq with the current time and advance the schedule.
      Called at the moment of transmission so the timestamp stays fresh.
      \param dataOut Pointer to the manager's answer buffer
      \returns Number of uplink bytes written
    */
    size_t buildUplink(uint8_t* dataOut) override;

#if !RADIOLIB_GODMODE
  protected:
#endif

    // build an AppTimeReq into the provided buffer; returns the number of bytes written
    size_t buildAppTimeReq(uint8_t* dataOut);

    SetSecondsCb_t setSeconds;      // callback to configure current GPS time in seconds since epoch
    uint8_t tokenReq;               // validation of AppTimeReq response
    uint8_t transmissions;          // maximum number of AppTimeReq (re)transmissions
    uint32_t periodicity;           // requested interval between AppTimeReq transmissions (s)
    RadioLibTime_t nextAppReqTime;  // next scheduled AppTimeReq transmission
    bool forceReq;                  // set the force bit on the next AppTimeReq

#if !RADIOLIB_GODMODE
  private:
#endif
    // private constructor, to not allow the user to create additional instances of this package
    /*!
      \brief Constructor
      \param pacMan Pointer to the package manager
      \param node Pointer to the LoRaWAN node
      \param secondsCb Pointer to getSeconds() function
    */
    LoRaWANPackageTS003(LoRaWANPackageManager* pacMan, RadioLibHal* hal, LoRaWANNode* node, GetSecondsCb_t secondsCb);
    
    // allow LoRaWANPackageManager to access the private constructor
    friend LoRaWANPackageManager;
};

#endif
