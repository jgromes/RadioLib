#include "Morse.h"

#include <ctype.h>

#if !RADIOLIB_EXCLUDE_MORSE

// Morse character table: - using codes defined in ITU-R M.1677-1
//                        - Morse code representation is saved LSb first, using additional bit as guard
//                        - position in array corresponds ASCII code minus RADIOLIB_MORSE_ASCII_OFFSET
//                        - ASCII characters marked RADIOLIB_MORSE_UNSUPPORTED do not have ITU-R M.1677-1 equivalent
static const uint8_t MorseTable[] RADIOLIB_NONVOLATILE = {
    0b00,                         // space
    0b110101,                     // ! (unsupported)
    0b1010010,                    // "
    RADIOLIB_MORSE_UNSUPPORTED,   // # (unsupported)
    RADIOLIB_MORSE_UNSUPPORTED,   // $ (unsupported)
    RADIOLIB_MORSE_UNSUPPORTED,   // % (unsupported)
    RADIOLIB_MORSE_UNSUPPORTED,   // & (unsupported)
    0b1011110,                    // '
    0b101101,                     // (
    0b1101101,                    // )
    RADIOLIB_MORSE_UNSUPPORTED,   // * (unsupported)
    0b101010,                     // +
    0b1110011,                    // ,
    0b1100001,                    // -
    0b1101010,                    // .
    0b101001,                     // /
    0b111111,                     // 0
    0b111110,                     // 1
    0b111100,                     // 2
    0b111000,                     // 3
    0b110000,                     // 4
    0b100000,                     // 5
    0b100001,                     // 6
    0b100011,                     // 7
    0b100111,                     // 8
    0b101111,                     // 9
    0b1000111,                    // :
    RADIOLIB_MORSE_UNSUPPORTED,   // ; (unsupported)
    RADIOLIB_MORSE_UNSUPPORTED,   // < (unsupported)
    0b110001,                     // =
    RADIOLIB_MORSE_UNSUPPORTED,   // > (unsupported)
    0b1001100,                    // ?
    0b1010110,                    // @
    0b110,                        // A
    0b10001,                      // B
    0b10101,                      // C
    0b1001,                       // D
    0b10,                         // E
    0b10100,                      // F
    0b1011,                       // G
    0b10000,                      // H
    0b100,                        // I
    0b11110,                      // J
    0b1101,                       // K
    0b10010,                      // L
    0b111,                        // M
    0b101,                        // N
    0b1111,                       // O
    0b10110,                      // P
    0b11011,                      // Q
    0b1010,                       // R
    0b1000,                       // S
    0b11,                         // T
    0b1100,                       // U
    0b11000,                      // V
    0b1110,                       // W
    0b11001,                      // X
    0b11101,                      // Y
    0b10011,                      // Z
    RADIOLIB_MORSE_UNSUPPORTED,   // [ (unsupported)
    RADIOLIB_MORSE_UNSUPPORTED,   // \ (unsupported)
    RADIOLIB_MORSE_UNSUPPORTED,   // ] (unsupported)
    0b1101000,                    // ^ (unsupported, used as alias for end of work)
    0b110101                      // _ (unsupported, used as alias for starting signal)
};

MorseClient::MorseClient(PhysicalLayer* phy) {
  phyLayer = phy;
  lineFeed = "^";
  #if !RADIOLIB_EXCLUDE_AFSK
  audioClient = nullptr;
  #endif
}

#if !RADIOLIB_EXCLUDE_AFSK
MorseClient::MorseClient(AFSKClient* audio) {
  phyLayer = audio->phyLayer;
  lineFeed = "^";
  audioClient = audio;
}
#endif

int16_t MorseClient::begin(float base, uint8_t speed) {
  // calculate 24-bit frequency
  baseFreqHz = base;
  baseFreq = (base * 1000000.0f) / phyLayer->freqStep;

  // calculate tone period for decoding
  basePeriod = (1000000.0f/base)/2.0f;

  // calculate symbol lengths (assumes PARIS as typical word)
  dotLength = 1200 / speed;
  dashLength = 3*dotLength;
  letterSpace = 3*dotLength;
  wordSpace = 4*dotLength;

  // configure for direct mode
  return(phyLayer->startDirect());
}

size_t MorseClient::startSignal() {
  return(MorseClient::write('_'));
}

char MorseClient::decode(uint8_t symbol, uint8_t len) {
  // add the guard bit
  symbol |= (RADIOLIB_MORSE_DASH << len);

  // iterate over the table
  for(uint8_t i = 0; i < sizeof(MorseTable); i++) {
    uint8_t* ptr = const_cast<uint8_t*>(&MorseTable[i]);
    uint8_t code = RADIOLIB_NONVOLATILE_READ_BYTE(ptr);
    if(code == symbol) {
      // match, return the index + ASCII offset
      return((char)(i + RADIOLIB_MORSE_ASCII_OFFSET));
    }
  }

  // nothing found
  return(RADIOLIB_MORSE_UNKNOWN_SYMBOL);
}

