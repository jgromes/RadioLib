#if !defined(_RADIOLIB_JDY08_H) && !defined(RADIOLIB_EXCLUDE_JDY08)
#define _RADIOLIB_JDY08_H

#include "../../ISerial.h"

/*!
  \class JDY08

  \brief Control class for %JDY08 module.
  Most methods supported by this module are implemented in ISerial interface.
*/
class JDY08: public ISerial {
  public:
    /*!
      \brief Default constructor.

      \param mod Instance of Module that will be used to communicate with the radio.
    */
    JDY08(Module* mod);

    /*!
      \brief Initialization method.

      \param speed Baud rate to use for UART interface.
    */
    void begin(long speed);
};

#endif
