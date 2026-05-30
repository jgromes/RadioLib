#if !defined(_RADIOLIB_ITA2_STRING_H)
#define _RADIOLIB_ITA2_STRING_H

#include "../../TypeDef.h"

#define RADIOLIB_ITA2_FIGS                                      0x1B
#define RADIOLIB_ITA2_LTRS                                      0x1F
#define RADIOLIB_ITA2_LENGTH                                    32

/*!
  \class ITA2String
  \brief ITA2-encoded string.
*/
class ITA2String {
  public:
    /*!
      \brief Default single-character constructor.
      \param c ASCII-encoded character to encode as ITA2.
    */
    explicit ITA2String(char c);

    /*!
      \brief Default string constructor.
      \param str ASCII-encoded string to encode as ITA2.
    */
    explicit ITA2String(const char* str);

    /*!
      \brief Copy constructor.
      \param ita2 ITA2String instance to copy.
    */
    ITA2String(const ITA2String& ita2);
    
    /*!
      \brief Overload for assignment operator.
      \param ita2 rvalue ITA2String.
    */
    ITA2String& operator=(const ITA2String& ita2);

    /*!
      \brief Default destructor.
    */
    ~ITA2String();

    /*!
      \brief Gets the length of the ITA2 string. This number is not the same as the length of ASCII-encoded string!
      \returns Length of ITA2-encoded string.
    */
    size_t length();

    /*!
      \brief Gets the ITA2 representation of the ASCII string set in constructor.
      \returns Pointer to dynamically allocated array, which contains ITA2-encoded bytes.
      It is the caller's responsibility to deallocate this memory!
    */
    uint8_t* byteArr();

#if !RADIOLIB_GODMODE
  private:
#endif
    #if RADIOLIB_STATIC_ONLY
      char strAscii[RADIOLIB_STATIC_ARRAY_SIZE];
    #else
      char* strAscii;
    #endif
    size_t asciiLen;
    size_t ita2Len;

    static uint16_t getBits(char c);
};

#endif
