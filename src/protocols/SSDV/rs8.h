/* Reed-Solomon encoder
 * Copyright 2004, Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 * 
 * This version tweaked by Philip Heron <phil@sanslogic.co.uk>
*/

#ifndef _RS8_H
#define _RS8_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern void encode_rs_8(uint8_t *data, uint8_t *parity, int pad);
extern int decode_rs_8(uint8_t *data, int *eras_pos, int no_eras, int pad);

#ifdef __cplusplus
}
#endif
#endif
