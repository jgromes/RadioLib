#ifndef _KITELIB_XBEE_H
#define _KITELIB_XBEE_H

#include "ISerial.h"
#include "TypeDef.h"

// API reserved characters
#define XBEE_API_START                                0x7E
#define XBEE_API_ESCAPE                               0x7D
#define XBEE_API_XON                                  0x11
#define XBEE_API_XOFF                                 0x13

// API frame IDs
#define XBEE_API_FRAME_AT_COMMAND                     0x08
#define XBEE_API_FRAME_AT_COMMAND_PARAMETER           0x09
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

class XBeeSerial: public ISerial {
  public:
    // constructor
    XBeeSerial(Module* mod);
    
    // basic methods
    uint8_t begin(long speed);
    
    // configuration methods
    uint8_t setDestinationAddress(const char* destinationAddressHigh, const char* destinationAddressLow);
    uint8_t setPanId(const char* panID);
    
  private:
    bool enterCmdMode();
    
};

class XBee {
  public:
    // constructor
    XBee(Module* mod);
    
    // basic methods
    uint8_t begin(long speed);
    uint8_t transmit(uint8_t* dest, const char* payload, uint8_t radius = 1);
    uint8_t transmit(uint8_t* dest, uint8_t* destNetwork, const char* payload, uint8_t radius = 1);
    size_t available();
    String getPacketSource();
    String getPacketData();
    
    // configuration methods
    uint8_t setPanId(uint8_t* panID);
  
  private:
    Module* _mod;
    uint8_t _frameID;
    
    void sendApiFrame(uint8_t type, uint8_t id, const char* data);
    void sendApiFrame(uint8_t type, uint8_t id, uint8_t* data, uint16_t length);
    uint8_t readApiFrame(uint8_t frameID, uint8_t codePos);
    
    uint16_t getNumBytes(uint32_t timeout = 10000, size_t minBytes = 10);
};

#endif
