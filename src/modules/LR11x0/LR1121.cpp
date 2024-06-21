#include "LR1121.h"
#if !RADIOLIB_EXCLUDE_LR11X0

LR1121::LR1121(Module* mod) : LR1120(mod) {
  chipType = RADIOLIB_LR11X0_DEVICE_LR1121;
}

int16_t LR1121::setOutputPower(int8_t power, bool useHighFreqPa)
{

    int16_t state = RADIOLIB_ERR_INVALID_OUTPUT_POWER;

    if (useHighFreqPa) {
        if ((-18 <= power ) && ( power <= 13 )) {
            state = setPaConfig(0x02, //  High-frequency Power Amplifier
                                0x00, //  Power amplifier supplied by the main regulator
                                0x04, //  Power Amplifier duty cycle (Default 0x04)
                                0x07  //  Number of slices for HPA (Default 0x07)
                               );
        } else {
            return RADIOLIB_ERR_INVALID_OUTPUT_POWER;
        }
    } else {
        if (( -17 <= power ) && (power <= 22 )) {
            if (power == 22) {
                state = setPaConfig(0x01, //  High-power Power Amplifier
                                    0x01, //  Power amplifier supplied by the battery
                                    0x04, //  Power Amplifier duty cycle (Default 0x04)
                                    0x07  //  Number of slices for HPA (Default 0x07)
                                   );
            } else {
                state = setPaConfig(0x00, //  Low-power Power Amplifier
                                    0x00, //  Power amplifier supplied by the main regulator
                                    0x04, //  Power Amplifier duty cycle (Default 0x04)
                                    0x07  //  Number of slices for HPA (Default 0x07)
                                   );
            }
        } else {
            return RADIOLIB_ERR_INVALID_OUTPUT_POWER;
        }
    }
    RADIOLIB_ASSERT(state);

    // set output power
    state = setTxParams(power, RADIOLIB_LR11X0_PA_RAMP_48U);
    return (state);
}
#endif