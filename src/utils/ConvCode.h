#if !defined(_RADIOLIB_CONV_CODE_H)
#define _RADIOLIB_CONV_CODE_H

#include "../TypeDef.h"
#include "../Module.h"

/*!
  \class RadioLibConvCode
  \brief Class to perform convolutional coding with variable rates.
  Only 1/2 and 1/3 rate is currently supported.

  Convolutional coder implementation in this class is adapted from Semtech's LR-FHSS demo:
  https://github.com/Lora-net/SWDM001/tree/master/lib/sx126x_driver

  Its SX126x driver is distributed under the Clear BSD License,
  and to comply with its terms, it is reproduced below.

  The Clear BSD License
  Copyright Semtech Corporation 2021. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted (subject to the limitations in the disclaimer
  below) provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the Semtech corporation nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
  THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
  NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
class RadioLibConvCode {
  public:
    /*!
      \brief Default constructor.
    */
    RadioLibConvCode();

    /*!
      \brief Initialization method.
      \param rt Encoding rate denominator (1/x). Only 1/2 and 1/3 encoding is currently supported.
    */
    void begin(uint8_t rt);

    /*!
      \brief Encoding method.
      \param in Input buffer (a byte array).
      \param in_bits Input length in bits.
      \param out Output buffer (a byte array). It is up to the caller
      to ensure the buffer is large enough to fit the encoded data!
      \param out_bits Pointer to a variable to save the number of encoded bits.
      Ignored if set to NULL.
      \returns \ref status_codes 
    */
    int16_t encode(const uint8_t* in, size_t in_bits, uint8_t* out, size_t* out_bits = NULL);

  private:
    uint8_t enc_state = 0;
    uint8_t rate = 0;
};

#endif
