#if !defined(_RADIOLIB_HC05_H) && !defined(RADIOLIB_EXCLUDE_HC05)
#define _RADIOLIB_HC05_H

#include "../../ISerial.h"

/*!
  \class HC05

  \brief Control class for %HC05 module.
  Most methods supported by this module are implemented in ISerial interface.
*/
class HC05: public ISerial {
  public:
    /*!
      \brief Default constructor.

      \param mod Instance of Module that will be used to communicate with the radio.
    */
    HC05(Module* mod);

    /*!
      \brief Initialization method.

      \param speed Baud rate to use for UART interface.
    */
    void begin(long speed);
};

#endif
