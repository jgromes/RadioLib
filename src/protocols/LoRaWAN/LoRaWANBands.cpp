#include "LoRaWAN.h"

#if !defined(RADIOLIB_EXCLUDE_LORAWAN)

uint8_t getDownlinkDataRate(uint8_t uplink, uint8_t offset, uint8_t base, uint8_t min, uint8_t max) {
  int8_t dr = uplink - offset + base;
  if(dr < min) {
    dr = min;
  } else if (dr > max) {
    dr = max;
  }
  return(dr);
}

const LoRaWANBand_t EU868 = {
  .payloadLenMax = {  59,  59,  59, 123, 230, 230, 230, 230,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 16,
  .powerNumSteps = 7,
  .cfListType = RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 868.100, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 1, .freq = 868.300, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 2, .freq = 868.500, .drMin = 0, .drMax = 5},
  },
  .txJoinReq = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DataRateBase = 0,
  .rx2 = { .enabled = true, .idx = 0, .freq = 869.525, .drMin = 0, .drMax = 0 },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_9 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7 | RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_FSK_50_K,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t US915 = {
  .payloadLenMax = {  19,  61, 133, 250, 250,   0,   0,   0,  41, 117, 230, 230, 230, 230,   0 },
  .powerMax = 30,
  .powerNumSteps = 10,
  .cfListType = RADIOLIB_LORAWAN_CFLIST_TYPE_MASK,
  .txFreqs = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .txJoinReq = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 2,
  .txSpans = {
    {
      .numChannels = 64,
      .freqStart = 902.300,
      .freqStep = 0.200,
      .drMin = 0,
      .drMax = 3,
      .joinRequestDataRate = 0
    },
    {
      .numChannels = 8,
      .freqStart = 903.000,
      .freqStep = 1.600,
      .drMin = 4,
      .drMax = 4,
      .joinRequestDataRate = 4
    }
  },
  .rx1Span = {
    .numChannels = 8,
    .freqStart = 923.300,
    .freqStep = 0.600,
    .drMin = 8,
    .drMax = 13,
    .joinRequestDataRate = RADIOLIB_LORAWAN_DATA_RATE_UNUSED 
  },
  .rx1DataRateBase = 10,
  .rx2 = { .enabled = true, .idx = 0, .freq = 923.300, .drMin = 8, .drMax = 8 },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_9 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_9 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t CN780 = {
  .payloadLenMax = {  59,  59,  59, 123, 230, 230, 250, 230,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 12,
  .powerNumSteps = 5,
  .cfListType = RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 779.500, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 1, .freq = 779.700, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 2, .freq = 779.900, .drMin = 0, .drMax = 5},
  },
  .txJoinReq = {
    { .enabled = true, .idx = 3, .freq = 780.500, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 4, .freq = 780.700, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 5, .freq = 780.900, .drMin = 0, .drMax = 5}
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DataRateBase = 0,
  .rx2 = { .enabled = true, .idx = 0, .freq = 786.000, .drMin = 0, .drMax = 0 },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_9 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7 | RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_FSK_50_K,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t EU433 = {
  .payloadLenMax = {  59,  59,  59, 123, 230, 230, 230, 230,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 12,
  .powerNumSteps = 5,
  .cfListType = RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 433.175, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 1, .freq = 433.375, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 2, .freq = 433.575, .drMin = 0, .drMax = 5},
  },
  .txJoinReq = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DataRateBase = 0,
  .rx2 = { .enabled = true, .idx = 0, .freq = 434.665, .drMin = 0, .drMax = 0 },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_9 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7 | RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_FSK_50_K,
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
  .payloadLenMax = {  59,  59,  59, 123, 230, 230, 230,   0,  41, 117, 230, 230, 230, 230,   0 },
  .powerMax = 30,
  .powerNumSteps = 10,
  .cfListType = RADIOLIB_LORAWAN_CFLIST_TYPE_MASK,
  .txFreqs = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .txJoinReq = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 2,
  .txSpans = {
    {
      .numChannels = 64,
      .freqStart = 915.200,
      .freqStep = 0.200,
      .drMin = 0,
      .drMax = 5,
      .joinRequestDataRate = 0
    },
    {
      .numChannels = 8,
      .freqStart = 915.900,
      .freqStep = 1.600,
      .drMin = 6,
      .drMax = 6,
      .joinRequestDataRate = 6
    }
  },
  .rx1Span = {
    .numChannels = 8,
    .freqStart = 923.300,
    .freqStep = 0.600,
    .drMin = 8,
    .drMax = 13,
    .joinRequestDataRate = RADIOLIB_LORAWAN_DATA_RATE_UNUSED 
  },
  .rx1DataRateBase = 8,
  .rx2 = { .enabled = true, .idx = 0, .freq = 923.300, .drMin = 8, .drMax = 8 },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_500_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED
  }
};

