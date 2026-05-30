#include "Hellschreiber.h"

#if !RADIOLIB_EXCLUDE_HELLSCHREIBER

// font definition: characters are stored in rows,
//                  least significant byte of each character is the first row
//                  Hellschreiber use 7x7 characters, but this simplified font uses only 5x5
//                  the extra bytes aren't stored
static const uint8_t HellFont[64][RADIOLIB_HELL_FONT_WIDTH - 2] RADIOLIB_NONVOLATILE = {
  { 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000 },  // space
  { 0b0001000, 0b0001000, 0b0001000, 0b0000000, 0b0001000 },  // !
  { 0b0010100, 0b0010100, 0b0000000, 0b0000000, 0b0000000 },  // "
  { 0b0010100, 0b0111110, 0b0010100, 0b0111110, 0b0010100 },  // #
  { 0b0111110, 0b0101000, 0b0111110, 0b0001010, 0b0111110 },  // $
  { 0b0110010, 0b0110100, 0b0001000, 0b0010110, 0b0100110 },  // %
  { 0b0010000, 0b0101000, 0b0010000, 0b0101000, 0b0110100 },  // &
  { 0b0001000, 0b0001000, 0b0000000, 0b0000000, 0b0000000 },  // '
  { 0b0000100, 0b0001000, 0b0001000, 0b0001000, 0b0000100 },  // (
  { 0b0010000, 0b0001000, 0b0001000, 0b0001000, 0b0010000 },  // )
  { 0b0010100, 0b0001000, 0b0010100, 0b0000000, 0b0000000 },  // *
  { 0b0001000, 0b0001000, 0b0111110, 0b0001000, 0b0001000 },  // +
  { 0b0000000, 0b0000000, 0b0000000, 0b0001000, 0b0010000 },  // ,
  { 0b0000000, 0b0000000, 0b0111110, 0b0000000, 0b0000000 },  // -
  { 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0001000 },  // .
  { 0b0000010, 0b0000100, 0b0001000, 0b0010000, 0b0100000 },  // /
  { 0b0011100, 0b0100110, 0b0101010, 0b0110010, 0b0011100 },  // 0
  { 0b0011000, 0b0001000, 0b0001000, 0b0001000, 0b0001000 },  // 1
  { 0b0011000, 0b0100100, 0b0001000, 0b0010000, 0b0111100 },  // 2
  { 0b0111100, 0b0000100, 0b0011100, 0b0000100, 0b0111100 },  // 3
  { 0b0100100, 0b0100100, 0b0111100, 0b0000100, 0b0000100 },  // 4
  { 0b0011100, 0b0100000, 0b0111100, 0b0000100, 0b0111100 },  // 5
  { 0b0111100, 0b0100000, 0b0111100, 0b0100100, 0b0111100 },  // 6
  { 0b0111100, 0b0000100, 0b0001000, 0b0010000, 0b0100000 },  // 7
  { 0b0111100, 0b0100100, 0b0011000, 0b0100100, 0b0111100 },  // 8
  { 0b0111100, 0b0100100, 0b0111100, 0b0000100, 0b0111100 },  // 9
  { 0b0000000, 0b0001000, 0b0000000, 0b0000000, 0b0001000 },  // :
  { 0b0000000, 0b0001000, 0b0000000, 0b0001000, 0b0001000 },  // ;
  { 0b0000100, 0b0001000, 0b0010000, 0b0001000, 0b0000100 },  // <
  { 0b0000000, 0b0111110, 0b0000000, 0b0111110, 0b0000000 },  // =
  { 0b0010000, 0b0001000, 0b0000100, 0b0001000, 0b0010000 },  // >
  { 0b0011100, 0b0000100, 0b0001000, 0b0000000, 0b0001000 },  // ?
  { 0b0011100, 0b0100010, 0b0101110, 0b0101010, 0b0001100 },  // @
  { 0b0111110, 0b0100010, 0b0111110, 0b0100010, 0b0100010 },  // A
  { 0b0111100, 0b0010010, 0b0011110, 0b0010010, 0b0111100 },  // B
  { 0b0011110, 0b0110000, 0b0100000, 0b0110000, 0b0011110 },  // C
  { 0b0111100, 0b0100010, 0b0100010, 0b0100010, 0b0111100 },  // D
  { 0b0111110, 0b0100000, 0b0111100, 0b0100000, 0b0111110 },  // E
  { 0b0111110, 0b0100000, 0b0111100, 0b0100000, 0b0100000 },  // F
  { 0b0111110, 0b0100000, 0b0101110, 0b0100010, 0b0111110 },  // G
  { 0b0100010, 0b0100010, 0b0111110, 0b0100010, 0b0100010 },  // H
  { 0b0011100, 0b0001000, 0b0001000, 0b0001000, 0b0011100 },  // I
  { 0b0111100, 0b0001000, 0b0001000, 0b0101000, 0b0111000 },  // J
  { 0b0100100, 0b0101000, 0b0110000, 0b0101000, 0b0100100 },  // K
  { 0b0100000, 0b0100000, 0b0100000, 0b0100000, 0b0111100 },  // L
  { 0b0100010, 0b0110110, 0b0101010, 0b0100010, 0b0100010 },  // M
  { 0b0100010, 0b0110010, 0b0101010, 0b0100110, 0b0100010 },  // N
  { 0b0011100, 0b0100010, 0b0100010, 0b0100010, 0b0011100 },  // O
  { 0b0111110, 0b0100010, 0b0111110, 0b0100000, 0b0100000 },  // P
  { 0b0111110, 0b0100010, 0b0100010, 0b0100110, 0b0111110 },  // Q
  { 0b0111110, 0b0100010, 0b0111110, 0b0100100, 0b0100010 },  // R
  { 0b0111110, 0b0100000, 0b0111110, 0b0000010, 0b0111110 },  // S
  { 0b0111110, 0b0001000, 0b0001000, 0b0001000, 0b0001000 },  // T
  { 0b0100010, 0b0100010, 0b0100010, 0b0100010, 0b0111110 },  // U
  { 0b0100010, 0b0100010, 0b0010100, 0b0010100, 0b0001000 },  // V
  { 0b0100010, 0b0100010, 0b0101010, 0b0110110, 0b0100010 },  // W
  { 0b0100010, 0b0010100, 0b0001000, 0b0010100, 0b0100010 },  // X
  { 0b0100010, 0b0010100, 0b0001000, 0b0001000, 0b0001000 },  // Y
  { 0b0111110, 0b0000100, 0b0001000, 0b0010000, 0b0111110 },  // Z
  { 0b0001100, 0b0001000, 0b0001000, 0b0001000, 0b0001100 },  // [
  { 0b0100000, 0b0010000, 0b0001000, 0b0000100, 0b0000010 },  // backslash
  { 0b0011000, 0b0001000, 0b0001000, 0b0001000, 0b0011000 },  // ]
  { 0b0001000, 0b0010100, 0b0000000, 0b0000000, 0b0000000 },  // ^
  { 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0111110 }   // _
};