#if !RADIOLIB_EXCLUDE_AFSK
int MorseClient::read(uint8_t* symbol, uint8_t* len, float low, float high) {
  Module* mod = phyLayer->getMod();

  // measure pulse duration in us
  uint32_t duration = mod->hal->pulseIn(audioClient->outPin, mod->hal->GpioLevelLow, 4*basePeriod);

  // decide if this is a signal, or pause
  if((duration > low*basePeriod) && (duration < high*basePeriod)) {
    // this is a signal
    signalCounter++;
  } else if(duration == 0) {
    // this is a pause
    pauseCounter++;
  }

  // update everything
  if((pauseCounter > 0) && (signalCounter == 1)) {
    // start of dot or dash
    pauseCounter = 0;
    signalStart = mod->hal->millis();
    uint32_t pauseLen = mod->hal->millis() - pauseStart;

    if((pauseLen >= low*(float)letterSpace) && (pauseLen <= high*(float)letterSpace)) {
      return(RADIOLIB_MORSE_CHAR_COMPLETE);
    } else if(pauseLen > wordSpace) {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN(RADIOLIB_LINE_FEED "<space>");
      return(RADIOLIB_MORSE_WORD_COMPLETE);
    }

  } else if((signalCounter > 0) && (pauseCounter == 1)) {
    // end of dot or dash
    signalCounter = 0;
    pauseStart = mod->hal->millis();
    uint32_t signalLen = mod->hal->millis() - signalStart;

    if((signalLen >= low*(float)dotLength) && (signalLen <= high*(float)dotLength)) {
      RADIOLIB_DEBUG_PROTOCOL_PRINT(".");
      (*symbol) |= (RADIOLIB_MORSE_DOT << (*len));
      (*len)++;
    } else if((signalLen >= low*(float)dashLength) && (signalLen <= high*(float)dashLength)) {
      RADIOLIB_DEBUG_PROTOCOL_PRINT("-");
      (*symbol) |= (RADIOLIB_MORSE_DASH << (*len));
      (*len)++;
    } else {
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("<len=%lums>", (long unsigned int)signalLen);
    }
  }

  return(RADIOLIB_MORSE_INTER_SYMBOL);
}
#endif

size_t MorseClient::write(uint8_t b) {
  Module* mod = phyLayer->getMod();

  // check unprintable ASCII characters and boundaries
  if((b < ' ') || (b == 0x60) || (b > 'z')) {
    return(0);
  }

  // inter-word pause (space)
  if(b == ' ') {
    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("space");
    standby();
    mod->waitForMicroseconds(mod->hal->micros(), wordSpace*1000);
    return(1);
  }

  // get morse code from lookup table
  uint8_t* ptr = const_cast<uint8_t*>(&MorseTable[(uint8_t)(toupper(b) - RADIOLIB_MORSE_ASCII_OFFSET)]);
  uint8_t code = RADIOLIB_NONVOLATILE_READ_BYTE(ptr);

  // check unsupported characters
  if(code == RADIOLIB_MORSE_UNSUPPORTED) {
    return(0);
  }

  // iterate through codeword until guard bit is reached
  RADIOLIB_DEBUG_PROTOCOL_PRINT("%c ", b);
  while(code > RADIOLIB_MORSE_GUARDBIT) {

    // send dot or dash
    if (code & RADIOLIB_MORSE_DASH) {
      RADIOLIB_DEBUG_PROTOCOL_PRINT_NOTAG("-");
      transmitDirect(baseFreq, baseFreqHz);
      mod->waitForMicroseconds(mod->hal->micros(), dashLength*1000);
    } else {
      RADIOLIB_DEBUG_PROTOCOL_PRINT_NOTAG(".");
      transmitDirect(baseFreq, baseFreqHz);
      mod->waitForMicroseconds(mod->hal->micros(), dotLength*1000);
    }

    // symbol space
    standby();
    mod->waitForMicroseconds(mod->hal->micros(), dotLength*1000);

    // move onto the next bit
    code >>= 1;
  }

  // letter space
  standby();
  mod->waitForMicroseconds(mod->hal->micros(), letterSpace*1000 - dotLength*1000);
  RADIOLIB_DEBUG_PROTOCOL_PRINT_NOTAG(RADIOLIB_LINE_FEED);

  return(1);
}

int16_t MorseClient::transmitDirect(uint32_t freq, uint32_t freqHz) {
  #if !RADIOLIB_EXCLUDE_AFSK
  if(audioClient != nullptr) {
    return(audioClient->tone(freqHz));
  }
  #endif
  return(phyLayer->transmitDirect(freq));
}

int16_t MorseClient::standby() {
  #if !RADIOLIB_EXCLUDE_AFSK
  if(audioClient != nullptr) {
    return(audioClient->noTone(true));
  }
  #endif
  return(phyLayer->standby());
}

#endif
