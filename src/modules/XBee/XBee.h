#if !defined(_RADIOLIB_XBEE_H) && !defined(RADIOLIB_EXCLUDE_XBEE)
#define _RADIOLIB_XBEE_H

#include "../../ISerial.h"
#include "../../TypeDef.h"

// API reserved characters
#define XBEE_API_START                                0x7E
#define XBEE_API_ESCAPE                               0x7D
#define XBEE_API_XON                                  0x11
#define XBEE_API_XOFF                                 0x13

// API frame IDs
#define XBEE_API_FRAME_AT_COMMAND                     0x08
#define XBEE_API_FRAME_AT_COMMAND_QUEUE               0x09
#define XBEE_API_FRAME_ZIGBEE_TRANSMIT_REQUEST        0x10
#define XBEE_API_FRAME_ZIGBEE_ADDRESS_EXPLICIT        0x11
#define XBEE_API_FRAME_REMOTE_COMMAND                 0x17
#define XBEE_API_FRAME_CREATE_SOURCE_ROUTE            0x21
#define XBEE_API_FRAME_AT_COMMAND_RESPONSE            0x88
#define XBEE_API_FRAME_MODEM_STATUS                   0x8A
#define XBEE_API_FRAME_ZIGBEE_TRANSMIT_STATUS         0x8B
#define XBEE_API_FRAME_ZIGBEE_RECEIVE_PACKET          0x90
#define XBEE_API_FRAME_ZIGBEE_EXPLICIT_RX             0x91
#define XBEE_API_FRAME_ZIGBEE_IO_DATA_SAMPLE_RX       0x92
#define XBEE_API_FRAME_SENSOR_READ                    0x94
#define XBEE_API_FRAME_NODE_ID                        0x95
#define XBEE_API_FRAME_REMOTE_COMMAND_RESPONSE        0x97
#define XBEE_API_FRAME_EXTENDED_MODEM_STATUS          0x98
#define XBEE_API_FRAME_OTA_FW_UPDATE_STATUS           0xA0
#define XBEE_API_FRAME_ROUTE_RECORD                   0xA1
#define XBEE_API_FRAME_MANY_TO_ONE_ROUTE_REQUEST      0xA3

/*!
  \class XBeeSerial

  \brief %XBee Serial interface. This class is used for XBees in transparent mode, i.e. when two XBees act as a "wireless UART".
*/
class XBeeSerial: public ISerial {
  public:
    /*!
      \brief Default constructor.

      \param mod Instance of Module that will be used to communicate with the radio.
    */
    XBeeSerial(Module* mod);

    // basic methods

    /*!
      \brief Initialization method.

      \param speed Baud rate to use for UART interface.

      \returns \ref status_codes
    */
    int16_t begin(long speed);

    /*!
      \brief Resets module using interrupt/GPIO pin 1.
    */
    void reset();

    // configuration methods

    /*!
      \brief Sets destination XBee address.

      \param destinationAddressHigh Higher 4 bytes of the destination XBee module, in the form of uppercase hexadecimal string (i.e. 8 characters).

      \param destinationAddressLow Lower 4 bytes of the destination XBee module, in the form of uppercase hexadecimal string (i.e. 8 characters).

      \returns \ref status_codes
    */
    int16_t setDestinationAddress(const char* destinationAddressHigh, const char* destinationAddressLow);

    /*!
      \brief Sets PAN (Personal Area Network) ID. Both XBees must be in the same PAN in order to use transparent mode.

      \param panId 8-byte PAN ID to be used, in the form of uppercase hexadecimal string (i.e. 16 characters).
    */
    int16_t setPanId(const char* panId);

#ifndef RADIOLIB_GODMODE
  private:
#endif
    bool enterCmdMode();

};

/*!
  \class XBee

  \brief Control class for %XBee modules.
*/
class XBee {
  public:
    /*!
      \brief Default constructor.

      \param mod Instance of Module that will be used to communicate with the radio.
    */
    XBee(Module* mod);

    // basic methods

    /*!
      \brief Initialization method.

      \param speed Baud rate to use for UART interface.

      \returns \ref status_codes
    */
    int16_t begin(long speed);

    /*!
      \brief Resets module using interrupt/GPIO pin 1.
    */
    void reset();

    /*!
      \brief Sends data to the destination 64-bit (global) address, when destination 16-bit (local) address is unknown.

      \param dest Destination 64-bit address, in the form of 8-byte array.

      \param payload String of payload bytes.

      \param radius Number of maximum "hops" for a broadcast transmission. Defaults to 1, set to 0 for unlimited number of hops.
    */
    int16_t transmit(uint8_t* dest, const char* payload, uint8_t radius = 1);

    /*!
      \brief Sends data to the destination 64-bit (global) address, when destination 16-bit (local) address is known.

      \param dest Destination 64-bit address, in the form of 8-byte array.

      \param destNetwork Destination 16-bit address, in the form of 2-byte array.

      \param payload String of payload bytes.

      \param radius Number of maximum "hops" for a broadcast transmission. Defaults to 1, set to 0 for unlimited number of hops.
    */
    int16_t transmit(uint8_t* dest, uint8_t* destNetwork, const char* payload, uint8_t radius = 1);

    /*!
      \brief Gets the number of payload bytes received.

      \returns Number of available payload bytes, or 0 if nothing was received.
    */
    size_t available();

    /*!
      \brief Gets packet source 64-bit address.

      \returns Packet source address, in the form of uppercase hexadecimal Arduino String (i.e. 16 characters).
    */
    String getPacketSource();

    /*!
      \brief Gets packet payload.

      \returns Packet payload, in the form of Arduino String.
    */
    String getPacketData();

    // configuration methods

    /*!
      \brief Sets PAN (Personal Area Network) ID. All XBees must be in the same PAN in order to communicate.

      \param panId 8-byte PAN ID to be used, in the form of uppercase hexadecimal string (i.e. 16 characters).
    */
    int16_t setPanId(uint8_t* panId);

#ifndef RADIOLIB_GODMODE
  private:
#endif
    Module* _mod;
    uint8_t _frameID = 0x01;
    size_t _frameLength = 0;
    bool _frameHeaderProcessed = false;

    #ifdef RADIOLIB_STATIC_ONLY
      char _packetData[RADIOLIB_STATIC_ARRAY_SIZE];
    #else
      char* _packetData = new char[0];
    #endif
    uint8_t _packetSource[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    int16_t confirmChanges();

    void sendApiFrame(uint8_t type, uint8_t id, const char* data);
    void sendApiFrame(uint8_t type, uint8_t id, uint8_t* data, uint16_t length);
    int16_t readApiFrame(uint8_t frameID, uint8_t codePos, uint16_t timeout = 5000);

    uint16_t getNumBytes(uint32_t timeout = 10000, size_t minBytes = 10);
};

#endif
