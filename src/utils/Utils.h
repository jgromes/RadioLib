#if !defined(_RADIOLIB_UTILS_H)
#define _RADIOLIB_UTILS_H

#include <stdint.h>
#include <stdlib.h>

#include "../BuildOpt.h"

// macros to access bits in byte array, from http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
#define SET_BIT_IN_ARRAY_MSB(A, k)                              ( A[((k)/8)] |= (1 << ((k)%8)) )
#define CLEAR_BIT_IN_ARRAY_MSB(A, k)                            ( A[((k)/8)] &= ~(1 << ((k)%8)) )
#define TEST_BIT_IN_ARRAY_MSB(A, k)                             ( A[((k)/8)] & (1 << ((k)%8)) )
#define GET_BIT_IN_ARRAY_MSB(A, k)                              ( (A[((k)/8)] & (1 << ((k)%8))) ? 1 : 0 )
#define SET_BIT_IN_ARRAY_LSB(A, k)                              ( A[((k)/8)] |= (1 << (7 - ((k)%8))) )
#define CLEAR_BIT_IN_ARRAY_LSB(A, k)                            ( A[((k)/8)] &= ~(1 << (7 - ((k)%8))) )
#define TEST_BIT_IN_ARRAY_LSB(A, k)                             ( A[((k)/8)] & (1 << (7 - ((k)%8))) )
#define GET_BIT_IN_ARRAY_LSB(A, k)                              ( (A[((k)/8)] & (1 << (7 - ((k)%8)))) ? 1 : 0 )

/*!
  \brief Function to reflect bits within a byte.
  \param in The input to reflect.
  \param bits Number of bits to reflect.
  \return The reflected input.
*/
uint32_t rlb_reflect(uint32_t in, uint8_t bits);

/*!
  \brief Function to dump data as hex into the debug port.
  \param level RadioLib debug level, set to NULL to not print.
  \param data Data to dump.
  \param len Number of bytes to dump.
  \param offset Address offset to add when printing the data.
  \param width Word width (1 for uint8_t, 2 for uint16_t, 4 for uint32_t).
  \param be Print multi-byte data as big endian. Defaults to false.
*/
void rlb_hexdump(const char* level, const uint8_t* data, size_t len, uint32_t offset = 0, uint8_t width = 1, bool be = false);

#if RADIOLIB_DEBUG && defined(RADIOLIB_BUILD_ARDUINO)
size_t rlb_printf(const char* format, ...);
#endif

#endif