const LoRaWANBand_t CN500 = {
  .payloadLenMax = {  59,  59,  59, 123, 230, 230,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 19,
  .powerNumSteps = 7,
  .cfListType = RADIOLIB_LORAWAN_CFLIST_TYPE_MASK,
  .txFreqs = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .txJoinReq = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 1,
  .txSpans = {
    {
      .numChannels = 96,
      .freqStart = 470.300,
      .freqStep = 0.200,
      .drMin = 0,
      .drMax = 5,
      .joinRequestDataRate = 0
    },
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = {
    .numChannels = 48,
    .freqStart = 500.300,
    .freqStep = 0.200,
    .drMin = 0,
    .drMax = 5,
    .joinRequestDataRate = RADIOLIB_LORAWAN_DATA_RATE_UNUSED 
  },
  .rx1DataRateBase = 0,
  .rx2 = { .enabled = true, .idx = 0, .freq = 505.300, .drMin = 0, .drMax = 0 },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_9  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7  | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_5,
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
  .payloadLenMax = {  59,  59,  59, 123, 230, 230, 230, 230,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 16,
  .powerNumSteps = 7,
  .cfListType = RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 923.200, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 1, .freq = 923.400, .drMin = 0, .drMax = 5},
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .txJoinReq = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DataRateBase = 0,
  .rx2 = { .enabled = true, .idx = 0, .freq = 923.200, .drMin = 2, .drMax = 2 },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_9 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7 | RADIOLIB_LORAWAN_DATA_RATE_BW_250_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_FSK_50_K,
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
  .payloadLenMax = {  59,  59,  59, 123, 230, 230,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 14,
  .powerNumSteps = 7,
  .cfListType = RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 922.100, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 1, .freq = 922.300, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 2, .freq = 922.500, .drMin = 0, .drMax = 5}
  },
  .txJoinReq = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DataRateBase = 0,
  .rx2 = { .enabled = true, .idx = 0, .freq = 921.900, .drMin = 0, .drMax = 0 },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_9 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
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
  .payloadLenMax = {  59,  59,  59, 123, 230, 230, 230, 230,   0,   0,   0,   0,   0,   0,   0 },
  .powerMax = 30,
  .powerNumSteps = 10,
  .cfListType = RADIOLIB_LORAWAN_CFLIST_TYPE_FREQUENCIES,
  .txFreqs = {
    { .enabled = true, .idx = 0, .freq = 865.0625, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 1, .freq = 865.4025, .drMin = 0, .drMax = 5},
    { .enabled = true, .idx = 2, .freq = 865.9850, .drMin = 0, .drMax = 5}
  },
  .txJoinReq = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DataRateBase = 0,
  .rx2 = { .enabled = true, .idx = 0, .freq = 866.550, .drMin = 2, .drMax = 2 },
  .dataRates = {
    RADIOLIB_LORAWAN_DATA_RATE_SF_12 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_11 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_10 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_9 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_8 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_SF_7 | RADIOLIB_LORAWAN_DATA_RATE_BW_125_KHZ | RADIOLIB_LORAWAN_DATA_RATE_CR_4_7,
    RADIOLIB_LORAWAN_DATA_RATE_UNUSED,
    RADIOLIB_LORAWAN_DATA_RATE_FSK_50_K,
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
