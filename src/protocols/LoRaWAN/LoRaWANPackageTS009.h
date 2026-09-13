#if !defined(_RADIOLIB_LORAWAN_PACKAGE_TS009_H) && !RADIOLIB_EXCLUDE_LORAWAN
#define _RADIOLIB_LORAWAN_PACKAGE_TS009_H

#include "LoRaWAN.h"
#include "LoRaWANPacMan.h"

// TS009 - LCTT (LoRaWAN Conformance Test Tool) Testing Package
// Implements commands for device testing and certification

#define RADIOLIB_LORAWAN_TS009_PACKAGE_VERSION        (0x00)
#define RADIOLIB_LORAWAN_TS009_DUT_RESET              (0x01)
#define RADIOLIB_LORAWAN_TS009_DUT_JOIN               (0x02)
#define RADIOLIB_LORAWAN_TS009_SWITCH_CLASS           (0x03)
#define RADIOLIB_LORAWAN_TS009_ADR_BIT_CHANGE         (0x04)
#define RADIOLIB_LORAWAN_TS009_REGIONAL_DUTY_CYCLE    (0x05)
#define RADIOLIB_LORAWAN_TS009_TX_PERIODICITY_CHANGE  (0x06)
#define RADIOLIB_LORAWAN_TS009_TX_FRAMES_CTRL         (0x07)
#define RADIOLIB_LORAWAN_TS009_ECHO_PAYLOAD           (0x08)
#define RADIOLIB_LORAWAN_TS009_RX_APP_CNT             (0x09)
#define RADIOLIB_LORAWAN_TS009_RX_APP_CNT_RESET       (0x0A)
#define RADIOLIB_LORAWAN_TS009_LINK_CHECK             (0x20)
#define RADIOLIB_LORAWAN_TS009_DEVICE_TIME            (0x21)
#define RADIOLIB_LORAWAN_TS009_FRAG_SESSION_CNT       (0x52)
#define RADIOLIB_LORAWAN_TS009_RELAY_MODE_CTRL        (0x53)
#define RADIOLIB_LORAWAN_TS009_TX_CW                  (0x7D)
#define RADIOLIB_LORAWAN_TS009_DUT_FPORT224_DISABLE   (0x7E)
#define RADIOLIB_LORAWAN_TS009_DUT_VERSIONS           (0x7F)

/*!
  \class LoRaWANPackageTS009
  \brief LoRaWAN Application package for TS009 Certification testing.
*/
class LoRaWANPackageTS009 : public LoRaWANPackage {
  public:

    typedef void (*DelaySecondsCb_t)(RadioLibTime_t seconds);
    typedef void (*UplinkIntervalCb_t)(RadioLibTime_t intervalSeconds);
    typedef void (*ConfirmedCb_t)(bool confirmed);
    typedef void (*RebootCb_t)();

    // copy constructor is removed to prevent users from creating copies of this class
    LoRaWANPackageTS009(const LoRaWANPackageTS009& obj) = delete;

    /*!
      \brief Set radio module reference (for TX_CW command)
    */
    void setPhysicalLayer(PhysicalLayer* radio);

    /*!
      \brief Set delay seconds callback (for delays in various commands)
    */
    void setDelaySecondsCallback(DelaySecondsCb_t delayCb);

    /*!
      \brief Set uplink interval callback (for TX_PERIODICITY_CHANGE command)
    */
    void setUplinkIntervalCallback(UplinkIntervalCb_t intervalCb);

    /*!
      \brief Set confirmed callback (for TX_FRAMES_CTRL command)
    */
    void setConfirmedCallback(ConfirmedCb_t confirmedCb);

    /*!
      \brief Set reboot callback (for DUT_RESET command)
    */
    void setRebootCallback(RebootCb_t rebootCb);

    /*!
      \brief Process downlink data for TS009 LCTT package
      \param dataDown Pointer to received downlink data
      \param lenDown Length of downlink data
      \param eventDown Pointer to downlink event data
      \returns Number of bytes processed
    */
    size_t processData(const uint8_t* dataDown, size_t lenDown, uint8_t* dataOut, size_t* lenOut, LoRaWANEvent_t* event) override;

#if !RADIOLIB_GODMODE
  protected:
#endif

    PhysicalLayer* radio;
    DelaySecondsCb_t delaySecondsCallback;
    UplinkIntervalCb_t uplinkIntervalCallback;
    ConfirmedCb_t confirmedCallback;
    RebootCb_t rebootCallback;

    void reboot() {
      if(this->rebootCallback != NULL) {
        this->rebootCallback();
      }
    }

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
    LoRaWANPackageTS009(LoRaWANPackageManager* pacMan, RadioLibHal* hal, LoRaWANNode* node, GetSecondsCb_t secondsCb);

    uint8_t enabled = false;
    
    // allow LoRaWANPackageManager to access the private constructor
    friend LoRaWANPackageManager;
};

#endif

