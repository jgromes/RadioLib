#if !defined(RADIOLIB_REED_SOLOMON_H)
#define RADIOLIB_REED_SOLOMON_H

#include <stdint.h>
#include <stddef.h>

// Reed-Solomon parameters
#define RADIOLIB_RS_SYM_BITS    (8)
#define RADIOLIB_RS_SYMB        (1 << RADIOLIB_RS_SYM_BITS)
#define RADIOLIB_RS_NROOTS      (32)
#define RADIOLIB_RS_NDATA       (RADIOLIB_RS_SYMB - RADIOLIB_RS_NROOTS - 1)
#define RADIOLIB_RS_BLOCK       RADIOLIB_RS_SYMB

/* Reed-Solomon encoder
 * Copyright 2004, Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 * 
 * This version tweaked by Philip Heron <phil@sanslogic.co.uk>
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern void rlb_encode_rs_8(const uint8_t *data, uint8_t *parity, int pad);
extern int rlb_decode_rs_8(uint8_t *data, int *eras_pos, int no_eras, int pad);

#ifdef __cplusplus
}
#endif

#endif
