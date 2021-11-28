#if !defined(_RADIOLIB_RADIOLIB_APRS_H)
#define _RADIOLIB_RADIOLIB_APRS_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_APRS)

#include "../PhysicalLayer/PhysicalLayer.h"
#include "../AX25/AX25.h"

// APRS data type identifiers
#define RADIOLIB_APRS_DATA_TYPE_POSITION_NO_TIME_NO_MSG         "!"
#define RADIOLIB_APRS_DATA_TYPE_GPS_RAW                         "$"
#define RADIOLIB_APRS_DATA_TYPE_ITEM                            ")"
#define RADIOLIB_APRS_DATA_TYPE_TEST                            ","
#define RADIOLIB_APRS_DATA_TYPE_POSITION_TIME_NO_MSG            "/"
#define RADIOLIB_APRS_DATA_TYPE_MSG                             ":"
#define RADIOLIB_APRS_DATA_TYPE_OBJECT                          ";"
#define RADIOLIB_APRS_DATA_TYPE_STATION_CAPABILITES             "<"
#define RADIOLIB_APRS_DATA_TYPE_POSITION_NO_TIME_MSG            "="
#define RADIOLIB_APRS_DATA_TYPE_STATUS                          ">"
#define RADIOLIB_APRS_DATA_TYPE_QUERY                           "?"
#define RADIOLIB_APRS_DATA_TYPE_POSITION_TIME_MSG               "@"
#define RADIOLIB_APRS_DATA_TYPE_TELEMETRY                       "T"
#define RADIOLIB_APRS_DATA_TYPE_MAIDENHEAD_BEACON               "["
#define RADIOLIB_APRS_DATA_TYPE_WEATHER_REPORT                  "_"
#define RADIOLIB_APRS_DATA_TYPE_USER_DEFINED                    "{"
#define RADIOLIB_APRS_DATA_TYPE_THIRD_PARTY                     "}"

/*!
  \class APRSClient

  \brief Client for APRS communication.
*/
class APRSClient {
  public:
    /*!
      \brief Default constructor.

      \param ax Pointer to the instance of AX25Client to be used for APRS.
    */
    explicit APRSClient(AX25Client* ax);

    // basic methods

    /*!
      \brief Initialization method.

      \param symbol APRS symbol to be displayed.

      \param alt Whether to use the primary (false) or alternate (true) symbol table. Defaults to primary table.

      \returns \ref status_codes
    */
    int16_t begin(char symbol, bool alt = false);

    /*!
      \brief Transmit position.

      \param destCallsign Destination station callsign.

      \param destSSID Destination station SSID.

      \param lat Latitude as a null-terminated string.

      \param long Longitude as a null-terminated string.

      \param msg Message to be transmitted. Defaults to NULL (no message).

      \param msg Position timestamp. Defaults to NULL (no timestamp).

      \returns \ref status_codes
    */
    int16_t sendPosition(char* destCallsign, uint8_t destSSID, char* lat, char* lon, char* msg = NULL, char* time = NULL);

    /*!
      \brief Transmit generic APRS frame.

      \param destCallsign Destination station callsign.

      \param destSSID Destination station SSID.

      \param info AX.25 info field contents.

      \returns \ref status_codes
    */
    int16_t sendFrame(char* destCallsign, uint8_t destSSID, char* info);

#if !defined(RADIOLIB_GODMODE)
  private:
#endif
    AX25Client* _ax;

    // default APRS symbol (car)
    char _symbol = '>';
    char _table = '/';
};

#endif

#endif