HellClient::HellClient(PhysicalLayer* phy) {
  phyLayer = phy;
  lineFeed = " ";
  #if !RADIOLIB_EXCLUDE_AFSK
  audioClient = nullptr;
  #endif
}

#if !RADIOLIB_EXCLUDE_AFSK
HellClient::HellClient(AFSKClient* audio) {
  phyLayer = audio->phyLayer;
  lineFeed = " ";
  audioClient = audio;
}
#endif

int16_t HellClient::begin(float base, float rate) {
  // calculate 24-bit frequency
  baseFreqHz = base;
  baseFreq = (base * 1000000.0f) / phyLayer->freqStep;

  // calculate "pixel" duration
  pixelDuration = 1000000.0f/rate;

  // configure for direct mode
  return(phyLayer->startDirect());
}

size_t HellClient::printGlyph(const uint8_t* buff) {
  // print the character
  Module* mod = phyLayer->getMod();
  bool transmitting = false;
  for(uint8_t mask = 0x40; mask >= 0x01; mask >>= 1) {
    for(int8_t i = RADIOLIB_HELL_FONT_HEIGHT - 1; i >= 0; i--) {
        RadioLibTime_t start = mod->hal->micros();
        if((buff[i] & mask) && (!transmitting)) {
          transmitting = true;
          transmitDirect(baseFreq, baseFreqHz);
        } else if((!(buff[i] & mask)) && (transmitting)) {
          transmitting = false;
          standby();
        }
        mod->waitForMicroseconds(start, pixelDuration);
    }
  }

  // make sure transmitter is off
  standby();

  return(1);
}

void HellClient::setInversion(bool inv) {
  invert = inv;
}

size_t HellClient::write(uint8_t b) {
  // convert to position in font buffer
  uint8_t pos = b;
  if((pos >= ' ') && (pos <= '_')) {
    pos -= ' ';
  } else if((pos >= 'a') && (pos <= 'z')) {
    pos -= (2*' ');
  } else {
    return(0);
  }

  // fetch character from flash
  uint8_t buff[RADIOLIB_HELL_FONT_WIDTH];
  buff[0] = 0x00;
  for(uint8_t i = 0; i < RADIOLIB_HELL_FONT_WIDTH - 2; i++) {
    uint8_t* ptr = const_cast<uint8_t*>(&HellFont[pos][i]);
    buff[i + 1] = RADIOLIB_NONVOLATILE_READ_BYTE(ptr);
  }
  buff[RADIOLIB_HELL_FONT_WIDTH - 1] = 0x00;

  // print the character
  return(printGlyph(buff));
}

int16_t HellClient::transmitDirect(uint32_t freq, uint32_t freqHz) {
  #if !RADIOLIB_EXCLUDE_AFSK
  if(audioClient != nullptr) {
    return(audioClient->tone(freqHz));
  }
  #endif
  return(phyLayer->transmitDirect(freq));
}

int16_t HellClient::standby() {
  #if !RADIOLIB_EXCLUDE_AFSK
  if(audioClient != nullptr) {
    return(audioClient->noTone(invert));
  }
  #endif
  return(phyLayer->standby(RADIOLIB_STANDBY_WARM));
}

#endif
