#include "LoRaWAN.h"

#if !RADIOLIB_EXCLUDE_LORAWAN

// array of pointers to currently supported LoRaWAN bands
const LoRaWANBand_t* LoRaWANBands[RADIOLIB_LORAWAN_NUM_SUPPORTED_BANDS] = {
  &EU868,
  &US915,
  &EU433,
  &AU915,
  &CN500,
  &AS923,
  &AS923_2,
  &AS923_3,
  &AS923_4,
  &KR920,
  &IN865,
};

const LoRaWANBand_t EU868 = {
  .bandNum = BandEU868,
  .bandType = RADIOLIB_LORAWAN_BAND_DYNAMIC,
  .freqMin = 8630000,
  .freqMax = 8700000,
  .payloadLenMax = {  51,  51,  51, 115, 242, 242, 242, 242,  50, 115,  50, 115,   0,   0,   0 },
  .powerMax = 16,
  .powerNumSteps = 7,
  .dutyCycle = 36000,
  .dwellTimeUp = 0,
  .dwellTimeDn = 0,
  .txParamSupported = false,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 8681000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 8683000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    { .enabled = true, .idx = 2, .freq = 8685000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DrTable = {
    {    0,    0,    0,    0,    0,    0, 0xFF, 0xFF },
    {    1,    0,    0,    0,    0,    0, 0xFF, 0xFF },
    {    2,    1,    0,    0,    0,    0, 0xFF, 0xFF },
    {    3,    2,    1,    0,    0,    0, 0xFF, 0xFF },
    {    4,    3,    2,    1,    0,    0, 0xFF, 0xFF },
    {    5,    4,    3,    2,    1,    0, 0xFF, 0xFF },
    {    6,    5,    4,    3,    2,    1, 0xFF, 0xFF },
    {    7,    6,    5,    4,    3,    2, 0xFF, 0xFF },
    {    1,    0,    0,    0,    0,    0, 0xFF, 0xFF },
    {    2,    1,    0,    0,    0,    0, 0xFF, 0xFF },
    {    1,    0,    0,    0,    0,    0, 0xFF, 0xFF },
    {    2,    1,    0,    0,    0,    0, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  },
  .rx2 = { .enabled = true, .idx = 0, .freq = 8695250, .drMin = 0, .drMax = 7, .dr = 0, .available = true },
  .txWoR = {
    { .enabled = true, .idx = 0, .freq = 8651000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 8655000, .drMin = 3, .drMax = 3, .dr = 3, .available = true }
  },
  .txAck = {
    { .enabled = true, .idx = 0, .freq = 8653000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 8659000, .drMin = 3, .drMax = 3, .dr = 3, .available = true }
  },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_FSK,
    RADIOLIB_LORAWAN_DATA_RATE_LR_FHSS | RADIOLIB_LORAWAN_DATA_RATE_CR_1_3 | RADIOLIB_LORAWAN_DATA_RATE_BW_137_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LR_FHSS | RADIOLIB_LORAWAN_DATA_RATE_CR_2_3 | RADIOLIB_LORAWAN_DATA_RATE_BW_137_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LR_FHSS | RADIOLIB_LORAWAN_DATA_RATE_CR_1_3 | RADIOLIB_LORAWAN_DATA_RATE_BW_336_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LR_FHSS | RADIOLIB_LORAWAN_DATA_RATE_CR_2_3 | RADIOLIB_LORAWAN_DATA_RATE_BW_336_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t US915 = {
  .bandNum = BandUS915,
  .bandType = RADIOLIB_LORAWAN_BAND_FIXED,
  .freqMin = 9020000,
  .freqMax = 9280000,
  .payloadLenMax = {  11,  53, 125, 242, 242,  50, 125,   0,  53, 129, 242, 242, 242, 242,   0 },
  .powerMax = 30,
  .powerNumSteps = 10,
  .dutyCycle = 0,
  .dwellTimeUp = RADIOLIB_LORAWAN_DWELL_TIME,
  .dwellTimeDn = 0,
  .txParamSupported = false,
  .txFreqs = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 2,
  .txSpans = {
    {
      .numChannels = 64,
      .freqStart = 9023000,
      .freqStep = 2000,
      .drMin = 0,
      .drMax = 3,
      .drJoinRequest = 0
    },
    {
      .numChannels = 8,
      .freqStart = 9030000,
      .freqStep = 16000,
      .drMin = 4,
      .drMax = 4,
      .drJoinRequest = 4
    }
  },
  .rx1Span = {
    .numChannels = 8,
    .freqStart = 9233000,
    .freqStep = 6000,
    .drMin = 8,
    .drMax = 13,
    .drJoinRequest = RADIOLIB_LORAWAN_DATA_RATE_UNUSED 
  },
  .rx1DrTable = {
    {   10,    9,    8,    8, 0xFF, 0xFF, 0xFF, 0xFF },
    {   11,   10,    9,    8, 0xFF, 0xFF, 0xFF, 0xFF },
    {   12,   11,   10,    9, 0xFF, 0xFF, 0xFF, 0xFF },
    {   13,   12,   11,   10, 0xFF, 0xFF, 0xFF, 0xFF },
    {   13,   13,   12,   11, 0xFF, 0xFF, 0xFF, 0xFF },
    {   10,    9,    8,    8, 0xFF, 0xFF, 0xFF, 0xFF },
    {   11,   10,    9,    8, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  },
  .rx2 = { .enabled = true, .idx = 0, .freq = 9233000, .drMin = 8, .drMax = 13, .dr = 8, .available = true },
  .txWoR = {
    { .enabled = true, .idx = 0, .freq = 9167000, .drMin = 10, .drMax = 10, .dr = 10, .available = true },
    { .enabled = true, .idx = 1, .freq = 9199000, .drMin = 10, .drMax = 10, .dr = 10, .available = true }
  },
  .txAck = {
    { .enabled = true, .idx = 0, .freq = 9183000, .drMin = 10, .drMax = 10, .dr = 10, .available = true },
    { .enabled = true, .idx = 1, .freq = 9215000, .drMin = 10, .drMax = 10, .dr = 10, .available = true }
  },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LR_FHSS | RADIOLIB_LORAWAN_DATA_RATE_CR_1_3 | RADIOLIB_LORAWAN_DATA_RATE_BW_1523_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LR_FHSS | RADIOLIB_LORAWAN_DATA_RATE_CR_2_3 | RADIOLIB_LORAWAN_DATA_RATE_BW_1523_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t EU433 = {
  .bandNum = BandEU433,
  .bandType = RADIOLIB_LORAWAN_BAND_DYNAMIC,
  .freqMin = 4330000,
  .freqMax = 4340000,
  .payloadLenMax = {  51,  51,  51, 115, 242, 242, 242, 242,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 12,
  .powerNumSteps = 5,
  .dutyCycle = 36000,
  .dwellTimeUp = 0,
  .dwellTimeDn = 0,
  .txParamSupported = false,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 4331750, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 4333750, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    { .enabled = true, .idx = 2, .freq = 4335750, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DrTable = {
    {    0,    0,    0,    0,    0,    0, 0xFF, 0xFF },
    {    1,    0,    0,    0,    0,    0, 0xFF, 0xFF },
    {    2,    1,    0,    0,    0,    0, 0xFF, 0xFF },
    {    3,    2,    1,    0,    0,    0, 0xFF, 0xFF },
    {    4,    3,    2,    1,    0,    0, 0xFF, 0xFF },
    {    5,    4,    3,    2,    1,    0, 0xFF, 0xFF },
    {    6,    5,    4,    3,    2,    1, 0xFF, 0xFF },
    {    7,    6,    5,    4,    3,    2, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  },
  .rx2 = { .enabled = true, .idx = 0, .freq = 4346650, .drMin = 0, .drMax = 7, .dr = 0, .available = true },
  .txWoR = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .txAck = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_FSK,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t AU915 = {
  .bandNum = BandAU915,
  .bandType = RADIOLIB_LORAWAN_BAND_FIXED,
  .freqMin = 9150000,
  .freqMax = 9280000,
  .payloadLenMax = {  51,  51,  51, 115, 242, 242, 242,  50,  53, 129, 242, 242, 242, 242,   0 },
  .powerMax = 30,
  .powerNumSteps = 10,
  .dutyCycle = 0,
  .dwellTimeUp = RADIOLIB_LORAWAN_DWELL_TIME,
  .dwellTimeDn = 0,
  .txParamSupported = true, // conflict: not implemented according to RP v1.1
  .txFreqs = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 2,
  .txSpans = {
    {
      .numChannels = 64,
      .freqStart = 9152000,
      .freqStep = 2000,
      .drMin = 0,
      .drMax = 5,
      .drJoinRequest = 2
    },
    {
      .numChannels = 8,
      .freqStart = 9159000,
      .freqStep = 16000,
      .drMin = 6,
      .drMax = 6,
      .drJoinRequest = 6
    }
  },
  .rx1Span = {
    .numChannels = 8,
    .freqStart = 9233000,
    .freqStep = 6000,
    .drMin = 8,
    .drMax = 13,
    .drJoinRequest = RADIOLIB_LORAWAN_DATA_RATE_UNUSED 
  },
  .rx1DrTable = {
    {    8,    8,    8,    8,    8,    8, 0xFF, 0xFF },
    {    9,    8,    8,    8,    8,    8, 0xFF, 0xFF },
    {   10,    9,    8,    8,    8,    8, 0xFF, 0xFF },
    {   11,   10,    9,    8,    8,    8, 0xFF, 0xFF },
    {   12,   11,   10,    9,    8,    8, 0xFF, 0xFF },
    {   13,   12,   11,   10,    9,    8, 0xFF, 0xFF },
    {   13,   13,   12,   11,   10,    9, 0xFF, 0xFF },
    {    9,    8,    8,    8,    8,    8, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  },
  .rx2 = { .enabled = true, .idx = 0, .freq = 9233000, .drMin = 8, .drMax = 13, .dr = 8, .available = true },
  .txWoR = {
    { .enabled = true, .idx = 0, .freq = 9167000, .drMin = 10, .drMax = 10, .dr = 10, .available = true },
    { .enabled = true, .idx = 1, .freq = 9199000, .drMin = 10, .drMax = 10, .dr = 10, .available = true }
  },
  .txAck = {
    { .enabled = true, .idx = 0, .freq = 9183000, .drMin = 10, .drMax = 10, .dr = 10, .available = true },
    { .enabled = true, .idx = 1, .freq = 9215000, .drMin = 10, .drMax = 10, .dr = 10, .available = true }
  },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LR_FHSS | RADIOLIB_LORAWAN_DATA_RATE_CR_1_3 | RADIOLIB_LORAWAN_DATA_RATE_BW_1523_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t CN500 = {
  .bandNum = BandCN500,
  .bandType = RADIOLIB_LORAWAN_BAND_FIXED,
  .freqMin = 4700000,
  .freqMax = 5100000,
  .payloadLenMax = {  51,  51,  51, 115, 242, 242,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 19,
  .powerNumSteps = 7,
  .dutyCycle = 0,
  .dwellTimeUp = 0,
  .dwellTimeDn = 0,
  .txParamSupported = false,
  .txFreqs = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 1,
  .txSpans = {
    {
      .numChannels = 96,
      .freqStart = 4703000,
      .freqStep = 2000,
      .drMin = 0,
      .drMax = 5,
      .drJoinRequest = 0
    },
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = {
    .numChannels = 48,
    .freqStart = 5003000,
    .freqStep = 2000,
    .drMin = 0,
    .drMax = 5,
    .drJoinRequest = RADIOLIB_LORAWAN_DATA_RATE_UNUSED 
  },
  .rx1DrTable = {
    {    0,    0,    0,    0,    0,    0, 0xFF, 0xFF },
    {    1,    1,    1,    1,    1,    1, 0xFF, 0xFF },
    {    2,    1,    1,    1,    1,    1, 0xFF, 0xFF },
    {    3,    2,    1,    1,    1,    1, 0xFF, 0xFF },
    {    4,    3,    2,    1,    1,    1, 0xFF, 0xFF },
    {    5,    4,    3,    2,    1,    1, 0xFF, 0xFF },
    {    6,    5,    4,    3,    2,    1, 0xFF, 0xFF },
    {    7,    6,    5,    4,    3,    2, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  },
  .rx2 = { .enabled = true, .idx = 0, .freq = 5053000, .drMin = 0, .drMax = 5, .dr = 0, .available = true },
  .txWoR = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .txAck = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t AS923 = {
  .bandNum = BandAS923,
  .bandType = RADIOLIB_LORAWAN_BAND_DYNAMIC,
  .freqMin = 9150000,
  .freqMax = 9280000,
  .payloadLenMax = {  51,  51, 115, 115, 242, 242, 242, 242,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 16,
  .powerNumSteps = 7,
  .dutyCycle = 36000,
  .dwellTimeUp = RADIOLIB_LORAWAN_DWELL_TIME,
  .dwellTimeDn = RADIOLIB_LORAWAN_DWELL_TIME,
  .txParamSupported = true,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 9232000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 9234000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DrTable = {
    {    0,    0,    0,    0,    0,    0,    1,    2 }, // note:
    {    1,    0,    0,    0,    0,    0,    2,    3 }, // when downlinkDwellTime is one
    {    2,    1,    0,    0,    0,    0,    3,    4 }, // we should clip any value <2 to 2
    {    3,    2,    1,    0,    0,    0,    4,    5 },
    {    4,    3,    2,    1,    0,    0,    5,    6 },
    {    5,    4,    3,    2,    1,    0,    6,    7 },
    {    6,    5,    4,    3,    2,    1,    7,    7 },
    {    7,    6,    5,    4,    3,    2,    7,    7 },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  },
  .rx2 = { .enabled = true, .idx = 0, .freq = 9232000, .drMin = 0, .drMax = 7, .dr = 2, .available = true },
  .txWoR = {
    { .enabled = true, .idx = 0, .freq = 9236000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .txAck = {
    { .enabled = true, .idx = 0, .freq = 9238000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_FSK,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t AS923_2 = {
  .bandNum = BandAS923_2,
  .bandType = RADIOLIB_LORAWAN_BAND_DYNAMIC,
  .freqMin = 9150000,
  .freqMax = 9280000,
  .payloadLenMax = {  51,  51, 115, 115, 242, 242, 242, 242,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 16,
  .powerNumSteps = 7,
  .dutyCycle = 36000,
  .dwellTimeUp = RADIOLIB_LORAWAN_DWELL_TIME,
  .dwellTimeDn = RADIOLIB_LORAWAN_DWELL_TIME,
  .txParamSupported = true,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 9214000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 9216000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DrTable = {
    {    0,    0,    0,    0,    0,    0,    1,    2 }, // note:
    {    1,    0,    0,    0,    0,    0,    2,    3 }, // when downlinkDwellTime is one
    {    2,    1,    0,    0,    0,    0,    3,    4 }, // we should clip any value <2 to 2
    {    3,    2,    1,    0,    0,    0,    4,    5 },
    {    4,    3,    2,    1,    0,    0,    5,    6 },
    {    5,    4,    3,    2,    1,    0,    6,    7 },
    {    6,    5,    4,    3,    2,    1,    7,    7 },
    {    7,    6,    5,    4,    3,    2,    7,    7 },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  },
  .rx2 = { .enabled = true, .idx = 0, .freq = 9214000, .drMin = 0, .drMax = 7, .dr = 2, .available = true },
  .txWoR = {
    { .enabled = true, .idx = 0, .freq = 9218000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .txAck = {
    { .enabled = true, .idx = 0, .freq = 9220000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_FSK,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t AS923_3 = {
  .bandNum = BandAS923_3,
  .bandType = RADIOLIB_LORAWAN_BAND_DYNAMIC,
  .freqMin = 9150000,
  .freqMax = 9280000,
  .payloadLenMax = {  51,  51, 115, 115, 242, 242, 242, 242,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 16,
  .powerNumSteps = 7,
  .dutyCycle = 36000,
  .dwellTimeUp = RADIOLIB_LORAWAN_DWELL_TIME,
  .dwellTimeDn = RADIOLIB_LORAWAN_DWELL_TIME,
  .txParamSupported = true,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 9166000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 9168000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DrTable = {
    {    0,    0,    0,    0,    0,    0,    1,    2 }, // note:
    {    1,    0,    0,    0,    0,    0,    2,    3 }, // when downlinkDwellTime is one
    {    2,    1,    0,    0,    0,    0,    3,    4 }, // we should clip any value <2 to 2
    {    3,    2,    1,    0,    0,    0,    4,    5 },
    {    4,    3,    2,    1,    0,    0,    5,    6 },
    {    5,    4,    3,    2,    1,    0,    6,    7 },
    {    6,    5,    4,    3,    2,    1,    7,    7 },
    {    7,    6,    5,    4,    3,    2,    7,    7 },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  },
  .rx2 = { .enabled = true, .idx = 0, .freq = 9166000, .drMin = 0, .drMax = 7, .dr = 2, .available = true },
  .txWoR = {
    { .enabled = true, .idx = 0, .freq = 9170000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .txAck = {
    { .enabled = true, .idx = 0, .freq = 9172000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_FSK,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t AS923_4 = {
  .bandNum = BandAS923_4,
  .bandType = RADIOLIB_LORAWAN_BAND_DYNAMIC,
  .freqMin = 9170000,
  .freqMax = 9200000,
  .payloadLenMax = {  51,  51, 115, 115, 242, 242, 242, 242,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 16,
  .powerNumSteps = 7,
  .dutyCycle = 36000,
  .dwellTimeUp = RADIOLIB_LORAWAN_DWELL_TIME,
  .dwellTimeDn = RADIOLIB_LORAWAN_DWELL_TIME,
  .txParamSupported = true,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 9173000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 9175000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DrTable = {
    {    0,    0,    0,    0,    0,    0,    1,    2 }, // note:
    {    1,    0,    0,    0,    0,    0,    2,    3 }, // when downlinkDwellTime is one
    {    2,    1,    0,    0,    0,    0,    3,    4 }, // we should clip any value <2 to 2
    {    3,    2,    1,    0,    0,    0,    4,    5 },
    {    4,    3,    2,    1,    0,    0,    5,    6 },
    {    5,    4,    3,    2,    1,    0,    6,    7 },
    {    6,    5,    4,    3,    2,    1,    7,    7 },
    {    7,    6,    5,    4,    3,    2,    7,    7 },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  },
  .rx2 = { .enabled = true, .idx = 0, .freq = 9173000, .drMin = 0, .drMax = 7, .dr = 2, .available = true },
  .txWoR = {
    { .enabled = true, .idx = 0, .freq = 9177000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .txAck = {
    { .enabled = true, .idx = 0, .freq = 9179000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_FSK,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t KR920 = {
  .bandNum = BandKR920,
  .bandType = RADIOLIB_LORAWAN_BAND_DYNAMIC,
  .freqMin = 9209000,
  .freqMax = 9233000,
  .payloadLenMax = {  51,  51,  51, 115, 242, 242,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 14,
  .powerNumSteps = 7,
  .dutyCycle = 0,
  .dwellTimeUp = 0,
  .dwellTimeDn = 0,
  .txParamSupported = false,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 9221000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 9223000, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    { .enabled = true, .idx = 2, .freq = 9225000, .drMin = 0, .drMax = 5, .dr = 3, .available = true }
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DrTable = {
    {    0,    0,    0,    0,    0,    0, 0xFF, 0xFF },
    {    1,    0,    0,    0,    0,    0, 0xFF, 0xFF },
    {    2,    1,    0,    0,    0,    0, 0xFF, 0xFF },
    {    3,    2,    1,    0,    0,    0, 0xFF, 0xFF },
    {    4,    3,    2,    1,    0,    0, 0xFF, 0xFF },
    {    5,    4,    3,    2,    1,    0, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  },
  .rx2 = { .enabled = true, .idx = 0, .freq = 9219000, .drMin = 0, .drMax = 5, .dr = 0, .available = true },
  .txWoR = {
    { .enabled = true, .idx = 0, .freq = 9227000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 9231000, .drMin = 3, .drMax = 3, .dr = 3, .available = true }
  },
  .txAck = {
    { .enabled = true, .idx = 0, .freq = 9229000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 9231000, .drMin = 3, .drMax = 3, .dr = 3, .available = true }
  },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t IN865 = {
  .bandNum = BandIN865,
  .bandType = RADIOLIB_LORAWAN_BAND_DYNAMIC,
  .freqMin = 8650000,
  .freqMax = 8670000,
  .payloadLenMax = {  51,  51,  51, 115, 242, 242,   0, 242,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 30,
  .powerNumSteps = 10,
  .dutyCycle = 0,
  .dwellTimeUp = 0,
  .dwellTimeDn = 0,
  .txParamSupported = false,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 8650625, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 8654025, .drMin = 0, .drMax = 5, .dr = 3, .available = true },
    { .enabled = true, .idx = 2, .freq = 8659850, .drMin = 0, .drMax = 5, .dr = 3, .available = true }
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DrTable = {
    {    0,    0,    0,    0,    0,    0,    1,    2 },
    {    1,    0,    0,    0,    0,    0,    2,    3 },
    {    2,    1,    0,    0,    0,    0,    3,    4 },
    {    3,    2,    1,    0,    0,    0,    4,    5 },
    {    4,    3,    2,    1,    0,    0,    5,    5 },
    {    5,    4,    3,    2,    1,    0,    5,    7 },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    {    7,    6,    5,    4,    3,    2,    7,    7 },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  },
  .rx2 = { .enabled = true, .idx = 0, .freq = 8665500, .drMin = 0, .drMax = 7, .dr = 2, .available = true },
  .txWoR = {
    { .enabled = true, .idx = 0, .freq = 8660000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 8667000, .drMin = 3, .drMax = 3, .dr = 3, .available = true }
  },
  .txAck = {
    { .enabled = true, .idx = 0, .freq = 8662000, .drMin = 3, .drMax = 3, .dr = 3, .available = true },
    { .enabled = true, .idx = 1, .freq = 8669000, .drMin = 3, .drMax = 3, .dr = 3, .available = true }
  },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_LORA | RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_FSK,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

#endif
