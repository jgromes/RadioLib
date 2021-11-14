#if !defined(_RADIOLIB_FSK4_H)
#define _RADIOLIB_FSK4_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_FSK4)

#include "../PhysicalLayer/PhysicalLayer.h"
#include "../AFSK/AFSK.h"

/*!
  \class FSK4Client

  \brief Client for FSK-4 communication. The public interface is the same as Arduino Serial.
*/
class FSK4Client {
  public:
    /*!
      \brief Constructor for FSK-4 mode.

      \param phy Pointer to the wireless module providing PhysicalLayer communication.
    */
    explicit FSK4Client(PhysicalLayer* phy);

    #if !defined(RADIOLIB_EXCLUDE_AFSK)
    /*!
      \brief Constructor for AFSK mode.

      \param audio Pointer to the AFSK instance providing audio.
    */
    explicit FSK4Client(AFSKClient* audio);
    #endif

    // basic methods

    /*!
      \brief Initialization method.

      \param base Base (space) frequency to be used in MHz (in FSK-4 mode), or the space tone frequency in Hz (in AFSK mode)

      \param shift Frequency shift between each tone in Hz.

      \param rate Baud rate to be used during transmission.


      \returns \ref status_codes
    */
    int16_t begin(float base, uint32_t shift, uint16_t rate);

    /*!
      \brief Send out idle condition (RF tone at mark frequency).
    */
    void idle();

    size_t write(uint8_t* buff, size_t len);
    size_t write(uint8_t b);


#if !defined(RADIOLIB_GODMODE)
  private:
#endif
    PhysicalLayer* _phy;
    #if !defined(RADIOLIB_EXCLUDE_AFSK)
    AFSKClient* _audio;
    #endif

    uint32_t _base = 0, _baseHz = 0;
    uint32_t _shift = 0, _shiftHz = 0;
    uint32_t _bitDuration = 0;
    uint32_t _tones[4];
    uint32_t _tonesHz[4];

    void tone(uint8_t i);

    int16_t transmitDirect(uint32_t freq = 0, uint32_t freqHz = 0);
    int16_t standby();
};

#endif

#endif
