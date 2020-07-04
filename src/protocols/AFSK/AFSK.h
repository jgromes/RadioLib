#if !defined(_RADIOLIB_AFSK_H)
#define _RADIOLIB_AFSK_H

#include "../../TypeDef.h"

#if !defined(RADIOLIB_EXCLUDE_AFSK)

#include "../../Module.h"

#include "../PhysicalLayer/PhysicalLayer.h"

/*!
  \class AFSKClient

  \brief Client for audio-based transmissions. Requires Arduino tone() function, and a module capable of direct mode transmission using DIO pins.
*/
class AFSKClient  {
  public:
    /*!
      \brief Default contructor.

      \param phy Pointer to the wireless module providing PhysicalLayer communication.

      \param pin The pin that will be used for audio output.
    */
    AFSKClient(PhysicalLayer* phy, RADIOLIB_PIN_TYPE pin);

    /*!
      \brief Start transmitting audio tone.

      \param freq Frequency of the tone in Hz.

      \param autoStart Whether to automatically enter transmission mode. Defaults to true.

      \returns \ref status_codes
    */
    int16_t tone(uint16_t freq, bool autoStart = true);

    /*!
      \brief Stops transmitting audio tone.

      \returns \ref status_codes
    */
    int16_t noTone();

#ifndef RADIOLIB_GODMODE
  private:
#endif
    PhysicalLayer* _phy;
    RADIOLIB_PIN_TYPE _pin;

    // allow specific classes access the private PhysicalLayer pointer
    friend class RTTYClient;
    friend class MorseClient;
    friend class HellClient;
    friend class SSTVClient;
    friend class AX25Client;
};

#endif

#endif
