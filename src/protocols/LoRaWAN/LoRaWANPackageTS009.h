#if !defined(_RADIOLIB_LORAWAN_PACKAGE_TS009_H) && !RADIOLIB_EXCLUDE_LORAWAN
#define _RADIOLIB_LORAWAN_PACKAGE_TS009_H

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
#define RADIOLIB_LORAWAN_TS009_PING_SLOT_INFO         (0x22)
#define RADIOLIB_LORAWAN_TS009_TX_CW                  (0x7D)
#define RADIOLIB_LORAWAN_TS009_DUT_FPORT224_DISABLE   (0x7E)
#define RADIOLIB_LORAWAN_TS009_DUT_VERSIONS           (0x7F)

/*!
  \class LoRaWANPackageTS009
  \brief LoRaWAN Application package for TS009 Certification testing.
*/
class LoRaWANPackageTS009 : public LoRaWANPackage {
  public:

    typedef void (*DelaySecondsCb_t)(uint32_t seconds);
    typedef void (*UplinkIntervalCb_t)(uint32_t intervalSeconds);
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
      \brief Set reboot callback (for DUT_RESET command)
    */
    void setRebootCallback(RebootCb_t rebootCb);

    /*!
      \brief Check whether subsequent uplinks should be confirmed
      \returns true if uplinks should be confirmed, false otherwise
    */
    bool getConfirmed();

    /*!
      \brief Process downlink data for TS009 LCTT package
      \param dataDown Pointer to received downlink data
      \param lenDown Length of downlink data
      \returns Number of bytes consumed
    */
    size_t processData(const uint8_t* dataDown, size_t lenDown) override;

#if !RADIOLIB_GODMODE
  protected:
#endif

    PhysicalLayer* radioModule;
    bool confirmed = false;
    DelaySecondsCb_t delaySecondsCallback;
    UplinkIntervalCb_t uplinkIntervalCallback;
    RebootCb_t rebootCallback;

#if !RADIOLIB_GODMODE
  private:
#endif
    // private constructor, to not allow the user to create additional instances of this package
    LoRaWANPackageTS009(LoRaWANNode* node, GetSecondsCb_t secondsCb);
    
    // allow LoRaWANPackageManager to access the private constructor
    friend LoRaWANPackageManager;
};

#endif

